# Specl specs make rules.


## ------------ ##
## Environment. ##
## ------------ ##

SPECL_ENV = $(LUA_ENV)


## ------ ##
## Specs. ##
## ------ ##

specl_SPECS =					\
	$(srcdir)/specs/posix_lua_spec.yaml	\
	$(NOTHING_ELSE)

include build-aux/specl.mk
