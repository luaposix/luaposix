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
 Group Database Operations.

 Query the system group database.

@see posix.pwd
@module posix.grp
*/

#include <config.h>

#include <grp.h>

#include "_helpers.c"


/***
Group record.
@table PosixGroup
@string gr_name name of group
@int gr_gid unique group id
@tfield list gr_mem a list of group members
*/

static int
pushgroup(lua_State *L, struct group *g)
{
	if (!g)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 3);
	setintegerfield(g, gr_gid);
	setstringfield(g, gr_name);
	if (g->gr_mem)
	{
		int i;
		lua_newtable(L);
		for (i = 0; g->gr_mem[i] != NULL; i++)
		{
			lua_pushstring(L, g->gr_mem[i]);
			lua_rawseti(L, -2, i + 1);
		}
		lua_setfield(L, -2, "gr_mem");
	}

	settypemetatable("PosixGroup");
	return 1;
}


/***
Release group database resources.
@function endgrent
@see getgrent
*/
static int
Pendgrent(lua_State *L)
{
	checknargs(L, 0);
	endgrent();
	return 0;
}


/***
Fetch next group.
@function getgrent
@treturn PosixGroup next group record
@see endgrent
@usage
  t = P.getgrent ()
  while t ~= nil do
    process (t)
    t = P.getgrent ()
  end
  P.endgrent ()
*/
static int
Pgetgrent(lua_State *L)
{
	struct group *g;
	checknargs(L, 0);
	g = getgrent();
	if (!g && errno == 0)
		endgrent();
	return pushgroup(L, g);
}


/***
Fetch group with given group id.
@function getgrgid
@int gid group id
@treturn PosixGroup group record for *gid*
@usage
  t = P.getgrgid (0)
*/
static int
Pgetgrgid(lua_State *L)
{
	gid_t gid = (gid_t) checkint(L, 1);
	struct group *g;
	checknargs(L, 1);

	errno = 0;	/* so we can recognise a successful empty result */
	g = getgrgid(gid);
	if (!g && errno != 0)
		return pusherror(L, "getgrgid");
	return pushgroup(L, g);
}


/***
Fetch named group.
@function getgrnam
@string name group name
@treturn PosixGroup group record for *name*
@usage
  t = P.getgrnam "wheel"
*/
static int
Pgetgrnam(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct group *g;
	checknargs(L, 1);

	errno = 0;	/* so we can recognise a successful empty result */
	g = getgrnam (name);
	if (!g && errno != 0)
		return pusherror(L, "getgrnam");
	return pushgroup(L, g);
}


/***
Rewind next @{getgrent} back to start of database.
@function setgrent
@see getgrent
*/
static int
Psetgrent(lua_State *L)
{
	checknargs(L, 0);
	setgrent();
	return 0;
}


static const luaL_Reg posix_grp_fns[] =
{
	LPOSIX_FUNC( Pendgrent		),
	LPOSIX_FUNC( Pgetgrent		),
	LPOSIX_FUNC( Pgetgrgid		),
	LPOSIX_FUNC( Pgetgrnam		),
	LPOSIX_FUNC( Psetgrent		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_grp(lua_State *L)
{
	luaL_register(L, "posix.grp", posix_grp_fns);
	lua_pushliteral(L, "posix.grp for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
