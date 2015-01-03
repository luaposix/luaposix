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
 Character tests.

@module posix.ctype
*/

#include <config.h>

#include <ctype.h>

#include "_helpers.c"


static int
bind_ctype(lua_State *L, int (*cb)(int))
{
	const char *s = luaL_checkstring(L, 1);
	char c = *s;
	checknargs(L, 1);
	lua_pop(L, 1);
	return pushintresult(cb((int)c));
}


/***
Check for any printable character except space.
@function isgraph
@see isgraph(3)
@string character to act on
@treturn int non-zero if character is not in the class
*/
static int
Pisgraph(lua_State *L)
{
	return bind_ctype(L, &isgraph);
}


/***
Check for any printable character including space.
@function isprint
@string character to act on
@treturn int non-zero if character is not in the class
@see isprint(3)
*/
static int
Pisprint(lua_State *L)
{
	return bind_ctype(L, &isprint);
}


static const luaL_Reg posix_ctype_fns[] =
{
	LPOSIX_FUNC( Pisgraph		),
	LPOSIX_FUNC( Pisprint		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_ctype(lua_State *L)
{
	luaL_register(L, "posix.ctype", posix_ctype_fns);
	lua_pushliteral(L, "posix.ctype for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
