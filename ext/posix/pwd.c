/***
@module posix.pwd
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

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>	/* for geteuid(2) */

#include "_helpers.c"


static void
Fgetpasswd(lua_State *L, int i, const void *data)
{
	const struct passwd *p=data;
	switch (i)
	{
		case 0:
			lua_pushstring(L, p->pw_name);
			break;
		case 1:
			lua_pushinteger(L, p->pw_uid);
			break;
		case 2:
			lua_pushinteger(L, p->pw_gid);
			break;
		case 3:
			lua_pushstring(L, p->pw_dir);
			break;
		case 4:
			lua_pushstring(L, p->pw_shell);
			break;
		case 5:
			lua_pushstring(L, p->pw_passwd);
			break;
	}
}

static const char *const Sgetpasswd[] =
{
	"name", "uid", "gid", "dir", "shell", "passwd", NULL
};


/***
Get the password entry for a user.
@function getpasswd
@tparam[opt=current user] string|int user name or id
@string ... field names, each one of "uid", "name", "gid", "passwd", "dir", "shell"
@return ... values, or table of all fields if *user* is `nil`
@usage for a,b in pairs(P.getpasswd("root")) do print(a,b) end
@usage print(P.getpasswd("root", "shell"))
*/
static int
Pgetpasswd(lua_State *L)
{
	struct passwd *p=NULL;
	if (lua_isnoneornil(L, 1))
		p = getpwuid(geteuid());
	else if (lua_isnumber(L, 1))
		p = getpwuid((uid_t)lua_tonumber(L, 1));
	else if (lua_isstring(L, 1))
		p = getpwnam(lua_tostring(L, 1));
	else
		luaL_typerror(L, 1, "string, int or nil");
	if (p==NULL)
		lua_pushnil(L);
	else
		return doselection(L, 2, Sgetpasswd, Fgetpasswd, p);
	return 1;
}


static const luaL_Reg posix_pwd_fns[] =
{
	LPOSIX_FUNC( Pgetpasswd		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_pwd(lua_State *L)
{
	luaL_register(L, "posix.pwd", posix_pwd_fns);
	lua_pushliteral(L, "posix.pwd for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
