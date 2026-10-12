# Trees, Transposition, and a new Container for C++29: open items

The revision-board and per-passage comment blocks were stripped from
`trees-and-transposition-for-29.org` on 2026-09-22 for publication. They
are in git history at the commit before that one. This file keeps only
what was still open.

## Open author decisions

- **"What I want" to "I can have".** Title and framing unchanged. Two
  voice suggestions: "as expressive as Python, Rust, Swift, or Haskell"
  would say more with two languages than four, and "excellent support"
  reads fine without the adjective.
- **What this post is** relative to the exhibit essay in `SPEC.md`: the
  map, or the teaser. If it is the teaser, say so in the close and link
  forward.
- **A standalone "why in the Standard" post** as well as the compact
  section: still open.

## Material available but unused

- **"Why in the Standard" gaps**, from the edit letter: `std::optional`
  as the anchor (everyone wrote their own, and that was the problem); the
  authoring-support pattern (iterator concept 1998, `std::iterator`
  removed, `iterator_interface` still in flight); one clause of safety
  tie-back; the regex objection answered by the scoping discipline; the
  gradient across the four papers (P3200 the pure case, P4325 the weak
  one). Do not use the P3294-is-C++29 timing argument.
- **Debug-build concession, not stratified.** Extra frames at `-O0` are
  the ranges and `std::execution` category; MSVC debug iterators are an
  ABI category. One clause saying which one this is.
- **Foldable `length` and the fingertree's cached measure.** Foldable's
  `length` defaults to O(n); the cached measure makes it O(1), and
  `Data.Sequence` overrides `length` and `null` for that reason. The
  fingertree paper is the motivating instance for the tree-algorithms
  paper's membership rule.
- `monad.hpp` in the transpose repo shows the mechanism carries Monad; a
  link is available for the When? paragraph if wanted.

## Verify before publishing

- **Private links.** Checked 2026-09-22: `github.com/steve-downey/backtick`
  and `github.com/steve-downey/llvm-project/tree/backtick` return 404
  anonymously. The gcc branch resolves. Make both public or drop them.
- **Paper numbers.** P3200, P4307, P4322, P4325 all return 404 on
  wg21.link as of 2026-09-22 (P2558 and P2011 resolve). Confirm P4307 is
  a real number and not penciled in.
- **Brno meeting date.** The post says "Spring 2026". Check the season.
- P2727 `iterator_interface` status against the latest mailing, if the
  standards argument gains the authoring-support paragraph.
- Runtime-parity godbolt check: derived instance vs flat aggregate, `-O2`
  and `-O0`.

## Publishing mechanics

- Exhibit tags `blog/trees-and-transposition-for-29` are on GitHub in all
  three source repos (checked 2026-09-22): transpose at bb32a37,
  tree_algorithms at da3d59f, fingertree at bb48221.
- In wctm's `plugins/orgmode/conf.el`, `orgit-file-transclusion-repo-alist`
  needs `/tree_algorithms/` and `/fingertree/` entries; transpose is
  already there.
- The `.html` and `.md` exports in this directory predate these edits.
  Re-export with the transpose repo's `papers/blog` toolchain.
- Header cleaned up 2026-09-22: one `toc:t`, one filled-in
  `#+DESCRIPTION:`, and `{{{TEASER_END}}}` is live after the four
  problems. The file defines the macro locally, and wctm's `init.el`
  defines it too, so it works in both places.
- A stale Emacs autosave, `#trees-and-transposition-for-29.org#` (dated
  2026-08-02), sits beside the source. It is older than the saved file.
