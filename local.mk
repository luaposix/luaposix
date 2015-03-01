# Local Make rules.
# Written by Gary V. Vaughan, 2013

# Copyright (C) 2013-2015 Gary V. Vaughan

# This file is part of luaposix.
# See README for license.

## ------------ ##
## Environment. ##
## ------------ ##

posix_cpath = $(abs_builddir)/ext/posix/$(objdir)/?$(shrext)

std_cpath = $(posix_cpath);$(LUA_CPATH)
std_path  = $(abs_srcdir)/lib/?.lua;$(LUA_PATH)

LUA_ENV   = LUA_PATH="$(std_path)" LUA_CPATH="$(std_cpath)"


## ---------- ##
## Bootstrap. ##
## ---------- ##

AM_CPPFLAGS  += -I $(srcdir)/ext/include -I $(srcdir)/ext/posix $(POSIX_EXTRA_CPPFLAGS)
AM_CFLAGS     = $(WERROR_CFLAGS) $(WARN_CFLAGS)
AM_LDFLAGS    = -module -avoid-version

old_NEWS_hash = d41d8cd98f00b204e9800998ecf8427e

update_copyright_env = \
	UPDATE_COPYRIGHT_HOLDER='(Gary V. Vaughan|Reuben Thomas|luaposix authors)' \
	UPDATE_COPYRIGHT_USE_INTERVALS=1 \
	UPDATE_COPYRIGHT_FORCE=1


## ------------- ##
## Declarations. ##
## ------------- ##

examplesdir		= $(docdir)/examples
modulesdir		= $(docdir)/modules
classesdir		= $(docdir)/classes

dist_data_DATA		=
dist_doc_DATA		=
dist_examples_DATA	=
dist_modules_DATA	=
dist_classes_DATA	=

include specs/specs.mk


## ------ ##
## Build. ##
## ------ ##

dist_lua_DATA +=			\
	lib/curses.lua			\
	$(NOTHING_ELSE)

luaposixdir = $(luadir)/posix

dist_luaposix_DATA =			\
	lib/posix/init.lua		\
	lib/posix/_argcheck.lua		\
	lib/posix/compat.lua		\
	lib/posix/deprecated.lua	\
	lib/posix/sys.lua		\
	lib/posix/util.lua		\
	$(NOTHING_ELSE)

luaexec_LTLIBRARIES += ext/posix/posix.la

ext_posix_posix_la_SOURCES =		\
	ext/posix/posix.c		\
	$(NOTHING_ELSE)
EXTRA_ext_posix_posix_la_SOURCES =	\
	ext/posix/ctype.c		\
	ext/posix/curses.c		\
	ext/posix/curses/chstr.c	\
	ext/posix/curses/window.c	\
	ext/posix/dirent.c		\
	ext/posix/errno.c		\
	ext/posix/fcntl.c		\
	ext/posix/fnmatch.c		\
	ext/posix/getopt.c		\
	ext/posix/glob.c		\
	ext/posix/grp.c			\
	ext/posix/libgen.c		\
	ext/posix/poll.c		\
	ext/posix/pwd.c			\
	ext/posix/sched.c		\
	ext/posix/signal.c		\
	ext/posix/stdio.c		\
	ext/posix/stdlib.c		\
	ext/posix/sys/msg.c		\
	ext/posix/sys/resource.c	\
	ext/posix/sys/socket.c		\
	ext/posix/sys/stat.c		\
	ext/posix/sys/statvfs.c		\
	ext/posix/sys/time.c		\
	ext/posix/sys/times.c		\
	ext/posix/sys/utsname.c		\
	ext/posix/sys/wait.c		\
	ext/posix/syslog.c		\
	ext/posix/termio.c		\
	ext/posix/time.c		\
	ext/posix/unistd.c		\
	ext/posix/utime.c		\
	$(NOTHING_ELSE)

ext_posix_posix_la_LDFLAGS = $(AM_LDFLAGS) $(LIBCRYPT) $(LIBSOCKET) $(LIBRT) $(CURSES_LIB)

luaexecposixdir = $(luaexecdir)/posix
luaexecposixsysdir = $(luaexecposixdir)/sys

# EXTRA_LTLIBRARIES don't have an RPATH by default.
luaexecposix_LDFLAGS = $(AM_LDFLAGS) -rpath '$(luaexecposixdir)'
luaexecposixsys_LDFLAGS = $(AM_LDFLAGS) -rpath '$(luaexecposixsysdir)'


## ---------------------- ##
## Standalone Submodules. ##
## ---------------------- ##


# We don't install these, but we do need to make sure they compile
# for people who want to copy some of the sources into their own
# projects for custom interpreters/libraries.

posix_submodules =			\
	ext/posix/ctype.la		\
	ext/posix/curses.la		\
	ext/posix/curses/chstr.la	\
	ext/posix/curses/window.la	\
	ext/posix/dirent.la		\
	ext/posix/errno.la		\
	ext/posix/fcntl.la		\
	ext/posix/fnmatch.la		\
	ext/posix/getopt.la		\
	ext/posix/glob.la		\
	ext/posix/grp.la		\
	ext/posix/libgen.la		\
	ext/posix/poll.la		\
	ext/posix/pwd.la		\
	ext/posix/sched.la		\
	ext/posix/signal.la		\
	ext/posix/stdio.la		\
	ext/posix/stdlib.la		\
	ext/posix/sys/msg.la		\
	ext/posix/sys/resource.la	\
	ext/posix/sys/socket.la		\
	ext/posix/sys/stat.la		\
	ext/posix/sys/statvfs.la	\
	ext/posix/sys/time.la		\
	ext/posix/sys/times.la		\
	ext/posix/sys/utsname.la	\
	ext/posix/sys/wait.la		\
	ext/posix/syslog.la		\
	ext/posix/termio.la		\
	ext/posix/time.la		\
	ext/posix/unistd.la		\
	ext/posix/utime.la		\
	$(NOTHING_ELSE)

EXTRA_LTLIBRARIES += $(posix_submodules)
check_local += $(posix_submodules)

ext_posix_ctype_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_curses_la_LDFLAGS = $(luaexecposix_LDFLAGS) $(CURSES_LIB)
ext_posix_dirent_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_errno_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_fcntl_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_fnmatch_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_getopt_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_glob_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_grp_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_libgen_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_poll_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_pwd_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_sched_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_signal_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_stdio_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_stdlib_la_LDFLAGS = $(luaexecposix_LDFLAGS) $(LIBCRYPT)
ext_posix_syslog_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_termio_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_time_la_LDFLAGS = $(luaexecposix_LDFLAGS) $(LIBRT)
ext_posix_unistd_la_LDFLAGS = $(luaexecposix_LDFLAGS)
ext_posix_utime_la_LDFLAGS = $(luaexecposix_LDFLAGS)

ext_posix_sys_msg_la_LDFLAGS = $(luaexecposixsys_LDFLAGS)
ext_posix_sys_resource_la_LDFLAGS = $(luaexecposixsys_LDFLAGS)
ext_posix_sys_socket_la_LDFLAGS = $(luaexecposixsys_LDFLAGS)
ext_posix_sys_stat_la_LDFLAGS = $(luaexecposixsys_LDFLAGS)
ext_posix_sys_statvfs_la_LDFLAGS = $(luaexecposixsys_LDFLAGS)
ext_posix_sys_time_la_LDFLAGS = $(luaexecposixsys_LDFLAGS)
ext_posix_sys_times_la_LDFLAGS = $(luaexecposixsys_LDFLAGS)
ext_posix_sys_utsname_la_LDFLAGS = $(luaexecposixsys_LDFLAGS)
ext_posix_sys_wait_la_LDFLAGS = $(luaexecposixsys_LDFLAGS)


clean-local:
	rm -f $(posix_submodules)


## -------------- ##
## Documentation. ##
## -------------- ##

dist_doc_DATA +=			\
	doc/index.html			\
	doc/ldoc.css			\
	$(NOTHING_ELSE)

dist_modules_DATA +=				\
	doc/modules/posix.ctype.html		\
	doc/modules/posix.curses.html		\
	doc/modules/posix.dirent.html		\
	doc/modules/posix.errno.html		\
	doc/modules/posix.fcntl.html		\
	doc/modules/posix.fnmatch.html		\
	doc/modules/posix.getopt.html		\
	doc/modules/posix.glob.html		\
	doc/modules/posix.grp.html		\
	doc/modules/posix.html			\
	doc/modules/posix.libgen.html		\
	doc/modules/posix.poll.html		\
	doc/modules/posix.pwd.html		\
	doc/modules/posix.sched.html		\
	doc/modules/posix.signal.html		\
	doc/modules/posix.stdio.html		\
	doc/modules/posix.stdlib.html		\
	doc/modules/posix.sys.msg.html		\
	doc/modules/posix.sys.resource.html	\
	doc/modules/posix.sys.socket.html	\
	doc/modules/posix.sys.stat.html		\
	doc/modules/posix.sys.statvfs.html	\
	doc/modules/posix.sys.time.html		\
	doc/modules/posix.sys.times.html	\
	doc/modules/posix.sys.utsname.html	\
	doc/modules/posix.sys.wait.html		\
	doc/modules/posix.syslog.html		\
	doc/modules/posix.termio.html		\
	doc/modules/posix.time.html		\
	doc/modules/posix.unistd.html		\
	doc/modules/posix.utime.html		\
	$(NOTHING_ELSE)

dist_classes_DATA +=				\
	doc/classes/posix.curses.chstr.html	\
	doc/classes/posix.curses.window.html	\
	$(NOTHING_ELSE)

dist_examples_DATA +=				\
	doc/examples/curses.lua.html		\
	doc/examples/dir.lua.html		\
	doc/examples/fork.lua.html		\
	doc/examples/fork2.lua.html		\
	doc/examples/getopt.lua.html		\
	doc/examples/glob.lua.html		\
	doc/examples/limit.lua.html		\
	doc/examples/lock.lua.html		\
	doc/examples/netlink-uevent.lua.html	\
	doc/examples/ping.lua.html		\
	doc/examples/poll.lua.html		\
	doc/examples/signal.lua.html		\
	doc/examples/socket.lua.html		\
	doc/examples/termios.lua.html		\
	doc/examples/tree.lua.html		\
	$(NOTHING_ELSE)

allhtml = $(dist_doc_DATA) $(dist_examples_DATA) $(dist_modules_DATA) $(dist_classes_DATA)

$(allhtml): $(EXTRA_ext_posix_posix_la_SOURCES) $(ext_posix_posix_la_SOURCES)
	test -d $(builddir)/doc || mkdir $(builddir)/doc
if HAVE_LDOC
	$(LDOC) -c build-aux/config.ld -d $(abs_srcdir)/doc .
else
	$(MKDIR_P) doc
	touch doc/index.html doc/ldoc.css
endif

doc: $(allhtml)


## ------------- ##
## Distribution. ##
## ------------- ##

EXTRA_DIST +=				\
	build-aux/config.ld.in		\
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
	ext/include/_helpers.c		\
	ext/include/compat-5.2.c	\
	ext/include/compat-5.2.h	\
	ext/include/strlcpy.c		\
	$(NOTHING_ELSE)

MAINTAINERCLEANFILES +=			\
	doc/index.html			\
	doc/ldoc.css			\
	$(dist_examples_DATA)		\
	$(dist_modules_DATA)		\
	$(NOTHING_ELSE)
