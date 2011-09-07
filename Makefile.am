## Process this file with automake to produce Makefile.in

SUBDIRS = lib

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = $(LUA_INCLUDE)
AM_CFLAGS = $(WERROR_CFLAGS) $(WARN_CFLAGS) $(POSIX_EXTRA_CFLAGS)

lualib_LTLIBRARIES = posix_c.la
dist_luadata_DATA = posix.lua

posix_c_la_SOURCES = lposix.c
posix_c_la_LDFLAGS = -module $(builddir)/lib/libgnu.la $(POSIX_EXTRA_LDFLAGS)

ChangeLog:
	git2cl > ChangeLog

LUA_ENV = LUA_INIT= LUA_PATH="$(abs_srcdir)/?.lua;;" LUA_CPATH="$(abs_srcdir)/$(objdir)/?$(shrext);;"

check-local:
	$(LUA_ENV) $(LUA) tests.lua

release: distcheck
	git diff --exit-code && \
	git push && \
	woger lua-l package=$(PACKAGE) package_name=$(PACKAGE_NAME) version="release $(VERSION)" description="Lua bindings for posix" notes=release-notes-$(VERSION)
	git tag -a -m "Release tag" rel-`echo $(VERSION) | sed -e 's/\./-/g'` && \
	git push --tags
	@cat release-notes-$(VERSION) && echo "\n\nDon't forget to release on LuaForge!"

tree:
	$(LUA_ENV) $(LUA) -l$(PACKAGE) tree.lua .

# The --no-print-directory option is non-POSIX, works with GNU make.
debug:
	@$(MAKE) --no-print-directory "OPTFLAGS=-g"

show-funcs:
	@echo "$(MYNAME) library:"
	@fgrep '/**' l$(MYLIB).c | cut -f2 -d/ | tr -d '*' | sort
