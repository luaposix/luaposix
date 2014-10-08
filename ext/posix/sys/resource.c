/***
@module posix.sys.resource
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

#include <sys/resource.h>

#include "_helpers.c"


/* get/setrlimit */
#define rlimit_map \
	MENTRY( _CORE   ) \
	MENTRY( _CPU    ) \
	MENTRY( _DATA   ) \
	MENTRY( _FSIZE  ) \
	MENTRY( _NOFILE ) \
	MENTRY( _STACK  ) \
	MENTRY( _AS     )

static const int Krlimit[] =
{
#define MENTRY(_f) LPOSIX_SPLICE(RLIMIT, _f),
	rlimit_map
#undef MENTRY
	-1
};

static const char *const Srlimit[] =
{
#define MENTRY(_f) LPOSIX_STR_1(_f),
	rlimit_map
#undef MENTRY
	NULL
};


/***
Get resource limits for this process.
@function getrlimit
@string resource one of "core", "cpu", "data", "fsize", "nofile", "stack"
  or "as"
@treturn[1] int softlimit
@treturn[1] int hardlimit, if successful
@return[2] nil
@treturn[2] string error message
@see getrlimit(2)
@see setrlimit
*/
static int
Pgetrlimit(lua_State *L)
{
	struct rlimit lim;
	int rid, rc;
	const char *rid_str = luaL_checkstring(L, 1);
	checknargs(L, 1);
	if ((rid = lookup_symbol(Srlimit, Krlimit, rid_str)) < 0) /* FIXME: Use doselection. */
		luaL_argerror(L, 1, lua_pushfstring(L, "invalid option '%s'", rid_str));
	rc = getrlimit(rid, &lim);
	if (rc < 0)
		return pusherror(L, "getrlimit");
	lua_pushinteger(L, lim.rlim_cur);
	lua_pushinteger(L, lim.rlim_max);
	return 2;
}


/***
Set a resource limit for subsequent child processes.
@function setrlimit
@string resource one of "core", "cpu", "data", "fsize",
 "nofile", "stack" or "as"
@param[opt] softlimit process may receive a signal when reached
@param[opt] hardlimit process may be terminated when reached
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see getrlimit(2)
@see limit.lua
@usage P.setrlimit ("nofile", 1000, 2000)
*/
static int
Psetrlimit(lua_State *L)
{
	int softlimit;
	int hardlimit;
	const char *rid_str;
	int rid = 0;
	struct rlimit lim;
	struct rlimit lim_current;
	int rc;

	rid_str = luaL_checkstring(L, 1);
	softlimit = optint(L, 2, -1);
	hardlimit = optint(L, 3, -1);
	checknargs(L, 3);

	if ((rid = lookup_symbol(Srlimit, Krlimit, rid_str)) < 0)
		luaL_argerror(L, 1, lua_pushfstring(L, "invalid option '%s'", rid_str));

	if (softlimit < 0 || hardlimit < 0)
		if ((rc = getrlimit(rid, &lim_current)) < 0)
			return pushresult(L, rc, "getrlimit");

	if (softlimit < 0)
		lim.rlim_cur = lim_current.rlim_cur;
	else
		lim.rlim_cur = softlimit;
	if (hardlimit < 0)
		lim.rlim_max = lim_current.rlim_max;
	else lim.rlim_max = hardlimit;

	return pushresult(L, setrlimit(rid, &lim), "setrlimit");
}


static const luaL_Reg posix_sys_resource_fns[] =
{
	LPOSIX_FUNC( Pgetrlimit		),
	LPOSIX_FUNC( Psetrlimit		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_sys_resource(lua_State *L)
{
	luaL_register(L, "posix.sys.resource", posix_sys_resource_fns);
	lua_pushliteral(L, "posix.sys.resource for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
