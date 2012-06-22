/*
 * Lua 5.2 compatibility
 * (c) Reuben Thomas (maintainer) <rrt@sc3d.org> 2010-2012
 * This file is in the public domain.
 *
 * Include this file after Lua headers; before the headers, write
 * #define LUA_COMPAT_ALL
*/

#if LUA_VERSION_NUM == 502
static int luaL_typerror(lua_State *L, int narg, const char *tname)
{
	const char *msg = lua_pushfstring(L, "%s expected, got %s",
					  tname, luaL_typename(L, narg));
	return luaL_argerror(L, narg, msg);
}
#endif
