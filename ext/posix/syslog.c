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
 Control System Log.

@module posix.syslog
*/

#include "_helpers.c"		/* For LPOSIX_2001_COMPLIANT */

#if LPOSIX_2001_COMPLIANT
#include <syslog.h>

/***
Open the system logger.
@function openlog
@string ident all messages will start with this
@int[opt] option bitwise OR of zero or more of `LOG_CONS`, `LOG_NDELAY`,
  or `LOG_PID`
@int[opt=`LOG_USER`] facility one of `LOG_AUTH`, `LOG_CRON`,
  `LOG_DAEMON`, `LOG_KERN`, `LOG_LPR`, `LOG_MAIL`, `LOG_NEWS`,
  `LOG_SECURITY`, `LOG_USER`, `LOG_UUCP` or `LOG_LOCAL0` through
  `LOG_LOCAL7`
@see openlog(3)
*/
static int
Popenlog(lua_State *L)
{
	const char *ident = luaL_checkstring(L, 1);
	int option = optint(L, 2, 0);
	int facility = optint(L, 3, LOG_USER);
	checknargs(L, 3);

	/* Save the ident string in our registry slot. */
	lua_pushlightuserdata(L, &Popenlog);
	lua_pushstring(L, ident);
	lua_rawset(L, LUA_REGISTRYINDEX);

	/* Use another copy of the same interned string for openlog(). */
	lua_pushstring(L, ident);
	openlog(lua_tostring(L, -1), option, facility);
	return 0;
}


/***
Write to the system logger.
@function syslog
@int priority one of `LOG_EMERG`, `LOG_ALERT`, `LOG_CRIT`, `LOG_WARNING`,
  `LOG_NOTICE`, `LOG_INFO` or `LOG_DEBUG`
@string message log message
@see syslog(3)
*/
static int
Psyslog(lua_State *L)
{
	int priority = checkint(L, 1);
	const char *msg = luaL_checkstring(L, 2);
	checknargs(L, 2);
	syslog(priority, "%s", msg);
	return 0;
}


/***
Close system log.
@function closelog
@see closelog(3)
*/
static int
Pcloselog(lua_State *L)
{
	checknargs(L, 0);
	closelog();
	return 0;
}


/***
Set log priority mask.
@function setlogmask
@int mask bitwise OR of @{LOG_MASK} bits.
@treturn[1] int previous mask, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see setlogmask(3)
*/
static int
Psetlogmask(lua_State *L)
{
	checknargs(L, 1);
	return pushresult(L, setlogmask(optint(L, 1, 0)), "setlogmask");
}


/***
Mask bit for given log priority.
@function LOG_MASK
@int priority one of `LOG_EMERG`, `LOG_ALERT`, `LOG_CRIT`, `LOG_WARNING`,
  `LOG_NOTICE`, `LOG_INFO` or `LOG_DEBUG`
@treturn int mask bit corresponding to *priority*
@see setlogmask(3)
*/
static int
PLOG_MASK(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(LOG_MASK(checkint(L, 1)));
}
#endif


static const luaL_Reg posix_syslog_fns[] =
{
#if LPOSIX_2001_COMPLIANT
	LPOSIX_FUNC( PLOG_MASK		),
	LPOSIX_FUNC( Popenlog		),
	LPOSIX_FUNC( Psyslog		),
	LPOSIX_FUNC( Pcloselog		),
	LPOSIX_FUNC( Psetlogmask	),
#endif
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
System logging constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.syslog
@int LOG_AUTH security/authorisation messages
@int LOG_CONS write directly to system console
@int LOG_CRON clock daemon
@int LOG_DAEMON system daemons
@int LOG_KERN kernel messages
@int LOG_LOCAL0 reserved for local use
@int LOG_LOCAL1 reserved for local use
@int LOG_LOCAL2 reserved for local use
@int LOG_LOCAL3 reserved for local use
@int LOG_LOCAL4 reserved for local use
@int LOG_LOCAL5 reserved for local use
@int LOG_LOCAL6 reserved for local use
@int LOG_LOCAL7 reserved for local use
@int LOG_LPR line printer subsystem
@int LOG_MAIL mail system
@int LOG_NDELAY open the connection immediately
@int LOG_NEWS network news subsystem
@int LOG_PID include process id with each log message
@int LOG_USER random user-level messages
@int LOG_UUCP unix-to-unix copy subsystem
@int LOG_EMERG system is unusable
@int LOG_ALERT action must be taken immediately
@int LOG_CRIT critical conditions
@int LOG_ERR error conditions
@int LOG_WARNING warning conditions
@int LOG_NOTICE normal but significant conditions
@int LOG_INFO informational
@int LOG_DEBUG debug-level messages
@usage
  -- Print syslog constants supported on this host.
  for name, value in pairs (require "posix.syslog") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_syslog(lua_State *L)
{
	luaL_newlib(L, posix_syslog_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("syslog"));
	lua_setfield(L, -2, "version");

#if LPOSIX_2001_COMPLIANT
	LPOSIX_CONST( LOG_CONS		);
	LPOSIX_CONST( LOG_NDELAY	);
	LPOSIX_CONST( LOG_PID		);

	LPOSIX_CONST( LOG_AUTH		);

	LPOSIX_CONST( LOG_CRON		);
	LPOSIX_CONST( LOG_DAEMON	);
	LPOSIX_CONST( LOG_KERN		);
	LPOSIX_CONST( LOG_LOCAL0	);
	LPOSIX_CONST( LOG_LOCAL1	);
	LPOSIX_CONST( LOG_LOCAL2	);
	LPOSIX_CONST( LOG_LOCAL3	);
	LPOSIX_CONST( LOG_LOCAL4	);
	LPOSIX_CONST( LOG_LOCAL5	);
	LPOSIX_CONST( LOG_LOCAL6	);
	LPOSIX_CONST( LOG_LOCAL7	);
	LPOSIX_CONST( LOG_LPR		);
	LPOSIX_CONST( LOG_MAIL		);
	LPOSIX_CONST( LOG_NEWS		);
	LPOSIX_CONST( LOG_USER		);
	LPOSIX_CONST( LOG_UUCP		);

	LPOSIX_CONST( LOG_EMERG		);
	LPOSIX_CONST( LOG_ALERT		);
	LPOSIX_CONST( LOG_CRIT		);
	LPOSIX_CONST( LOG_ERR		);
	LPOSIX_CONST( LOG_WARNING	);
	LPOSIX_CONST( LOG_NOTICE	);
	LPOSIX_CONST( LOG_INFO		);
	LPOSIX_CONST( LOG_DEBUG		);

	/* Remainder for backwards compatibility - not defined by POSIX */
#  ifdef LOG_AUTHPRIV
	LPOSIX_CONST( LOG_AUTHPRIV	);
#  endif
#  ifdef LOG_FTP
	LPOSIX_CONST( LOG_FTP		);
#  endif
#  ifdef LOG_SYSLOG
	LPOSIX_CONST( LOG_SYSLOG	);
#  endif
#endif

	return 1;
}
