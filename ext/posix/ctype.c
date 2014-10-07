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

#include <ctype.h>

#include "_helpers.h"


/***
Character Type Functions.
@section ctype
*/


static int
bind_ctype(lua_State *L, int (*cb)(int))
{
	const char *s = luaL_checkstring(L, 1);
	char c = *s;
	checknargs(L, 1);
	lua_pop(L, 1);
	lua_pushboolean(L, cb((int)c));
	return 1;
}


/***
Check for any printable character except space.
@function isgraph
@see isgraph(3)
@string character to act on
@treturn bool non-`false` if character is in the class
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
@treturn bool non-`false` if character is in the class
@see isprint(3)
*/
static int
Pisprint(lua_State *L)
{
	return bind_ctype(L, &isprint);
}
