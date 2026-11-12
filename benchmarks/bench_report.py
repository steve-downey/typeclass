#!/usr/bin/env python3
# benchmarks/bench_report.py
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
"""Render the tree_algorithms benchmarks as one self-contained HTML report.

Inputs, both optional (the report shows whatever is present):

  runtime   Catch2 XML written by the runtime microbenchmarks. `make bench`
            runs each benchmark executable with an extra
            `-r xml::out=<build-dir>/bench-results/<name>.xml` reporter.
  compile   The compile-time matrix (ct_fix_<N>, ct_plain_<N>) as recorded
            in the CMake build telemetry under <build-dir>/.trace; the same
            events compile_trace_report.py reads.

Output: <out> (default <build-dir>/bench-results/report.html), a single file
with inline SVG charts, light/dark themes, hover tooltips, and a table view
under every chart. A plain-text summary also goes to stdout.

No third-party Python packages: the charts are hand-built SVG, so the report
renders anywhere the build does.

Usage:
    bench_report.py --build-dir .build/build-gcc-16 [--config Bench] [--out report.html]
"""

from __future__ import annotations

import argparse
import datetime as _dt
import glob
import html
import math
import os
import platform
import re
import subprocess
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import compile_trace_report as ctr  # noqa: E402  (sibling module)

# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------


@dataclass
class Bench:
    name: str
    mean: float  # ns
    lo: float  # ns, 95% CI lower bound on the mean
    hi: float  # ns, 95% CI upper bound on the mean
    stddev: float  # ns
    samples: int
    iterations: int
    outlier_variance: float  # fraction of variance explained by outliers

    @property
    def role(self) -> str:
        """Who wrote the code under the timer: the role picks the color."""
        n = self.name
        if "inline control" in n:
            return "control"
        if n.startswith("hand") or not re.search(r"fold_|unfold_|refold|Fix<", n):
            return "baseline"
        return "library"


@dataclass
class Case:
    name: str
    source: str  # executable stem
    benches: list[Bench] = field(default_factory=list)


ROLE_LABEL = {
    "baseline": "Hand-written / non-library",
    "library": "Library verb or Fix tree",
    "control": "Explicit inline control",
}
ROLE_SLOT = {"baseline": 1, "library": 2, "control": 3}


def parse_catch2_xml(path: str) -> list[Case]:
    stem = os.path.splitext(os.path.basename(path))[0]
    root = ET.parse(path).getroot()
    cases: list[Case] = []
    for tc in root.iter("TestCase"):
        results = tc.findall(".//BenchmarkResults")
        if not results:
            continue
        case = Case(tc.get("name", "?"), stem)
        for br in results:
            mean = br.find("mean")
            sd = br.find("standardDeviation")
            out = br.find("outliers")
            case.benches.append(
                Bench(
                    name=br.get("name", "?"),
                    mean=float(mean.get("value")),
                    lo=float(mean.get("lowerBound")),
                    hi=float(mean.get("upperBound")),
                    stddev=float(sd.get("value")) if sd is not None else 0.0,
                    samples=int(br.get("samples", 0)),
                    iterations=int(br.get("iterations", 0)),
                    outlier_variance=float(out.get("variance", 0)) if out is not None else 0.0,
                )
            )
        cases.append(case)
    return cases


# ---------------------------------------------------------------------------
# Formatting helpers
# ---------------------------------------------------------------------------

_UNITS = [(1e9, "s"), (1e6, "ms"), (1e3, "µs"), (1.0, "ns")]


def pick_unit(ns: float) -> tuple[float, str]:
    for scale, unit in _UNITS:
        if ns >= scale:
            return scale, unit
    return 1.0, "ns"


def fmt_ns(ns: float) -> str:
    scale, unit = pick_unit(ns)
    v = ns / scale
    return f"{v:.3g} {unit}" if v < 100 else f"{v:.0f} {unit}"


def fmt_ratio(r: float) -> str:
    if r >= 100:
        return f"{r:,.0f}×"
    return f"{r:.2f}×"


def esc(s: str) -> str:
    return html.escape(s, quote=True)


def nice_max(v: float) -> float:
    if v <= 0:
        return 1.0
    exp = 10 ** math.floor(math.log10(v))
    for m in (1, 1.2, 1.5, 2, 2.5, 3, 4, 5, 6, 8, 10):
        if m * exp >= v:
            return m * exp
    return 10 * exp


def nice_ticks(vmax: float, n: int = 4) -> list[float]:
    step = vmax / n
    return [step * i for i in range(n + 1)]


# ---------------------------------------------------------------------------
# Environment
# ---------------------------------------------------------------------------


def _run(cmd: list[str], cwd: str | None = None) -> str:
    try:
        return subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=10).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return ""


def environment(build_dir: str, config: str | None) -> list[tuple[str, str]]:
    cache: dict[str, str] = {}
    try:
        with open(os.path.join(build_dir, "CMakeCache.txt")) as fh:
            for line in fh:
                m = re.match(r"([A-Za-z_]+):[A-Z]+=(.*)", line)
                if m:
                    cache[m.group(1)] = m.group(2)
    except OSError:
        pass
    cxx = cache.get("CMAKE_CXX_COMPILER", "")
    if not cxx:
        # Toolchain files set the compiler without caching it; CMake records
        # the resolved path in its per-version compiler-identification file.
        for ident in sorted(glob.glob(os.path.join(build_dir, "CMakeFiles", "*", "CMakeCXXCompiler.cmake"))):
            with open(ident) as fh:
                m = re.search(r'set\(CMAKE_CXX_COMPILER "([^"]+)"\)', fh.read())
            if m:
                cxx = m.group(1)
    cxx_ver = _run([cxx, "--version"]).splitlines()[0] if cxx else ""
    flags_key = f"CMAKE_CXX_FLAGS_{(config or 'Bench').upper()}"
    flags = " ".join(x for x in (cache.get("CMAKE_CXX_FLAGS", ""), cache.get(flags_key, "")) if x)
    cpu = ""
    try:
        with open("/proc/cpuinfo") as fh:
            for line in fh:
                if line.startswith("model name"):
                    cpu = line.split(":", 1)[1].strip()
                    break
    except OSError:
        cpu = platform.processor()
    here = os.path.dirname(os.path.abspath(__file__))
    commit = _run(["git", "rev-parse", "--short=10", "HEAD"], cwd=here)
    if _run(["git", "status", "--porcelain", "--untracked-files=no"], cwd=here):
        commit += " + uncommitted changes"
    branch = _run(["git", "rev-parse", "--abbrev-ref", "HEAD"], cwd=here)
    rows = [
        ("Generated", _dt.datetime.now().strftime("%Y-%m-%d %H:%M")),
        ("Commit", f"{commit} ({branch})" if branch else commit),
        ("Compiler", cxx_ver or cxx or "unknown"),
        ("Config / flags", f"{config or '?'}: {flags}" if flags else (config or "?")),
        ("CPU", f"{cpu} × {os.cpu_count()} threads" if cpu else "unknown"),
        ("Kernel", platform.platform()),
    ]
    return rows


# ---------------------------------------------------------------------------
# SVG charts
# ---------------------------------------------------------------------------

FONT_PX = 12
CHAR_PX = 6.6  # average glyph width at 12px system sans, for label layout


def runtime_chart(case: Case) -> str:
    """Horizontal bars, one per benchmark, 95% CI whisker, value at the tip."""
    benches = case.benches
    fastest = min(b.mean for b in benches)
    vmax = nice_max(max(b.hi for b in benches))
    scale, unit = pick_unit(vmax)
    label_w = int(max(len(b.name) for b in benches) * CHAR_PX) + 16
    plot_w = 460
    value_w = 150
    row_h = 30
    bar_h = 16
    top = 8
    axis_h = 24
    w = label_w + plot_w + value_w
    h = top + row_h * len(benches) + axis_h
    x0 = label_w

    def X(ns: float) -> float:
        return x0 + plot_w * ns / vmax

    parts = [
        f'<svg class="chart" viewBox="0 0 {w} {h}" width="{w}" height="{h}" role="img" '
        f'aria-label="{esc(case.name)}: mean time per benchmark">'
    ]
    for t in nice_ticks(vmax):
        x = X(t)
        parts.append(f'<line class="grid" x1="{x:.1f}" x2="{x:.1f}" y1="{top}" y2="{h - axis_h}"/>')
        parts.append(
            f'<text class="tick" x="{x:.1f}" y="{h - axis_h + 16}" text-anchor="middle">'
            f"{t / scale:.3g}{' ' + unit if t == 0 else ''}</text>"
        )
    parts.append(f'<line class="axis" x1="{x0}" x2="{x0}" y1="{top}" y2="{h - axis_h}"/>')
    for i, b in enumerate(benches):
        cy = top + row_h * i + row_h / 2
        y = cy - bar_h / 2
        bw = max(X(b.mean) - x0, 1.0)
        r = min(4.0, bw / 2)
        slot = ROLE_SLOT[b.role]
        tip = (
            f"{b.name}|mean {fmt_ns(b.mean)}|95% CI {fmt_ns(b.lo)} – {fmt_ns(b.hi)}"
            f"|σ {fmt_ns(b.stddev)}|{fmt_ratio(b.mean / fastest)} fastest"
            f"|{b.samples} samples × {b.iterations} iter|outlier variance {b.outlier_variance:.0%}"
            f"|{ROLE_LABEL[b.role]}"
        )
        parts.append(f'<g class="hit" data-tip="{esc(tip)}">')
        parts.append(f'<rect class="band" x="0" y="{cy - row_h / 2}" width="{w}" height="{row_h}"/>')
        parts.append(
            f'<text class="label" x="{x0 - 8}" y="{cy + 4}" text-anchor="end">{esc(b.name)}</text>'
        )
        # Square at the baseline, 4px rounded at the data end.
        parts.append(
            f'<path class="s{slot}" d="M{x0},{y} h{bw - r:.2f} a{r},{r} 0 0 1 {r},{r} '
            f"v{bar_h - 2 * r} a{r},{r} 0 0 1 {-r},{r} h{-(bw - r):.2f} z\"/>"
        )
        xl, xh = X(b.lo), X(b.hi)
        if xh - xl > 2:
            parts.append(f'<line class="ci" x1="{xl:.1f}" x2="{xh:.1f}" y1="{cy}" y2="{cy}"/>')
            parts.append(f'<line class="ci" x1="{xl:.1f}" x2="{xl:.1f}" y1="{cy - 4}" y2="{cy + 4}"/>')
            parts.append(f'<line class="ci" x1="{xh:.1f}" x2="{xh:.1f}" y1="{cy - 4}" y2="{cy + 4}"/>')
        ratio = b.mean / fastest
        rtxt = "fastest" if ratio < 1.005 else fmt_ratio(ratio)
        parts.append(
            f'<text class="value" x="{max(X(b.hi), X(b.mean)) + 8:.1f}" y="{cy + 4}">{fmt_ns(b.mean)}'
            f'<tspan class="muted"> · {rtxt}</tspan></text>'
        )
        parts.append("</g>")
    parts.append("</svg>")
    return "".join(parts)


def runtime_table(case: Case) -> str:
    fastest = min(b.mean for b in case.benches)
    rows = "".join(
        f"<tr><td>{esc(b.name)}</td><td>{ROLE_LABEL[b.role]}</td><td class=n>{fmt_ns(b.mean)}</td>"
        f"<td class=n>{fmt_ns(b.lo)} – {fmt_ns(b.hi)}</td><td class=n>{fmt_ns(b.stddev)}</td>"
        f"<td class=n>{fmt_ratio(b.mean / fastest)}</td><td class=n>{b.outlier_variance:.0%}</td></tr>"
        for b in case.benches
    )
    return (
        "<details><summary>Table view</summary><table><thead><tr><th>Benchmark</th><th>Role</th>"
        "<th>Mean</th><th>95% CI</th><th>σ</th><th>× fastest</th><th>Outlier var.</th></tr></thead>"
        f"<tbody>{rows}</tbody></table></details>"
    )


def linear_fit(series: list[tuple[int, float]]) -> tuple[float, float] | None:
    """Least-squares time = a + b*N. a is the fixed cost of the translation
    unit (headers, instantiation); b is the per-node constexpr cost."""
    if len(series) < 2:
        return None
    xs = [float(n) for n, _ in series]
    ys = [v for _, v in series]
    mx, my = sum(xs) / len(xs), sum(ys) / len(ys)
    den = sum((x - mx) ** 2 for x in xs)
    if not den:
        return None
    b = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / den
    return my - b * mx, b


def compile_series(latest: dict, sizes: list[int]) -> dict[str, list[tuple[int, float]]]:
    return {k: [(n, latest[(k, n)][0]) for n in sizes if (k, n) in latest] for k in ("plain", "fix")}


def compile_chart(latest: dict, sizes: list[int]) -> str:
    """Compile time vs N on linear axes: measured points plus the fitted
    a + b*N line drawn back to N = 0, so the fixed cost reads as the
    intercept and the per-node cost as the slope."""
    pts = compile_series(latest, sizes)
    allv = [v for series in pts.values() for _, v in series]
    if not allv:
        return ""
    fits = {k: linear_fit(v) for k, v in pts.items()}
    w, h = 720, 320
    ml, mr, mt, mb = 56, 210, 16, 44
    pw, ph = w - ml - mr, h - mt - mb
    xmax = nice_max(sizes[-1])
    ymax = nice_max(max(allv))

    def X(n: float) -> float:
        return ml + pw * n / xmax

    def Y(ms: float) -> float:
        return mt + ph * (1 - ms / ymax)

    parts = [
        f'<svg class="chart" viewBox="0 0 {w} {h}" width="{w}" height="{h}" role="img" '
        'aria-label="Compile time versus tree size with fitted fixed and per-node cost">'
    ]
    for t in nice_ticks(ymax):
        y = Y(t)
        parts.append(f'<line class="grid" x1="{ml}" x2="{ml + pw}" y1="{y:.1f}" y2="{y:.1f}"/>')
        parts.append(f'<text class="tick" x="{ml - 6}" y="{y + 4:.1f}" text-anchor="end">{t / 1000:g} s</text>')
    for t in nice_ticks(xmax):
        parts.append(f'<text class="tick" x="{X(t):.1f}" y="{mt + ph + 16}" text-anchor="middle">{t:,.0f}</text>')
    parts.append(f'<line class="axis" x1="{ml}" x2="{ml + pw}" y1="{mt + ph}" y2="{mt + ph}"/>')
    parts.append(
        f'<text class="tick" x="{ml + pw / 2}" y="{h - 6}" text-anchor="middle">'
        "nodes built and folded in constexpr</text>"
    )
    label_ys: list[float] = []
    for kind, slot in (("fix", 2), ("plain", 1)):
        series = pts[kind]
        if not series:
            continue
        fit = fits[kind]
        if fit:
            a, b = fit
            parts.append(
                f'<line class="fit s{slot}l" x1="{X(0):.1f}" y1="{Y(a):.1f}" x2="{X(sizes[-1]):.1f}" '
                f'y2="{Y(a + b * sizes[-1]):.1f}"/>'
            )
            tip = f"{kind}: fitted line|fixed cost {a:,.0f} ms (intercept)|per node {b * 1000:.1f} µs (slope)"
            parts.append(
                f'<g class="hit" data-tip="{esc(tip)}"><circle class="hitpad" cx="{X(0):.1f}" cy="{Y(a):.1f}" '
                f'r="12"/><circle class="ring s{slot}l" cx="{X(0):.1f}" cy="{Y(a):.1f}" r="4"/></g>'
            )
        for n, v in series:
            tip = f"{kind} · {n:,} nodes|{v:,.0f} ms measured|object file {latest[(kind, n)][1]:,} B"
            parts.append(
                f'<g class="hit" data-tip="{esc(tip)}"><circle class="hitpad" cx="{X(n):.1f}" cy="{Y(v):.1f}" '
                f'r="12"/><circle class="dot s{slot}" cx="{X(n):.1f}" cy="{Y(v):.1f}" r="4.5"/></g>'
            )
        n_last, v_last = series[-1]
        ly = Y(v_last) + 4
        for other in label_ys:
            if abs(ly - other) < 16:
                ly = other + 16
        label_ys.append(ly)
        txt = kind if not fit else f"{kind}: {fit[0] / 1000:.2f} s + {fit[1] * 1000:.0f} µs/node"
        parts.append(f'<text class="label" x="{X(n_last) + 10:.1f}" y="{ly:.1f}">{esc(txt)}</text>')
    parts.append("</svg>")
    return "".join(parts)


def compile_stats(latest: dict, sizes: list[int]) -> list[tuple[str, str, str]]:
    fits = {k: linear_fit(v) for k, v in compile_series(latest, sizes).items()}
    f, p = fits.get("fix"), fits.get("plain")
    if not (f and p) or p[1] <= 0:
        return []
    return [
        ("Compile: per-node cost, fix ÷ plain", fmt_ratio(f[1] / p[1]), "slope ratio; 1.00× = Fix adds nothing per node"),
        ("Compile: extra fixed cost of fix", f"{(f[0] - p[0]) / 1000:+.2f} s", "intercept difference: library headers"),
    ]


def compile_table(latest: dict, sizes: list[int]) -> str:
    rows = []
    for n in sizes:
        f = latest.get(("fix", n))
        p = latest.get(("plain", n))
        ratio = f"{f[0] / p[0]:.2f}×" if f and p and p[0] else "—"
        rows.append(
            f"<tr><td class=n>{n:,}</td><td class=n>{f[0]:,.0f}</td><td class=n>{p[0]:,.0f}</td>"
            f"<td class=n>{ratio}</td><td class=n>{f[1]:,}</td><td class=n>{p[1]:,}</td></tr>"
            if f and p
            else f"<tr><td class=n>{n:,}</td><td colspan=5>incomplete</td></tr>"
        )
    return (
        "<details open><summary>Table view</summary><table><thead><tr><th>Nodes</th><th>fix ms</th>"
        "<th>plain ms</th><th>fix / plain</th><th>fix object B</th><th>plain object B</th></tr></thead>"
        f"<tbody>{''.join(rows)}</tbody></table></details>"
    )


# ---------------------------------------------------------------------------
# Page
# ---------------------------------------------------------------------------

CSS = """
.viz-root{color-scheme:light;--surface-1:#fcfcfb;--page:#f9f9f7;--text-primary:#0b0b0b;
--text-secondary:#52514e;--muted:#898781;--grid:#e1e0d9;--axis:#c3c2b7;--border:rgba(11,11,11,.10);
--s1:#2a78d6;--s2:#eb6834;--s3:#1baf7a;--band:rgba(11,11,11,.04)}
@media (prefers-color-scheme:dark){:root:where(:not([data-theme="light"])) .viz-root{color-scheme:dark;
--surface-1:#1a1a19;--page:#0d0d0d;--text-primary:#f0efec;--text-secondary:#c3c2b7;--grid:#2c2c2a;
--axis:#383835;--border:rgba(255,255,255,.10);--s1:#3987e5;--s2:#d95926;--s3:#199e70;--band:rgba(255,255,255,.05)}}
:root[data-theme="dark"] .viz-root{color-scheme:dark;--surface-1:#1a1a19;--page:#0d0d0d;--text-primary:#f0efec;
--text-secondary:#c3c2b7;--grid:#2c2c2a;--axis:#383835;--border:rgba(255,255,255,.10);--s1:#3987e5;
--s2:#d95926;--s3:#199e70;--band:rgba(255,255,255,.05)}
html,body{margin:0}
.viz-root{background:var(--page);color:var(--text-primary);font:14px/1.45 system-ui,-apple-system,"Segoe UI",sans-serif;
min-height:100vh;padding:24px 32px 64px;box-sizing:border-box}
main{max-width:1180px;margin:0 auto}
h1{font-size:22px;margin:0 0 4px}h2{font-size:17px;margin:36px 0 4px}h3{font-size:15px;margin:0 0 2px}
p.lede,p.note{color:var(--text-secondary);margin:4px 0 12px;max-width:80ch}
.card{background:var(--surface-1);border:1px solid var(--border);border-radius:10px;padding:16px 20px;margin:12px 0;
overflow-x:auto}
.legend{display:flex;gap:18px;flex-wrap:wrap;color:var(--text-secondary);font-size:13px;margin:6px 0 10px}
.legend span::before{content:"";display:inline-block;width:10px;height:10px;border-radius:2px;margin-right:6px;
vertical-align:-1px;background:var(--c)}
.stats{display:flex;gap:12px;flex-wrap:wrap;margin:12px 0}
.stat{background:var(--surface-1);border:1px solid var(--border);border-radius:10px;padding:12px 16px;min-width:180px;flex:1}
.stat .k{color:var(--text-secondary);font-size:12px}.stat .v{font-size:24px;font-weight:600;margin-top:2px}
.stat .d{color:var(--muted);font-size:12px}
svg.chart{display:block;font:12px system-ui,-apple-system,"Segoe UI",sans-serif;max-width:100%;height:auto}
.grid{stroke:var(--grid);stroke-width:1}.axis{stroke:var(--axis);stroke-width:1}
.ref{stroke:var(--muted);stroke-width:1;stroke-dasharray:4 4}
.tick{fill:var(--muted);font-variant-numeric:tabular-nums}.label{fill:var(--text-primary)}
.value{fill:var(--text-primary);font-variant-numeric:tabular-nums}.muted{fill:var(--muted)}
.ci{stroke:var(--text-secondary);stroke-width:1.25}
.s1{fill:var(--s1)}.s2{fill:var(--s2)}.s3{fill:var(--s3)}
.line{fill:none;stroke-width:2;stroke-linejoin:round;stroke-linecap:round}
.s1l{stroke:var(--s1)}.s2l{stroke:var(--s2)}
.dot{stroke:var(--surface-1);stroke-width:2}.fit{stroke-width:2;opacity:.55}
.ring{fill:var(--surface-1);stroke-width:2}.legend span:not([style])::before{display:none}.hitpad{fill:transparent}
.band{fill:transparent}.hit:hover .band{fill:var(--band)}
table{border-collapse:collapse;margin-top:8px;font-size:13px}
th,td{padding:4px 10px;border-bottom:1px solid var(--grid);text-align:left}
th{color:var(--text-secondary);font-weight:600}td.n{text-align:right;font-variant-numeric:tabular-nums}
details summary{cursor:pointer;color:var(--text-secondary);font-size:13px;margin-top:8px}
dl.env{display:grid;grid-template-columns:max-content 1fr;gap:2px 16px;margin:0;font-size:13px}
dl.env dt{color:var(--text-secondary)}dl.env dd{margin:0;font-family:ui-monospace,monospace;word-break:break-word}
#tip{position:fixed;pointer-events:none;background:var(--surface-1);color:var(--text-primary);border:1px solid var(--border);
border-radius:8px;padding:8px 10px;font-size:12px;box-shadow:0 4px 16px rgba(0,0,0,.18);display:none;max-width:360px;z-index:9}
#tip b{display:block;margin-bottom:2px}#tip div{color:var(--text-secondary);font-variant-numeric:tabular-nums}
.toggle{float:right;background:var(--surface-1);color:var(--text-secondary);border:1px solid var(--border);
border-radius:6px;padding:4px 10px;cursor:pointer;font:inherit;font-size:12px}
"""

JS = """
const tip=document.getElementById('tip');
document.querySelectorAll('.hit').forEach(g=>{
 g.addEventListener('mousemove',e=>{const [h,...r]=g.dataset.tip.split('|');
  tip.innerHTML='<b></b>'+r.map(()=>'<div></div>').join('');tip.firstChild.textContent=h;
  r.forEach((t,i)=>tip.children[i+1].textContent=t);tip.style.display='block';
  const x=Math.min(e.clientX+14,innerWidth-tip.offsetWidth-8);tip.style.left=x+'px';tip.style.top=(e.clientY+14)+'px';});
 g.addEventListener('mouseleave',()=>tip.style.display='none');});
document.getElementById('theme').addEventListener('click',()=>{const r=document.documentElement;
 const dark=r.dataset.theme?r.dataset.theme==='dark':matchMedia('(prefers-color-scheme: dark)').matches;
 r.dataset.theme=dark?'light':'dark';});
"""

CASE_NOTES = {
    "fold: sum a very large tree": (
        "Consuming ~1M nodes: every route visits every node once. The question is whether the "
        "Fix/verb route costs more than hand recursion over the same layout. Memory layout dominates: "
        "the preorder-allocated unique_ptr tree is the only one laid out in traversal order, so compare "
        "the Fix rows against the postorder unique_ptr row, which matches them in node size and allocation order."
    ),
    "build: grow a large tree": (
        "Producing 65,535 nodes from a seed. Every row performs one allocation per node; "
        "the verb should cost no more than the hand-written builder."
    ),
    "copy: value semantics vs structural sharing": (
        "Ownership policy, not Fix: shared_ptr copies are an O(1) refcount bump; Box and "
        "std::indirect deep-copy O(N). The unique_ptr trees have no row because they cannot be copied."
    ),
    "refold: fuse build into fold": (
        "refold computes fold(unfold(seed)) without materializing the tree; it should beat "
        "build-then-fold by the whole allocation bill."
    ),
}


def legend(roles: list[str]) -> str:
    return (
        '<div class="legend">'
        + "".join(f'<span style="--c:var(--s{ROLE_SLOT[r]})">{ROLE_LABEL[r]}</span>' for r in roles)
        + "</div>"
    )


def headline_stats(cases: list[Case]) -> list[tuple[str, str, str]]:
    """A few computed ratios that answer the benchmarks' questions directly."""
    by = {(c.name, b.name): b for c in cases for b in c.benches}
    out = []

    def ratio(case, a, b):
        x, y = by.get((case, a)), by.get((case, b))
        return (x.mean / y.mean) if x and y else None

    f = "fold: sum a very large tree"
    r = ratio(f, "fold_fix / Fix<Box>", "hand recursion (unique_ptr, postorder alloc)")
    if r:
        out.append(("fold_fix vs naive unique_ptr tree", fmt_ratio(r), "same node size and allocation order"))
    r = ratio(f, "hand recursion (unique_ptr, postorder alloc)", "hand recursion (unique_ptr)")
    if r:
        out.append(("Layout alone: postorder vs preorder", fmt_ratio(r), "identical unique_ptr tree and traversal"))
    r = ratio(f, "fold_fix / Fix<Box>", "hand recursion (Fix<Box> tree)")
    if r:
        out.append(("fold_fix vs hand recursion", fmt_ratio(r), "same Fix<Box> tree; 1.00× = zero overhead"))
    r = ratio(f, "fold_map / native (direct verb)", "hand recursion (shared_ptr)")
    if r:
        out.append(("fold_map (native) vs hand", fmt_ratio(r), "shared_ptr tree, no Fix materialized"))
    r = ratio("build: grow a large tree", "unfold_fix build (Fix<Box>)", "hand recursive build (Fix<Box>)")
    if r:
        out.append(("unfold_fix vs hand build", fmt_ratio(r), "65,535 nodes, one allocation each"))
    rf = "refold: fuse build into fold"
    r = ratio(rf, "unfold_fix then fold_fix (tree materialized)", "refold (fused, no tree)")
    if r:
        out.append(("refold speed-up", fmt_ratio(r), "build-then-fold ÷ fused refold"))
    return out


def render(cases: list[Case], latest: dict, sizes: list[int], env: list[tuple[str, str]], sources: list[str]) -> str:
    body = []
    body.append('<button class="toggle" id="theme" type="button">Toggle theme</button>')
    body.append("<h1>beman.tree_algorithms benchmarks</h1>")
    body.append(
        '<p class="lede">The cost of the Fix operator and the recursion-scheme verbs at runtime and compile time, '
        "against hand-written code over the same and over naive tree representations. "
        "Hover a bar or point for its statistics; every chart has a table view.</p>"
    )
    stats = headline_stats(cases) + compile_stats(latest, sizes)
    if stats:
        body.append(
            '<div class="stats">'
            + "".join(
                f'<div class="stat"><div class="k">{esc(k)}</div><div class="v">{esc(v)}</div>'
                f'<div class="d">{esc(d)}</div></div>'
                for k, v, d in stats
            )
            + "</div>"
        )

    if cases:
        body.append("<h2>Runtime</h2>")
        body.append(
            '<p class="note">Catch2 benchmark mean per call. Whiskers are the bootstrapped 95% confidence '
            "interval on the mean; the muted figure is the ratio to the fastest row in the same chart. "
            "The interval describes one run: absolute times move between runs (CPU frequency, other load), "
            "so compare rows within a chart rather than across reports.</p>"
        )
        all_benches = [b for c in cases for b in c.benches]
        noisy = sum(b.outlier_variance > 0.5 for b in all_benches)
        if noisy * 2 > len(all_benches):
            body.append(
                f'<p class="note">⚠ In {noisy} of {len(all_benches)} rows, outliers account for most of the '
                "sample variance (Catch2's outlier-variance estimate &gt; 50%). The host is noisy; small "
                "differences between rows are not significant.</p>"
            )
        for c in cases:
            roles = [r for r in ("baseline", "library", "control") if any(b.role == r for b in c.benches)]
            note = CASE_NOTES.get(c.name, "")
            body.append(
                f'<div class="card"><h3>{esc(c.name)}</h3><p class="note">{esc(note)}</p>{legend(roles)}'
                f"{runtime_chart(c)}{runtime_table(c)}</div>"
            )
    else:
        body.append(
            '<h2>Runtime</h2><p class="note">No runtime results found. Run <code>make bench</code>.</p>'
        )

    body.append("<h2>Compile time</h2>")
    if latest:
        fits = {k: linear_fit(v) for k, v in compile_series(latest, sizes).items()}
        fit_txt = "; ".join(
            f"{k}: {f[0]:,.0f} ms fixed + {f[1] * 1000:.0f} µs per node" for k, f in fits.items() if f
        )
        body.append(
            '<p class="note">Wall-clock compile time of each generated translation unit, which builds a balanced '
            "tree of N nodes and folds it entirely in constexpr. <code>fix</code> uses the library "
            "(<code>unfold_fix</code>/<code>fold_fix</code> over <code>BinaryTreeFix</code>); <code>plain</code> is a "
            "hand-written control that includes no library headers. Time is modelled as "
            "<i>fixed + per-node × N</i>: the intercept is what the headers cost, the slope is constexpr "
            "evaluation. Compare slopes for the cost of Fix itself; a ratio of raw times mixes the two. "
            f"Fitted: {esc(fit_txt)}. One compile per cell; expect roughly ±10% noise.</p>"
        )
        body.append(
            '<div class="card"><h3>Compile time vs tree size</h3>'
            '<div class="legend"><span style="--c:var(--s1)">plain (hand-written control)</span>'
            '<span style="--c:var(--s2)">fix (library Fix tree)</span>'
            "<span>hollow point on the axis = fitted fixed cost</span></div>"
            f"{compile_chart(latest, sizes)}{compile_table(latest, sizes)}</div>"
        )
    else:
        body.append('<p class="note">No compile-time events found. Run <code>make bench-compile</code>.</p>')

    body.append("<h2>Environment</h2>")
    body.append(
        '<div class="card"><dl class="env">'
        + "".join(f"<dt>{esc(k)}</dt><dd>{esc(v)}</dd>" for k, v in env)
        + "".join(f"<dt>Source</dt><dd>{esc(s)}</dd>" for s in sources)
        + "</dl></div>"
    )
    return (
        '<!doctype html><html lang="en"><head><meta charset="utf-8">'
        '<meta name="viewport" content="width=device-width,initial-scale=1">'
        f"<title>tree_algorithms benchmarks</title><style>{CSS}</style></head>"
        f'<body class="viz-root"><main>{"".join(body)}</main><div id="tip"></div><script>{JS}</script></body></html>'
    )


def text_summary(cases: list[Case], latest: dict, sizes: list[int]) -> str:
    lines = []
    for c in cases:
        fastest = min(b.mean for b in c.benches)
        w = max(len(b.name) for b in c.benches)
        lines.append(f"== {c.name}")
        for b in c.benches:
            lines.append(
                f"  {b.name:<{w}}  {fmt_ns(b.mean):>10}  ±{fmt_ns((b.hi - b.lo) / 2):>9}  {fmt_ratio(b.mean / fastest):>9}"
            )
        lines.append("")
    if latest:
        lines.append("== compile time (ms): nodes  fix  plain  fix/plain")
        for n in sizes:
            f, p = latest.get(("fix", n)), latest.get(("plain", n))
            if f and p:
                lines.append(f"  {n:>8}  {f[0]:>8.0f}  {p[0]:>8.0f}  {f[0] / p[0]:>6.2f}")
        for kind, series in compile_series(latest, sizes).items():
            fit = linear_fit(series)
            if fit:
                lines.append(f"  fit {kind:<5}: {fit[0]:>6.0f} ms fixed + {fit[1] * 1000:6.1f} us/node")
    return "\n".join(lines)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--build-dir", required=True)
    ap.add_argument("--config", default="Bench")
    ap.add_argument("--results-dir", default=None, help="Catch2 XML directory (default <build-dir>/bench-results)")
    ap.add_argument("--out", default=None, help="Output HTML (default <results-dir>/report.html)")
    opts = ap.parse_args()

    results_dir = opts.results_dir or os.path.join(opts.build_dir, "bench-results")
    out = opts.out or os.path.join(results_dir, "report.html")

    cases: list[Case] = []
    sources: list[str] = []
    if os.path.isdir(results_dir):
        for name in sorted(os.listdir(results_dir)):
            if name.endswith(".xml"):
                path = os.path.join(results_dir, name)
                try:
                    cases.extend(parse_catch2_xml(path))
                    sources.append(path)
                except ET.ParseError as exc:
                    print(f"warning: skipping {path}: {exc}", file=sys.stderr)

    # Catch2 3 shuffles test cases by default; present them in a fixed order.
    order = list(CASE_NOTES)
    cases.sort(key=lambda c: (order.index(c.name) if c.name in order else len(order), c.name))

    trace_dir = os.path.join(opts.build_dir, ".trace")
    latest = ctr._latest_by_key(ctr._iter_compile_events(trace_dir)) if os.path.isdir(trace_dir) else {}
    sizes = sorted({n for (_k, n) in latest})
    if latest:
        sources.append(trace_dir)

    if not cases and not latest:
        print("error: no benchmark results found; run `make bench` and/or `make bench-compile`", file=sys.stderr)
        return 1

    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, "w", encoding="utf-8") as fh:
        fh.write(render(cases, latest, sizes, environment(opts.build_dir, opts.config), sources))
    print(text_summary(cases, latest, sizes))
    print(f"\nreport: {os.path.abspath(out)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
