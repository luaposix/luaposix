## maintainer rules.

ME = GNUmakefile

# If the user runs GNU make but didn't ./configure yet, do it for them.
dont-forget-to-bootstrap = $(wildcard Makefile.in)

ifeq ($(dont-forget-to-bootstrap),)

%::
	@echo '$(ME): rebootstrap'
	@test -f Makefile.in || ./bootstrap
	@test -f Makefile || ./configure
	$(MAKE) $@

else


include build-aux/release.mk
include build-aux/sanity.mk

# Run sanity checks as part of distcheck.
distcheck: $(local-check)

## ------ ##
## Specl. ##
## ------ ##

# Use 'make check V=1' for verbose output, or set SPECL_OPTS to
# pass alternative options to specl command.

SPECL_OPTS     ?= $(specl_verbose_$(V))
specl_verbose_  = $(specl_verbose_$(AM_DEFAULT_VERBOSITY))
specl_verbose_0 =
specl_verbose_1 = --verbose --formatter=report


endif
