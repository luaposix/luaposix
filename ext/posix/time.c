/***
@module posix.time
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

#include <sys/time.h>
#include <time.h>

#include "_helpers.c"


#if defined _XOPEN_REALTIME && _XOPEN_REALTIME != -1
static int
get_clk_id_const(const char *str)
{
	if (str == NULL)
		return CLOCK_REALTIME;
	else if (STREQ (str, "monotonic"))
		return CLOCK_MONOTONIC;
	else if (STREQ (str, "process_cputime_id"))
		return CLOCK_PROCESS_CPUTIME_ID;
	else if (STREQ (str, "thread_cputime_id"))
		return CLOCK_THREAD_CPUTIME_ID;
	else
		return CLOCK_REALTIME;
}


/***
Find the precision of a clock.
@function clock_getres
@string name name of clock, one of "monotonic", "process\_cputime\_id", or
"thread\_cputime\_id", or nil for realtime clock.
@treturn[1] int seconds
@treturn[1] int nanoseconds, if successful
@treturn[2] nil
@treturn[2] string error message
@see clock_getres(3)
*/
static int
Pclock_getres(lua_State *L)
{
	struct timespec res;
	const char *str = luaL_checkstring(L, 1);
	checknargs(L, 1);
	if (clock_getres(get_clk_id_const(str), &res) == -1)
		return pusherror(L, "clock_getres");
	lua_pushinteger(L, res.tv_sec);
	lua_pushinteger(L, res.tv_nsec);
	return 2;
}


/***
Read a clock.
@function clock_gettime
@string name name of clock, one of "monotonic", "process\_cputime\_id", or
"thread\_cputime\_id", or nil for realtime clock.
@treturn[1] int seconds
@treturn[1] int nanoseconds, if successful
@treturn[2] nil
@treturn[2] string error message
@see clock_gettime(3)
*/
static int
Pclock_gettime(lua_State *L)
{
	struct timespec res;
	const char *str = luaL_checkstring(L, 1);
	checknargs(L, 1);
	if (clock_gettime(get_clk_id_const(str), &res) == -1)
		return pusherror(L, "clock_gettime");
	lua_pushinteger(L, res.tv_sec);
	lua_pushinteger(L, res.tv_nsec);
	return 2;
}
#endif


static void
pushtm(lua_State *L, struct tm t)
{
	lua_createtable(L, 0, 10);
	lua_pushinteger(L, t.tm_sec);
	lua_setfield(L, -2, "sec");
	lua_pushinteger(L, t.tm_min);
	lua_setfield(L, -2, "min");
	lua_pushinteger(L, t.tm_hour);
	lua_setfield(L, -2, "hour");
	lua_pushinteger(L, t.tm_mday);
	lua_setfield(L, -2, "monthday");
	lua_pushinteger(L, t.tm_mday);
	lua_setfield(L, -2, "day");
	lua_pushinteger(L, t.tm_mon + 1);
	lua_setfield(L, -2, "month");
	lua_pushinteger(L, t.tm_year + 1900);
	lua_setfield(L, -2, "year");
	lua_pushinteger(L, t.tm_wday);
	lua_setfield(L, -2, "weekday");
	lua_pushinteger(L, t.tm_yday);
	lua_setfield(L, -2, "yearday");
	lua_pushboolean(L, t.tm_isdst);
	lua_setfield(L, -2, "is_dst");
}


/***
Convert UTC time in seconds to table.
@function gmtime
@see gmtime(3)
@int[opt=now] t time in seconds since epoch
@return time table as in `localtime`
*/
static int
Pgmtime(lua_State *L)
{
	struct tm res;
	time_t t = optint(L, 1, time(NULL));
	checknargs(L, 1);
	if (gmtime_r(&t, &res) == NULL)
		return pusherror(L, "localtime");
	pushtm(L, res);
	return 1;
}


/***
Convert time in seconds to table.
@function localtime
@see localtime(3)
@int[default=now] t time in seconds since epoch
@return time table: contains "is_dst","yearday","hour","min","year","month",
 "sec","weekday","monthday", "day" (the same as "monthday")
*/
static int
Plocaltime(lua_State *L)
{
	struct tm res;
	time_t t = optint(L, 1, time(NULL));
	checknargs(L, 1);
	if (localtime_r(&t, &res) == NULL)
		return pusherror(L, "localtime");
	pushtm(L, res);
	return 1;
}


/* N.B. In Lua broken-down time tables the month field is 1-based not
   0-based, and the year field is the full year, not years since
   1900. */

static void
totm(lua_State *L, int n, struct tm *tp)
{
	luaL_checktype(L, n, LUA_TTABLE);
	lua_getfield(L, n, "sec");
	tp->tm_sec = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "min");
	tp->tm_min = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "hour");
	tp->tm_hour = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	/* For compatibility to Lua os.date() read "day" if "monthday"
		 does not yield a number */
	lua_getfield(L, n, "monthday");
	if (!lua_isnumber(L, -1))
		lua_getfield(L, n, "day");
	tp->tm_mday = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "month");
	tp->tm_mon = luaL_optint(L, -1, 0) - 1;
	lua_pop(L, 1);
	lua_getfield(L, n, "year");
	tp->tm_year = luaL_optint(L, -1, 0) - 1900;
	lua_pop(L, 1);
	lua_getfield(L, n, "weekday");
	tp->tm_wday = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "yearday");
	tp->tm_yday = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "is_dst");
	tp->tm_isdst = (lua_type(L, -1) == LUA_TBOOLEAN) ? lua_toboolean(L, -1) : 0;
	lua_pop(L, 1);
}

/***
Convert a time table into a time value.
@function mktime
@param tm time table as in `localtime`
@return time in seconds since epoch
*/
static int
Pmktime(lua_State *L)
{
	struct tm t;
	time_t r;
	checknargs(L, 1);
	totm(L, 1, &t);
	r = mktime(&t);
	if (r == -1)
		return 0;
	return pushintresult(r);
}


/***
Sleep with nanosecond precision.
@function nanosleep
@int seconds
@int nanoseconds
@treturn[1] int `0` if requested time has elapsed
@return[2] nil
@treturn[2] string error message
@treturn[2] int errno
@treturn[2] int unslept seconds remaining, if interrupted
@treturn[2] int unslept nanoseconds remaining, if interrupted
@see nanosleep(2)
@see posix.unistd.sleep
*/
static int
Pnanosleep(lua_State *L)
{
	struct timespec req;
	struct timespec rem;
	int r;
	req.tv_sec = checkint(L, 1);
	req.tv_nsec = checkint(L, 2);
	checknargs(L, 2);
	r = pushresult (L, nanosleep(&req, &rem), NULL);
	if (r == 3 && errno == EINTR)
	{
		lua_pushinteger (L, rem.tv_sec);
		lua_pushinteger (L, rem.tv_nsec);
		r += 2;
	}
	return r;
}


/***
Write a time out according to a format.
@function strftime
@see strftime(3)
@param tm optional time table (e.g from `localtime`; default current time)
*/
static int
Pstrftime(lua_State *L)
{
	char tmp[256];
	const char *format = luaL_checkstring(L, 1);
	struct tm t;

	checknargs(L, 1);
	if (lua_isnil(L, 2))
	{
		time_t now = time(NULL);
		localtime_r(&now, &t);
	}
	else
		totm(L, 2, &t);

	strftime(tmp, sizeof(tmp), format, &t);
	return pushstringresult(tmp);
}

/***
Parse a date string.
@function strptime
@see strptime(3)
@string s
@string format same as for `strftime`
@usage posix.strptime('20','%d').monthday == 20
@return time table, like `localtime`
@return next index of first character not parsed as part of the date
*/
static int
Pstrptime(lua_State *L)
{
	struct tm t;
	const char *s = luaL_checkstring(L, 1);
	const char *fmt = luaL_checkstring(L, 2);
	char *ret;
	checknargs(L, 2);

	memset(&t, 0, sizeof(struct tm));
	ret = strptime(s, fmt, &t);
	if (ret)
	{
		pushtm(L, t);
		lua_pushinteger(L, ret - s);
		return 2;
	} else
		return 0;
}


/***
Get current time.
@function time
@see time(2)
@return time in seconds since epoch
*/
static int
Ptime(lua_State *L)
{
	time_t t = time(NULL);
	checknargs(L, 0);
	if ((time_t)-1 == t)
		return pusherror(L, "time");
	lua_pushinteger(L, t);
	return 1;
}


static const luaL_Reg posix_time_fns[] =
{
#if defined _XOPEN_REALTIME && _XOPEN_REALTIME != -1
	LPOSIX_FUNC( Pclock_getres	),
	LPOSIX_FUNC( Pclock_gettime	),
#endif
	LPOSIX_FUNC( Pgmtime		),
	LPOSIX_FUNC( Plocaltime		),
	LPOSIX_FUNC( Pmktime		),
	LPOSIX_FUNC( Pnanosleep		),
	LPOSIX_FUNC( Pstrftime		),
	LPOSIX_FUNC( Pstrptime		),
	LPOSIX_FUNC( Ptime		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_time(lua_State *L)
{
	luaL_register(L, "posix.time", posix_time_fns);
	lua_pushliteral(L, "posix.time for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
