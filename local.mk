# Local Make rules.

## ------------ ##
## Environment. ##
## ------------ ##

curses_cpath = $(abs_builddir)/ext/curses/$(objdir)/?$(shrext)
posix_cpath = $(abs_builddir)/ext/posix/$(objdir)/?$(shrext)

std_cpath = $(curses_cpath);$(posix_cpath);$(LUA_CPATH)
std_path  = $(abs_srcdir)/lib/?.lua;$(LUA_PATH)

LUA_ENV   = LUA_PATH="$(std_path)" LUA_CPATH="$(std_cpath)"


## ---------- ##
## Bootstrap. ##
## ---------- ##

AM_CPPFLAGS  += -I $(srcdir)/ext/include
AM_CFLAGS     = $(WERROR_CFLAGS) $(WARN_CFLAGS)
old_NEWS_hash = 73b45ab8155db972364f61d1f2dc27f2


## ------------- ##
## Declarations. ##
## ------------- ##

dist_data_DATA		=
dist_doc_DATA		=

include specs/specs.mk

## ------ ##
## Build. ##
## ------ ##

EXTRA_LTLIBRARIES	+= ext/curses/curses_c.la
lib_LTLIBRARIES		+= ext/posix/posix_c.la $(WANTEDLIBS)
dist_data_DATA		+= lib/posix.lua $(WANTEDLUA)

ext_posix_posix_c_la_SOURCES =		\
	ext/posix/posix.c		\
	$(NOTHING_ELSE)
ext_posix_posix_c_la_CFLAGS  =		\
	$(POSIX_EXTRA_CFLAGS)
ext_posix_posix_c_la_LDFLAGS =		\
	-module -avoid-version $(POSIX_EXTRA_LDFLAGS)

ext_curses_curses_c_la_SOURCES =	\
	ext/curses/curses.c		\
	$(NOTHING_ELSE)
ext_curses_curses_c_la_CPPFLAGS =	\
	$(AM_CPPFLAGS) -I $(srcdir)/ext/curses
ext_curses_curses_c_la_LDFLAGS =	\
	-module -avoid-version $(CURSES_LIB) -rpath '$(libdir)'


## -------------- ##
## Documentation. ##
## -------------- ##

dist_doc_DATA +=			\
	doc/curses.html			\
	doc/curses_c.html		\
	doc/index.html			\
	doc/ldoc.css			\
	$(NOTHING_ELSE)

examplesdir = $(docdir)/examples
examples_DATA = $(wildcard examples/*.lua)

doc: $(dist_doc_DATA)


## ------------- ##
## Distribution. ##
## ------------- ##

EXTRA_DIST +=				\
	build-aux/make_lcurses_doc.pl	\
	examples/dir.lua		\
	examples/fork.lua		\
	examples/fork2.lua		\
	examples/getopt.lua		\
	examples/glob.lua		\
	examples/limit.lua		\
	examples/poll.lua		\
	examples/signal.lua		\
	examples/socket.lua		\
	examples/termios.lua		\
	ext/curses/strlcpy.c		\
	ext/include/lua52compat.h	\
	ext/posix/config.ld		\
	$(NOTHING_ELSE)

$(dist_doc_DATA): ext/curses/curses.c build-aux/make_lcurses_doc.pl
	test -d $(builddir)/doc || mkdir $(builddir)/doc
	$(PERL) build-aux/make_lcurses_doc.pl
if HAVE_LDOC
	$(LDOC) $(srcdir)/ext/posix
else
	$(MKDIR_P) doc
	touch doc/index.html doc/ldoc.css
endif

MAINTAINERCLEANFILES +=			\
	doc/index.html			\
	doc/ldoc.css			\
	$(NOTHING_ELSE)
