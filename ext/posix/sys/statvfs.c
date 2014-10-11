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
 Retrieve File System Information.

 Where supported by the underlying system, query the file system.  If the
 module loads, but there is no kernel support, then `posix.sys.statvfs.version`
 will be set, but the unsupported APIs will be `nil`.

@module posix.sys.statvfs
*/

#include <config.h>

#if defined HAVE_STATVFS

#include <sys/statvfs.h>

#include "_helpers.c"


static void
Fstatvfs(lua_State *L, int i, const void *data)
{
	const struct statvfs *s=data;
	switch (i)
	{
		case 0:
			lua_pushinteger(L, s->f_bsize);
			break;
		case 1:
			lua_pushinteger(L, s->f_frsize);
			break;
		case 2:
			lua_pushnumber(L, s->f_blocks);
			break;
		case 3:
			lua_pushnumber(L, s->f_bfree);
			break;
		case 4:
			lua_pushnumber(L, s->f_bavail);
			break;
		case 5:
			lua_pushnumber(L, s->f_files);
			break;
		case 6:
			lua_pushnumber(L, s->f_ffree);
			break;
		case 7:
			lua_pushnumber(L, s->f_favail);
			break;
		case 8:
			lua_pushinteger(L, s->f_fsid);
			break;
		case 9:
			lua_pushinteger(L, s->f_flag);
			break;
		case 10:
			lua_pushinteger(L, s->f_namemax);
			break;
	}
}


static const char *const Sstatvfs[] =
{
	"bsize", "frsize", "blocks", "bfree", "bavail",
	"files", "ffree", "favail", "fsid", "flag", "namemax",
	NULL
};


/***
Get file system statistics.
@function statvfs
@string path any path within the mounted file system
@string ... field names, each one of "bsize", "frsize", "blocks", "bfree", "bavail",
"files", "ffree", "favail", "fsid", "flag", "namemax"
@return ... values, or table of all fields if no option given
@see statvfs(3)
*/
static int
Pstatvfs(lua_State *L)
{
	struct statvfs s;
	const char *path=luaL_checkstring(L, 1);
	if (statvfs(path,&s)==-1)
		return pusherror(L, path);
	return doselection(L, 2, Sstatvfs, Fstatvfs, &s);
}


static const luaL_Reg posix_sys_statvfs_fns[] =
{
	LPOSIX_FUNC( Pstatvfs		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_sys_statvfs(lua_State *L)
{
	luaL_register(L, "posix.sys.statvfs", posix_sys_statvfs_fns);
	lua_pushliteral(L, "posix.sys.statvfs for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
#endif
