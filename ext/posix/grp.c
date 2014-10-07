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

#include <grp.h>

#include "_helpers.h"


/***
Group Functions.
@section grp
*/


/***
Information about a group.
@function getgroup
@tparam int|string group id or group name
@treturn table `{name=name,gid=gid,0=member0,1=member1,...}`
@usage
print (P.getgroup (P.getpasswd ().gid).name)
*/
static int
Pgetgroup(lua_State *L)
{
	struct group *g = NULL;
	if (lua_isnumber(L, 1))
		g = getgrgid((gid_t)lua_tonumber(L, 1));
	else if (lua_isstring(L, 1))
		g = getgrnam(lua_tostring(L, 1));
	else
		luaL_typerror(L, 1, "string or int");
	checknargs(L, 1);
	if (g == NULL)
		lua_pushnil(L);
	else
	{
		int i;
		lua_newtable(L);
		lua_pushstring(L, g->gr_name);
		lua_setfield(L, -2, "name");
		lua_pushinteger(L, g->gr_gid);
		lua_setfield(L, -2, "gid");
		for (i = 0; g->gr_mem[i] != NULL; i++)
		{
			lua_pushstring(L, g->gr_mem[i]);
			lua_rawseti(L, -2, i + 1);
		}
	}
	return 1;
}
