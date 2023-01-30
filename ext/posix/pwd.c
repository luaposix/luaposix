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
 Password Database Operations.

 Query the system password database.

@module posix.pwd
*/

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
@see endpwent(3)
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
@see getpwent(3)
@see endpwent
@usage
  local pwd = require "posix.pwd"
  t = pwd.getpwent ()
  while t ~= nil do
    process (t)
    t = pwd.getpwent ()
  end
  pwd.endpwent ()
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
@treturn[1] PosixPasswd passwd record for *name*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see getpwnam(3)
@usage
  local pwd = require "posix.pwd"
  t = pwd.getpwnam "root"
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
@treturn[1] PosixPasswd passwd record for *uid*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see getpwuid(3)
@usage
  local pwd = require "posix.pwd"
  t = pwd.getpwuid (0)
*/
static int
Pgetpwuid(lua_State *L)
{
	uid_t uid = (uid_t)checkinteger(L, 1);
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
@see setpwent(3)
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
	luaL_newlib(L, posix_pwd_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("pwd"));
	lua_setfield(L, -2, "version");

	return 1;
}
