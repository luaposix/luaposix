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
 Generate pathnames matching a shell-style pattern.

 Functions generating a table of filenames that match a shell-style
 pattern string.

@module posix.glob
*/

#include <config.h>

#include <glob.h>

#include "_helpers.c"


/***
Find all files in this directory matching a shell pattern.
@function glob
@string[opt="*"] pat shell glob pattern
@treturn table matching filenames
@see glob(3)
@see glob.lua
*/
static int
Pglob(lua_State *L)
{
	const char *pattern = optstring(L, 1, "*");
	glob_t globres;

	checknargs(L, 1);
	if (glob(pattern, 0, NULL, &globres))
		return pusherror(L, pattern);
	else
	{
		unsigned int i;
		lua_newtable(L);
		for (i=1; i<=globres.gl_pathc; i++)
		{
			lua_pushstring(L, globres.gl_pathv[i-1]);
			lua_rawseti(L, -2, i);
		}
		globfree(&globres);
		return 1;
	}
}


static const luaL_Reg posix_glob_fns[] =
{
	LPOSIX_FUNC( Pglob		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_glob(lua_State *L)
{
	luaL_register(L, "posix.glob", posix_glob_fns);
	lua_pushliteral(L, "posix.glob for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
