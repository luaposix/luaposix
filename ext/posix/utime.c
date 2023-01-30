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
 Set File Times.

@module posix.utime
*/

#include <time.h>
#include <utime.h>

#include "_helpers.c"


/***
Change file last access and modification times.
@function utime
@string path existing file path
@int[opt=now] mtime modification time
@int[opt=now] atime access time
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see utime(2)
*/
static int
Putime(lua_State *L)
{
	struct utimbuf times;
	time_t currtime = time(NULL);
	const char *path = luaL_checkstring(L, 1);
	times.modtime = (time_t)optinteger(L, 2, currtime);
	times.actime  = (time_t)optinteger(L, 3, currtime);
	checknargs(L, 3);
	return pushresult(L, utime(path, &times), path);
}


static const luaL_Reg posix_utime_fns[] =
{
	LPOSIX_FUNC( Putime		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_utime(lua_State *L)
{
	luaL_newlib(L, posix_utime_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("utime"));
	lua_setfield(L, -2, "version");

	return 1;
}
