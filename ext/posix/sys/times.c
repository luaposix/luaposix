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
/***
 Process Times.

@module posix.sys.times
*/

#include <config.h>

#include <sys/times.h>
#include <unistd.h>	/* for sysconf(3) */

#include "_helpers.c"


struct mytimes
{
	struct tms t;
	clock_t elapsed;
};

#define pushtime(L,x)	lua_pushnumber(L, ((lua_Number)x)/clk_tck)



static void
Ftimes(lua_State *L, int i, const void *data)
{
	static long clk_tck = 0;
	const struct mytimes *t=data;

	if (!clk_tck)
		clk_tck= sysconf(_SC_CLK_TCK);
	switch (i)
	{
		case 0:	pushtime(L, t->t.tms_utime); break;
		case 1:	pushtime(L, t->t.tms_stime); break;
		case 2:	pushtime(L, t->t.tms_cutime); break;
		case 3:	pushtime(L, t->t.tms_cstime); break;
		case 4:	pushtime(L, t->elapsed); break;
	}
}

static const char *const Stimes[] =
{
	"utime", "stime", "cutime", "cstime", "elapsed", NULL
};

/***
Get the current process times.
@function times
@string ... field names, each one of "utime", "stime", "cutime", "cstime"
  or "elapsed"
@return ... times, or table of all times if no option given
@see times(2)
*/
static int
Ptimes(lua_State *L)
{
	struct mytimes t;
	t.elapsed = times(&t.t);
	return doselection(L, 1, Stimes, Ftimes, &t);
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
