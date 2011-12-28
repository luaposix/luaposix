/*
 * Lua 5.2 compatibility
 * (c) Reuben Thomas (maintainer) <rrt@sc3d.org> 2010-2011
 * This file is in the public domain.
*/

#if LUA_VERSION_NUM == 502
#define lua_objlen lua_rawlen

#include <assert.h>
static int luaL_typerror(lua_State *L, int narg, const char *tname)
{
	const char *msg = lua_pushfstring(L, "%s expected, got %s",
					  tname, luaL_typename(L, narg));
	return luaL_argerror(L, narg, msg);
}
#endif
