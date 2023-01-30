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
 Time and Clock Functions.

@module posix.time
*/

#include <sys/time.h>
#include <time.h>

#include "_helpers.c"


static const char *Stimespec_fields[] = { "tv_sec", "tv_nsec" };

static void
totimespec(lua_State *L, int index, struct timespec *ts)
{
	luaL_checktype(L, index, LUA_TTABLE);
	ts->tv_sec  = (time_t)optintegerfield(L, index, "tv_sec", 0);
	ts->tv_nsec = optlongfield(L, index, "tv_nsec", 0);

	checkfieldnames(L, index, Stimespec_fields);
}


/***
Timespec record.
@table PosixTimespec
@int tv_sec seconds
@int tv_nsec nanoseconds
@see posix.sys.time.PosixTimeval
*/
static int
pushtimespec(lua_State *L, struct timespec *ts)
{
	if (!ts)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 2);
	setintegerfield(ts, tv_sec);
	setintegerfield(ts, tv_nsec);

	settypemetatable("PosixTimespec");
	return 1;
}

/* for compatibility, we accept tm_gmtoff and tm_zone as fields even
 * if the underlying OS doesn't. */
static const char *Stm_fields[] = {
  "tm_sec", "tm_min", "tm_hour", "tm_mday", "tm_mon", "tm_year", "tm_wday",
  "tm_yday", "tm_isdst", "tm_gmtoff", "tm_zone"
};

static void
totm(lua_State *L, int index, struct tm *t)
{
	static const struct tm nulltm;
	*t = nulltm;

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
#if HAVE_TM_GMTOFF
	t->tm_gmtoff = optintfield(L, index, "tm_gmtoff", 0);
#endif
#if HAVE_TM_ZONE
	t->tm_zone = optstringfield(L, index, "tm_zone", NULL);
#endif

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
	setintegerfield(t, tm_sec);
	setintegerfield(t, tm_min);
	setintegerfield(t, tm_hour);
	setintegerfield(t, tm_mday);
	setintegerfield(t, tm_mon);
	setintegerfield(t, tm_year);
	setintegerfield(t, tm_wday);
	setintegerfield(t, tm_yday);
	setintegerfield(t, tm_isdst);
#if HAVE_TM_GMTOFF
	setintegerfield(t, tm_gmtoff);
#endif
#if HAVE_TM_ZONE
	setstringfield(t, tm_zone);
#endif

	settypemetatable("PosixTm");
	return 1;
}


#if defined _POSIX_TIMERS && _POSIX_TIMERS != -1
/***
Find the precision of a clock.
@function clock_getres
@int clk name of clock, one of `CLOCK_REALTIME`, `CLOCK_PROCESS_CPUTIME_ID`,
  `CLOCK_MONOTONIC` or `CLOCK_THREAD_CPUTIME_ID`
@treturn[1] PosixTimespec resolution of *clk*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see clock_getres(3)
*/
static int
Pclock_getres(lua_State *L)
{
	struct timespec resolution;
	int clk = checkint(L, 1);
	checknargs(L, 1);
	if (clock_getres(clk, &resolution) == -1)
		return pusherror(L, "clock_getres");
	return pushtimespec(L, &resolution);
}


/***
Read a clock.
@function clock_gettime
@int clk name of clock, one of `CLOCK_REALTIME`, `CLOCK_PROCESS_CPUTIME_ID`,
  `CLOCK_MONOTONIC` or `CLOCK_THREAD_CPUTIME_ID`
@treturn[1] PosixTimespec current value of *clk*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see clock_gettime(3)
*/
static int
Pclock_gettime(lua_State *L)
{
	struct timespec ts;
	int clk = checkint(L, 1);
	checknargs(L, 1);
	if (clock_gettime(clk, &ts) == -1)
		return pusherror(L, "clock_gettime");
	return pushtimespec(L, &ts);
}
#endif


/***
Convert epoch time value to a broken-down UTC time.
@function gmtime
@int t seconds since epoch
@treturn[1] PosixTm broken-down time, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see gmtime(3)
*/
static int
Pgmtime(lua_State *L)
{
	struct tm t;
	time_t epoch = checklong(L, 1);
	checknargs(L, 1);
	if (gmtime_r(&epoch, &t) == NULL)
		return pusherror(L, "gmtime");
	return pushtm(L, &t);
}


/***
Convert epoch time value to a broken-down local time.
@function localtime
@int t seconds since epoch
@treturn[1] PosixTm broken-down time, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see localtime(3)
@see mktime
*/
static int
Plocaltime(lua_State *L)
{
	struct tm t;
	time_t epoch = checklong(L, 1);
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
	return pushintegerresult(epoch);
}


/***
Sleep with nanosecond precision.
@function nanosleep
@tparam PosixTimespec requested sleep time
@treturn[1] int `0` if requested time has elapsed, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@treturn[2] PosixTimespec unslept time remaining, if interrupted
@see nanosleep(2)
@see posix.unistd.sleep
*/
static int
Pnanosleep(lua_State *L)
{
	struct timespec req;
	struct timespec rem;
	int r;

	totimespec(L, 1, &req);
	checknargs(L, 1);
	r = pushresult (L, nanosleep(&req, &rem), "nanosleep");
	if (r == 3 && errno == EINTR)
		r = r + pushtimespec (L, &rem);
	return r;
}


/***
Return a time string according to *format*.
Note that if your host POSIX library provides a `strftime` that
assumes the local timezone, %z will always print the local UTC
offset, regardless of the `tm_gmoffset` field value passed in.
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
@treturn[1] int next index of first character not parsed as part of the date, if successful
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
@treturn[1] int time in seconds since epoch, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see time(2)
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
#if defined _POSIX_TIMERS && _POSIX_TIMERS != -1
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


/***
Constants.
@section constants
*/

/***
Standard constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.time
@int CLOCK_MONOTONIC the identifier for the system-wide monotonic clock
@int CLOCK_PROCESS_CPUTIME_ID the identifier for the current process CPU-time clock
@int CLOCK_REALTIME the identifier for the system-wide realtime clock
@int CLOCK_THREAD_CPUTIME_ID the identifier for the current thread CPU-time clock
@usage
  -- Print posix.time constants supported on this host.
  for name, value in pairs (require "posix.time") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_time(lua_State *L)
{
	luaL_newlib(L, posix_time_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("time"));
	lua_setfield(L, -2, "version");

#if defined CLOCK_MONOTONIC
	LPOSIX_CONST( CLOCK_MONOTONIC		);
#endif
#if defined CLOCK_PROCESS_CPUTIME_ID
	LPOSIX_CONST( CLOCK_PROCESS_CPUTIME_ID	);
#endif
#if defined CLOCK_REALTIME
	LPOSIX_CONST( CLOCK_REALTIME		);
#endif
#if defined CLOCK_THREAD_CPUTIME_ID
	LPOSIX_CONST( CLOCK_THREAD_CPUTIME_ID	);
#endif

	return 1;
}
