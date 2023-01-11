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
 A few Standard I/O functions not already in Lua core.

@module posix.stdio
*/

#include <stdio.h>

#include "_helpers.c"

/***
Name of controlling terminal.
@function ctermid
@treturn string controlling terminal for current process
@see ctermid(3)
*/
static int
Pctermid(lua_State *L)
{
	char b[L_ctermid];
	checknargs(L, 0);
	return pushstringresult(ctermid(b));
}


/***
File descriptor corresponding to a Lua file object.
@function fileno
@tparam file file Lua file object
@treturn[1] int file descriptor for *file*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see fileno(3)
@usage
local stdio = require "posix.stdio"
local unistd = require "posix.unistd"
assert (stdio.fileno (io.stdout) == unistd.STDOUT_FILENO)
*/
static int
Pfileno(lua_State *L)
{
	FILE *f = *(FILE**) luaL_checkudata(L, 1, LUA_FILEHANDLE);
	checknargs(L, 1);
	return pushresult(L, fileno(f), NULL);
}


/* This helper function is adapted from Lua 5.3's liolib.c */
static int
stdio_fclose (lua_State *L) {
	int res = -1;    
	luaL_Stream *p = ((luaL_Stream *)luaL_checkudata(L, 1, LUA_FILEHANDLE));
	if (p->f == NULL)
		return 0;
	res = fclose(p->f);
	p->f = NULL;
	return luaL_fileresult(L, (res == 0), NULL);
}

/* This function could be used more generally; see */
static int
pushfile (lua_State *L, int fd, const char *mode) {
	luaL_Stream *p = (luaL_Stream *)lua_newuserdata(L, sizeof(luaL_Stream));
	luaL_getmetatable(L, LUA_FILEHANDLE);
	lua_setmetatable(L, -2);
#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM == 501
	/* PUC-Rio Lua uses lconfig_h as include guard for luaconf.h,
	   LuaJIT uses luaconf_h. If you use PUC-Rio's include files
	   but LuaJIT's library, you're on your own here!  */
#  if !defined(luaconf_h)
	lua_createtable(L, 0, 1);
	lua_pushcfunction(L, stdio_fclose);
	lua_setfield(L, -2, "__close");
	lua_setfenv(L, -2);  /* set fenv for 'fdopen' */
#  endif
#endif
	p->closef = stdio_fclose;
	p->f = fdopen(fd, mode);
	return p->f != NULL;
}

/***
Create a Lua file object from a file descriptor.
@function fdopen
@tparam int fd file descriptor
@tparam string mode the mode in which to open the file descriptor
@treturn[1] file Lua file object *fd*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see fdopen(3)
@usage
local fdopen = require "posix.stdio".fdopen
local STDOUT_FILENO = require "posix.unistd".STDOUT_FILENO
stdout = fdopen (STDOUT_FILENO, "w")
*/
static int
Pfdopen(lua_State *L)	/** fdopen(fd, mode) */
{
	int fd = checkint(L, 1);
	const char *mode = luaL_checkstring(L, 2);
	checknargs(L, 2);
	if (!pushfile(L, fd, mode))
		return pusherror(L, "fdopen");
	return 1;
}

/***
Change the name or location of a file
@function rename
@tparam string oldpath
@tparam string newpath
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see rename(2)
@usage
local ok, errmsg = P.rename (oldpath, newpath)
if not ok then error (errmsg) end
*/
static int
Prename(lua_State *L)	/** rename(oldpath, newpath) */
{
	const char *oldpath = luaL_checkstring(L, 1);
	const char *newpath = luaL_checkstring(L, 2);
	checknargs(L, 2);
	return pushresult(L, rename(oldpath, newpath), NULL);
}


static const luaL_Reg posix_stdio_fns[] =
{
	LPOSIX_FUNC( Pctermid		),
	LPOSIX_FUNC( Pfileno		),
	LPOSIX_FUNC( Pfdopen		),
	LPOSIX_FUNC( Prename		),
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Stdio constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.stdio
@int _IOFBF fully buffered
@int _IOLBF line buffered
@int _IONBF unbuffered
@int BUFSIZ size of buffer
@int EOF end of file
@int FOPEN_MAX maximum number of open files
@int FILENAME_MAX maximum length of filename
@usage
  -- Print stdio constants supported on this host.
  for name, value in pairs (require "posix.stdio") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_stdio(lua_State *L)
{
	luaL_newlib(L, posix_stdio_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("stdio"));
	lua_setfield(L, -2, "version");

	/* stdio.h constants */
	/* Those that are omitted already have a Lua interface, or alternative. */
	LPOSIX_CONST( _IOFBF		);
	LPOSIX_CONST( _IOLBF		);
	LPOSIX_CONST( _IONBF		);
	LPOSIX_CONST( BUFSIZ		);
	LPOSIX_CONST( EOF		);
	LPOSIX_CONST( FOPEN_MAX		);
	LPOSIX_CONST( FILENAME_MAX	);

	return 1;
}
