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
 Directory Iterators.

@module posix.dirent
*/

#include <dirent.h>

#include "_helpers.c"


/***
Contents of directory.
@function dir
@string[opt="."] path directory to act on
@treturn table contents of *path*
@see dir.lua
*/
static int
Pdir(lua_State *L)
{
	const char *path = optstring(L, 1, ".");
	DIR *d;
	checknargs(L, 1);
	d = opendir(path);
	/* Throw an argument error for consistency with eg. io.lines */
	if (d == NULL)
	{
		const char *msg = strerror (errno);
		msg = lua_pushfstring(L, "%s: %s", path, msg);
		return luaL_argerror(L, 1, msg);
	}
	else
	{
		int i;
		struct dirent *entry;
		lua_newtable(L);
		for (i=1; (entry = readdir(d)) != NULL; i++)
		{
			lua_pushstring(L, entry->d_name);
			lua_rawseti(L, -2, i);
		}
		closedir(d);
		return 1;
	}
}


static int
aux_files(lua_State *L)
{
	DIR **p = (DIR **)lua_touserdata(L, lua_upvalueindex(1));
	DIR *d = *p;
	struct dirent *entry;
	if (d == NULL)
		return 0;
	entry = readdir(d);
	if (entry == NULL)
	{
		closedir(d);
		*p=NULL;
		return 0;
	}
	return pushstringresult(entry->d_name);
}


static int
dir_gc (lua_State *L)
{
	DIR *d = *(DIR **)lua_touserdata(L, 1);
	if (d!=NULL)
		closedir(d);
	return 0;
}


/***
Iterator over all files in named directory.
@function files
@string[opt="."] path directory to act on
@return an iterator
*/
static int
Pfiles(lua_State *L)
{
	const char *path = optstring(L, 1, ".");
	DIR **d;
	checknargs(L, 1);
	d = (DIR **)lua_newuserdata(L, sizeof(DIR *));
	*d = opendir(path);
	/* Throw an argument error for consistency with eg. io.lines */
	if (*d == NULL)
	{
		const char *msg = strerror (errno);
		msg = lua_pushfstring(L, "%s: %s", path, msg);
		return luaL_argerror(L, 1, msg);
	}
	if (luaL_newmetatable(L, PACKAGE " dir handle"))
	{
		lua_pushcfunction(L, dir_gc);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);
	lua_pushcclosure(L, aux_files, 1);
	return 1;
}


static const luaL_Reg posix_dirent_fns[] =
{
	LPOSIX_FUNC( Pdir		),
	LPOSIX_FUNC( Pfiles		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_dirent(lua_State *L)
{
	luaL_newlib(L, posix_dirent_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("dirent"));
	lua_setfield(L, -2, "version");

	return 1;
}
