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
 Process Times.

@module posix.sys.times
*/

#include <config.h>

#include <sys/times.h>

#include "_helpers.c"

#define pushtimefield(k,x) pushintegerfield((k), ((lua_Integer)x)/clk_tck)

/***
Process times record.
All times are measured in cpu seconds.
@table PosixTms
@int tms_utime time spent executing user instructions
@int tms_stime time spent in system execution
@int tms_cutime sum of *tms_utime* for calling process and its children
@int tms_cstime sum of *tms_stime* for calling process and its children
*/
static int
pushtms(lua_State *L)
{
	static long clk_tck = 0;

	struct tms t;
	clock_t elapsed = times(&t);

	if (elapsed == (clock_t) -1)
		return pusherror(L, "times");

	if (clk_tck == 0)
		clk_tck = sysconf(_SC_CLK_TCK);

	lua_createtable(L, 0, 5);

	/* record elapsed time, and save accounting to struct */
	pushtimefield("elapsed", elapsed);

	/* record struct entries */
	pushtimefield("tms_utime", t.tms_utime);
	pushtimefield("tms_stime", t.tms_stime);
	pushtimefield("tms_cutime", t.tms_cutime);
	pushtimefield("tms_cstime", t.tms_cstime);

	settypemetatable("PosixTms");
	return 1;
}

/***
Get the current process times.
@function times
@treturn[1] PosixTms time accounting for this process, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see times(2)
*/
static int
Ptimes(lua_State *L)
{
	checknargs(L, 0);
	return pushtms(L);
}


static const luaL_Reg posix_sys_times_fns[] =
{
	LPOSIX_FUNC( Ptimes		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_sys_times(lua_State *L)
{
	luaL_register(L, "posix.sys.times", posix_sys_times_fns);
	lua_pushliteral(L, "posix.sys.times for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
