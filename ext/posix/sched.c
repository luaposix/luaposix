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
/***
 Kernel Thread Scheduling Priority.

 Where supported by the underlying system, functions to discover and
 change the kernel thread scheduling priority.  If the module loads
 successfully, but there is no kernel support, then `posix.sched.version`
 will be set, but the unsupported APIs will be `nil`.
@module posix.sched
*/

/* cannot use unistd.h for _POSIX_PRIORITY_SCHEDULING, because on Linux
   glibc it is defined even though the APIs are not implemented :-(     */

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif

#include "_helpers.c"


/***
get scheduling policy
@function sched_getscheduler
@int[opt=0] pid process to act on, or `0` for caller process
@treturn[1] int scheduling policy `SCHED_FIFO`, `SCHED_RR`, `SCHED_OTHER`,
  if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see sched_getscheduler(2)
*/
#if HAVE_SCHED_GETSCHEDULER
static int
Psched_getscheduler(lua_State *L)
{
	struct sched_param sched_param  = {0};
	pid_t pid = (pid_t)optinteger(L, 1, 0);
	checknargs(L, 1);
	return pushresult(L, sched_getscheduler(pid), NULL);
}
#endif


/***
set scheduling policy/priority
@function sched_setscheduler
@int[opt=0] pid process to act on, or `0` for caller process
@int[opt=`SCHED_OTHER`] policy one of `SCHED_FIFO`, `SCHED_RR` or
  `SCHED_OTHER`
@int[opt=0] priority must be `0` for `SCHED_OTHER`, or else a positive
  number below 100 for real-time policies
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see sched_setscheduler(2)
*/
#if HAVE_SCHED_SETSCHEDULER
static int
Psched_setscheduler(lua_State *L)
{
	struct sched_param sched_param = {0};
	pid_t pid = (pid_t)optinteger(L, 1, 0);
	int policy = optint(L, 2, SCHED_OTHER);
	sched_param.sched_priority =  optint (L, 3, 0);
	checknargs(L, 3);
	return pushresult(L, sched_setscheduler(pid, policy, &sched_param), NULL);
}
#endif


static const luaL_Reg posix_sched_fns[] =
{
#if HAVE_SCHED_GETSCHEDULER
	LPOSIX_FUNC( Psched_getscheduler	),
#endif
#if HAVE_SCHED_SETSCHEDULER
	LPOSIX_FUNC( Psched_setscheduler	),
#endif
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Scheduler constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.sched
@int SCHED_FIFO  first-in, first-out scheduling policy
@int SCHED_RR round-robin scheduling policy
@int SCHED_OTHER another scheduling policy
@usage
  -- Print scheduler constants supported on this host.
  for name, value in pairs (require "posix.sched") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_sched(lua_State *L)
{
	luaL_newlib(L, posix_sched_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("sched"));
	lua_setfield(L, -2, "version");

	/* Psched_setscheduler flags */
#ifdef SCHED_FIFO
	LPOSIX_CONST( SCHED_FIFO	);
#endif
#ifdef SCHED_RR
	LPOSIX_CONST( SCHED_RR		);
#endif
#ifdef SCHED_OTHER
	LPOSIX_CONST( SCHED_OTHER	);
#endif

	return 1;
}
