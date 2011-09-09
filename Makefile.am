## Process this file with automake to produce Makefile.in

SUBDIRS = lib

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = $(LUA_INCLUDE)
AM_CFLAGS = $(WERROR_CFLAGS) $(WARN_CFLAGS)

EXTRA_LTLIBRARIES = curses_c.la
lualib_LTLIBRARIES = posix_c.la $(WANTEDLIBS)
dist_luadata_DATA = posix.lua $(WANTEDLUA)

posix_c_la_SOURCES = lposix.c
posix_c_la_CFLAGS = $(POSIX_EXTRA_CFLAGS)
posix_c_la_LDFLAGS = -module $(builddir)/lib/libgnu.la $(POSIX_EXTRA_LDFLAGS)

curses_c_la_SOURCES = lcurses.c
curses_c_la_LDFLAGS = -module $(CURSES_LIB) -rpath '$(lualibdir)'

ChangeLog:
	git2cl > ChangeLog

LUA_ENV = LUA_INIT= LUA_PATH="$(abs_srcdir)/?.lua;;" LUA_CPATH="$(abs_srcdir)/$(objdir)/?$(shrext);;"

HTML = lcurses.html lcurses_c.html

LUATESTS = tests-posix.lua tests-curses.lua

EXTRA_DIST = $(HTML) $(LUATESTS) strlcpy.c

$(HTML): lcurses.c make_lcurses_doc.pl
	$(PERL) make_lcurses_doc.pl

doc: $(HTML)

check-local:
	echo $(LUATESTS) | $(LUA_ENV) xargs $(LUA)

show-funcs:
	@echo "posix library:"
	@fgrep '/**' lposix.c | cut -f2 -d/ | tr -d '*' | sort

release: distcheck
	git diff --exit-code && \
	git push && \
	woger lua-l package=$(PACKAGE) package_name=$(PACKAGE_NAME) version="release $(VERSION)" description="Lua bindings for POSIX (including curses)" notes=release-notes-$(VERSION)
	git tag -a -m "Release tag" rel-`echo $(VERSION) | sed -e 's/\./-/g'` && \
	git push --tags
	@cat release-notes-$(VERSION) && echo "\n\nDon't forget to release on LuaForge!"
