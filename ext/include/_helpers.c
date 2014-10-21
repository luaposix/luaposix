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

#ifndef LUAPOSIX__HELPERS_C
#define LUAPOSIX__HELPERS_C 1

#include <config.h>

#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if HAVE_STRINGS_H
#  include <strings.h> /* for strcasecmp() */
#endif

#if HAVE_CURSES
# if HAVE_NCURSESW_CURSES_H
#    include <ncursesw/curses.h>
# elif HAVE_NCURSESW_H
#    include <ncursesw.h>
# elif HAVE_NCURSES_CURSES_H
#    include <ncurses/curses.h>
# elif HAVE_NCURSES_H
#    include <ncurses.h>
# elif HAVE_CURSES_H
#    include <curses.h>
# endif
#include <term.h>
#endif

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "lua52compat.h"

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
checkinteger(lua_State *L, int narg, const char *expected)
{
	lua_Integer d = lua_tointeger(L, narg);
	if (d == 0 && !lua_isnumber(L, narg))
		argtypeerror(L, narg, expected);
	return d;
}

static int
checkint(lua_State *L, int narg)
{
	return (int)checkinteger(L, narg, "int");
}

static long
checklong(lua_State *L, int narg)
{
	return (long)checkinteger(L, narg, "int");
}


#if HAVE_CURSES
static chtype
checkch(lua_State *L, int narg)
{
	if (lua_type(L, narg) == LUA_TNUMBER)
		return luaL_checknumber(L, narg);
	if (lua_type(L, narg) == LUA_TSTRING)
		return *lua_tostring(L, narg);

	return argtypeerror(L, narg, "int or char");
}


static chtype
optch(lua_State *L, int narg, chtype def)
{
	if (lua_isnoneornil(L, narg))
		return def;
	if (lua_isnumber(L, narg) || lua_isstring(L, narg))
		return checkch(L, narg);
	return argtypeerror(L, narg, "int or char or nil");
}
#endif


static int
optboolean(lua_State *L, int narg, int def)
{
	if (lua_isnoneornil(L, narg))
		return def;
	checktype (L, narg, LUA_TBOOLEAN, "boolean or nil");
	return (int)lua_toboolean(L, narg);
}

static int
optint(lua_State *L, int narg, lua_Integer def)
{
	if (lua_isnoneornil(L, narg))
		return (int) def;
	return (int)checkinteger(L, narg, "int or nil");
}

static const char *
optstring(lua_State *L, int narg, const char *def)
{
	const char *s;
	if (lua_isnoneornil(L, narg))
		return def;
	s = lua_tolstring(L, narg, NULL);
	if (!s)
		argtypeerror(L, narg, "string or nil");
	return s;
}

static void
checknargs(lua_State *L, int maxargs)
{
	int nargs = lua_gettop(L);
	lua_pushfstring(L, "no more than %d argument%s expected, got %d",
		        maxargs, maxargs > 1 ? "s" : "", nargs);
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
		expected, k, lua_typename(L, got_type));
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

static int
checkintfield(lua_State *L, int index, const char *k)
{
	int r;
	checkfieldtype(L, index, k, LUA_TNUMBER, "int");
	r = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return r;
}

static const char *
checkstringfield(lua_State *L, int index, const char *k)
{
	const char *r;
	checkfieldtype(L, index, k, LUA_TSTRING, NULL);
	r = lua_tostring(L, -1);
	lua_pop(L, 1);
	return r;
}

static int
optintfield(lua_State *L, int index, const char *k, int def)
{
	int got_type;
	lua_getfield(L, index, k);
	got_type = lua_type(L, -1);
	lua_pop(L, 1);
	if (got_type == LUA_TNONE || got_type == LUA_TNIL)
		return def;
	return checkintfield(L, index, k);
}

static const char *
optstringfield(lua_State *L, int index, const char *k, const char *def)
{
	const char *r;
	int got_type;
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

#define pushintresult(n)	(lua_pushinteger(L, (n)), 1)

#define pushstringresult(s)	(lua_pushstring(L, (s)), 1)

static int
pushresult(lua_State *L, int i, const char *info)
{
	if (i==-1)
		return pusherror(L, info);
	return pushintresult(i);
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


#define pushnumberfield(k,v) LPOSIX_STMT_BEG {				\
	lua_pushnumber(L, (lua_Integer) v); lua_setfield(L, -2, k);	\
} LPOSIX_STMT_END

#define pushstringfield(k,v) LPOSIX_STMT_BEG {				\
	if (v) {							\
		lua_pushstring(L, (const char *) v);			\
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

#define setnumberfield(_p, _n) pushnumberfield(LPOSIX_STR(_n), _p->_n)
#define setstringfield(_p, _n) pushstringfield(LPOSIX_STR(_n), _p->_n)


static int
pushtimeval(lua_State *L, struct timeval *tv)
{
	if (!tv)
		return lua_pushnil(L), 1;

	lua_createtable(L, 0, 2);
	setnumberfield(tv, tv_sec);
	setnumberfield(tv, tv_usec);

	settypemetatable("PosixTimeval");
	return 1;
}


typedef void (*Selector)(lua_State *L, int i, const void *data);

static int
doselection(lua_State *L, int i, int n, const char *const S[], Selector F, const void *data)
{
	if (lua_isnoneornil(L, i) || lua_istable(L, i))
	{
		int j;
		checknargs(L, i);
		if (lua_isnoneornil(L, i))
			lua_createtable(L,0,n);
		else
			lua_settop(L, i);
		for (j=0; S[j]!=NULL; j++)
		{
			F(L, j, data);
			lua_setfield(L, -2, S[j]);
		}
		return 1;
	}
	else if (lua_isstring(L, i))
	{
		int k,n=lua_gettop(L);
		for (k=i; k<=n; k++)
		{
			int j;
			luaL_checkstring(L, k);
			j=luaL_checkoption(L, k, NULL, S);
			F(L, j, data);
			lua_replace(L, k);
		}
		return n-i+1;
	}

	return argtypeerror(L, i, "table, string or nil");
}
#define doselection(L,i,S,F,d) (doselection)(L,i,sizeof(S)/sizeof(*S)-1,S,F,d)


static int
lookup_symbol(const char * const S[], const int K[], const char *str)
{
	int i;
	for (i = 0; S[i] != NULL; i++)
		if (strcasecmp(S[i], str) == 0)
			return K[i];
	return -1;
}

/* File mode translation between octal codes and `rwxrwxrwx' strings,
   and between octal masks and `ugoa+-=rwx' strings. */

static const struct { char c; mode_t b; } M[] =
{
	{'r', S_IRUSR}, {'w', S_IWUSR}, {'x', S_IXUSR},
	{'r', S_IRGRP}, {'w', S_IWGRP}, {'x', S_IXGRP},
	{'r', S_IROTH}, {'w', S_IWOTH}, {'x', S_IXOTH},
};

static void
pushmode(lua_State *L, mode_t mode)
{
	char m[9];
	int i;
	for (i=0; i<9; i++)
		m[i]= (mode & M[i].b) ? M[i].c : '-';
	if (mode & S_ISUID)
		m[2]= (mode & S_IXUSR) ? 's' : 'S';
	if (mode & S_ISGID)
		m[5]= (mode & S_IXGRP) ? 's' : 'S';
	lua_pushlstring(L, m, 9);
}

static int
rwxrwxrwx(mode_t *mode, const char *p)
{
	int count;
	mode_t tmp_mode = *mode;

	tmp_mode &= ~(S_ISUID | S_ISGID); /* turn off suid and sgid flags */
	for (count=0; count<9; count ++)
	{
		if (*p == M[count].c)
			tmp_mode |= M[count].b;	/* set a bit */
		else if (*p == '-')
			tmp_mode &= ~M[count].b;	/* clear a bit */
		else if (*p=='s')
			switch (count)
			{
				case 2: /* turn on suid flag */
					tmp_mode |= S_ISUID | S_IXUSR;
					break;
				case 5: /* turn on sgid flag */
					tmp_mode |= S_ISGID | S_IXGRP;
					break;
				default:
					return -4; /* failed! -- bad rwxrwxrwx mode change */
					break;
			}
		p++;
	}
	*mode = tmp_mode;
	return 0;
}

static int
octal_mode(mode_t *mode, const char *p)
{
	mode_t tmp_mode = 0;
	char* endp = NULL;
	if (strlen(p) > 8)
		return -4; /* error -- bad syntax, string too long */
	tmp_mode = strtol(p, &endp, 8);
	if (p && endp && *p != '\0' && *endp == '\0')
	{
		*mode = tmp_mode;
		return 0;
	}
	return -4; /* error -- bad syntax */
}

static int
mode_munch(mode_t *mode, const char* p)
{
	char op=0;
	mode_t affected_bits, ch_mode;
	int done = 0;

	while (!done)
	{
		/* step 0 -- clear temporary variables */
		affected_bits=0;
		ch_mode=0;

		/* step 1 -- who's affected? */

		/* mode string given in rwxrwxrwx format */
		if (*p == 'r' || *p == '-')
			return rwxrwxrwx(mode, p);

		/* mode string given in octal */
		if (*p >= '0' && *p <= '7')
			return octal_mode(mode, p);

		/* mode string given in ugoa+-=rwx format */
		for ( ; ; p++)
			switch (*p)
			{
				case 'u':
					affected_bits |= 04700;
					break;
				case 'g':
					affected_bits |= 02070;
					break;
				case 'o':
					affected_bits |= 01007;
					break;
				case 'a':
					affected_bits |= 07777;
					break;
				case ' ': /* ignore spaces */
					break;
				default:
					goto no_more_affected;
			}

	no_more_affected:
		/* If none specified, affect all bits. */
		if (affected_bits == 0)
			affected_bits = 07777;

		/* step 2 -- how is it changed? */
		switch (*p)
		{
			case '+':
			case '-':
			case '=':
				op = *p;
				break;
			case ' ': /* ignore spaces */
				break;
			default:
				return -1; /* failed! -- bad operator */
		}

		/* step 3 -- what are the changes? */
		for (p++ ; *p!=0 ; p++)
			switch (*p)
			{
				case 'r':
					ch_mode |= 00444;
					break;
				case 'w':
					ch_mode |= 00222;
					break;
				case 'x':
					ch_mode |= 00111;
					break;
				case 's':
					/* Set the setuid/gid bits if `u' or `g' is selected. */
					ch_mode |= 06000;
					break;
				case ' ':
					/* ignore spaces */
					break;
				default:
					goto specs_done;
			}

	specs_done:
		/* step 4 -- apply the changes */
		if (*p != ',')
			done = 1;
		if (*p != 0 && *p != ' ' && *p != ',')
			return -2; /* failed! -- bad mode change */
		p++;
		if (ch_mode)
			switch (op)
			{
				case '+':
					*mode |= ch_mode & affected_bits;
					break;
				case '-':
					*mode &= ~(ch_mode & affected_bits);
					break;
				case '=':
					*mode = (*mode & ~affected_bits) | (ch_mode & affected_bits);
					break;
				default:
					return -3; /* failed! -- unknown error */
			}
	}

	return 0; /* successful call */
}


static uid_t
mygetuid(lua_State *L, int i)
{
	if (lua_isnoneornil(L, i))
		return (uid_t)-1;
	else if (lua_isnumber(L, i))
		return (uid_t) lua_tonumber(L, i);
	else if (lua_isstring(L, i))
	{
		struct passwd *p=getpwnam(lua_tostring(L, i));
		return (p==NULL) ? (uid_t)-1 : p->pw_uid;
	}
	else
		return argtypeerror(L, i, "string, int or nil");
}

static gid_t
mygetgid(lua_State *L, int i)
{
	if (lua_isnoneornil(L, i))
		return (gid_t)-1;
	else if (lua_isnumber(L, i))
		return (gid_t) lua_tonumber(L, i);
	else if (lua_isstring(L, i))
	{
		struct group *g=getgrnam(lua_tostring(L, i));
		return (g==NULL) ? (uid_t)-1 : g->gr_gid;
	}
	else
		return argtypeerror(L, i, "string, int or nil");
}

#endif /*LUAPOSIX__HELPERS_C*/
