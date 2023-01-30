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
 Standard Posix Library functions.

@module posix.stdlib
*/

#include <fcntl.h>	/* for open(2) */
#include <stdlib.h>

#include "_helpers.c"


/***
Abort the program immediately.
@function abort
@see abort(3)
*/
static int
Pabort(lua_State *L)
{
	checknargs(L, 0);
	abort();
	return 0; /* Avoid a compiler warning (or possibly cause one
		     if the compiler's too clever, sigh). */
}


/***
Get value of environment variable, or _all_ variables.
@function getenv
@string[opt] name if nil, get all
@return value if name given, otherwise a name-indexed table of values.
@see getenv(3)
@usage for a,b in pairs(posix.getenv()) do print(a, b) end
*/
static int
Pgetenv(lua_State *L)
{
	checknargs(L, 1);
	if (lua_isnoneornil(L, 1))
	{
		extern char **environ;
		char **e;
		lua_newtable(L);
		for (e=environ; *e!=NULL; e++)
		{
			char *s=*e;
			char *eq=strchr(s, '=');
			if (eq==NULL)		/* will this ever happen? */
			{
				lua_pushstring(L, s);
				lua_pushboolean(L, 1);
			}
			else
			{
				lua_pushlstring(L, s, eq-s);
				lua_pushstring(L, eq+1);
			}
			lua_settable(L, -3);
		}
	}
	else
		lua_pushstring(L, getenv(optstring(L, 1,
			"lua_isnoneornil prevents this happening")));
	return 1;
}


/***
Grant access to a slave pseudoterminal
@function grantpt
@int fd descriptor returned by openpt
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see grantpt(3)
@see openpt
@see ptsname
@see unlockpt
*/
static int
Pgrantpt(lua_State *L)
{
	int fd = checkint(L, 1);
	checknargs(L, 1);
	return pushresult(L, grantpt(fd), "grantpt");
}


/***
Create a unique temporary directory.
@function mkdtemp
@string templ pattern that ends in six 'X' characters
@treturn[1] string path to directory, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see mkdtemp(3)
*/
static int
Pmkdtemp(lua_State *L)
{
#if defined LPOSIX_2008_COMPLIANT
	const char *path = luaL_checkstring(L, 1);
	size_t path_len = strlen(path) + 1;
	void *ud;
	lua_Alloc lalloc;
	char *tmppath;
	char *r;
	checknargs(L, 1);
	lalloc = lua_getallocf(L, &ud);

	if ((tmppath = lalloc(ud, NULL, 0, path_len)) == NULL)
		return pusherror(L, "lalloc");
	strcpy(tmppath, path);

	if ((r = mkdtemp(tmppath)))
		lua_pushstring(L, tmppath);
	lalloc(ud, tmppath, path_len, 0);
	return (r == NULL) ? pusherror(L, path) : 1;
#else
	return binding_notimplemented(L, "mkdtemp", "C");
#endif
}


/***
Create a unique temporary file.
@function mkstemp
@string templ pattern that ends in six 'X' characters
@treturn[1] int open file descriptor
@treturn[1] string path to file, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see mkstemp(3)
@usage
local stdlib = require "posix.stdlib"
stdlib.mkstemp 'wooXXXXXX'
*/
static int
Pmkstemp(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	size_t path_len = strlen(path) + 1;
	void *ud;
	lua_Alloc lalloc;
	char *tmppath;
	int r;
	checknargs(L, 1);
	lalloc = lua_getallocf(L, &ud);

	if ((tmppath = lalloc(ud, NULL, 0, path_len)) == NULL)
		return pusherror(L, "lalloc");
	strcpy(tmppath, path);
	r = mkstemp(tmppath);

	if (r != -1)
	{
		lua_pushinteger(L, r);
		lua_pushstring(L, tmppath);
	}

	lalloc(ud, tmppath, path_len, 0);
	return (r == -1) ? pusherror(L, path) : 2;
}


/***
Open a pseudoterminal.
@function openpt
@int oflags bitwise OR of zero or more of `O_RDWR` and `O_NOCTTY`
@return[1] file descriptor of pseudoterminal, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see posix_openpt(3)
@see grantpt
@see ptsname
@see unlockpt
*/
static int
Popenpt(lua_State *L)
{
	int flags = checkint(L, 1);
	checknargs(L, 1);
	/* The name of the pseudo-device is specified by POSIX */
	return pushresult(L, open("/dev/ptmx", flags), NULL);
}


/***
Get the name of a slave pseudo-terminal
@function ptsname
@int fd descriptor returned by @{openpt}
@return[1] path name of the slave terminal device, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see ptsname(3)
@see grantpt
@see unlockpt
*/
static int
Pptsname(lua_State *L)
{
	int fd = checkint(L, 1);
	const char* slave;
	checknargs(L, 1);
	slave = ptsname(fd);
	if (!slave)
		return pusherror(L, "getptsname");
	return pushstringresult(slave);
}


/***
Find canonicalized absolute pathname.
@function realpath
@string path file to act on
@treturn[1] string canonicalized absolute path, if successful
@return[2] nil
@treturn[2] string error messag
@treturn[2] int errnum
@see realpath(3)
*/
static int
Prealpath(lua_State *L)
{
	char *s;
	checknargs(L, 1);
	if ((s = realpath(luaL_checkstring(L, 1), NULL)) == NULL)
		return pusherror(L, "realpath");
	lua_pushstring(L, s);
	free(s);
	return 1;
}


/***
Set an environment variable for this process.
(Child processes will inherit this)
@function setenv
@string name
@string[opt] value (maybe nil, meaning 'unset')
@param[opt] overwrite non-nil prevents overwriting a variable
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see setenv(3)
*/
static int
Psetenv(lua_State *L)
{
	const char *name=luaL_checkstring(L, 1);
	const char *value=optstring(L, 2, NULL);
	checknargs(L, 3);
	if (value==NULL)
	{
		unsetenv(name);
		return pushresult(L, 0, NULL);
	}
	else
	{
		int overwrite=lua_isnoneornil(L, 3) || lua_toboolean(L, 3);
		return pushresult(L, setenv(name,value,overwrite), NULL);
	}
}


/***
Unlock a pseudoterminal master/slave pair
@function unlockpt
@int fd descriptor returned by openpt
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see unlockpt(3)
@see openpt
@see ptsname
@see grantpt
*/
static int
Punlockpt(lua_State *L)
{
	int fd = checkint(L, 1);
	checknargs(L, 1);
	return pushresult(L, unlockpt(fd), "unlockpt");
}


static const luaL_Reg posix_stdlib_fns[] =
{
	LPOSIX_FUNC( Pabort		),
	LPOSIX_FUNC( Pgetenv		),
	LPOSIX_FUNC( Pgrantpt		),
	LPOSIX_FUNC( Pmkdtemp		),
	LPOSIX_FUNC( Pmkstemp		),
	LPOSIX_FUNC( Popenpt		),
	LPOSIX_FUNC( Pptsname		),
	LPOSIX_FUNC( Prealpath		),
	LPOSIX_FUNC( Psetenv		),
	LPOSIX_FUNC( Punlockpt		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_stdlib(lua_State *L)
{
	luaL_newlib(L, posix_stdlib_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("stdlib"));
	lua_setfield(L, -2, "version");

	return 1;
}
