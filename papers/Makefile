# papers/Makefile                                                 -*-makefile-*-
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# Papers are built with MPark.WG21, vendored as a squashed subtree under
# wg21/. Its flat layout expects the papers to sit beside the framework, which
# is what this directory is.
#
#   make D3200R0.html   # generated/D3200R0.html from D3200R0.md
#   make D3200R0.pdf
#   make                # every paper, in the default format
#
# defaults.yaml adds the wording transclusion filter; see
# ../docs/transpose/wording-pipeline.md.

include wg21/flat.mk

# Transcluded wording. A paper must rebuild when a fragment it transcludes
# changes, but the fragments cannot be its prerequisites directly: base.mk
# hands every `%.md` prerequisite to pandoc as a further input file. So each
# paper that transcludes gets a dependency-only file beside its outputs,
# $(OUTDIR)/<paper>.transcluded, whose prerequisites are the fragments the
# paper itself names and the filter that splices them; the paper's outputs
# depend on that file. It is remade exactly when one of them is newer, so a
# paper is out of date exactly when its wording is. Nothing else writes it.
transcluded_fragments = $(shell sed -n 's/.*{\.include file="\([^"]*\)".*/\1/p' $(1))

define transclusion_rule
ifneq ($$(strip $$(call transcluded_fragments,$(1))),)
$$(OUTDIR)/$(1:.md=.transcluded): $$(call transcluded_fragments,$(1)) filters/transclude.py | $$(OUTDIR)
	touch $$@
$$(OUTDIR)/$(1:.md=.html) $$(OUTDIR)/$(1:.md=.latex) $$(OUTDIR)/$(1:.md=.pdf): $$(OUTDIR)/$(1:.md=.transcluded)
endif
endef
$(foreach paper,$(src),$(eval $(call transclusion_rule,$(paper))))

.PHONY: clean-transcluded
clean-transcluded:
	rm -f $(OUTDIR)/*.transcluded

clean: clean-transcluded
