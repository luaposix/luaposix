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
 Get System Identification.

@module posix.sys.utsname
*/

#include <sys/utsname.h>

#include "_helpers.c"


/***
System identification record.
@table utsname
@string machine hardware platform name
@string nodename network node name
@string release operating system release level
@string sysname operating system name
@string version operating system version
*/

static int
pushutsname(lua_State *L, struct utsname *u)
{
	if (!u)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 5);
	setstringfield(u, machine);
	setstringfield(u, nodename);
	setstringfield(u, release);
	setstringfield(u, sysname);
	setstringfield(u, version);

	settypemetatable("PosixUtsname");
	return 1;
}


/***
Return information about this machine.
@function uname
@treturn[1] utsname system information, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see uname(2)
*/
static int
Puname(lua_State *L)
{
	struct utsname u;
	checknargs(L, 0);
	if (uname(&u)==-1)
		return pusherror(L, "uname");
	return pushutsname(L, &u);
}


static const luaL_Reg posix_sys_utsname_fns[] =
{
	LPOSIX_FUNC( Puname		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_sys_utsname(lua_State *L)
{
	luaL_newlib(L, posix_sys_utsname_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("sys.utsname"));
	lua_setfield(L, -2, "version");

	return 1;
}
