dnl Process this file with autoconf to produce a configure script

AC_INIT(luaposix, 5.1.22, rrt@sc3d.org)
AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([-Wall -Werror std-options])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl Check for programs
AC_PROG_CC
gl_EARLY

AC_USE_SYSTEM_EXTENSIONS
AC_ARG_ENABLE([gcc-warnings],
  [AS_HELP_STRING([--enable-gcc-warnings],
                  [turn on lots of GCC warnings (for developers)])],
  [case $enableval in
     yes|no) ;;
     *)      AC_MSG_ERROR([bad value $enableval for gcc-warnings option]) ;;
   esac
   gl_gcc_warnings=$enableval],
  [gl_gcc_warnings=no]
)

if test "$gl_gcc_warnings" = yes; then
  gl_WARN_ADD([-Werror], [WERROR_CFLAGS])

  # Set up the list of undesired warnings.
  nw=
  nw="$nw -Wsystem-headers"   # Don’t let system headers trigger warnings
  nw="$nw -Wundef"            # All compiler preprocessors support #if UNDEF
  nw="$nw -Wtraditional"      # All compilers nowadays support ANSI C
  nw="$nw -Wstrict-overflow"  # Use a lower level (see below).
  nw="$nw -Wconversion"       # These warnings usually don’t point to mistakes.
  nw="$nw -Wsign-conversion"  # Likewise.
  nw="$nw -Waggregate-return" # We do this on purpose.
  nw="$nw -Wtraditional-conversion" # Don't care.
  nw="$nw -Wpadded"           # Don't care.
  nw="$nw -Wc++-compat"       # Don't care.
  nw="$nw -Woverlength-strings" # Don't care.
  nw="$nw -Wmissing-format-attribute" # Don't care.
  nw="$nw -Wunreachable-code" # Seems buggy.
  nw="$nw -Wunused-macros"    # those macros might come in handy later

  gl_MANYWARN_ALL_GCC([warnings])

  # Enable all GCC warnings not in this list.
  gl_MANYWARN_COMPLEMENT([warnings], [$warnings], [$nw])
  for w in $warnings; do
    gl_WARN_ADD([$w])
  done

  # Add an extra warning
  gl_WARN_ADD([-Wstrict-overflow=1])
  # Add some more safety measures
  gl_WARN_ADD([-D_FORTIFY_SOURCE=2])
  gl_WARN_ADD([-fmudflap])
fi

LT_PREREQ([2.2.6])
LT_INIT([disable-static])
LT_OUTPUT

eval `./libtool --config |grep '^objdir='`
eval `./libtool --config |grep '^shrext_cmds='`
module=yes eval shrext=$shrext_cmds

AC_SUBST([objdir])
AC_SUBST([shrext])
AC_SUBST(LIBTOOL_DEPS)

gl_INIT

dnl Check for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([crypt.h sys/statvfs.h])

dnl Search libs
AC_SEARCH_LIBS([crypt], [crypt],
  [AC_DEFINE([HAVE_CRYPT], [], [Define this if your system has a crypt() function])])

dnl Check for functions
AC_CHECK_FUNCS([strlcpy statvfs])

dnl Compiler flags for POSIX
dnl Get this from autotools/gnulib
if test Darwin != `uname`; then
  # FIXME: Make -lrt conditional on _XOPEN_REALTIME
  POSIX_EXTRA_CFLAGS=-D_XOPEN_SOURCE=700
  POSIX_EXTRA_LDFLAGS=-lrt
fi
AC_SUBST(POSIX_EXTRA_CFLAGS)
AC_SUBST(POSIX_EXTRA_LDFLAGS)

dnl Curses
AX_WITH_CURSES
if test "$ax_cv_curses" = "yes"; then
  WANTEDLIBS=curses_c.la
  WANTEDLUA=curses.lua
fi
AC_ARG_VAR(CURSES_LIB, [linker flags for curses library])
AC_SUBST(WANTEDLIBS)
AC_SUBST(WANTEDLUA)

dnl Lua 5.1 or 5.2
AX_PROG_LUA(501, 503)
AX_LUA_HEADERS
AX_LUA_HEADERS_VERSION(501, 503)
AC_SUBST(LUA)
AC_SUBST(LUA_INCLUDE)

dnl Perl
AX_WITH_PROG([PERL], [perl])
if test -z "$PERL"; then
  AC_MSG_FAILURE([cannot find perl])
fi

dnl md5sum
AX_WITH_PROG([MD5SUM], [md5sum], [false])

dnl Generate output files
AC_CONFIG_MACRO_DIR(m4)
AC_CONFIG_HEADER(config.h)
AC_SUBST(PACKAGE) # so that automake can dependencies for ${PACKAGE}.rockspec
AC_CONFIG_FILES(Makefile lib/Makefile ${PACKAGE}.rockspec)
AC_OUTPUT
