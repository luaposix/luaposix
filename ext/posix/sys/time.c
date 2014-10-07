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

#include <sys/time.h>

#include "_helpers.h"


/***
System Time Tables
@section timetables
*/

/***
Time value.
@table timeval
@int sec seconds elapsed
@int usec remainder in microseconds
*/



/***
System Time Functions.
@section systime
*/

/***
Get time of day.
@function gettimeofday
@treturn timeval time elapsed since *epoch*
@see gettimeofday(2)
*/
static int
Pgettimeofday(lua_State *L)
{
	struct timeval tv;
	checknargs(L, 0);
	if (gettimeofday(&tv, NULL) == -1)
		return pusherror(L, "gettimeofday");

	lua_newtable(L);
	lua_pushstring(L, "sec");
	lua_pushinteger(L, tv.tv_sec);
	lua_settable(L, -3);
	lua_pushstring(L, "usec");
	lua_pushinteger(L, tv.tv_usec);
	lua_settable(L, -3);
	return 1;
}
