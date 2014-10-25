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
 Time and Clock Functions.

@module posix.time
*/

#include <config.h>

#include <sys/time.h>
#include <time.h>

#include "_helpers.c"


static const char *Stm_fields[] = {
  "tm_sec", "tm_min", "tm_hour", "tm_mday", "tm_mon", "tm_year", "tm_wday",
  "tm_yday", "tm_isdst",
};

static void
totm(lua_State *L, int index, struct tm *t)
{
	luaL_checktype(L, index, LUA_TTABLE);
	t->tm_sec   = optintfield(L, index, "tm_sec", 0);
	t->tm_min   = optintfield(L, index, "tm_min", 0);
	t->tm_hour  = optintfield(L, index, "tm_hour", 0);
	t->tm_mday  = optintfield(L, index, "tm_mday", 0);
	t->tm_mon   = optintfield(L, index, "tm_mon", 0);
	t->tm_year  = optintfield(L, index, "tm_year", 0);
	t->tm_wday  = optintfield(L, index, "tm_wday", 0);
	t->tm_yday  = optintfield(L, index, "tm_yday", 0);
	t->tm_isdst = optintfield(L, index, "tm_isdst", 0);

	checkfieldnames(L, index, Stm_fields);
}


/***
Datetime record.
@table PosixTm
@int tm_sec second [0,60]
@int tm_min minute [0,59]
@int tm_hour hour [0,23]
@int tm_mday day of month [1, 31]
@int tm_mon month of year [0,11]
@int tm_year years since 1900
@int tm_wday day of week [0=Sunday,6]
@int tm_yday day of year [0,365[
@int tm_isdst 0 if daylight savings time is not in effect
*/
static int
pushtm(lua_State *L, struct tm *t)
{
	if (!t)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 9);
	setnumberfield(t, tm_sec);
	setnumberfield(t, tm_min);
	setnumberfield(t, tm_hour);
	setnumberfield(t, tm_mday);
	setnumberfield(t, tm_mday);
	setnumberfield(t, tm_mon);
	setnumberfield(t, tm_year);
	setnumberfield(t, tm_wday);
	setnumberfield(t, tm_yday);
	setnumberfield(t, tm_isdst);

	settypemetatable("PosixTm");
	return 1;
}


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


/***
Convert epoch time value to a broken-down UTC time.
@function gmtime
@int t seconds since epoch
@treturn PosixTm broken-down time
@see gmtime(3)
*/
static int
Pgmtime(lua_State *L)
{
	struct tm t;
	time_t epoch = checkint(L, 1);
	checknargs(L, 1);
	if (gmtime_r(&epoch, &t) == NULL)
		return pusherror(L, "gmtime");
	return pushtm(L, &t);
}


/***
Convert epoch time value to a broken-down local time.
@function localtime
@int t seconds since epoch
@treturn PosixTm broken-down time
@see localtime(3)
@see mktime
*/
static int
Plocaltime(lua_State *L)
{
	struct tm t;
	time_t epoch = checkint(L, 1);
	checknargs(L, 1);
	if (localtime_r(&epoch, &t) == NULL)
		return pusherror(L, "localtime");
	return pushtm(L, &t);
}


/***
Convert a broken-down localtime table into an epoch time.
@function mktime
@tparam PosixTm broken-down localtime
@treturn int seconds since epoch
@see mktime(3)
@see localtime
*/
static int
Pmktime(lua_State *L)
{
	struct tm t;
	time_t epoch;
	checknargs(L, 1);
	totm(L, 1, &t);
	if ((epoch = mktime(&t)) < 0)
		return 0;
	return pushintresult(epoch);
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
@string format specifier with `%` place-holders
@tparam PosixTm tm broken-down local time
@treturn string *format* with place-holders plugged with *tm* values
@see strftime(3)
*/
static int
Pstrftime(lua_State *L)
{
	char tmp[256];
	const char *fmt = luaL_checkstring(L, 1);
	struct tm t;

	totm(L, 2, &t);
	checknargs(L, 2);

	strftime(tmp, sizeof(tmp), fmt, &t);
	return pushstringresult(tmp);
}


/***
Parse a date string.
@function strptime
@string s
@string format same as for `strftime`
@usage posix.strptime('20','%d').monthday == 20
@treturn[1] PosixTm broken-down local time
@treturn[1] int next index of first character not parsed as part of the date
@return[2] nil
@see strptime(3)
*/
static int
Pstrptime(lua_State *L)
{
	struct tm t;
	const char *s = luaL_checkstring(L, 1);
	const char *fmt = luaL_checkstring(L, 2);
	char *r;
	checknargs(L, 2);

	memset(&t, 0, sizeof(struct tm));
	r = strptime(s, fmt, &t);
	if (r)
	{
		pushtm(L, &t);
		lua_pushinteger(L, r - s + 1);
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
	if ((time_t) -1 == t)
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
