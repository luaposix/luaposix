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
 Get System Identification.

@module posix.sys.utsname
*/

#include <config.h>

#include <sys/utsname.h>

#include "_helpers.c"


/***
Return information about this machine.
@function uname
@see uname(2)
@string[opt="%s %n %r %v %m"] format contains zero or more of:

 * %m  machine name
 * %n  node name
 * %r  release
 * %s  sys name
 * %v  version

@treturn[1] string filled *format* string, if successful
@return[2] nil
@treturn string error message
*/
static int
Puname(lua_State *L)
{
	struct utsname u;
	luaL_Buffer b;
	const char *s;
	checknargs(L, 1);
	if (uname(&u)==-1)
		return pusherror(L, NULL);
	luaL_buffinit(L, &b);
	for (s=optstring(L, 1, "%s %n %r %v %m"); *s; s++)
		if (*s!='%')
			luaL_addchar(&b, *s);
		else switch (*++s)
		{
			case '%':	luaL_addchar(&b, *s); break;
			case 'm':	luaL_addstring(&b,u.machine); break;
			case 'n':	luaL_addstring(&b,u.nodename); break;
			case 'r':	luaL_addstring(&b,u.release); break;
			case 's':	luaL_addstring(&b,u.sysname); break;
			case 'v':	luaL_addstring(&b,u.version); break;
			default:	badoption(L, 2, "format", *s); break;
		}
	luaL_pushresult(&b);
	return 1;
}


static const luaL_Reg posix_sys_utsname_fns[] =
{
	LPOSIX_FUNC( Puname		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_sys_utsname(lua_State *L)
{
	luaL_register(L, "posix.sys.utsname", posix_sys_utsname_fns);
	lua_pushliteral(L, "posix.sys.utsname for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
