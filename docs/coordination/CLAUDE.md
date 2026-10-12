# CLAUDE.md — tree-coordination meta-repo

This repo coordinates five sibling checkouts (see README.md for the
map). It holds coordination artifacts only — no code.

Rules for working here:

- Read `STATUS.md` first; it is the work board. Keep it current: when an
  item completes in a source repo, move it to Done here.
- Every coordination decision gets a dated, append-only entry in
  `SYNC.md` pinning each repo's SHA and pending-file state.
- The canonical `tree-proposals-coherence` skill lives at
  `.claude/skills/tree-proposals-coherence/SKILL.md`. Per-repo copies
  are mirrors: edit here first, then copy outward. The skill's
  invariants (division of labor, naming, repo-surface ≠ paper-surface,
  DEV-01/03/04 test discipline) bind work in all six repos including
  this one.
- Cross-repo greps: the sources are the sibling checkouts under
  `~/src/{trees,transpose,fingertree,fixpoint,tree_algorithms}` — not
  mirrors inside this repo (none exist unless `mirror/` appears; if it
  does, treat it as read-only, edits flow through source repos).
- Do not edit source-repo files from this checkout without saying so in
  the commit/summary; each source repo has its own conventions
  (DECISIONS.md, DEVIATIONS.md, CODING_RULES.md) that govern there.
