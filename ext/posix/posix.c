/*
 * POSIX library for Lua 5.1, 5.2, 5.3 & 5.4.
 * Copyright (C) 2013-2023 Gary V. Vaughan
 * Copyright (C) 2010-2013 Reuben Thomas <rrt@sc3d.org>
 * Copyright (C) 2008-2010 Natanael Copa <natanael.copa@gmail.com>
 * Clean up and bug fixes by Leo Razoumov <slonik.az@gmail.com> 2006-10-11
 * Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br> 07 Apr 2006 23:17:49
 * Based on original by Claudio Terra for Lua 3.x.
 * With contributions by Roberto Ierusalimschy.
 * With documentation from Steve Donovan 2012
 */

#include "ctype.c"
#include "dirent.c"
#include "errno.c"
#include "fcntl.c"
#include "fnmatch.c"
#include "glob.c"
#include "grp.c"
#include "libgen.c"
#include "poll.c"
#include "pwd.c"
#include "sched.c"
#include "signal.c"
#include "stdio.c"
#include "stdlib.c"
#include "sys/msg.c"
#include "sys/resource.c"
#include "sys/socket.c"
#include "sys/stat.c"
#include "sys/statvfs.c"
#include "sys/time.c"
#include "sys/times.c"
#include "sys/utsname.c"
#include "sys/wait.c"
#include "syslog.c"
#include "termio.c"
#include "time.c"
#include "unistd.c"
#include "utime.c"
