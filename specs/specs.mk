# Specl specs make rules.


## ------------ ##
## Environment. ##
## ------------ ##

SPECL_ENV +=					\
	LUA='$(LUA)'				\
	abs_top_builddir='$(abs_top_builddir)'	\
	abs_top_srcdir='$(abs_top_srcdir)'	\
	top_builddir='$(top_builddir)'		\
	top_srcdir='$(top_srcdir)'		\
	$(NOTHING_ELSE)


## ------ ##
## Specs. ##
## ------ ##

specl_SPECS =					\
	$(srcdir)/specs/curses_spec.yaml	\
	$(srcdir)/specs/lposix_spec.yaml	\
	$(srcdir)/specs/posix_spec.yaml		\
	$(NOTHING_ELSE)

EXTRA_DIST +=					\
	$(srcdir)/specs/spec_helper.lua		\
	$(NOTHING_ELSE)

include build-aux/specl.mk
