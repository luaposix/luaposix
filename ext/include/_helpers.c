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

#ifndef LUAPOSIX__HELPERS_C
#define LUAPOSIX__HELPERS_C 1

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable threading extensions on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>		/* for _POSIX_VERSION */

#if !defined PATH_MAX
# define PATH_MAX 1024
#endif

/* Some systems set _POSIX_C_SOURCE over _POSIX_VERSION! */
#if _POSIX_C_SOURCE >= 200112L || _POSIX_VERSION >= 200112L || _XOPEN_SOURCE >= 600
# define LPOSIX_2001_COMPLIANT 1
#endif

#if _POSIX_C_SOURCE >= 200809L || _XOPEN_SOURCE >= 700
# define LPOSIX_2008_COMPLIANT 1
# ifndef LPOSIX_2001_COMPLIANT
#   define LPOSIX_2001_COMPLIANT
# endif
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#if LUA_VERSION_NUM < 503
#  define lua_isinteger lua_isnumber
#  if LUA_VERSION_NUM == 501
#    include "compat-5.2.c"
#  endif
#endif

#if LUA_VERSION_NUM == 502 || LUA_VERSION_NUM == 503 || LUA_VERSION_NUM == 504
#  define lua_objlen lua_rawlen
#  define lua_strlen lua_rawlen
#endif

#define _LPOSIX_VERSION_STRING(m) 					\
	("posix." m " for " LUA_VERSION " / " PACKAGE " " VERSION)
#define LPOSIX_VERSION_STRING(m) _LPOSIX_VERSION_STRING(m)

#ifndef STREQ
#  define STREQ(a, b)     (strcmp (a, b) == 0)
#endif

/* Mark unused parameters required only to match a function type
   specification. */
#ifdef __GNUC__
#  define LPOSIX_UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define LPOSIX_UNUSED(x) UNUSED_ ## x
#endif

/* LPOSIX_STMT_BEG/END are used to create macros that expand to a
   single compound statement in a portable way. */
#if defined __GNUC__ && !defined __STRICT_ANSI__ && !defined __cplusplus
#  define LPOSIX_STMT_BEG	(void)(
#  define LPOSIX_STMT_END	)
#else
#  if (defined sun || defined __sun__)
#    define LPOSIX_STMT_BEG	if (1)
#    define LPOSIX_STMT_END	else (void)0
#  else
#    define LPOSIX_STMT_BEG	do
#    define LPOSIX_STMT_END	while (0)
#  endif
#endif


/* The extra indirection to these macros is required so that if the
   arguments are themselves macros, they will get expanded too.  */
#define LPOSIX__SPLICE(_s, _t)	_s##_t
#define LPOSIX_SPLICE(_s, _t)	LPOSIX__SPLICE(_s, _t)

#define LPOSIX__STR(_s)		#_s
#define LPOSIX_STR(_s)		LPOSIX__STR(_s)

/* The +1 is to step over the leading '_' that is required to prevent
   premature expansion of MENTRY arguments if we didn't add it.  */
#define LPOSIX__STR_1(_s)	(#_s + 1)
#define LPOSIX_STR_1(_s)	LPOSIX__STR_1(_s)

#define LPOSIX_CONST(_f)	LPOSIX_STMT_BEG {			\
					lua_pushinteger(L, _f);		\
					lua_setfield(L, -2, #_f);	\
				} LPOSIX_STMT_END

#define LPOSIX_FUNC(_s)		{LPOSIX_STR_1(_s), (_s)}

#define pushokresult(b)	pushboolresult((int) (b) == OK)

#ifndef errno
extern int errno;
#endif


/* ========================= *
 * Bad argument diagnostics. *
 * ========================= */


static int
argtypeerror(lua_State *L, int narg, const char *expected)
{
	const char *got = luaL_typename(L, narg);
	return luaL_argerror(L, narg,
		lua_pushfstring(L, "%s expected, got %s", expected, got));
}

static void
checktype(lua_State *L, int narg, int t, const char *expected)
{
	if (lua_type(L, narg) != t)
		argtypeerror (L, narg, expected);
}

static lua_Integer
expectinteger(lua_State *L, int narg, const char *expected)
{
	int isconverted = 0;
	lua_Integer d = lua_tointegerx(L, narg, &isconverted);
	if (!isconverted)
		argtypeerror(L, narg, expected);
	return d;
}
/* As soon as specl's badargs.diagnose can handle it... change these to "integer"! */
#define checkinteger(L,n) (expectinteger(L,n,"integer"))
#define checkint(L,n)     ((int)expectinteger(L,n,"integer"))
#define checklong(L,n)    ((long)expectinteger(L,n,"integer"))

static int
optboolean(lua_State *L, int narg, int def)
{
	if (lua_isnoneornil(L, narg))
		return def;
	checktype (L, narg, LUA_TBOOLEAN, "boolean or nil");
	return (int)lua_toboolean(L, narg);
}

static lua_Integer
expectoptinteger(lua_State *L, int narg, lua_Integer def, const char *expected)
{
	if (lua_isnoneornil(L, narg))
		return def;
	return expectinteger(L, narg, expected);
}
/* As soon as specl's badargs.diagnose can handle it... change these to "integer or nil"! */
#define optinteger(L,n,d) (expectoptinteger(L,n,d,"integer or nil"))
#define optint(L,n,d)     ((int)expectoptinteger(L,n,d,"integer or nil"))
#define optlong(L,n,d)    ((long)expectoptinteger(L,n,d,"integer or nil"))

static const char *
optstring(lua_State *L, int narg, const char *def)
{
	const char *s;
	if (lua_isnoneornil(L, narg))
		return def;
	s = lua_tolstring(L, narg, NULL);
	if (!s)
		argtypeerror(L, narg, "nil or string");
	return s;
}

static void
checknargs(lua_State *L, int maxargs)
{
	int nargs = lua_gettop(L);
	lua_pushfstring(L, "no more than %d argument%s expected, got %d",
		        maxargs, maxargs == 1 ? "" : "s", nargs);
	luaL_argcheck(L, nargs <= maxargs, maxargs + 1, lua_tostring (L, -1));
	lua_pop(L, 1);
}

/* Try a lua_getfield from the table on the given index. On success the field
 * is pushed and 0 is returned, on failure nil and an error message is pushed and 2
 * is returned */
static void
checkfieldtype(lua_State *L, int index, const char *k, int expect_type, const char *expected)
{
	int got_type;
	lua_getfield(L, index, k);
	got_type = lua_type(L, -1);

	if (expected == NULL)
		expected = lua_typename(L, expect_type);

	lua_pushfstring(L, "%s expected for field '%s', got %s",
		expected, k, got_type == LUA_TNIL ? "no value" : lua_typename(L, got_type));
	luaL_argcheck(L, got_type == expect_type, index, lua_tostring(L, -1));
	lua_pop(L, 1);
}

#define NEXT_IKEY	-2
#define NEXT_IVALUE	-1
static void
checkismember(lua_State *L, int index, int n, const char *const S[])
{
	/* Diagnose non-string type field names. */
	int got_type = lua_type(L, NEXT_IKEY);
	luaL_argcheck(L, lua_isstring(L, NEXT_IKEY), index,
		lua_pushfstring(L, "invalid %s field name", lua_typename(L, got_type)));

	/* Check field name is listed in S. */
	{
		const char *k = lua_tostring(L, NEXT_IKEY);
		int i;
		for (i = 0; i < n; ++i)
			if (STREQ(S[i], k)) return;
	}


	/* Diagnose invalid field name. */
	luaL_argcheck(L, 0, index,
		lua_pushfstring(L, "invalid field name '%s'", lua_tostring(L, NEXT_IKEY)));
}
#undef NEXT_IKEY
#undef NEXT_IVALUE

static void
checkfieldnames(lua_State *L, int index, int n, const char * const S[])
{
	for (lua_pushnil(L); lua_next(L, index); lua_pop(L, 1))
		checkismember(L, index, n, S);
}
#define checkfieldnames(L,i,S) (checkfieldnames)(L,i,sizeof(S)/sizeof(*S),S)

static lua_Integer
checkintegerfield(lua_State *L, int index, const char *k)
{
	lua_Integer r;
	checkfieldtype(L, index, k, LUA_TNUMBER, "integer");
	r = lua_tointeger(L, -1);
	lua_pop(L, 1);
	return r;
}
#define checkintfield(L,i,k)  ((int)checkintegerfield(L,i,k))
#define checklongfield(L,i,k) ((long)checkintegerfield(L,i,k))

static int
checknumberfield(lua_State *L, int index, const char *k)
{
	int r;
	checkfieldtype(L, index, k, LUA_TNUMBER, "number");
	r = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return r;
}

static const char *
checklstringfield(lua_State *L, int index, const char *k, size_t *plen)
{
	const char *r;
	checkfieldtype(L, index, k, LUA_TSTRING, NULL);
	r = lua_tolstring(L, -1, plen);
	lua_pop(L, 1);
	return r; /* XXX UNSAFE! */
}
#define checkstringfield(L,i,s) (checklstringfield(L,i,s,NULL))

static lua_Integer
optintegerfield(lua_State *L, int index, const char *k, int def)
{
	int got_type;
	lua_getfield(L, index, k);
	got_type = lua_type(L, -1);
	lua_pop(L, 1);
	if (got_type == LUA_TNONE || got_type == LUA_TNIL)
		return def;
	return checkintegerfield(L, index, k);
}
#define optintfield(L,i,k,d)  ((int)optintegerfield(L,i,k,d))
#define optlongfield(L,i,k,d) ((long)optintegerfield(L,i,k,d))

static const char *
optstringfield(lua_State *L, int index, const char *k, const char *def)
{
	int got_type;
	lua_getfield(L, index, k);
	got_type = lua_type(L, -1);
	lua_pop(L, 1);
	if (got_type == LUA_TNONE || got_type == LUA_TNIL)
		return def;
	return checkstringfield(L, index, k);
}

static int
pusherror(lua_State *L, const char *info)
{
	lua_pushnil(L);
	if (info==NULL)
		lua_pushstring(L, strerror(errno));
	else
		lua_pushfstring(L, "%s: %s", info, strerror(errno));
	lua_pushinteger(L, errno);
	return 3;
}

#define pushboolresult(b)	(lua_pushboolean(L, (b)), 1)

#define pushintegerresult(n)	(lua_pushinteger(L, (n)), 1)

#define pushstringresult(s)	(lua_pushstring(L, (s)), 1)

static int
pushresult(lua_State *L, int i, const char *info)
{
	if (i==-1)
		return pusherror(L, info);
	return pushintegerresult(i);
}

static void
badoption(lua_State *L, int i, const char *what, int option)
{
	luaL_argerror(L, i,
		lua_pushfstring(L, "invalid %s option '%c'", what, option));
}



/* ================== *
 * Utility functions. *
 * ================== */


static int
binding_notimplemented(lua_State *L, const char *fname, const char *libname)
{
	lua_pushnil(L);
	lua_pushfstring(L, "'%s' is not implemented by host %s library",
			fname, libname);
	return 2;
}


#define pushintegerfield(k,v) LPOSIX_STMT_BEG {				\
	lua_pushinteger(L, (lua_Integer) v); lua_setfield(L, -2, k);	\
} LPOSIX_STMT_END

#define pushnumberfield(k,v) LPOSIX_STMT_BEG {				\
	lua_pushnumber(L, (lua_Number) v); lua_setfield(L, -2, k);	\
} LPOSIX_STMT_END

#define pushstringfield(k,v) LPOSIX_STMT_BEG {				\
	if (v) {							\
		lua_pushstring(L, (const char *) v);			\
		lua_setfield(L, -2, k);					\
	}								\
} LPOSIX_STMT_END

#define pushlstringfield(k,v,l) LPOSIX_STMT_BEG {			\
	if (l > 0 && v) {						\
		lua_pushlstring(L, (const char *) v, (size_t) l);	\
		lua_setfield(L, -2, k);					\
	}								\
} LPOSIX_STMT_END

#define pushliteralfield(k,v) LPOSIX_STMT_BEG {				\
	if (v) {							\
		lua_pushliteral(L, v);					\
		lua_setfield(L, -2, k);					\
	}								\
} LPOSIX_STMT_END

#define settypemetatable(t) LPOSIX_STMT_BEG {				\
	if (luaL_newmetatable(L, t) == 1)				\
		pushliteralfield("_type", t);				\
	lua_setmetatable(L, -2);					\
} LPOSIX_STMT_END

#define setintegerfield(_p, _n) pushintegerfield(LPOSIX_STR(_n), _p->_n)
#define setnumberfield(_p, _n) pushnumberfield(LPOSIX_STR(_n), _p->_n)
#define setstringfield(_p, _n) pushstringfield(LPOSIX_STR(_n), _p->_n)

#endif /*LUAPOSIX__HELPERS_C*/
