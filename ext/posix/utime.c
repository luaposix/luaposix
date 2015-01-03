/*
 * POSIX library for Lua 5.1, 5.2 & 5.3.
 * (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2015
 * (c) Reuben Thomas <rrt@sc3d.org> 2010-2013
 * (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
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

#include <config.h>

#include <time.h>
#include <utime.h>

#include "_helpers.c"


/***
Change file last access and modification times.
@function utime
@see utime(2)
@string path existing file path
@int[opt=now] mtime modification time
@int[opt=now] atime access time
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
*/
static int
Putime(lua_State *L)
{
	struct utimbuf times;
	time_t currtime = time(NULL);
	const char *path = luaL_checkstring(L, 1);
	times.modtime = optint(L, 2, currtime);
	times.actime  = optint(L, 3, currtime);
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
	luaL_register(L, "posix.utime", posix_utime_fns);
	lua_pushliteral(L, "posix.utime for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
