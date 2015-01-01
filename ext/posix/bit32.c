/*
 * POSIX library for Lua 5.1, 5.2 & 5.3.
 * (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2015
 */
/***
 Bitwise operators.

@module posix.bit32
*/

#include <config.h>

#include <stdint.h>

#include "_helpers.c"


/***
Bitwise and operation.
@function band
@int ... 32-bit integers to act on
@treturn int bitwise and of all arguments, truncated to 32 bits
*/
static int
Pband(lua_State *L)
{
	int i, n = lua_gettop(L);
	uint32_t r = ~(uint32_t)0;
	for (i = 1; i <= n; ++i)
		r &= (uint32_t) optint(L, i, 0);
	return pushintresult(r);
}


/***
Bitwise not operation.
@function bnot
@int i 32-bit integer to act on
@treturn int bitwise negation of *i*
*/
static int
Pbnot(lua_State *L)
{
	checknargs(L, 1);
	return pushintresult (~(uint32_t) checkint(L, 1));
}


/***
Bitwise or operation.
@function bor
@int ... 32-bit integers to act on
@treturn int bitwise or of all arguments, truncated to 32 bits
*/
static int
Pbor(lua_State *L)
{
	int i, n = lua_gettop(L);
	uint32_t r = 0;
	for (i = 1; i <= n; ++i)
		r |= (uint32_t) optint(L, i, 0);
	return pushintresult(r);
}


static const luaL_Reg posix_bit32_fns[] =
{
	LPOSIX_FUNC( Pband		),
	LPOSIX_FUNC( Pbnot		),
	LPOSIX_FUNC( Pbor		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_bit32(lua_State *L)
{
	luaL_register(L, "posix.bit32", posix_bit32_fns);
	lua_pushliteral(L, "posix.bit32 for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
