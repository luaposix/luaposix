## configure.ac
## Written by Gary V. Vaughan, 2012

## Copyright (c) 2012-2015 Gary V. Vaughan

## This file is part of luaposix.
## See README for license.
## ------------------------------------------------------------------------

AC_INIT([luaposix], [33.3.1], [http://github.com/luaposix/luaposix/issues])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AS_BOX([Configuring AC_PACKAGE_TARNAME AC_PACKAGE_VERSION])
echo

AM_INIT_AUTOMAKE([-Wall subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

## Check for programs
AC_PROG_CC
gl_EARLY
AC_PROG_MKDIR_P
AC_PROG_EGREP
AC_PROG_SED


## ------------------------------------------------------------------------
## Compiler flags for POSIX 2008 or better compliance.
## ------------------------------------------------------------------------

AC_USE_SYSTEM_EXTENSIONS
AC_CANONICAL_HOST
case $host_os in
  *nto*|*qnx*)
    # QNX is only POSIX 2001, but _XOPEN_SOURCE turns off other
    # functions luaposix can bind.
    AC_SUBST([POSIX_EXTRA_CPPFLAGS], [-DLPOSIX_2001_COMPLIANT])
    ;;

  darwin*|openbsd*)
    # Setting _XOPEN_SOURCE here messes up header declarations, but
    # all functions luaposix can bind are available without it.
    AC_SUBST([POSIX_EXTRA_CPPFLAGS], [-DLPOSIX_2008_COMPLIANT])
    ;;

  *)
    # Enable POSIX 2008.  Please send a $host_os match if your host
    # is not compliant.
    AC_SUBST([POSIX_EXTRA_CPPFLAGS], [-D_XOPEN_SOURCE=700])
    ;;
esac


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
  nw="$nw -Wsystem-headers"   # Don't let system headers trigger warnings
  nw="$nw -Wundef"            # All compiler preprocessors support #if UNDEF
  nw="$nw -Wtraditional"      # All compilers nowadays support ANSI C
  nw="$nw -Wstrict-overflow"  # Use a lower level (see below).
  nw="$nw -Wconversion"       # These warnings usually don't point to mistakes.
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

## required by automake 1.12.x, not available in 1.10.x
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

LT_PREREQ([2.2.6])
LT_INIT([disable-static])

module=yes eval shrext=$shrext_cmds

AC_SUBST([objdir])
AC_SUBST([shrext])
AC_SUBST(LIBTOOL_DEPS)

gl_INIT

## Check for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([crypt.h strings.h sys/statvfs.h])
AC_CHECK_HEADERS([net/if.h linux/netlink.h], [], [], [
#include <sys/socket.h>
])

## SysV messaging
AC_CHECK_HEADERS([sys/msg.h])
AC_CHECK_FUNCS([msgrcv msgsnd])
case $ac_cv_header_sys_msg_h,$ac_cv_func_msgrcv,$ac_cv_func_msgsnd in
  yes,yes,yes)
    AC_DEFINE([HAVE_SYSV_MESSAGING], [1], [Define if your system has SysV message APIs])
    ;;
esac


AC_CHECK_DECLS_ONCE([fdatasync])

## Search libs
save_LIBS=$LIBS
LIBS=
AC_SEARCH_LIBS([crypt], [crypt],
  [AC_DEFINE([HAVE_CRYPT], [1], [Define this if your system has a crypt() function])])
AC_SUBST([LIBCRYPT], [$LIBS])
LIBS=$save_LIBS

## Socket (required for qnx, at least)
save_LIBS=$LIBS
LIBS=
AC_SEARCH_LIBS([socket], [socket], [])
AC_SUBST([LIBSOCKET], [$LIBS])
LIBS=$save_LIBS

# FIXME: Make -lrt conditional on _XOPEN_REALTIME
save_LIBS=$LIBS
LIBS=
AC_CHECK_LIB([rt], [clock_gettime])
AC_SUBST([LIBRT], [$LIBS])
LIBS=$save_LIBS

## Curses
AX_WITH_CURSES
AC_ARG_VAR(CURSES_LIB, [linker flags for curses library])

save_LIBS=$LIBS
LIBS="$CURSES_LIB $LIBS"
AC_CHECK_FUNCS([resizeterm])
LIBS=$save_LIBS

## Use system implementation, or bundled replacement?
AC_CHECK_FUNCS([strlcpy])

## ------------------------------------------------------------------------
## Don't add more AC_CHECK_FUNCS, unless for a mainstream platform for one
## of the reasons below (to detect whether to enable an optional set of
## POSIX APIs; to disable a deprecated or very new and not yet widely
## supported API).  This is a POSIX binding, and requires a reasonably
## POSIX host to bind to - we definitely DON'T want to clutter up the code
## and configury to support partially compliant hosts!
## ------------------------------------------------------------------------

## Thread scheduling priority
AC_CHECK_FUNCS([sched_getsheduler sched_setscheduler])

## Statvfs support
AC_CHECK_FUNCS([statvfs])

## Deprecated or otherwise spottily supported POSIX functions
AC_CHECK_FUNCS([gethostid posix_fadvise])


## Lua
AX_PROG_LUA(5.1, 5.4)
AX_LUA_HEADERS
AC_PATH_PROG([SPECL], [specl], [:])

## Perl
AC_PATH_PROG([PERL], [perl])
if test -z "$PERL"; then
  AC_MSG_FAILURE([cannot find perl])
fi

## ldoc
AC_PATH_PROG([LDOC], [ldoc], [false])
AM_CONDITIONAL([HAVE_LDOC], [test false != "$LDOC"])

## Generate output files
AC_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile build-aux/config.ld lib/posix/init.lua:lib/posix.lua.in])
AC_OUTPUT
