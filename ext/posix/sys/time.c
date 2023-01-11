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
 Get and Set System Date and Time.

@module posix.sys.time
*/


#include <sys/time.h>

#include "_helpers.c"


/***
Time value.
@table PosixTimeval
@int tv_sec seconds elapsed
@int tv_usec remainder in microseconds
@see posix.time.PosixTimespec
*/
static int
pushtimeval(lua_State *L, struct timeval *tv)
{
	if (!tv)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 2);
	setintegerfield(tv, tv_sec);
	setintegerfield(tv, tv_usec);

	settypemetatable("PosixTimeval");
	return 1;
}


/***
Get time of day.
@function gettimeofday
@treturn[1] PosixTimeval time elapsed since *epoch*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see gettimeofday(2)
*/
static int
Pgettimeofday(lua_State *L)
{
	struct timeval tv;
	checknargs(L, 0);
	if (gettimeofday(&tv, NULL) == -1)
		return pusherror(L, "gettimeofday");

	return pushtimeval(L, &tv);
}


static const luaL_Reg posix_sys_time_fns[] =
{
	LPOSIX_FUNC( Pgettimeofday	),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_sys_time(lua_State *L)
{
	luaL_newlib(L, posix_sys_time_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("sys.time"));
	lua_setfield(L, -2, "version");

	return 1;
}
