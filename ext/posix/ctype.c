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
 Character tests.

@module posix.ctype
*/

#include <ctype.h>

#include "_helpers.c"


static int
bind_ctype(lua_State *L, int (*cb)(int))
{
	const char *s = luaL_checkstring(L, 1);
	char c = *s;
	checknargs(L, 1);
	lua_pop(L, 1);
	return pushintegerresult(cb((int)c));
}


/***
Check for any printable character except space.
@function isgraph
@string character to act on
@treturn int non-zero if character is not in the class
@see isgraph(3)
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
	luaL_newlib(L, posix_ctype_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("ctype"));
	lua_setfield(L, -2, "version");

	return 1;
}
