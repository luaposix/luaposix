/*
* lposix.c
* POSIX library for Lua 5.1/5.2.
* (c) Reuben Thomas (maintainer) <rrt@sc3d.org> 2010-2012
* (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
* Clean up and bug fixes by Leo Razoumov <slonik.az@gmail.com> 2006-10-11
* Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br> 07 Apr 2006 23:17:49
* Based on original by Claudio Terra for Lua 3.x.
* With contributions by Roberto Ierusalimschy.
*/

#include <config.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <getopt.h>
#include <glob.h>
#include <grp.h>
#include <libgen.h>
#include <limits.h>
#include <poll.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>
#if HAVE_CRYPT_H
#  include <crypt.h>
#endif
#if HAVE_SYS_STATVFS_H
#  include <sys/statvfs.h>
#endif

#ifndef VERSION
#  define VERSION "unknown"
#endif

#define MYNAME		"posix"
#define MYVERSION	MYNAME " library for " LUA_VERSION " / " VERSION

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


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

#include "lua52compat.h"

/* ISO C functions missing from the standard Lua libraries. */

static int Pabort(lua_State *L) /* abort() */
{
	(void)L; /* Avoid a compiler warning. */
	abort();
	return 0; /* Avoid a compiler warning (or possibly cause one
		     if the compiler's too clever, sigh). */
}

static int Praise(lua_State *L)
{
	int sig = luaL_checkint(L, 1);
	lua_pop(L, 1);
	lua_pushinteger(L, raise(sig));
	return 1;
}

static int bind_ctype(lua_State *L, int (*cb)(int))
{
	const char *s = luaL_checkstring(L, 1);
	char c = *s;
	lua_pop(L, 1);
	lua_pushboolean(L, cb((int)c));
	return 1;
}

static int Pisgraph(lua_State *L)
{
	return bind_ctype(L, &isgraph);
}

static int Pisprint(lua_State *L)
{
	return bind_ctype(L, &isprint);
}


/* File mode translation between octal codes and `rwxrwxrwx' strings,
   and between octal masks and `ugoa+-=rwx' strings. */

static const struct { char c; mode_t b; } M[] =
{
	{'r', S_IRUSR}, {'w', S_IWUSR}, {'x', S_IXUSR},
	{'r', S_IRGRP}, {'w', S_IWGRP}, {'x', S_IXGRP},
	{'r', S_IROTH}, {'w', S_IWOTH}, {'x', S_IXOTH},
};

static void pushmode(lua_State *L, mode_t mode)
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

static int rwxrwxrwx(mode_t *mode, const char *p)
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
			switch(count)
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

static int octal_mode(mode_t *mode, const char *p)
{
	mode_t tmp_mode = 0;
	char* endp = NULL;
	if (strlen(p) > 8)
		return -4; /* error -- bad syntax, string too long */
	tmp_mode = strtol(p, &endp, 8);
	if (p && endp && *p != '\0' && *endp == '\0') {
		*mode = tmp_mode;
		return 0;
	}
	return -4; /* error -- bad syntax */
}

static int mode_munch(mode_t *mode, const char* p)
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


/* Utility functions. */

typedef void (*Selector)(lua_State *L, int i, const void *data);

static int doselection(lua_State *L, int i, int n,
		       const char *const S[],
		       Selector F,
		       const void *data)
{
	if (lua_isnone(L, i) || lua_istable(L, i))
	{
		int j;
		if (lua_isnone(L, i))
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
	else
	{
		int k,n=lua_gettop(L);
		for (k=i; k<=n; k++)
		{
			int j=luaL_checkoption(L, k, NULL, S);
			F(L, j, data);
			lua_replace(L, k);
		}
		return n-i+1;
	}
}
#define doselection(L,i,S,F,d) (doselection)(L,i,sizeof(S)/sizeof(*S)-1,S,F,d)

static int lookup_symbol(const char * const S[], const int K[], const char *str)
{
	int i;
	for (i = 0; S[i] != NULL; i++)
		if (strcasecmp(S[i], str) == 0)
			return K[i];
	return -1;
}

static int pusherror(lua_State *L, const char *info)
{
	lua_pushnil(L);
	if (info==NULL)
		lua_pushstring(L, strerror(errno));
	else
		lua_pushfstring(L, "%s: %s", info, strerror(errno));
	lua_pushinteger(L, errno);
	return 3;
}

static int pushresult(lua_State *L, int i, const char *info)
{
	if (i==-1)
		return pusherror(L, info);
	lua_pushinteger(L, i);
	return 1;
}

static void badoption(lua_State *L, int i, const char *what, int option)
{
	luaL_argerror(L, 2,
		lua_pushfstring(L, "unknown %s option '%c'", what, option));
}

static uid_t mygetuid(lua_State *L, int i)
{
	if (lua_isnone(L, i))
		return -1;
	else if (lua_isnumber(L, i))
		return (uid_t) lua_tonumber(L, i);
	else if (lua_isstring(L, i))
	{
		struct passwd *p=getpwnam(lua_tostring(L, i));
		return (p==NULL) ? (uid_t)-1 : p->pw_uid;
	}
	else
		return luaL_typerror(L, i, "string or number");
}

static gid_t mygetgid(lua_State *L, int i)
{
	if (lua_isnone(L, i))
		return -1;
	else if (lua_isnumber(L, i))
		return (gid_t) lua_tonumber(L, i);
	else if (lua_isstring(L, i))
	{
		struct group *g=getgrnam(lua_tostring(L, i));
		return (g==NULL) ? (uid_t)-1 : g->gr_gid;
	}
	else
		return luaL_typerror(L, i, "string or number");
}


/* API functions */

static int Perrno(lua_State *L)			/** errno([n]) */
{
	int n = luaL_optint(L, 1, errno);
	lua_pushstring(L, strerror(n));
	lua_pushinteger(L, n);
	return 2;
}

static int Pset_errno(lua_State *L)
{
	errno = luaL_checkint(L, 1);
	return 0;
}

static int Pbasename(lua_State *L)		/** basename(path) */
{
	char *b;
	size_t len;
	void *ud;
	lua_Alloc lalloc = lua_getallocf(L, &ud);
	const char *path = luaL_checklstring(L, 1, &len);
	if ((b = lalloc(ud, NULL, 0, strlen(path) + 1)) == NULL)
		return pusherror(L, "lalloc");
	lua_pushstring(L, basename(strcpy(b,path)));
	lalloc(ud, b, 0, 0);
	return 1;
}

static int Pdirname(lua_State *L)		/** dirname(path) */
{
	char *b;
	size_t len;
	void *ud;
	lua_Alloc lalloc = lua_getallocf(L, &ud);
	const char *path = luaL_checklstring(L, 1, &len);
	if ((b = lalloc(ud, NULL, 0, strlen(path) + 1)) == NULL)
		return pusherror(L, "lalloc");
	lua_pushstring(L, dirname(strcpy(b,path)));
	lalloc(ud, b, 0, 0);
	return 1;
}

static int Pdir(lua_State *L)			/** dir([path]) */
{
	const char *path = luaL_optstring(L, 1, ".");
	DIR *d = opendir(path);
	if (d == NULL)
		return pusherror(L, path);
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
		lua_pushinteger(L, i-1);
		return 2;
	}
}

static int Pfnmatch(lua_State *L)     /** fnmatch(pattern, string [,flags]) */
{
	const char *pattern = lua_tostring(L, 1);
	const char *string = lua_tostring(L, 2);
	int flags = luaL_optint(L, 3, 0);
	int res = fnmatch(pattern, string, flags);
	if (res == 0)
		lua_pushboolean(L, 1);
	else if (res == FNM_NOMATCH)
		lua_pushboolean(L, 0);
	else
	{
		lua_pushstring(L, "fnmatch failed");
		lua_error(L);
	}
	return 1;
}

static int Pglob(lua_State *L)                  /** glob(pattern) */
{
	const char *pattern = luaL_optstring(L, 1, "*");
	glob_t globres;

	if (glob(pattern, 0, NULL, &globres))
		return pusherror(L, pattern);
	else
	{
		unsigned int i;
		lua_newtable(L);
		for (i=1; i<=globres.gl_pathc; i++)
		{
			lua_pushstring(L, globres.gl_pathv[i-1]);
			lua_rawseti(L, -2, i);
		}
		globfree(&globres);
		return 1;
	}
}

static int aux_files(lua_State *L)
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
	else
	{
		lua_pushstring(L, entry->d_name);
		return 1;
	}
}

static int dir_gc (lua_State *L)
{
	DIR *d = *(DIR **)lua_touserdata(L, 1);
	if (d!=NULL)
		closedir(d);
	return 0;
}

static int Pfiles(lua_State *L)			/** files([path]) */
{
	const char *path = luaL_optstring(L, 1, ".");
	DIR **d = (DIR **)lua_newuserdata(L, sizeof(DIR *));
	if (luaL_newmetatable(L, MYNAME " dir handle"))
	{
		lua_pushcfunction(L, dir_gc);
		lua_setfield(L, -2, "__gc");
	}
	lua_setmetatable(L, -2);
	*d = opendir(path);
	if (*d == NULL)
		return pusherror(L, path);
	lua_pushcclosure(L, aux_files, 1);
	return 1;
}

static int Pgetcwd(lua_State *L)		/** getcwd() */
{
	long size = pathconf(".", _PC_PATH_MAX);
	void *ud;
	lua_Alloc lalloc = lua_getallocf(L, &ud);
	char *b, *ret;
	if (size == -1)
		size = _POSIX_PATH_MAX; /* FIXME: Retry if this is not long enough */
	if ((b = lalloc(ud, NULL, 0, (size_t)size + 1)) == NULL)
		return pusherror(L, "lalloc");
	ret = getcwd(b, (size_t)size);
	if (ret != NULL)
		lua_pushstring(L, b);
	lalloc(ud, b, 0, 0);
	return (ret == NULL) ? pusherror(L, ".") : 1;
}

static int Pmkdir(lua_State *L)			/** mkdir(path) */
{
	const char *path = luaL_checkstring(L, 1);
	return pushresult(L, mkdir(path, 0777), path);
}


static int Pchdir(lua_State *L)			/** chdir(path) */
{
	const char *path = luaL_checkstring(L, 1);
	return pushresult(L, chdir(path), path);
}

static int Prmdir(lua_State *L)			/** rmdir(path) */
{
	const char *path = luaL_checkstring(L, 1);
	return pushresult(L, rmdir(path), path);
}

static int Punlink(lua_State *L)		/** unlink(path) */
{
	const char *path = luaL_checkstring(L, 1);
	return pushresult(L, unlink(path), path);
}

static int Plink(lua_State *L)			/** link(old,new,[symbolic]) */
{
	const char *oldpath = luaL_checkstring(L, 1);
	const char *newpath = luaL_checkstring(L, 2);
	return pushresult(L,
		(lua_toboolean(L,3) ? symlink : link)(oldpath, newpath), NULL);
}

static int Preadlink(lua_State *L)		/** readlink(path) */
{
	char *b;
	struct stat s;
	const char *path = luaL_checkstring(L, 1);
	void *ud;
	lua_Alloc lalloc = lua_getallocf(L, &ud);
	if (stat(path, &s))
		return pusherror(L, path);
	if ((b = lalloc(ud, NULL, 0, s.st_size + 1)) == NULL)
		return pusherror(L, "lalloc");
	ssize_t n = readlink(path, b, s.st_size);
	if (n != -1)
		lua_pushlstring(L, b, n);
	lalloc(ud, b, 0, 0);
	return (n == -1) ? pusherror(L, path) : 1;
}

static int Paccess(lua_State *L)		/** access(path,[mode]) */
{
	int mode=F_OK;
	const char *path=luaL_checkstring(L, 1);
	const char *s;
	for (s=luaL_optstring(L, 2, "f"); *s!=0 ; s++)
		switch (*s)
		{
			case ' ': break;
			case 'r': mode |= R_OK; break;
			case 'w': mode |= W_OK; break;
			case 'x': mode |= X_OK; break;
			case 'f': mode |= F_OK; break;
			default: badoption(L, 2, "mode", *s); break;
		}
	return pushresult(L, access(path, mode), path);
}

static int Pfileno(lua_State *L)	/** fileno(filehandle) */
{
	FILE *f = *(FILE**) luaL_checkudata(L, 1, LUA_FILEHANDLE);
	return pushresult(L, fileno(f), NULL);
}


static int Pmkfifo(lua_State *L)		/** mkfifo(path) */
{
	const char *path = luaL_checkstring(L, 1);
	return pushresult(L, mkfifo(path, 0777), path);
}

static int Pmkstemp(lua_State *L)                 /** mkstemp(path) */
{
	const char *path = luaL_checkstring(L, 1);
	void *ud;
	lua_Alloc lalloc = lua_getallocf(L, &ud);
	char *tmppath;
	int res;

	if ((tmppath = lalloc(ud, NULL, 0, strlen(path) + 1)) == NULL)
		return pusherror(L, "lalloc");
	strcpy(tmppath, path);
	res = mkstemp(tmppath);

	if (res == -1)
	{
		lalloc(ud, tmppath, 0, 0);
		return pusherror(L, path);
	}

	lua_pushinteger(L, res);
	lua_pushstring(L, tmppath);
	lalloc(ud, tmppath, 0, 0);
	return 2;
}

static int Pmkdtemp(lua_State *L)		/** mkdtemp(path) */
{
	const char *path = luaL_checkstring(L, 1);
	void *ud;
	lua_Alloc lalloc = lua_getallocf(L, &ud);
	char *tmppath;
	char *res;

	if ((tmppath = lalloc(ud, NULL, 0, strlen(path) + 1)) == NULL)
		return pusherror(L, "lalloc");
	strcpy(tmppath, path);
	res = mkdtemp(tmppath);

	if (res == NULL) {
		lalloc(ud, tmppath, 0, 0);
		return pusherror(L, path);
	}
	lua_pushstring(L, tmppath);
	lalloc(ud, tmppath, 0, 0);
	return 1;
}

static int runexec(lua_State *L, int use_shell)
{
	const char *path = luaL_checkstring(L, 1);
	int i,n=lua_gettop(L);
	char **argv = lua_newuserdata(L,(n+1)*sizeof(char*));
	argv[0] = (char*)path;
	for (i=1; i<n; i++)
		argv[i] = (char*)luaL_checkstring(L, i+1);
	argv[n] = NULL;
	if (use_shell)
		execvp(path, argv);
	else
		execv(path, argv);
	return pusherror(L, path);
}

static int Pexec(lua_State *L)			/** exec(path,[args]) */
{
	return runexec(L, 0);
}

static int Pexecp(lua_State *L)			/** execp(path,[args]) */
{
	return runexec(L, 1);
}

static int Pfork(lua_State *L)			/** fork() */
{
	return pushresult(L, fork(), NULL);
}

static int P_exit(lua_State *L) /* _exit() */
{
	pid_t ret = luaL_checkint(L, 1);
	_exit(ret);
	return 0; /* Avoid a compiler warning (or possibly cause one
		     if the compiler's too clever, sigh). */
}

/* from http://lua-users.org/lists/lua-l/2007-11/msg00346.html */
static int Prpoll(lua_State *L)   /** poll(filehandle, timeout) */
{
	struct pollfd fds;
	FILE* file = *(FILE**)luaL_checkudata(L,1,LUA_FILEHANDLE);
	int timeout = luaL_checkint(L,2);
	fds.fd = fileno(file);
	fds.events = POLLIN;
	return pushresult(L, poll(&fds,1,timeout), NULL);
}

static struct {
	short       bit;
	const char *name;
} Ppoll_event_map[] = {
#define MAP(_NAME) \
	{POLL##_NAME, #_NAME}
	MAP(IN),
	MAP(PRI),
	MAP(OUT),
	MAP(ERR),
	MAP(HUP),
	MAP(NVAL),
#undef MAP
};

#define PPOLL_EVENT_NUM (sizeof(Ppoll_event_map) / sizeof(*Ppoll_event_map))

static void Ppoll_events_createtable(lua_State *L)
{
	lua_createtable(L, 0, PPOLL_EVENT_NUM);
}

static short Ppoll_events_from_table(lua_State *L, int table)
{
	short   events  = 0;
	size_t  i;

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	for (i = 0; i < PPOLL_EVENT_NUM; i++)
	{
		lua_getfield(L, table, Ppoll_event_map[i].name);
		if (lua_toboolean(L, -1))
			events |= Ppoll_event_map[i].bit;
		lua_pop(L, 1);
	}

	return events;
}

static void Ppoll_events_to_table(lua_State *L, int table, short events)
{
	size_t  i;

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	for (i = 0; i < PPOLL_EVENT_NUM; i++)
	{
		lua_pushboolean(L, events & Ppoll_event_map[i].bit);
		lua_setfield(L, table, Ppoll_event_map[i].name);
	}
}

static nfds_t Ppoll_fd_list_check_table(lua_State  *L,
					int         table)
{
	nfds_t          fd_num      = 0;

	/*
	 * Assume table is an argument number.
	 * Should be an assert(table > 0).
	 */

	luaL_checktype(L, table, LUA_TTABLE);

	/* Nil key - the one before first */
	lua_pushnil(L);

	/* Push each key/value pair, popping previous key */
	while (lua_next(L, 1) != 0)
	{
		/* Verify the fd key */
		luaL_argcheck(L, lua_isnumber(L, -2), table,
					  "contains non-integer key(s)");

		/* Verify the table value */
		luaL_argcheck(L, lua_istable(L, -1), table,
					  "contains non-table value(s)");
		lua_getfield(L, -1, "events");
		luaL_argcheck(L, lua_istable(L, -1), table,
					  "contains invalid value table(s)");
		lua_pop(L, 1);
		lua_getfield(L, -1, "revents");
		luaL_argcheck(L, lua_isnil(L, -1) || lua_istable(L, -1), table,
					  "contains invalid value table(s)");
		lua_pop(L, 1);

		/* Remove value (but leave the key) */
		lua_pop(L, 1);

		/* Count the fds */
		fd_num++;
	}

	return fd_num;
}

static void Ppoll_fd_list_from_table(lua_State         *L,
				     int                table,
				     struct pollfd     *fd_list)
{
	struct pollfd  *pollfd  = fd_list;

	/*
	 * Assume the table didn't change since
	 * the call to Ppoll_fd_list_check_table
	 */

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	/* Nil key - the one before first */
	lua_pushnil(L);

	/* Push each key/value pair, popping previous key */
	while (lua_next(L, table) != 0)
	{
		/* Transfer the fd key */
		pollfd->fd = lua_tointeger(L, -2);

		/* Transfer "events" field from the value */
		lua_getfield(L, -1, "events");
		pollfd->events = Ppoll_events_from_table(L, -1);
		lua_pop(L, 1);

		/* Remove value (but leave the key) */
		lua_pop(L, 1);

		/* Proceed to next fd */
		pollfd++;
	}
}

static void Ppoll_fd_list_to_table(lua_State           *L,
				   int                  table,
				   const struct pollfd *fd_list)
{
	const struct pollfd    *pollfd  = fd_list;

	/*
	 * Assume the table didn't change since
	 * the call to Ppoll_fd_list_check_table.
	 */

	/* Convert to absolute index */
	if (table < 0)
		table = lua_gettop(L) + table + 1;

	/* Nil key - the one before first */
	lua_pushnil(L);

	/* Push each key/value pair, popping previous key */
	while (lua_next(L, 1) != 0)
	{
		/* Transfer "revents" field to the value */
		lua_getfield(L, -1, "revents");
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			Ppoll_events_createtable(L);
			lua_pushvalue(L, -1);
			lua_setfield(L, -3, "revents");
		}
		Ppoll_events_to_table(L, -1, pollfd->revents);
		lua_pop(L, 1);

		/* Remove value (but leave the key) */
		lua_pop(L, 1);

		/* Proceed to next fd */
		pollfd++;
	}
}

static int Ppoll(lua_State *L)          /** poll(pollfds, [timeout]) */
{
	struct pollfd   static_fd_list[16];
	struct pollfd  *fd_list;
	nfds_t          fd_num;
	int             timeout;
	int             result;

	fd_num = Ppoll_fd_list_check_table(L, 1);
	timeout = luaL_optint(L, 2, -1);

	fd_list = (fd_num <= sizeof(static_fd_list) / sizeof(*static_fd_list))
					? static_fd_list
					: lua_newuserdata(L, sizeof(*fd_list) * fd_num);


	Ppoll_fd_list_from_table(L, 1, fd_list);

	result = poll(fd_list, fd_num, timeout);

	/* If any of the descriptors changed state */
	if (result > 0)
		Ppoll_fd_list_to_table(L, 1, fd_list);

	return pushresult(L, result, NULL);
}

static int Pwait(lua_State *L)			/** wait([pid]) */
{
	int status = 0;
	pid_t pid = luaL_optint(L, 1, -1);
	int options = luaL_optint(L, 2, 0);

	pid = waitpid(pid, &status, options);
	if (pid == -1)
		return pusherror(L, NULL);
	lua_pushinteger(L, pid);
	if (WIFEXITED(status))
	{
		lua_pushliteral(L,"exited");
		lua_pushinteger(L, WEXITSTATUS(status));
		return 3;
	}
	else if (WIFSIGNALED(status))
	{
		lua_pushliteral(L,"killed");
		lua_pushinteger(L, WTERMSIG(status));
		return 3;
	}
	else if (WIFSTOPPED(status))
	{
		lua_pushliteral(L,"stopped");
		lua_pushinteger(L, WSTOPSIG(status));
		return 3;
	}
	return 1;
}

static int Pkill(lua_State *L)			/** kill(pid,[sig]) */
{
	pid_t pid = luaL_checkint(L, 1);
	int sig = luaL_optint(L, 2, SIGTERM);
	return pushresult(L, kill(pid, sig), NULL);
}

static int Psetpid(lua_State *L)		/** setpid(option,...) */
{
	const char *what=luaL_checkstring(L, 1);
	switch (*what)
	{
		case 'U':
			return pushresult(L, seteuid(mygetuid(L, 2)), NULL);
		case 'u':
			return pushresult(L, setuid(mygetuid(L, 2)), NULL);
		case 'G':
			return pushresult(L, setegid(mygetgid(L, 2)), NULL);
		case 'g':
			return pushresult(L, setgid(mygetgid(L, 2)), NULL);
		case 's':
			return pushresult(L, setsid(), NULL);
		case 'p':
		{
			pid_t pid  = luaL_checkint(L, 2);
			pid_t pgid = luaL_checkint(L, 3);
			return pushresult(L, setpgid(pid,pgid), NULL);
		}
		default:
			badoption(L, 2, "id", *what);
			return 0;
	}
}

static int Psleep(lua_State *L)			/** sleep(seconds) */
{
	unsigned int seconds = luaL_checkint(L, 1);
	lua_pushinteger(L, sleep(seconds));
	return 1;
}

static int Pnanosleep(lua_State *L)		/** nanosleep(seconds, nseconds) */
{
	struct timespec req;
	struct timespec rem;
	int ret;
	req.tv_sec = luaL_checkint(L, 1);
	req.tv_nsec = luaL_checkint(L, 2);
	ret = pushresult (L, nanosleep(&req, &rem), NULL);
	if (ret == 3 && errno == EINTR)
	{
		lua_pushinteger (L, rem.tv_sec);
		lua_pushinteger (L, rem.tv_nsec);
		ret += 2;
	}
	return ret;
}

static int Psetenv(lua_State *L)		/** setenv(name,value,[over]) */
{
	const char *name=luaL_checkstring(L, 1);
	const char *value=luaL_optstring(L, 2, NULL);
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

static int Pgetenv(lua_State *L)		/** getenv([name]) */
{
	if (lua_isnone(L, 1))
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
				lua_pushstring(L,s);
				lua_pushboolean(L,1);
			}
			else
			{
				lua_pushlstring(L,s,eq-s);
				lua_pushstring(L,eq+1);
			}
			lua_settable(L,-3);
		}
	}
	else
		lua_pushstring(L, getenv(luaL_checkstring(L, 1)));
	return 1;
}

static int Pumask(lua_State *L)			/** umask([mode]) */
{
	mode_t mode;
	umask(mode=umask(0));
	mode=(~mode)&0777;
	if (!lua_isnone(L, 1))
	{
		if (mode_munch(&mode, luaL_checkstring(L, 1)))
		{
			lua_pushnil(L);
			return 1;
		}
		mode&=0777;
		umask(~mode);
	}
	pushmode(L, mode);
	return 1;
}

static int Popen(lua_State *L)			/** open(path, flags[, mode]) */
{
	const char *path = luaL_checkstring(L, 1);
	int flags = luaL_checkint(L, 2);
	mode_t mode;
	if (flags & O_CREAT) {
		const char *modestr = luaL_checkstring(L, 3);
		if (mode_munch(&mode, modestr))
			luaL_argerror(L, 3, "bad mode");
	}
	return pushresult(L, open(path, flags, mode), path);
}

static int Pclose(lua_State *L)			/** close(n) */
{
	int fd = luaL_checkint(L, 1);
	return pushresult(L, close(fd), NULL);
}

static int Pdup(lua_State *L)           /** dup(fd) */
{
	int fd = luaL_checkint(L, 1);
	return pushresult(L, dup(fd), NULL);
}

static int Pdup2(lua_State *L)          /** dup2(oldfd,newfd) */
{
	int oldfd = luaL_checkint(L, 1);
	int newfd = luaL_checkint(L, 2);
	return pushresult(L, dup2(oldfd, newfd), NULL);
}

static int Ppipe(lua_State *L)          /** pipe(pipefd[2]) */
{
	int pipefd[2];
	int rc = pipe(pipefd);
	if(rc < 0)
		return pusherror(L, "pipe");
	lua_pushinteger(L, pipefd[0]);
	lua_pushinteger(L, pipefd[1]);
	return 2;
}

static int Pchmod(lua_State *L)			/** chmod(path,mode) */
{
	mode_t mode;
	struct stat s;
	const char *path = luaL_checkstring(L, 1);
	const char *modestr = luaL_checkstring(L, 2);
	if (stat(path, &s))
		return pusherror(L, path);
	mode = s.st_mode;
	if (mode_munch(&mode, modestr))
		luaL_argerror(L, 2, "bad mode");
	return pushresult(L, chmod(path, mode), path);
}

static int Pread(lua_State *L)			/** buf = read(fd, count) */
{
	int fd = luaL_checkint(L, 1);
	int count = luaL_checkint(L, 2), ret;
	void *ud, *buf;
	lua_Alloc lalloc = lua_getallocf(L, &ud);

	/* Reset errno in case lalloc doesn't set it */
	errno = 0;
	if ((buf = lalloc(ud, NULL, 0, count)) == NULL && count > 0)
		return pusherror(L, "lalloc");

	ret = read(fd, buf, count);
	if (ret < 0)
		return pusherror(L, NULL);

	lua_pushlstring(L, buf, ret);

	lalloc(ud, buf, 0, 0);

	return 1;
}

static int Pwrite(lua_State *L)			/** write(fd, buf) */
{
	int fd = luaL_checkint(L, 1);
	const char *buf = luaL_checkstring(L, 2);
	return pushresult(L, write(fd, buf, lua_objlen(L, 2)), NULL);
}

static int Pchown(lua_State *L)			/** chown(path,uid,gid) */
{
	const char *path = luaL_checkstring(L, 1);
	uid_t uid = mygetuid(L, 2);
	gid_t gid = mygetgid(L, 3);
	return pushresult(L, chown(path, uid, gid), path);
}

static int Putime(lua_State *L)			/** utime(path,[mtime,atime]) */
{
	struct utimbuf times;
	time_t currtime = time(NULL);
	const char *path = luaL_checkstring(L, 1);
	times.modtime = luaL_optnumber(L, 2, currtime);
	times.actime  = luaL_optnumber(L, 3, currtime);
	return pushresult(L, utime(path, &times), path);
}


static void FgetID(lua_State *L, int i, const void *data)
{
	switch (i)
	{
	case 0:
		lua_pushinteger(L, getegid());
		break;
	case 1:
		lua_pushinteger(L, geteuid());
		break;
	case 2:
		lua_pushinteger(L, getgid());
		break;
	case 3:
		lua_pushinteger(L, getuid());
		break;
	case 4:
		lua_pushinteger(L, getpgrp());
		break;
	case 5:
		lua_pushinteger(L, getpid());
		break;
	case 6:
		lua_pushinteger(L, getppid());
		break;
	}
}

static const char *const SgetID[] =
{
	"egid", "euid", "gid", "uid", "pgrp", "pid", "ppid", NULL
};

static int Pgetpid(lua_State *L)		/** getpid([options]) */
{
	return doselection(L, 1, SgetID, FgetID, NULL);
}


static int Phostid(lua_State *L)		/** hostid() */
{
	lua_pushinteger(L, gethostid());
	return 1;
}


static int Pttyname(lua_State *L)		/** ttyname([fd]) */
{
	int fd=luaL_optint(L, 1, 0);
	lua_pushstring(L, ttyname(fd));
	return 1;
}


static int Pctermid(lua_State *L)		/** ctermid() */
{
	char b[L_ctermid];
	lua_pushstring(L, ctermid(b));
	return 1;
}


static int Pgetlogin(lua_State *L)		/** getlogin() */
{
	lua_pushstring(L, getlogin());
	return 1;
}


static void Fgetpasswd(lua_State *L, int i, const void *data)
{
	const struct passwd *p=data;
	switch (i)
	{
	case 0:
		lua_pushstring(L, p->pw_name);
		break;
	case 1:
		lua_pushinteger(L, p->pw_uid);
		break;
	case 2:
		lua_pushinteger(L, p->pw_gid);
		break;
	case 3:
		lua_pushstring(L, p->pw_dir);
		break;
	case 4:
		lua_pushstring(L, p->pw_shell);
		break;
	case 5:
		lua_pushstring(L, p->pw_passwd);
		break;
	}
}

static const char *const Sgetpasswd[] =
{
	"name", "uid", "gid", "dir", "shell", "passwd", NULL
};


static int Pgetpasswd(lua_State *L)		/** getpasswd(name|id,[sel]) */
{
	struct passwd *p=NULL;
	if (lua_isnoneornil(L, 1))
		p = getpwuid(geteuid());
	else if (lua_isnumber(L, 1))
		p = getpwuid((uid_t)lua_tonumber(L, 1));
	else if (lua_isstring(L, 1))
		p = getpwnam(lua_tostring(L, 1));
	else
		luaL_typerror(L, 1, "string or number");
	if (p==NULL)
		lua_pushnil(L);
	else
		return doselection(L, 2, Sgetpasswd, Fgetpasswd, p);
	return 1;
}


static int Pgetgroup(lua_State *L)		/** getgroup(name|id) */
{
	struct group *g = NULL;
	if (lua_isnumber(L, 1))
		g = getgrgid((gid_t)lua_tonumber(L, 1));
	else if (lua_isstring(L, 1))
		g = getgrnam(lua_tostring(L, 1));
	else
		luaL_typerror(L, 1, "string or number");
	if (g == NULL)
		lua_pushnil(L);
	else {
		int i;
		lua_newtable(L);
		lua_pushstring(L, g->gr_name);
		lua_setfield(L, -2, "name");
		lua_pushinteger(L, g->gr_gid);
		lua_setfield(L, -2, "gid");
		for (i = 0; g->gr_mem[i] != NULL; i++)
		{
			lua_pushstring(L, g->gr_mem[i]);
			lua_rawseti(L, -2, i + 1);
		}
	}
	return 1;
}

#if _POSIX_VERSION >= 200112L
static int Pgetgroups(lua_State *L)		/** getgroups() */
{
	int n_group_slots = getgroups(0, NULL);

	if (n_group_slots < 0)
		return pusherror(L, NULL);
	else if (n_group_slots == 0)
		lua_newtable(L);
	else {
		gid_t  *group;
		int     n_groups;
		int     i;

		group = lua_newuserdata(L, sizeof(*group) * n_group_slots);

		n_groups = getgroups(n_group_slots, group);
		if (n_groups < 0)
			return pusherror(L, NULL);

		lua_createtable(L, n_groups, 0);
		for (i = 0; i < n_groups; i++) {
			lua_pushinteger(L, group[i]);
			lua_rawseti(L, -2, i + 1);
		}
	}

	return 1;
}
#endif

struct mytimes
{
 struct tms t;
 clock_t elapsed;
};

#define pushtime(L,x)	lua_pushnumber(L, ((lua_Number)x)/clk_tck)

static void Ftimes(lua_State *L, int i, const void *data)
{
    static long clk_tck = 0;
	const struct mytimes *t=data;

    if( !clk_tck){ clk_tck= sysconf(_SC_CLK_TCK);}
	switch (i)
	{
		case 0: pushtime(L, t->t.tms_utime); break;
		case 1: pushtime(L, t->t.tms_stime); break;
		case 2: pushtime(L, t->t.tms_cutime); break;
		case 3: pushtime(L, t->t.tms_cstime); break;
		case 4: pushtime(L, t->elapsed); break;
	}
}

static const char *const Stimes[] =
{
	"utime", "stime", "cutime", "cstime", "elapsed", NULL
};

static int Ptimes(lua_State *L)			/** times([options]) */
{
	struct mytimes t;
	t.elapsed = times(&t.t);
	return doselection(L, 1, Stimes, Ftimes, &t);
}


static const char *filetype(mode_t m)
{
	if (S_ISREG(m))
		return "regular";
	else if (S_ISLNK(m))
		return "link";
	else if (S_ISDIR(m))
		return "directory";
	else if (S_ISCHR(m))
		return "character device";
	else if (S_ISBLK(m))
		return "block device";
	else if (S_ISFIFO(m))
		return "fifo";
	else if (S_ISSOCK(m))
		return "socket";
	else
		return "?";
}

static void Fstat(lua_State *L, int i, const void *data)
{
	const struct stat *s=data;
	switch (i)
	{
	case 0:
		pushmode(L, s->st_mode);
		break;
	case 1:
		lua_pushinteger(L, s->st_ino);
		break;
	case 2:
		lua_pushinteger(L, s->st_dev);
		break;
	case 3:
		lua_pushinteger(L, s->st_nlink);
		break;
	case 4:
		lua_pushinteger(L, s->st_uid);
		break;
	case 5:
		lua_pushinteger(L, s->st_gid);
		break;
	case 6:
		lua_pushinteger(L, s->st_size);
		break;
	case 7:
		lua_pushinteger(L, s->st_atime);
		break;
	case 8:
		lua_pushinteger(L, s->st_mtime);
		break;
	case 9:
		lua_pushinteger(L, s->st_ctime);
		break;
	case 10:
		lua_pushstring(L, filetype(s->st_mode));
		break;
	}
}

static const char *const Sstat[] =
{
	"mode", "ino", "dev", "nlink", "uid", "gid",
	"size", "atime", "mtime", "ctime", "type",
	NULL
};

static int Pstat(lua_State *L)			/** stat(path,[options]) */
{
	struct stat s;
	const char *path=luaL_checkstring(L, 1);
	if (lstat(path,&s)==-1)
		return pusherror(L, path);
	return doselection(L, 2, Sstat, Fstat, &s);
}

#if defined (HAVE_STATVFS)
static void Fstatvfs(lua_State *L, int i, const void *data)
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

static int Pstatvfs(lua_State *L)		/** statvfs(path,[options]) */
{
	struct statvfs s;
	const char *path=luaL_checkstring(L, 1);
	if (statvfs(path,&s)==-1)
		return pusherror(L, path);
	return doselection(L, 2, Sstatvfs, Fstatvfs, &s);
}
#endif

static int Pfcntl(lua_State *L)
{
	int fd = luaL_optint(L, 1, 0);
	int cmd = luaL_checkint(L, 2);
	int arg = luaL_optint(L, 3, 0);
	return pushresult(L, fcntl(fd, cmd, arg), "fcntl");
}


static int Puname(lua_State *L)			/** uname([string]) */
{
	struct utsname u;
	luaL_Buffer b;
	const char *s;
	if (uname(&u)==-1)
		return pusherror(L, NULL);
	luaL_buffinit(L, &b);
	for (s=luaL_optstring(L, 1, "%s %n %r %v %m"); *s; s++)
		if (*s!='%')
			luaL_addchar(&b, *s);
		else switch (*++s)
		{
			case '%': luaL_addchar(&b, *s); break;
			case 'm': luaL_addstring(&b,u.machine); break;
			case 'n': luaL_addstring(&b,u.nodename); break;
			case 'r': luaL_addstring(&b,u.release); break;
			case 's': luaL_addstring(&b,u.sysname); break;
			case 'v': luaL_addstring(&b,u.version); break;
			default: badoption(L, 2, "format", *s); break;
		}
	luaL_pushresult(&b);
	return 1;
}

#define pathconf_map \
	MENTRY( _LINK_MAX         ) \
	MENTRY( _MAX_CANON        ) \
	MENTRY( _MAX_INPUT        ) \
	MENTRY( _NAME_MAX         ) \
	MENTRY( _PATH_MAX         ) \
	MENTRY( _PIPE_BUF         ) \
	MENTRY( _CHOWN_RESTRICTED ) \
	MENTRY( _NO_TRUNC         ) \
	MENTRY( _VDISABLE         )

static const int Kpathconf[] =
{
#define MENTRY(_f) LPOSIX_SPLICE(_PC, _f),
	pathconf_map
#undef MENTRY
	-1
};

static void Fpathconf(lua_State *L, int i, const void *data)
{
	const char *path=data;
	lua_pushinteger(L, pathconf(path, Kpathconf[i]));
}

static const char *const Spathconf[] =
{
#define MENTRY(_f) LPOSIX_STR_1(_f),
	pathconf_map
#undef MENTRY
	NULL
};

static int Ppathconf(lua_State *L)		/** pathconf([path,options]) */
{
	const char *path = luaL_optstring(L, 1, ".");
	return doselection(L, 2, Spathconf, Fpathconf, path);
}

#define sysconf_map \
	MENTRY( _ARG_MAX     ) \
	MENTRY( _CHILD_MAX   ) \
	MENTRY( _CLK_TCK     ) \
	MENTRY( _NGROUPS_MAX ) \
	MENTRY( _STREAM_MAX  ) \
	MENTRY( _TZNAME_MAX  ) \
	MENTRY( _OPEN_MAX    ) \
	MENTRY( _JOB_CONTROL ) \
	MENTRY( _SAVED_IDS   ) \
	MENTRY( _VERSION     )

static const int Ksysconf[] =
{
#define MENTRY(_f) LPOSIX_SPLICE(_SC, _f),
	sysconf_map
#undef MENTRY
	-1
};

static void Fsysconf(lua_State *L, int i, const void *data)
{
	lua_pushinteger(L, sysconf(Ksysconf[i]));
}

static const char *const Ssysconf[] =
{
#define MENTRY(_f) LPOSIX_STR_1(_f),
	sysconf_map
#undef MENTRY
	NULL
};

static int Psysconf(lua_State *L)		/** sysconf([options]) */
{
	return doselection(L, 1, Ssysconf, Fsysconf, NULL);
}

#if _POSIX_VERSION >= 200112L
/* syslog funcs */
static int Popenlog(lua_State *L)	/** openlog(ident, [option], [facility]) */
{
	const char *ident = luaL_checkstring(L, 1);
	int option = 0;
	int facility = luaL_optint(L, 3, LOG_USER);
	const char *s = luaL_optstring(L, 2, "");
	while (*s) {
		switch (*s) {
			case ' ': break;
			case 'c': option |= LOG_CONS; break;
			case 'n': option |= LOG_NDELAY; break;
			case 'p': option |= LOG_PID; break;
			default: badoption(L, 2, "option", *s); break;
		}
		s++;
	}
	openlog(ident, option, facility);
	return 0;
}


static int Psyslog(lua_State *L)		/** syslog(priority, message) */
{
	int priority = luaL_checkint(L, 1);
	const char *msg = luaL_checkstring(L, 2);
	syslog(priority, "%s", msg);
	return 0;
}

static int Pcloselog(lua_State *L)		/** closelog() */
{
	closelog();
	return 0;
}

static int Psetlogmask(lua_State* L)            /** setlogmask(priority...) */
{
	int argno = lua_gettop(L);
	int mask = 0;
	int i;

	for (i=1; i <= argno; i++)
		mask |= LOG_MASK(luaL_checkint(L,i));

	return pushresult(L, setlogmask(mask),"setlogmask");
}
#endif

#if defined(HAVE_CRYPT)
static int Pcrypt(lua_State *L)		/** crypt(string,salt) */
{
	const char *str, *salt;
	char *res;

	str = luaL_checkstring(L, 1);
	salt = luaL_checkstring(L, 2);
	if (strlen(salt) < 2)
		luaL_error(L, "not enough salt");

	res = crypt(str, salt);
	lua_pushstring(L, res);

	return 1;
}
#endif


/*	Like POSIX's setrlimit()/getrlimit() API functions.
 *
 *	Syntax:
 *	posix.setrlimit(resource, softlimit, hardlimit)
 *
 *	A negative or nil limit will be replaced by the current
 *	limit, obtained by calling getrlimit().
 *
 *	Valid resources are:
 *		"core", "cpu", "data", "fsize",
 *		"nofile", "stack", "as"
 *
 *	Example usage:
 *	posix.setrlimit("nofile", 1000, 2000)
 */

#define rlimit_map \
	MENTRY( _CORE   ) \
	MENTRY( _CPU    ) \
	MENTRY( _DATA   ) \
	MENTRY( _FSIZE  ) \
	MENTRY( _NOFILE ) \
	MENTRY( _STACK  ) \
	MENTRY( _AS     )

static const int Krlimit[] =
{
#define MENTRY(_f) LPOSIX_SPLICE(RLIMIT, _f),
	rlimit_map
#undef MENTRY
	-1
};

static const char *const Srlimit[] =
{
#define MENTRY(_f) LPOSIX_STR_1(_f),
	rlimit_map
#undef MENTRY
	NULL
};

static int Psetrlimit(lua_State *L)     /** setrlimit(resource,soft[,hard]) */
{
	int softlimit;
	int hardlimit;
	const char *rid_str;
	int rid = 0;
	struct rlimit lim;
	struct rlimit lim_current;
	int rc;

	rid_str = luaL_checkstring(L, 1);
	softlimit = luaL_optint(L, 2, -1);
	hardlimit = luaL_optint(L, 3, -1);
	rid = lookup_symbol(Srlimit, Krlimit, rid_str);

	if (softlimit < 0 || hardlimit < 0)
		if ((rc = getrlimit(rid, &lim_current)) < 0)
			return pushresult(L, rc, "getrlimit");

	if (softlimit < 0)
		lim.rlim_cur = lim_current.rlim_cur;
	else
		lim.rlim_cur = softlimit;
	if (hardlimit < 0)
		lim.rlim_max = lim_current.rlim_max;
	else lim.rlim_max = hardlimit;

	return pushresult(L, setrlimit(rid, &lim), "setrlimit");
}

/* FIXME: Use doselection. */
static int Pgetrlimit(lua_State *L)     /** getrlimit(resource) */
{
	struct rlimit lim;
	int rid, rc;
	const char *rid_str = luaL_checkstring(L, 1);
	rid = lookup_symbol(Srlimit, Krlimit, rid_str);
	rc = getrlimit(rid, &lim);
	if (rc < 0)
		return pusherror(L, "getrlimit");
	lua_pushinteger(L, lim.rlim_cur);
	lua_pushinteger(L, lim.rlim_max);
	return 2;
}


/* Time and date */

static int Pgettimeofday(lua_State *L)		/** gettimeofday() */
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) == -1)
		return pusherror(L, "gettimeofday");
	lua_pushinteger(L, tv.tv_sec);
	lua_pushinteger(L, tv.tv_usec);
	return 2;
}

static int Ptime(lua_State *L)			/** time() */
{
	time_t t = time(NULL);
	if ((time_t)-1 == t)
		return pusherror(L, "time");
	lua_pushinteger(L, t);
	return 1;
}

/* N.B. In Lua broken-down time tables the month field is 1-based not
   0-based, and the year field is the full year, not years since
   1900. */

static void totm(lua_State *L, int n, struct tm *tp)
{
	luaL_checktype(L, n, LUA_TTABLE);
	lua_getfield(L, n, "sec");
	tp->tm_sec = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "min");
	tp->tm_min = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "hour");
	tp->tm_hour = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "monthday");
	tp->tm_mday = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "month");
	tp->tm_mon = luaL_optint(L, -1, 0) - 1;
	lua_pop(L, 1);
	lua_getfield(L, n, "year");
	tp->tm_year = luaL_optint(L, -1, 0) - 1900;
	lua_pop(L, 1);
	lua_getfield(L, n, "weekday");
	tp->tm_wday = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "yearday");
	tp->tm_yday = luaL_optint(L, -1, 0);
	lua_pop(L, 1);
	lua_getfield(L, n, "is_dst");
	tp->tm_isdst = (lua_type(L, -1) == LUA_TBOOLEAN) ? lua_toboolean(L, -1) : 0;
	lua_pop(L, 1);
}

static void pushtm(lua_State *L, struct tm t)
{
	lua_createtable(L, 0, 9);
	lua_pushinteger(L, t.tm_sec);
	lua_setfield(L, -2, "sec");
	lua_pushinteger(L, t.tm_min);
	lua_setfield(L, -2, "min");
	lua_pushinteger(L, t.tm_hour);
	lua_setfield(L, -2, "hour");
	lua_pushinteger(L, t.tm_mday);
	lua_setfield(L, -2, "monthday");
	lua_pushinteger(L, t.tm_mon + 1);
	lua_setfield(L, -2, "month");
	lua_pushinteger(L, t.tm_year + 1900);
	lua_setfield(L, -2, "year");
	lua_pushinteger(L, t.tm_wday);
	lua_setfield(L, -2, "weekday");
	lua_pushinteger(L, t.tm_yday);
	lua_setfield(L, -2, "yearday");
	lua_pushboolean(L, t.tm_isdst);
	lua_setfield(L, -2, "is_dst");
}

static int Plocaltime(lua_State *L)		/** localtime([time]) */
{
	struct tm res;
	time_t t = luaL_optint(L, 1, time(NULL));
	if (localtime_r(&t, &res) == NULL)
		return pusherror(L, "localtime");
	pushtm(L, res);
	return 1;
}

static int Pgmtime(lua_State *L)		/** gmtime([time]) */
{
	struct tm res;
	time_t t = luaL_optint(L, 1, time(NULL));
	if (gmtime_r(&t, &res) == NULL)
		return pusherror(L, "localtime");
	pushtm(L, res);
	return 1;
}

#if defined (_XOPEN_REALTIME) && _XOPEN_REALTIME != -1
static int get_clk_id_const(const char *str)
{
	if (str == NULL)
		return CLOCK_REALTIME;
	else if (strcmp(str, "monotonic") == 0)
		return CLOCK_MONOTONIC;
	else if (strcmp(str, "process_cputime_id") == 0)
		return CLOCK_PROCESS_CPUTIME_ID;
	else if (strcmp(str, "thread_cputime_id") == 0)
		return CLOCK_THREAD_CPUTIME_ID;
	else
		return CLOCK_REALTIME;
}

static int Pclock_getres(lua_State *L)		/** clock_getres([clockid]) */
{
	struct timespec res;
	const char *str = lua_tostring(L, 1);
	if (clock_getres(get_clk_id_const(str), &res) == -1)
		return pusherror(L, "clock_getres");
	lua_pushinteger(L, res.tv_sec);
	lua_pushinteger(L, res.tv_nsec);
	return 2;
}

static int Pclock_gettime(lua_State *L)		/** clock_gettime([clockid]) */
{
	struct timespec res;
	const char *str = lua_tostring(L, 1);
	if (clock_gettime(get_clk_id_const(str), &res) == -1)
		return pusherror(L, "clock_gettime");
	lua_pushinteger(L, res.tv_sec);
	lua_pushinteger(L, res.tv_nsec);
	return 2;
}
#endif

static int Pstrftime(lua_State *L)		/** strftime(format, [time]) */
{
	char tmp[256];
	const char *format = luaL_checkstring(L, 1);

	struct tm t;
	if (lua_isnil(L, 2)) {
		time_t now = time(NULL);
		localtime_r(&now, &t);
	} else {
		totm(L, 2, &t);
	}

	strftime(tmp, sizeof(tmp), format, &t);
	lua_pushstring(L, tmp);
	return 1;
}

static int Pstrptime(lua_State *L)		/** tm, next = strptime(s, format) */
{
	struct tm t;
	const char *s = luaL_checkstring(L, 1);
	const char *fmt = luaL_checkstring(L, 2);
	char *ret;

	memset(&t, 0, sizeof(struct tm));
	ret = strptime(s, fmt, &t);
	if (ret) {
		pushtm(L, t);
		lua_pushinteger(L, ret - s);
		return 2;
	} else
		return 0;
}

static int Pmktime(lua_State *L)		/** ctime = mktime(tm) */
{
	struct tm t;
	time_t ret;
	totm(L, 1, &t);
	ret = mktime(&t);
	if (ret == -1)
		return 0;
	lua_pushinteger(L, ret);
	return 1;
}


/* getopt_long */

/* N.B. We don't need the symbolic constants no_argument,
   required_argument and optional_argument, since their values are
   defined as 0, 1 and 2 respectively. */
static const char *const arg_types[] = {
	"none", "required", "optional", NULL
};

static int iter_getopt_long(lua_State *L)
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
	else {
		lua_pushinteger(L, ret);
		lua_pushinteger(L, longindex);
		lua_pushinteger(L, optind);
		lua_pushstring(L, optarg);
		return 4;
	}
}

/* for ret, longindex, optind, optarg in posix.getopt_long (arg, shortopts, longopts, opterr, optind) do ... end */
static int Pgetopt_long(lua_State *L)
{
	int argc, i, n;
	const char *shortopts;
	char **argv;
	struct option *longopts;

	luaL_checktype(L, 1, LUA_TTABLE);
	shortopts = luaL_checkstring(L, 2);
	luaL_checktype(L, 3, LUA_TTABLE);
	opterr = luaL_optinteger (L, 4, 0);
	optind = luaL_optinteger (L, 5, 1);

	argc = (int)lua_objlen(L, 1) + 1;

	lua_pushinteger(L, argc);

	lua_pushstring(L, shortopts);

	argv = lua_newuserdata(L, (argc + 1) * sizeof(char *));
	argv[argc] = NULL;
	for (i = 0; i < argc; i++) {
		lua_pushinteger(L, i);
		lua_gettable(L, 1);
		argv[i] = (char *)luaL_checkstring(L, -1);
	}

	n = (int)lua_objlen(L, 3);
	longopts = lua_newuserdata(L, (n + 1) * sizeof(struct option));
	longopts[n].name = NULL;
	longopts[n].has_arg = 0;
	longopts[n].flag = NULL;
	longopts[n].val = 0;
	for (i = 1; i <= n; i++) {
		const char *name;
		int has_arg, val;

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
		val = luaL_checkint(L, -1);
		lua_pop(L, 1);

		longopts[i - 1].name = name;
		longopts[i - 1].has_arg = has_arg;
		longopts[i - 1].flag = NULL;
		longopts[i - 1].val = val;
		lua_pop(L, 1);
	}

	/* Push remaining upvalues, and make and push closure. */
	lua_pushcclosure(L, iter_getopt_long, 4 + argc + n);

	return 1;
}


/* Signals */

static lua_State *signalL;
#define SIGNAL_QUEUE_MAX 25
static volatile sig_atomic_t signal_pending, defer_signal;
static volatile sig_atomic_t signal_count = 0;
static volatile sig_atomic_t signals[SIGNAL_QUEUE_MAX];

#define sigmacros_map \
	MENTRY( _DFL ) \
	MENTRY( _IGN )

static const char *const Ssigmacros[] =
{
#define MENTRY(_s) LPOSIX_STR_1(LPOSIX_SPLICE(_SIG, _s)),
	sigmacros_map
#undef MENTRY
	NULL
};

static void (*Fsigmacros[])(int) =
{
#define MENTRY(_s) LPOSIX_SPLICE(SIG, _s),
	sigmacros_map
#undef MENTRY
	NULL
};

static void sig_handle (lua_State *L, lua_Debug *ar) {
	/* Block all signals until we have run the Lua signal handler */
	sigset_t mask, oldmask;
	sigfillset(&mask);
	sigprocmask(SIG_SETMASK, &mask, &oldmask);

	(void)ar;  /* unused arg. */

	lua_sethook(L, NULL, 0, 0);

	/* Get signal handlers table */
	lua_pushlightuserdata(L, &signalL);
	lua_rawget(L, LUA_REGISTRYINDEX);

    /* Empty the signal queue */
    while (signal_count--) {
        sig_atomic_t signalno = signals[signal_count];
        /* Get handler */
        lua_pushinteger(L, signalno);
        lua_gettable(L, -2);
	
		/* Call handler with signal number */
		lua_pushinteger(L, signalno);
		if (lua_pcall(L, 1, 0, 0) != 0) 
            fprintf(stderr,"error in signal handler %d: %s\n",signalno,lua_tostring(L,-1));
	}
    signal_count = 0;  /* reset global to initial state */
    
	/* Having run the Lua signal handler, restore original signal mask */    
	sigprocmask(SIG_SETMASK, &oldmask, NULL);
}

static void sig_postpone (int i) {
    if (defer_signal) {
        signal_pending = i;
        return;
    }
    if (signal_count == SIGNAL_QUEUE_MAX)
        return;        
    defer_signal++;
    /* Queue signals */
    signals[signal_count] = i;
    signal_count ++;    
	lua_sethook(signalL, sig_handle, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
    defer_signal--;
    /* re-raise any pending signals */
    if (defer_signal == 0 && signal_pending != 0)
        raise (signal_pending);
}

static int sig_handler_wrap (lua_State *L) {
	int sig = luaL_checkinteger(L, lua_upvalueindex(1));
	void (*handler)(int) = lua_touserdata(L, lua_upvalueindex(2));
	handler(sig);
	return 0;
}

static int Psignal (lua_State *L)		/** old_handler = signal(signum, handler) */
/* N.B. Although this is the same API as signal(2), it uses sigaction for guaranteed semantics. */
{
	struct sigaction sa, oldsa;
	int sig = luaL_checkinteger(L, 1), ret;
	void (*handler)(int) = sig_postpone;

	/* Check handler is OK */
	switch (lua_type(L, 2)) {
	case LUA_TNIL:
	case LUA_TSTRING:
		handler = Fsigmacros[luaL_checkoption(L, 2, "SIG_DFL", Ssigmacros)];
		break;
	case LUA_TFUNCTION:
		if (lua_tocfunction(L, 2) == sig_handler_wrap) {
			lua_getupvalue(L, 2, 1);
			handler = lua_touserdata(L, -1);
			lua_pop(L, 1);
		}
	}

	/* Set up C signal handler, getting old handler */
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigfillset(&sa.sa_mask);
	ret = sigaction(sig, &sa, &oldsa);
	if (ret == -1)
		return 0;

	/* Set Lua handler if necessary */
	if (handler == sig_postpone) {
		lua_pushlightuserdata(L, &signalL); /* We could use an upvalue, but we need this for sig_handle anyway. */
		lua_rawget(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, 1);
		lua_pushvalue(L, 2);
		lua_rawset(L, -3);
		lua_pop(L, 1);
	}

	/* Push old handler as result */
	if (oldsa.sa_handler == sig_postpone) {
		lua_pushlightuserdata(L, &signalL);
		lua_rawget(L, LUA_REGISTRYINDEX);
		lua_pushvalue(L, 1);
		lua_rawget(L, -2);
	} else if (oldsa.sa_handler == SIG_DFL)
		lua_pushstring(L, "SIG_DFL");
	else if (oldsa.sa_handler == SIG_IGN)
		lua_pushstring(L, "SIG_IGN");
	else {
		lua_pushinteger(L, sig);
		lua_pushlightuserdata(L, oldsa.sa_handler);
		lua_pushcclosure(L, sig_handler_wrap, 2);
	}
	return 1;
}


static const luaL_Reg R[] =
{
#define MENTRY(_s) {LPOSIX_STR_1(_s), (_s)}
	MENTRY( P_exit		),
	MENTRY( Pabort		),
	MENTRY( Paccess		),
	MENTRY( Pbasename	),
	MENTRY( Pchdir		),
	MENTRY( Pchmod		),
	MENTRY( Pchown		),
#if defined (_XOPEN_REALTIME) && _XOPEN_REALTIME != -1
	MENTRY( Pclock_getres	),
	MENTRY( Pclock_gettime	),
#endif
	MENTRY( Pclose		),
#if defined (HAVE_CRYPT)
	MENTRY( Pcrypt		),
#endif
	MENTRY( Pctermid	),
	MENTRY( Pdirname	),
	MENTRY( Pdir		),
	MENTRY( Pdup		),
	MENTRY( Pdup2		),
	MENTRY( Perrno		),
	MENTRY( Pexec		),
	MENTRY( Pexecp		),
	MENTRY( Pfcntl		),
	MENTRY( Pfileno		),
	MENTRY( Pfiles		),
	MENTRY( Pfnmatch	),
	MENTRY( Pfork		),
	MENTRY( Pgetcwd		),
	MENTRY( Pgetenv		),
	MENTRY( Pgetgroup	),
#if _POSIX_VERSION >= 200112L
	MENTRY( Pgetgroups	),
#endif
	MENTRY( Pgetlogin	),
	MENTRY( Pgetopt_long	),
	MENTRY( Pgetpasswd	),
	MENTRY( Pgetpid		),
	MENTRY( Pgetrlimit	),
	MENTRY( Pgettimeofday	),
	MENTRY( Pglob		),
	MENTRY( Pgmtime		),
	MENTRY( Phostid		),
	MENTRY( Pisgraph	),
	MENTRY( Pisprint	),
	MENTRY( Pkill		),
	MENTRY( Plink		),
	MENTRY( Plocaltime	),
	MENTRY( Pmkdir		),
	MENTRY( Pmkfifo		),
	MENTRY( Pmkstemp	),
	MENTRY( Pmkdtemp	),
	MENTRY( Pmktime		),
	MENTRY( Popen		),
	MENTRY( Ppathconf	),
	MENTRY( Ppipe		),
	MENTRY( Praise		),
	MENTRY( Pread		),
	MENTRY( Preadlink	),
	MENTRY( Prmdir		),
	MENTRY( Prpoll		),
	MENTRY( Ppoll		),
	MENTRY( Pset_errno	),
	MENTRY( Psetenv		),
	MENTRY( Psetpid		),
	MENTRY( Psetrlimit	),
	MENTRY( Psignal		),
	MENTRY( Psleep		),
	MENTRY( Pnanosleep	),
	MENTRY( Pstat		),
	MENTRY( Pstrftime	),
	MENTRY( Pstrptime	),
	MENTRY( Psysconf	),
	MENTRY( Ptime		),
	MENTRY( Ptimes		),
	MENTRY( Pttyname	),
	MENTRY( Punlink		),
	MENTRY( Pumask		),
	MENTRY( Puname		),
	MENTRY( Putime		),
	MENTRY( Pwait		),
	MENTRY( Pwrite		),

#if _POSIX_VERSION >= 200112L
	MENTRY( Popenlog	),
	MENTRY( Psyslog		),
	MENTRY( Pcloselog	),
	MENTRY( Psetlogmask	),
#endif
#if defined (HAVE_STATVFS)
	MENTRY( Pstatvfs	),
#endif
#undef MENTRY
	{NULL,	NULL}
};

#define set_integer_const(key, value)	\
	lua_pushinteger(L, value);	\
	lua_setfield(L, -2, key)

LUALIB_API int luaopen_posix_c (lua_State *L)
{
	luaL_register(L, MYNAME, R);

	lua_pushliteral(L, MYVERSION);
	lua_setfield(L, -2, "version");

	/* stdio.h constants */
	/* Those that are omitted already have a Lua interface, or alternative. */
	set_integer_const( "_IOFBF",		_IOFBF		);
	set_integer_const( "_IOLBF",		_IOLBF		);
	set_integer_const( "_IONBF",		_IONBF		);
	set_integer_const( "BUFSIZ",		BUFSIZ		);
	set_integer_const( "EOF",		EOF		);
	set_integer_const( "FOPEN_MAX",		FOPEN_MAX	);
	set_integer_const( "FILENAME_MAX",	FILENAME_MAX	);

	/* Darwin fails to define O_RSYNC. */
#ifndef O_RSYNC
#define O_RSYNC 0
#endif

	/* file creation & status flags */
#define MENTRY(_f) set_integer_const(LPOSIX_STR_1(LPOSIX_SPLICE(_O_, _f)), LPOSIX_SPLICE(O_, _f))
	MENTRY( RDONLY   );
	MENTRY( WRONLY   );
	MENTRY( RDWR     );
	MENTRY( APPEND   );
	MENTRY( CREAT    );
	MENTRY( DSYNC    );
	MENTRY( EXCL     );
	MENTRY( NOCTTY   );
	MENTRY( NONBLOCK );
	MENTRY( RSYNC    );
	MENTRY( SYNC     );
	MENTRY( TRUNC    );
#undef MENTRY

	/* Miscellaneous */
	set_integer_const( "WNOHANG",		WNOHANG		);

	/* errno values */
#define MENTRY(_e) set_integer_const(LPOSIX_STR_1(LPOSIX_SPLICE(_E, _e)), LPOSIX_SPLICE(E, _e))
	MENTRY( 2BIG		);
	MENTRY( ACCES		);
	MENTRY( ADDRINUSE	);
	MENTRY( ADDRNOTAVAIL	);
	MENTRY( AFNOSUPPORT	);
	MENTRY( AGAIN		);
	MENTRY( ALREADY		);
	MENTRY( BADF		);
	MENTRY( BADMSG		);
	MENTRY( BUSY		);
	MENTRY( CANCELED	);
	MENTRY( CHILD		);
	MENTRY( CONNABORTED	);
	MENTRY( CONNREFUSED	);
	MENTRY( CONNRESET	);
	MENTRY( DEADLK		);
	MENTRY( DESTADDRREQ	);
	MENTRY( DOM		);
	MENTRY( EXIST		);
	MENTRY( FAULT		);
	MENTRY( FBIG		);
	MENTRY( HOSTUNREACH	);
	MENTRY( IDRM		);
	MENTRY( ILSEQ		);
	MENTRY( INPROGRESS	);
	MENTRY( INTR		);
	MENTRY( INVAL		);
	MENTRY( IO		);
	MENTRY( ISCONN		);
	MENTRY( ISDIR		);
	MENTRY( LOOP		);
	MENTRY( MFILE		);
	MENTRY( MLINK		);
	MENTRY( MSGSIZE		);
	MENTRY( NAMETOOLONG	);
	MENTRY( NETDOWN		);
	MENTRY( NETRESET	);
	MENTRY( NETUNREACH	);
	MENTRY( NFILE		);
	MENTRY( NOBUFS		);
	MENTRY( NODEV		);
	MENTRY( NOENT		);
	MENTRY( NOEXEC		);
	MENTRY( NOLCK		);
	MENTRY( NOMEM		);
	MENTRY( NOMSG		);
	MENTRY( NOPROTOOPT	);
	MENTRY( NOSPC		);
	MENTRY( NOSYS		);
	MENTRY( NOTCONN		);
	MENTRY( NOTDIR		);
	MENTRY( NOTEMPTY	);
	MENTRY( NOTSOCK		);
	MENTRY( NOTSUP		);
	MENTRY( NOTTY		);
	MENTRY( NXIO		);
	MENTRY( OPNOTSUPP	);
	MENTRY( OVERFLOW	);
	MENTRY( PERM		);
	MENTRY( PIPE		);
	MENTRY( PROTO		);
	MENTRY( PROTONOSUPPORT	);
	MENTRY( PROTOTYPE	);
	MENTRY( RANGE		);
	MENTRY( ROFS		);
	MENTRY( SPIPE		);
	MENTRY( SRCH		);
	MENTRY( TIMEDOUT	);
	MENTRY( TXTBSY		);
	MENTRY( WOULDBLOCK	);
	MENTRY( XDEV		);
#undef MENTRY

	/* Signals */
#define MENTRY(_e) set_integer_const(LPOSIX_STR_1(LPOSIX_SPLICE(_SIG, _e)), LPOSIX_SPLICE(SIG, _e))
	MENTRY( ABRT	);
	MENTRY( ALRM	);
	MENTRY( BUS	);
	MENTRY( CHLD	);
	MENTRY( CONT	);
	MENTRY( FPE	);
	MENTRY( HUP	);
	MENTRY( ILL	);
	MENTRY( INT	);
	MENTRY( KILL	);
	MENTRY( PIPE	);
	MENTRY( QUIT	);
	MENTRY( SEGV	);
	MENTRY( STOP	);
	MENTRY( TERM	);
	MENTRY( TSTP	);
	MENTRY( TTIN	);
	MENTRY( TTOU	);
	MENTRY( USR1	);
	MENTRY( USR2	);
	MENTRY( SYS	);
	MENTRY( TRAP	);
	MENTRY( URG	);
	MENTRY( VTALRM	);
	MENTRY( XCPU	);
	MENTRY( XFSZ	);
#undef MENTRY

#if _POSIX_VERSION >= 200112L
#  define MENTRY(_e) set_integer_const(LPOSIX_STR_1(LPOSIX_SPLICE(_LOG, _e)), LPOSIX_SPLICE(LOG, _e))
	MENTRY( _AUTH		);
	MENTRY( _AUTHPRIV	);
	MENTRY( _CRON		);
	MENTRY( _DAEMON		);
	MENTRY( _FTP		);
	MENTRY( _KERN		);
	MENTRY( _LOCAL0		);
	MENTRY( _LOCAL1		);
	MENTRY( _LOCAL2		);
	MENTRY( _LOCAL3		);
	MENTRY( _LOCAL4		);
	MENTRY( _LOCAL5		);
	MENTRY( _LOCAL6		);
	MENTRY( _LOCAL7		);
	MENTRY( _LPR		);
	MENTRY( _MAIL		);
	MENTRY( _NEWS		);
	MENTRY( _SYSLOG		);
	MENTRY( _USER		);
	MENTRY( _UUCP		);

	MENTRY( _EMERG		);
	MENTRY( _ALERT		);
	MENTRY( _CRIT		);
	MENTRY( _ERR		);
	MENTRY( _WARNING	);
	MENTRY( _NOTICE		);
	MENTRY( _INFO		);
	MENTRY( _DEBUG		);
#  undef MENTRY
#endif

#define MENTRY(_e) set_integer_const(LPOSIX_STR_1(LPOSIX_SPLICE(_F, _e)), LPOSIX_SPLICE(F, _e))
	MENTRY( _DUPFD	);
	MENTRY( _GETFD	);
	MENTRY( _SETFD	);
	MENTRY( _GETFL	);
	MENTRY( _SETFL	);
	MENTRY( _GETLK	);
	MENTRY( _SETLK	);
	MENTRY( _SETLKW	);
	MENTRY( _GETOWN	);
	MENTRY( _SETOWN	);
#undef MENTRY

	/* from fnmatch.h */
#define MENTRY(_e) set_integer_const(LPOSIX_STR_1(LPOSIX_SPLICE(_FNM, _e)), LPOSIX_SPLICE(FNM, _e))
	MENTRY( _PATHNAME	);
	MENTRY( _NOESCAPE	);
	MENTRY( _PERIOD		);
#undef MENTRY

	/* Signals table stored in registry for Psignal and sig_handle */
	lua_pushlightuserdata(L, &signalL);
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);

	signalL = L; /* For sig_postpone */

	return 1;
}

/*EOF*/
