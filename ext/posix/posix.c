/***
@module posix
*/
/*
 * POSIX library for Lua 5.1/5.2.
 * (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2014
 * (c) Reuben Thomas <rrt@sc3d.org> 2010-2013
 * (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
 * Clean up and bug fixes by Leo Razoumov <slonik.az@gmail.com> 2006-10-11
 * Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br> 07 Apr 2006 23:17:49
 * Based on original by Claudio Terra for Lua 3.x.
 * With contributions by Roberto Ierusalimschy.
 * With documentation from Steve Donovan 2012
 */

#include <config.h>

#include "_helpers.c"
#include "ctype.c"
#include "dirent.c"
#include "errno.c"
#include "fcntl.c"
#include "fnmatch.c"
#include "getopt.c"
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

#define MYNAME		"posix"
#define MYVERSION	MYNAME " library for " LUA_VERSION " / " VERSION


static const luaL_Reg R[] =
{
#define MENTRY(_s) {LPOSIX_STR_1(_s), (_s)}
	MENTRY( P_exit		),
	MENTRY( Pabort		),
	MENTRY( Paccess		),
	MENTRY( Pbasename	),
	MENTRY( Pchdir		),
	MENTRY( Pchmod		),
	MENTRY( Pchown		),
#if defined _XOPEN_REALTIME && _XOPEN_REALTIME != -1
	MENTRY( Pclock_getres	),
	MENTRY( Pclock_gettime	),
#endif
	MENTRY( Pclose		),
	MENTRY( Pcreat		),
#if defined HAVE_CRYPT
	MENTRY( Pcrypt		),
#endif
	MENTRY( Pctermid	),
	MENTRY( Pdirname	),
	MENTRY( Pdir		),
	MENTRY( Pdup		),
	MENTRY( Pdup2		),
	MENTRY( Perrno		),
	MENTRY( Pexec		),
	MENTRY( Pexecp		),
#if HAVE_POSIX_FADVISE
	MENTRY( Pfadvise	),
#endif
#if _POSIX_VERSION >= 200112L
	MENTRY( Pfdatasync	),
#endif
	MENTRY( Pfcntl		),
	MENTRY( Pfileno		),
	MENTRY( Pfiles		),
	MENTRY( Pfnmatch	),
	MENTRY( Pfork		),
	MENTRY( Pfsync		),
	MENTRY( Pgetcwd		),
	MENTRY( Pgetenv		),
	MENTRY( Pgetgroup	),
#if _POSIX_VERSION >= 200112L
	MENTRY( Pgetgroups	),
#endif
	MENTRY( Pgetlogin	),
	MENTRY( Pgetopt		),
	MENTRY( Pgetpasswd	),
	MENTRY( Pgetpid		),
	MENTRY( Pgetrlimit	),
#if defined _POSIX_PRIORITY_SCHEDULING && HAVE_SCHED_SETSCHEDULER && HAVE_SCHED_GETSCHEDULER
	MENTRY( Psched_setscheduler	),
	MENTRY( Psched_getscheduler	),
#endif
	MENTRY( Pgettimeofday	),
	MENTRY( Pglob		),
	MENTRY( Pgmtime		),
	MENTRY( Pgrantpt	),
	MENTRY( Phostid		),
	MENTRY( Pisatty		),
	MENTRY( Pisgraph	),
	MENTRY( Pisprint	),
	MENTRY( Pkill		),
	MENTRY( Pkillpg		),
	MENTRY( Plink		),
	MENTRY( Plocaltime	),
	MENTRY( Plseek		),
	MENTRY( Pmkdir		),
	MENTRY( Pmkfifo		),
	MENTRY( Pmkstemp	),
	MENTRY( Pmkdtemp	),
	MENTRY( Pmktime		),
	MENTRY( Pnice		),
	MENTRY( Popen		),
	MENTRY( Popenpt		),
	MENTRY( Ppathconf	),
	MENTRY( Ppipe		),
	MENTRY( Praise		),
	MENTRY( Pread		),
	MENTRY( Preadlink	),
	MENTRY( Prealpath	),
	MENTRY( Prmdir		),
	MENTRY( Prpoll		),
	MENTRY( Ppoll		),
	MENTRY( Pptsname	),
	MENTRY( Pset_errno	),
	MENTRY( Psetenv		),
	MENTRY( Psetpid		),
	MENTRY( Psetrlimit	),
	MENTRY( Psignal		),
	MENTRY( Psleep		),
	MENTRY( Pnanosleep	),
	MENTRY( Pmsgget		),
	MENTRY( Pmsgsnd		),
	MENTRY( Pmsgrcv		),
	MENTRY( Pstat		),
	MENTRY( Plstat		),
	MENTRY( Pstrftime	),
	MENTRY( Pstrptime	),
	MENTRY( Psync		),
	MENTRY( Psysconf	),
	MENTRY( Ptime		),
	MENTRY( Ptimes		),
	MENTRY( Pttyname	),
	MENTRY( Punlink		),
	MENTRY( Punlockpt	),
	MENTRY( Pumask		),
	MENTRY( Puname		),
	MENTRY( Putime		),
	MENTRY( Pwait		),
	MENTRY( Pwrite		),

	MENTRY( Ptcsetattr	),
	MENTRY( Ptcgetattr	),
	MENTRY( Ptcsendbreak	),
	MENTRY( Ptcdrain	),
	MENTRY( Ptcflush	),
	MENTRY( Ptcflow		),

#if _POSIX_VERSION >= 200112L
	MENTRY( Psocket		),
	MENTRY( Psocketpair	),
	MENTRY( Pgetaddrinfo	),
	MENTRY( Pconnect	),
	MENTRY( Pbind		),
	MENTRY( Plisten		),
	MENTRY( Paccept		),
	MENTRY( Precv		),
	MENTRY( Precvfrom	),
	MENTRY( Psend		),
	MENTRY( Psendto		),
	MENTRY( Pshutdown	),
	MENTRY( Psetsockopt	),
#endif

#if _POSIX_VERSION >= 200112L
	MENTRY( Popenlog	),
	MENTRY( Psyslog		),
	MENTRY( Pcloselog	),
	MENTRY( Psetlogmask	),
#endif
#if defined HAVE_STATVFS
	MENTRY( Pstatvfs	),
#endif
#undef MENTRY
	{NULL,	NULL}
};


LUALIB_API int
luaopen_posix_c(lua_State *L)
{
	luaL_register(L, MYNAME, R);

	lua_pushliteral(L, MYVERSION);
	lua_setfield(L, -2, "version");

	errno_setconst(L);
	fcntl_setconst(L);
	fnmatch_setconst(L);
	sched_setconst(L);
	signal_setconst(L);
	stdio_setconst(L);
	sysmsg_setconst(L);
	syssocket_setconst(L);
	syswait_setconst(L);
	syslog_setconst(L);
	termio_setconst(L);
	unistd_setconst(L);

	return 1;
}
