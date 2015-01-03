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
 Password Database Operations.

 Query the system password database.

@module posix.pwd
*/

#include <config.h>

#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>	/* for geteuid(2) */

#include "_helpers.c"


/***
Password record.
@table PosixPasswd
@string pw_name user's login name
@int pw_uid unique user id
@int pw_gid user's default group id
@string pw_dir initial working directory
@string pw_shell user's login shell path
*/

static int
pushpasswd(lua_State *L, struct passwd *p)
{
	if (!p)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 6);
	setintegerfield(p, pw_uid);
	setintegerfield(p, pw_gid);
	setstringfield(p, pw_name);
	setstringfield(p, pw_dir);
	setstringfield(p, pw_shell);
	setstringfield(p, pw_passwd);

	settypemetatable("PosixPasswd");
	return 1;
}


/***
Release password database resources.
@function endpwent
@see getpwent
*/
static int
Pendpwent(lua_State *L)
{
	checknargs(L, 0);
	endpwent();
	return 0;
}


/***
Fetch next password entry.
@function getpwent
@treturn PosixPasswd next password record
@see endpwent
@usage
  t = P.getpwent ()
  while t ~= nil do
    process (t)
    t = P.getpwent ()
  end
  P.endpwent ()
*/
static int
Pgetpwent(lua_State *L)
{
	struct passwd *p;
	checknargs(L, 0);
	p = getpwent();
	if (!p && errno == 0)
		endpwent();
	return pushpasswd(L, p);
}


/***
Fetch named user.
@function getpwnam
@string name user name
@treturn PosixPasswd passwd record for *name*
@usage
  t = P.getpwnam "root"
*/
static int
Pgetpwnam(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	struct passwd *p;
	checknargs(L, 1);

	errno = 0;	/* so we can recognise a successful empty result */
	p = getpwnam (name);
	if (!p && errno != 0)
		return pusherror(L, "getpwnam");
	return pushpasswd(L, p);
}


/***
Fetch password entry with given user id.
@function getpwuid
@int uid user id
@treturn PosixPasswd passwd record for *uid*
@usage
  t = P.getpwuid (0)
*/
static int
Pgetpwuid(lua_State *L)
{
	uid_t uid = (uid_t) checkint(L, 1);
	struct passwd *p;
	checknargs(L, 1);

	errno = 0;	/* so we can recognise a successful empty result */
	p = getpwuid(uid);
	if (!p && errno != 0)
		return pusherror(L, "getpwuid");
	return pushpasswd(L, p);
}


/***
Rewind next @{getpwent} back to start of database.
@function setpwent
@see getpwent
*/
static int
Psetpwent(lua_State *L)
{
	checknargs(L, 0);
	setpwent();
	return 0;
}


static const luaL_Reg posix_pwd_fns[] =
{
	LPOSIX_FUNC( Pendpwent		),
	LPOSIX_FUNC( Pgetpwent		),
	LPOSIX_FUNC( Pgetpwnam		),
	LPOSIX_FUNC( Pgetpwuid		),
	LPOSIX_FUNC( Psetpwent		),
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
