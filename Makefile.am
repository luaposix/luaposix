## Process this file with automake to produce Makefile.in

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = $(LUA_INCLUDE)
AM_CFLAGS = $(WERROR_CFLAGS) $(WARN_CFLAGS)

EXTRA_LTLIBRARIES = curses_c.la
lib_LTLIBRARIES = posix_c.la $(WANTEDLIBS)
dist_data_DATA = posix.lua $(WANTEDLUA)

posix_c_la_SOURCES = lposix.c
posix_c_la_CFLAGS = $(POSIX_EXTRA_CFLAGS)
posix_c_la_LDFLAGS = -module -avoid-version $(POSIX_EXTRA_LDFLAGS)

curses_c_la_SOURCES = lcurses.c
curses_c_la_LDFLAGS = -module -avoid-version $(CURSES_LIB) -rpath '$(libdir)'

ChangeLog:
	git2cl > ChangeLog

LUA_PATH ?= ;
LUA_CPATH ?= ;
LUA_ENV = LUA_PATH="$(abs_srcdir)/?.lua;$(LUA_PATH)" LUA_CPATH="$(abs_builddir)/$(objdir)/?$(shrext);$(LUA_CPATH)"

LUATESTS =				\
	$(srcdir)/tests-posix.lua	\
	$(srcdir)/tests-curses.lua

LUNITTESTS=				\
	$(srcdir)/tests-fcntl.lua

dist_doc_DATA = 			\
	curses.html			\
	lcurses_c.html			\
	docs/index.html			\
	docs/ldoc.css
examplesdir = $(docdir)/examples
examples_DATA = $(wildcard examples/*.lua)

EXTRA_DIST =				\
	$(LUATESTS)			\
	$(LUNITTESTS)			\
	make_lcurses_doc.pl		\
	strlcpy.c			\
	lua52compat.h

$(dist_doc_DATA): lcurses.c make_lcurses_doc.pl
	$(PERL) make_lcurses_doc.pl
if HAVE_LDOC
	$(LDOC) .
else
	$(MKDIR_P) docs
	touch docs/index.html docs/ldoc.css
endif

MAINTAINERCLEANFILES = docs/index.html docs/ldoc.css

doc: $(dist_doc_DATA)

check-local:
	ls $(LUATESTS) | $(LUA_ENV) xargs -L 1 $(LUA)
	ls $(LUNITTESTS) | $(LUA_ENV) xargs -L 1 lunit --interpreter $(LUA)

rockspecs:
	rm -f *.rockspec
	$(LUA) mkrockspecs.lua $(PACKAGE) $(VERSION)
	$(LUA) mkrockspecs.lua $(PACKAGE) git

tag-release:
	git diff --exit-code && \
	git tag -a -m "Release tag" v$(VERSION) && \
	git push && git push --tags

check-in-release: distcheck
	cd ../$(PACKAGE)-release && \
	tar zxf ../$(PACKAGE)/$(PACKAGE)-$(VERSION).tar.gz && \
	cp -af $(PACKAGE)-$(VERSION)/* . && \
	rm -rf $(PACKAGE)-$(VERSION)/ && \
	git add . && git ci -m "Release v$(VERSION)" && \
	git tag -a -m "Full source release tag" release-v$(VERSION) && \
	git push && git push --tags

release: rockspecs
	$(MAKE) tag-release && \
	$(MAKE) check-in-release && \
	LUAROCKS_CONFIG=$(abs_srcdir)/luarocks-config.lua luarocks --tree=$(abs_srcdir)/luarocks build $(PACKAGE)-$(VERSION)-1.rockspec && \
	woger lua package=$(PACKAGE) package_name=$(PACKAGE_NAME) version=$(VERSION) description="`LUA_INIT= LUA_PATH='$(abs_srcdir)/?-git-1.rockspec' $(LUA) -l$(PACKAGE) -e 'print (description.summary)'`" notes=release-notes-$(VERSION) home="`LUA_INIT= LUA_PATH='$(abs_srcdir)/?-git-1.rockspec' $(LUA) -l$(PACKAGE) -e 'print (description.homepage)'`"
