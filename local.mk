# Local Make rules.

## ------------ ##
## Environment. ##
## ------------ ##

std_path  = $(abs_srcdir)/?.lua;$(LUA_PATH)
std_cpath = $(abs_builddir)/$(objdir)/?$(shrext);$(LUA_CPATH)

LUA_ENV   = LUA_PATH="$(std_path)" LUA_CPATH="$(std_cpath)"


## ---------- ##
## Bootstrap. ##
## ---------- ##

AM_CFLAGS     = $(WERROR_CFLAGS) $(WARN_CFLAGS)
old_NEWS_hash = 3f835fe73525970ee6b423cc22d4b81b


## ------------- ##
## Declarations. ##
## ------------- ##

dist_data_DATA		=
dist_doc_DATA		=

include specs/specs.mk

## ------ ##
## Build. ##
## ------ ##

EXTRA_LTLIBRARIES	+= curses_c.la
lib_LTLIBRARIES		+= posix_c.la $(WANTEDLIBS)
dist_data_DATA		+= posix.lua $(WANTEDLUA)

posix_c_la_SOURCES = lposix.c
posix_c_la_CFLAGS  = $(POSIX_EXTRA_CFLAGS)
posix_c_la_LDFLAGS = -module -avoid-version $(POSIX_EXTRA_LDFLAGS)

curses_c_la_SOURCES = lcurses.c
curses_c_la_LDFLAGS = -module -avoid-version $(CURSES_LIB) -rpath '$(libdir)'


## -------------- ##
## Documentation. ##
## -------------- ##

dist_doc_DATA +=			\
	curses.html			\
	lcurses_c.html			\
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
	config.ld			\
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
	make_lcurses_doc.pl		\
	strlcpy.c			\
	lua52compat.h

$(dist_doc_DATA): lcurses.c make_lcurses_doc.pl
	$(PERL) make_lcurses_doc.pl
if HAVE_LDOC
	$(LDOC) .
else
	$(MKDIR_P) doc
	touch doc/index.html doc/ldoc.css
endif

MAINTAINERCLEANFILES +=			\
	doc/index.html			\
	doc/ldoc.css			\
	$(NOTHING_ELSE)
