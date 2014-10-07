/*
 * Lua 5.2 compatibility
 * (c) Reuben Thomas (maintainer) <rrt@sc3d.org> 2010-2012
 * This file is in the public domain.
 *
 * Include this file after Lua headers
*/

#ifndef LUA52COMPAT_H
#define LUA52COMPAT_H 1

#if LUA_VERSION_NUM == 502
static int luaL_typerror(lua_State *L, int narg, const char *tname)
{
        const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                          tname, luaL_typename(L, narg));
        return luaL_argerror(L, narg, msg);
}
#define lua_objlen lua_rawlen
#define lua_strlen lua_rawlen
#define luaL_openlib(L,n,l,nup) luaL_setfuncs((L),(l),(nup))
#define luaL_register(L,n,l) (luaL_newlib(L,l))
#endif

#endif
