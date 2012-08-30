## Process this file with automake to produce Makefile.in

SUBDIRS = lib

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = $(LUA_INCLUDE)
AM_CFLAGS = $(WERROR_CFLAGS) $(WARN_CFLAGS)

EXTRA_LTLIBRARIES = curses_c.la
lib_LTLIBRARIES = posix_c.la $(WANTEDLIBS)
dist_data_DATA = posix.lua $(WANTEDLUA)

posix_c_la_SOURCES = lposix.c
posix_c_la_CFLAGS = $(POSIX_EXTRA_CFLAGS)
posix_c_la_LDFLAGS = -module -avoid-version $(builddir)/lib/libgnu.la $(POSIX_EXTRA_LDFLAGS)

curses_c_la_SOURCES = lcurses.c
curses_c_la_LDFLAGS = -module -avoid-version $(CURSES_LIB) -rpath '$(libdir)'

ChangeLog:
	git2cl > ChangeLog

LUA_PATH ?= ;
LUA_CPATH ?= ;
LUA_ENV = LUA_PATH="$(abs_srcdir)/?.lua;$(LUA_PATH)" LUA_CPATH="$(abs_builddir)/$(objdir)/?$(shrext);$(LUA_CPATH)"

HTML = curses.html lcurses_c.html

LUATESTS =				\
	$(srcdir)/tests-posix.lua	\
	$(srcdir)/tests-curses.lua

LUNITTESTS=				\
	$(srcdir)/tests-fcntl.lua

ROCKSPEC = $(PACKAGE)-$(VERSION)-1.rockspec

$(ROCKSPEC): $(PACKAGE).rockspec dist
	sed -e 's/@MD5@/'`$(MD5SUM) $(distdir).tar.gz | cut -d " " -f 1`'/g' < $(PACKAGE).rockspec > $@

EXTRA_DIST =				\
	$(HTML)				\
	$(LUATESTS)			\
	$(LUNITTESTS)			\
	make_lcurses_doc.pl		\
	$(PACKAGE).rockspec.in		\
	strlcpy.c			\
	lua52compat.h

DISTCLEANFILES = $(PACKAGE).rockspec

$(HTML): lcurses.c make_lcurses_doc.pl
	$(PERL) make_lcurses_doc.pl

doc: $(HTML)
	ldoc .

check-local:
	ls $(LUATESTS) | $(LUA_ENV) xargs -L 1 $(LUA)
	ls $(LUNITTESTS) | $(LUA_ENV) xargs -L 1 lunit --interpreter $(LUA)

release: distcheck $(ROCKSPEC)
	git diff --exit-code && \
	git push && \
	woger lua,github package=$(PACKAGE) package_name=$(PACKAGE_NAME) version=$(VERSION) description="`LUA_INIT= LUA_PATH='$(abs_srcdir)/?.rockspec.in' $(LUA) -l$(PACKAGE) -e 'print (description.summary)'`" notes=release-notes-$(VERSION) dist_type="tar.gz" github_user=rrthomas && \
	git tag -a -m "Release tag" v$(VERSION) && \
	git push --tags
