# Local Make rules.
# Written by Gary V. Vaughan, 2013

# Copyright (C) 2013-2014 Gary V. Vaughan

# This file is part of luaposix.
# See README for license.

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
old_NEWS_hash = 0892063d70673912ae06083cb59215a5

update_copyright_env = \
	UPDATE_COPYRIGHT_HOLDER='(Gary V. Vaughan|Reuben Thomas|luaposix authors)' \
	UPDATE_COPYRIGHT_USE_INTERVALS=1 \
	UPDATE_COPYRIGHT_FORCE=1


## ------------- ##
## Declarations. ##
## ------------- ##

examplesdir		= $(docdir)/examples
modulesdir		= $(docdir)/modules

dist_data_DATA		=
dist_doc_DATA		=
dist_examples_DATA	=
dist_modules_DATA	=

include specs/specs.mk


## ------ ##
## Build. ##
## ------ ##

EXTRA_LTLIBRARIES	+= ext/curses/curses_c.la
lib_LTLIBRARIES		+= ext/posix/posix_c.la $(WANTEDLIBS)

dist_lua_DATA +=			\
	lib/posix.lua			\
	$(WANTEDLUA)			\
	$(NOTHING_ELSE)

luaposixdir = $(luadir)/posix

dist_luaposix_DATA =			\
	lib/posix/sys.lua		\
	$(NOTHING_ELSE)

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

dist_modules_DATA +=			\
	doc/modules/posix.html		\
	doc/modules/posix.sys.html	\
	$(NOTHING_ELSE)

dist_examples_DATA +=			\
	doc/examples/dir.lua.html	\
	doc/examples/fork.lua.html	\
	doc/examples/fork2.lua.html	\
	doc/examples/getopt.lua.html	\
	doc/examples/glob.lua.html	\
	doc/examples/limit.lua.html	\
	doc/examples/lock.lua.html	\
	doc/examples/netlink-uevent.lua.html	\
	doc/examples/ping.lua.html	\
	doc/examples/poll.lua.html	\
	doc/examples/signal.lua.html	\
	doc/examples/socket.lua.html	\
	doc/examples/termios.lua.html	\
	doc/examples/tree.lua.html	\
	$(NOTHING_ELSE)

$(dist_doc_DATA): ext/curses/curses.c build-aux/make_lcurses_doc.pl
	test -d $(builddir)/doc || mkdir $(builddir)/doc
	$(PERL) build-aux/make_lcurses_doc.pl
if HAVE_LDOC
	$(LDOC) -c build-aux/config.ld -d $(abs_srcdir)/doc .
else
	$(MKDIR_P) doc
	touch doc/index.html doc/ldoc.css
endif

doc: $(dist_doc_DATA) $(dist_examples_DATA) $(dist_modules_DATA)


## ------------- ##
## Distribution. ##
## ------------- ##

EXTRA_DIST +=				\
	build-aux/config.ld.in		\
	build-aux/make_lcurses_doc.pl	\
	examples/dir.lua		\
	examples/fork.lua		\
	examples/fork2.lua		\
	examples/getopt.lua		\
	examples/glob.lua		\
	examples/limit.lua		\
	examples/lock.lua		\
	examples/netlink-uevent.lua	\
	examples/ping.lua		\
	examples/poll.lua		\
	examples/signal.lua		\
	examples/socket.lua		\
	examples/termios.lua		\
	examples/tree.lua		\
	ext/include/strlcpy.c		\
	ext/include/lua52compat.h	\
	$(NOTHING_ELSE)

MAINTAINERCLEANFILES +=			\
	doc/index.html			\
	doc/ldoc.css			\
	$(dist_examples_DATA)		\
	$(dist_modules_DATA)		\
	$(NOTHING_ELSE)
