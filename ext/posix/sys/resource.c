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
 Control Maximum System Resource Consumption.

@module posix.sys.resource
*/

#include <sys/resource.h>

#include "_helpers.c"

/* OpenBSD 5.6 recommends using RLIMIT_DATA in place of missing RLIMIT_AS */
#ifndef RLIMIT_AS
#  define RLIMIT_AS RLIMIT_DATA
#endif


/***
Resource limit record.
@table PosixRlimit
@int rlim_cur current soft limit
@int rlim_max hard limit
*/
static int
pushrlimit(lua_State *L, struct rlimit *lim)
{
	if (!lim)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 2);

	setintegerfield(lim, rlim_cur);
	setintegerfield(lim, rlim_max);

	settypemetatable("PosixRlimit");
	return 1;
}


/***
Get resource limits for this process.
@function getrlimit
@int resource one of `RLIMIT_CORE`, `RLIMIT_CPU`, `RLIMIT_DATA`, `RLIMIT_FSIZE`,
  `RLIMIT_NOFILE`, `RLIMIT_STACK` or `RLIMIT_AS`
@treturn[1] int softlimit
@treturn[1] int hardlimit, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see getrlimit(2)
@see setrlimit
@usage
  local sysres = require "posix.sys.resource"
  sysres.getrlimit(sysres.RLIMIT_NOFILE)
*/
static int
Pgetrlimit(lua_State *L)
{
	struct rlimit lim;
	int r;
	checknargs(L, 1);
	r = getrlimit(checkint(L, 1), &lim);
	if (r < 0)
		return pusherror(L, "getrlimit");
	return pushrlimit(L, &lim);
}


/***
Set a resource limit for subsequent child processes.
@function setrlimit
@int resource one of `RLIMIT_CORE`, `RLIMIT_CPU`, `RLIMIT_DATA`, `RLIMIT_FSIZE`,
  `RLIMIT_NOFILE`, `RLIMIT_STACK` or `RLIMIT_AS`
@param[opt] softlimit process may receive a signal when reached
@param[opt] hardlimit process may be terminated when reached
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see setrlimit(2)
@see getrlimit
@see limit.lua
@usage
  local sysres = require "posix.sys.resource"
  local lim = sysres.getlimit(sysres.RLIMIT_NOFILE)
  lim.rlim_cur = lim.rlim_cur / 2
  sysres.setrlimit(sysres.RLIMIT_NOFILE, lim)
*/

static const char *Srlimit_fields[] = { "rlim_cur", "rlim_max" };

static int
Psetrlimit(lua_State *L)
{
	struct rlimit lim;
	int rid = checkint(L, 1);

	luaL_checktype(L, 2, LUA_TTABLE);
	checknargs(L, 2);

	lim.rlim_cur = (rlim_t)checkintegerfield(L, 2, "rlim_cur");
	lim.rlim_max = (rlim_t)checkintegerfield(L, 2, "rlim_max");
	checkfieldnames(L, 2, Srlimit_fields);

	return pushresult(L, setrlimit(rid, &lim), "setrlimit");
}


static const luaL_Reg posix_sys_resource_fns[] =
{
	LPOSIX_FUNC( Pgetrlimit		),
	LPOSIX_FUNC( Psetrlimit		),
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Rlimit constants.
@table posix.sys.resource
@int RLIM_INFINITY unlimited resource usage
@int RLIM_SAVED_CUR saved current resource soft limit
@int RLIM_SAVED_MAX saved resource hard limit
@int RLIMIT_CORE maximum bytes allowed for a core file
@int RLIMIT_CPU maximum cputime secconds allowed per process
@int RLIMIT_DATA maximum data segment bytes per process
@int RLIMIT_FSIZE maximum bytes in any file
@int RLIMIT_NOFILE maximum number of open files per process
@int RLIMIT_STACK maximum stack segment bytes per process
@int RLIMIT_AS maximum bytes total address space per process
@usage
  -- Print resource constants supported on this host.
  for name, value in pairs (require "posix.sys.resource") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_sys_resource(lua_State *L)
{
	luaL_newlib(L, posix_sys_resource_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("sys.resource"));
	lua_setfield(L, -2, "version");

	LPOSIX_CONST( RLIM_INFINITY	);
#if defined RLIM_SAVED_CUR
	LPOSIX_CONST( RLIM_SAVED_CUR	);
#endif
#if defined RLIM_SAVED_MAX
	LPOSIX_CONST( RLIM_SAVED_MAX	);
#endif
	LPOSIX_CONST( RLIMIT_CORE	);
	LPOSIX_CONST( RLIMIT_CPU	);
	LPOSIX_CONST( RLIMIT_DATA	);
	LPOSIX_CONST( RLIMIT_FSIZE	);
	LPOSIX_CONST( RLIMIT_NOFILE	);
	LPOSIX_CONST( RLIMIT_STACK	);
	LPOSIX_CONST( RLIMIT_AS		);

	return 1;
}
