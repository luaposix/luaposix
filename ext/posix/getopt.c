/*
 * POSIX library for Lua 5.1, 5.2 & 5.3.
 * (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2015
 * (c) Reuben Thomas <rrt@sc3d.org> 2010-2013
 * (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
 * Clean up and bug fixes by Leo Razoumov <slonik.az@gmail.com> 2006-10-11
 * Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br> 07 Apr 2006 23:17:49
 * Based on original by Claudio Terra for Lua 3.x.
 * With contributions by Roberto Ierusalimschy.
 * With documentation from Steve Donovan 2012
 */
/***
 Command Line Argument Processing.

@module posix.getopt
*/

#include <config.h>

#include <getopt.h>

#include "_helpers.c"


/* getopt_long */

/* N.B. We don't need the symbolic constants no_argument,
   required_argument and optional_argument, since their values are
   defined as 0, 1 and 2 respectively. */
static const char *const arg_types[] =
{
	"none", "required", "optional", NULL
};


static int
iter_getopt_long(lua_State *L)
{
	int longindex = 0, ret, argc = lua_tointeger(L, lua_upvalueindex(1));
	char **argv = (char **)lua_touserdata(L, lua_upvalueindex(3));
	struct option *longopts = (struct option *)lua_touserdata(L, lua_upvalueindex(3 + argc + 1));

	if (argv == NULL) /* If we have already completed, return now. */
		return 0;

	/* Fetch upvalues to pass to getopt_long. */
	ret = getopt_long(argc, argv,
			  lua_tostring(L, lua_upvalueindex(2)),
			  longopts,
			  &longindex);
	if (ret == -1)
		return 0;
	else
	{
		char c = ret;
		lua_pushlstring(L, &c, 1);
		lua_pushstring(L, optarg);
		lua_pushinteger(L, optind);
		lua_pushinteger(L, longindex);
		return 4;
	}
}


/***
Parse command-line options.
@function getopt
@param arg command line arguments
@string shortopts short option specifier
@tparam[opt] table longopts long options table
@int[opt=0] opterr index of the option with an error
@int[opt=1] optind index of the next unprocessed option
@return option iterator, returning 4 values
@see getopt(3)
@see getopt_long(3)
@usage
local longopts = {
  {"help", "none", 1},
  {"version", "none", 3},
}
for r, longindex, err, i in P.getopt (arg, "ho:v", longopts, err, i) do
  process (arg, err, i)
end
@see getopt.lua
*/
static int
Pgetopt(lua_State *L)
{
	int argc, i, n = 0;
	const char *shortopts;
	char **argv;
	struct option *longopts;

	checknargs(L, 5);
	checktype(L, 1, LUA_TTABLE, "list");
	shortopts = luaL_checkstring(L, 2);
	if (!lua_isnoneornil(L, 3))
		checktype(L, 3, LUA_TTABLE, "table or nil");
	opterr = optint(L, 4, 0);
	optind = optint(L, 5, 1);

	argc = (int)lua_objlen(L, 1) + 1;

	lua_pushinteger(L, argc);

	lua_pushstring(L, shortopts);

	argv = lua_newuserdata(L, (argc + 1) * sizeof(char *));
	argv[argc] = NULL;
	for (i = 0; i < argc; i++)
	{
		lua_pushinteger(L, i);
		lua_gettable(L, 1);
		argv[i] = (char *)luaL_checkstring(L, -1);
	}

	if (lua_type(L, 3) == LUA_TTABLE)
	{
		n = (int)lua_objlen(L, 3);
	}
	longopts = lua_newuserdata(L, (n + 1) * sizeof(struct option));
	longopts[n].name = NULL;
	longopts[n].has_arg = 0;
	longopts[n].flag = NULL;
	longopts[n].val = 0;
	for (i = 1; i <= n; i++)
	{
		const char *name, *val;
		int has_arg;

		lua_pushinteger(L, i);
		lua_gettable(L, 3);
		luaL_checktype(L, -1, LUA_TTABLE);

		lua_pushinteger(L, 1);
		lua_gettable(L, -2);
		name = luaL_checkstring(L, -1);

		lua_pushinteger(L, 2);
		lua_gettable(L, -3);
		has_arg = luaL_checkoption(L, -1, NULL, arg_types);
		lua_pop(L, 1);

		lua_pushinteger(L, 3);
		lua_gettable(L, -3);
		val = luaL_checkstring(L, -1);
		lua_pop(L, 1);

		longopts[i - 1].name = name;
		longopts[i - 1].has_arg = has_arg;
		longopts[i - 1].flag = NULL;
		longopts[i - 1].val = val[0];
		lua_pop(L, 1);
	}

	/* Push remaining upvalues, and make and push closure. */
	lua_pushcclosure(L, iter_getopt_long, 4 + argc + n);

	return 1;
}

static const luaL_Reg posix_getopt_fns[] =
{
	LPOSIX_FUNC( Pgetopt		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_getopt(lua_State *L)
{
	luaL_register(L, "posix.getopt", posix_getopt_fns);
	lua_pushliteral(L, "posix.getopt for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	return 1;
}
