/*
 * POSIX library for Lua 5.1, 5.2 & 5.3.
 * Copyright (C) 2013-2018 Gary V. Vaughan
 * Copyright (C) 2010-2013 Reuben Thomas <rrt@sc3d.org>
 * Copyright (C) 2008-2010 Natanael Copa <natanael.copa@gmail.com>
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

#include <glob.h>

#include "_helpers.c"


/***
Find all files in this directory matching a shell pattern.
@function glob
@string[opt="*"] pat shell glob pattern
@param flags currently limited to posix.glob.GLOB_MARK
@treturn table matching filenames
@see glob(3)
@see glob.lua
*/
static int
Pglob(lua_State *L)
{
	const char *pattern = optstring(L, 1, "*");
	int flags = checkint(L, 2);
	glob_t globres;

	checknargs(L, 2);
	if (glob(pattern, flags, NULL, &globres))
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


/***
Constants.
@section constants
*/

/***
Glob constants.
@table posix.glob
@int GLOB_MARK append slashes to matches that are directories.
@usage
  -- Print glob constants supported on this host.
  for name, value in pairs (require "posix.glob") do
    if type (value) == "number" then
      print (name, value)
    end
  end
*/


LUALIB_API int
luaopen_posix_glob(lua_State *L)
{
	luaL_newlib(L, posix_glob_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("glob"));
	lua_setfield(L, -2, "version");
	LPOSIX_CONST( GLOB_MARK );

	return 1;
}
