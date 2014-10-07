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

#ifdef _POSIX_PRIORITY_SCHEDULING
#include <sched.h>
#endif
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <sys/wait.h>
#include <unistd.h>

#include "_helpers.h"


/***
Scheduler Functions.
@section sched
*/


#if defined _POSIX_PRIORITY_SCHEDULING && HAVE_SCHED_SETSCHEDULER && HAVE_SCHED_GETSCHEDULER
/***
set scheduling policy/priority
@function sched_setscheduler
@see sched_setscheduler(2)
@int[opt=0] pid process to act on, or `0` for caller process
@int[opt=`SCHED_OTHER`] policy one of `SCHED_FIFO`, `SCHED_RR` or
  `SCHED_OTHER`
@int[opt=0] priority must be `0` for `SCHED_OTHER`, or else a positive
  number below 100 for real-time policies
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
*/
static int
Psched_setscheduler(lua_State *L)
{
	struct sched_param sched_param = {0};
	pid_t pid = optint(L, 1, 0);
	int policy = optint(L, 2, SCHED_OTHER);
	sched_param.sched_priority =  optint (L, 3, 0);
	checknargs(L, 3);
	return pushresult(L, sched_setscheduler(pid, policy, &sched_param), NULL);
}

/***
get scheduling policy
@function sched_getscheduler
@int[opt=0] pid process to act on, or `0` for caller process
@treturn[1] int scheduling policy `SCHED_FIFO`, `SCHED_RR`, `SCHED_OTHER`,
  if successful
@return[2] nil
@treturn[2] string error message
@see sched_getscheduler(2)
*/
static int
Psched_getscheduler(lua_State *L)
{
	struct sched_param sched_param  = {0};
	pid_t pid = luaL_optint(L, 1, 0);
	checknargs(L, 1);
	return pushresult(L, sched_getscheduler(pid), NULL);
}
#endif


static void
sched_setconst(lua_State *L)
{
	/* Psched_setscheduler flags */
#ifdef SCHED_FIFO
	PCONST( SCHED_FIFO	);
#endif
#ifdef SCHED_RR
	PCONST( SCHED_RR	);
#endif
#ifdef SCHED_OTHER
	PCONST( SCHED_OTHER	);
#endif
}
