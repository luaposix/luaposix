/*
* lposix.c
* POSIX library for Lua 5.1.
* (c) Reuben Thomas (maintainer) <rrt@sc3d.org> 2010-2011
* (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
* Clean up and bug fixes by Leo Razoumov <slonik.az@gmail.com> 2006-10-11
* Luiz Henrique de Figueiredo <lhf@tecgraf.puc-rio.br> 07 Apr 2006 23:17:49
* Based on original by Claudio Terra for Lua 3.x.
* With contributions by Roberto Ierusalimschy.
*/

#include <config.h>

#include <sys/stat.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
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

#ifndef VERSION
#  define VERSION "unknown"
#endif

#define MYNAME		"posix"
#define MYVERSION	MYNAME " library for " LUA_VERSION " / " VERSION

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


/* Lua 5.2 compatibility */
#if LUA_VERSION_NUM > 501
#define lua_objlen lua_rawlen

#include <assert.h>
#define lua_setenv(L, n)  assert(lua_setuservalue(L, -2) == 1)
static int luaL_typerror(lua_State *L, int narg, const char *tname)
{
	const char *msg = lua_pushfstring(L, "%s expected, got %s",
					  tname, luaL_typename(L, narg));
	return luaL_argerror(L, narg, msg);
}
#endif


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
		if (*p== 'r' || *p == '-')
			return rwxrwxrwx(mode, p);

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
				*mode = ch_mode & affected_bits;
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
		if (strcmp(S[i], str) == 0)
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
	char b[PATH_MAX];
	size_t len;
	const char *path = luaL_checklstring(L, 1, &len);
	if (len>=sizeof(b))
		luaL_argerror(L, 1, "too long");
	lua_pushstring(L, basename(strcpy(b,path)));
	return 1;
}

static int Pdirname(lua_State *L)		/** dirname(path) */
{
	char b[PATH_MAX];
	size_t len;
	const char *path = luaL_checklstring(L, 1, &len);
	if (len>=sizeof(b))
		luaL_argerror(L, 1, "too long");
	lua_pushstring(L, dirname(strcpy(b,path)));
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
	char b[PATH_MAX];
	if (getcwd(b, sizeof(b)) == NULL)
		return pusherror(L, ".");
	lua_pushstring(L, b);
	return 1;
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
	char b[PATH_MAX];
	const char *path = luaL_checkstring(L, 1);
	int n = readlink(path, b, sizeof(b));
	if (n==-1)
		return pusherror(L, path);
	lua_pushlstring(L, b, n);
	return 1;
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
		return 0;
	strcpy(tmppath, path);
	res = mkstemp(tmppath);

	if (res == -1)
		return pusherror(L, path);

	lua_pushinteger(L, res);
	lua_pushstring(L, tmppath);
	lalloc(ud, tmppath, 0, 0);
	return 2;
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
static int Ppoll(lua_State *L)   /** poll(filehandle, timeout) */
{
	struct pollfd fds;
	FILE* file = *(FILE**)luaL_checkudata(L,1,LUA_FILEHANDLE);
	int timeout = luaL_checkint(L,2);
	fds.fd = fileno(file);
	fds.events = POLLIN;
	return pushresult(L, poll(&fds,1,timeout), NULL);
}

static int Pwait(lua_State *L)			/** wait([pid]) */
{
	int status;
	pid_t pid = luaL_optint(L, 1, -1);
	pid = waitpid(pid, &status, 0);
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

#ifndef O_RSYNC
#define O_RSYNC 0
#endif

#define oflags_map \
	MENTRY( "rdonly",   O_RDONLY   ) \
	MENTRY( "wronly",   O_WRONLY   ) \
	MENTRY( "rdwr",     O_RDWR     ) \
	MENTRY( "append",   O_APPEND   ) \
	MENTRY( "creat",    O_CREAT    ) \
	MENTRY( "dsync",    O_DSYNC    ) \
	MENTRY( "excl",     O_EXCL     ) \
	MENTRY( "noctty",   O_NOCTTY   ) \
	MENTRY( "nonblock", O_NONBLOCK ) \
	MENTRY( "rsync",    O_RSYNC    ) \
	MENTRY( "sync",     O_SYNC     ) \
	MENTRY( "trunc",    O_TRUNC    )

static const int Koflag[] =
{
#define MENTRY(_n, _f) (_f),
	oflags_map
#undef MENTRY
	-1
};

static const char *const Soflag[] =
{
#define MENTRY(_n, _f) (_n),
	oflags_map
#undef MENTRY
	NULL
};

static int make_oflags(lua_State *L, int i)
{
	int oflags = 0;
	lua_pushnil(L);
	while (lua_next(L, i) != 0) {
		int flag = lookup_symbol(Soflag, Koflag, luaL_checkstring(L, -1));
		if (flag == -1)
			return -1;
		oflags |= flag;
		lua_pop(L, 1);
	}
	return oflags;
}

static int Popen(lua_State *L)			/** open(path, flags[, mode]) */
{
	const char *path = luaL_checkstring(L, 1);
	int flags;
	mode_t mode;
	const char *modestr = luaL_optstring(L, 3, NULL);
	luaL_checktype(L, 2, LUA_TTABLE);
	flags = make_oflags(L, 2);
	if (flags == -1)
		luaL_argerror(L, 2, "bad flags");
	if (modestr && mode_munch(&mode, modestr))
		luaL_argerror(L, 3, "bad mode");
	return pushresult(L, open(path, flags, mode), path);
}

static int Pclose(lua_State *L)			/** close(n) */
{
	int fd = luaL_checkint(L, 1);
	return pushresult(L, close(fd), NULL);
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
	if ((buf = lalloc(ud, NULL, 0, count)) == NULL)
		return 0;
	ret = read(fd, buf, count);
	if (ret < 0)
		return pusherror(L, NULL);
	lua_pushlstring(L, buf, ret);
	lalloc(ud, buf, 0, 0);
	return 2;
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
	char b[32];
	sprintf(b,"%ld",gethostid());
	lua_pushstring(L, b);
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
	struct group *g=NULL;
	if (lua_isnumber(L, 1))
		g = getgrgid((gid_t)lua_tonumber(L, 1));
	else if (lua_isstring(L, 1))
		g = getgrnam(lua_tostring(L, 1));
	else
		luaL_typerror(L, 1, "string or number");
	if (g==NULL)
		lua_pushnil(L);
	else
	{
		int i;
		lua_newtable(L);
		lua_pushstring(L, g->gr_name);
		lua_setfield(L, -2, "name");
		lua_pushinteger(L, g->gr_gid);
		lua_setfield(L, -2, "gid");
		for (i=0; g->gr_mem[i]!=NULL; i++)
		{
			lua_pushstring(L, g->gr_mem[i]);
			lua_rawseti(L, -2, i);
		}
	}
	return 1;
}

#if _POSIX_VERSION >= 200112L
static int Pgetgroups(lua_State *L)		/** getgroups() */
{
	int n_group_slots = getgroups(0, NULL);

	if (n_group_slots >= 0) {
		int n_groups;
		void *ud;
		gid_t *group;
		lua_Alloc lalloc = lua_getallocf(L, &ud);

		if ((group = lalloc(ud, NULL, 0, n_group_slots * sizeof *group)) == NULL)
			return 0;

		if ((n_groups = getgroups(n_group_slots, group)) >= 0) {
			int i;
			lua_createtable(L, n_groups, 0);
			for (i = 0; i < n_groups; i++) {
				lua_pushinteger(L, group[i]);
				lua_rawseti(L, -2, i + 1);
			}
			free (group);
			return 1;
		}

		free(group);
	}

	return 0;
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
	MENTRY( "link_max",         _PC_LINK_MAX         ) \
	MENTRY( "max_canon",        _PC_MAX_CANON        ) \
	MENTRY( "max_input",        _PC_MAX_INPUT        ) \
	MENTRY( "name_max",         _PC_NAME_MAX         ) \
	MENTRY( "path_max",         _PC_PATH_MAX         ) \
	MENTRY( "pipe_buf",         _PC_PIPE_BUF         ) \
	MENTRY( "chown_restricted", _PC_CHOWN_RESTRICTED ) \
	MENTRY( "no_trunc",         _PC_NO_TRUNC         ) \
	MENTRY( "vdisable",         _PC_VDISABLE         )

static const int Kpathconf[] =
{
#define MENTRY(_n, _f) (_f),
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
#define MENTRY(_n, _f) (_n),
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
	MENTRY( "arg_max",     _SC_ARG_MAX     ) \
	MENTRY( "child_max",   _SC_CHILD_MAX   ) \
	MENTRY( "clk_tck",     _SC_CLK_TCK     ) \
	MENTRY( "ngroups_max", _SC_NGROUPS_MAX ) \
	MENTRY( "stream_max",  _SC_STREAM_MAX  ) \
	MENTRY( "tzname_max",  _SC_TZNAME_MAX  ) \
	MENTRY( "open_max",    _SC_OPEN_MAX    ) \
	MENTRY( "job_control", _SC_JOB_CONTROL ) \
	MENTRY( "saved_ids",   _SC_SAVED_IDS   ) \
	MENTRY( "version",     _SC_VERSION     )

static const int Ksysconf[] =
{
#define MENTRY(_n, _f) (_f),
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
#define MENTRY(_n, _f) (_n),
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
#ifdef LOG_PERROR
			case 'e': option |= LOG_PERROR; break;
#endif
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

static int Psetlogmask(lua_State* L) 		/** setlogmask(priority...) */
{
	int argno = lua_gettop(L);
	int mask = 0;
	int i;

	for (i=1; i <= argno; i++)
		mask |= LOG_MASK(luaL_checkint(L,i));

	return pushresult(L, setlogmask(mask),"setlogmask");
}

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
 *	Valid resouces are:
 *		"core", "cpu", "data", "fsize",
 *		"nofile", "stack", "as"
 *
 *	Example usage:
 *	posix.setrlimit("nofile", 1000, 2000)
 */

#define rlimit_map \
	MENTRY( "core",   RLIMIT_CORE   ) \
	MENTRY( "cpu",    RLIMIT_CPU    ) \
	MENTRY( "data",   RLIMIT_DATA   ) \
	MENTRY( "fsize",  RLIMIT_FSIZE  ) \
	MENTRY( "nofile", RLIMIT_NOFILE ) \
	MENTRY( "stack",  RLIMIT_STACK  ) \
	MENTRY( "as",     RLIMIT_AS     )

static const int Krlimit[] =
{
#define MENTRY(_n, _f) (_f),
	rlimit_map
#undef MENTRY
	-1
};

static const char *const Srlimit[] =
{
#define MENTRY(_n, _f) (_n),
	rlimit_map
#undef MENTRY
	NULL
};

static int Psetrlimit(lua_State *L) 	/** setrlimit(resource,soft[,hard]) */
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
static int Pgetrlimit(lua_State *L) 	/** getrlimit(resource) */
{
	struct rlimit lim;
	int rid, rc;
	const char *rid_str = luaL_checkstring(L, 1);
	rid = lookup_symbol(Srlimit, Krlimit, rid_str);
	rc = getrlimit(rid, &lim);
	if (rc < 0)
		return pusherror(L, "getrlimit");
	lua_pushnumber(L, lim.rlim_cur);
	lua_pushnumber(L, lim.rlim_max);
	return 2;
}

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
	lua_pushnumber(L, t);
	return 1;
}

static int Plocaltime(lua_State *L)		/** localtime([time]) */
{
	struct tm res;
	time_t t = luaL_optint(L, 1, time(NULL));
	if (localtime_r(&t, &res) == NULL)
		return pusherror(L, "localtime");
	lua_createtable(L, 0, 9);
	lua_pushnumber(L, res.tm_sec);
	lua_setfield(L, -2, "sec");
	lua_pushnumber(L, res.tm_min);
	lua_setfield(L, -2, "min");
	lua_pushnumber(L, res.tm_hour);
	lua_setfield(L, -2, "hour");
	lua_pushnumber(L, res.tm_mday);
	lua_setfield(L, -2, "monthday");
	lua_pushnumber(L, res.tm_mon + 1);
	lua_setfield(L, -2, "month");
	lua_pushnumber(L, res.tm_year + 1900);
	lua_setfield(L, -2, "year");
	lua_pushnumber(L, res.tm_wday);
	lua_setfield(L, -2, "weekday");
	lua_pushnumber(L, res.tm_yday);
	lua_setfield(L, -2, "yearday");
	lua_pushboolean(L, res.tm_isdst);
	lua_setfield(L, -2, "is_dst");
	return 1;
}

static int Pgmtime(lua_State *L)		/** gmtime([time]) */
{
	struct tm res;
	time_t t = luaL_optint(L, 1, time(NULL));
	if (gmtime_r(&t, &res) == NULL)
		return pusherror(L, "localtime");
	lua_createtable(L, 0, 9);
	lua_pushnumber(L, res.tm_sec);
	lua_setfield(L, -2, "sec");
	lua_pushnumber(L, res.tm_min);
	lua_setfield(L, -2, "min");
	lua_pushnumber(L, res.tm_hour);
	lua_setfield(L, -2, "hour");
	lua_pushnumber(L, res.tm_mday);
	lua_setfield(L, -2, "monthday");
	lua_pushnumber(L, res.tm_mon + 1);
	lua_setfield(L, -2, "month");
	lua_pushnumber(L, res.tm_year + 1900);
	lua_setfield(L, -2, "year");
	lua_pushnumber(L, res.tm_wday);
	lua_setfield(L, -2, "weekday");
	lua_pushnumber(L, res.tm_yday);
	lua_setfield(L, -2, "yearday");
	lua_pushboolean(L, res.tm_isdst);
	lua_setfield(L, -2, "is_dst");
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
	lua_pushnumber(L, res.tv_sec);
	lua_pushnumber(L, res.tv_nsec);
	return 2;
}

static int Pclock_gettime(lua_State *L)		/** clock_gettime([clockid]) */
{
	struct timespec res;
	const char *str = lua_tostring(L, 1);
	if (clock_gettime(get_clk_id_const(str), &res) == -1)
		return pusherror(L, "clock_gettime");
	lua_pushnumber(L, res.tv_sec);
	lua_pushnumber(L, res.tv_nsec);
	return 2;
}
#endif

static int Pstrftime(lua_State *L)		/** strftime(format, [time]) */
{
	char tmp[256];
	const char *format = luaL_checkstring(L, 1);

	struct tm t;
	if (lua_istable(L, 2)) {
		lua_getfield(L, 2, "sec");
		t.tm_sec = luaL_optint(L, -1, 0);
		lua_pop(L, 1);
		lua_getfield(L, 2, "min");
		t.tm_min = luaL_optint(L, -1, 0);
		lua_pop(L, 1);
		lua_getfield(L, 2, "hour");
		t.tm_hour = luaL_optint(L, -1, 0);
		lua_pop(L, 1);
		lua_getfield(L, 2, "monthday");
		t.tm_mday = luaL_optint(L, -1, 0);
		lua_pop(L, 1);
		lua_getfield(L, 2, "month");
		t.tm_mon = luaL_optint(L, -1, 0);
		lua_pop(L, 1);
		lua_getfield(L, 2, "year");
		t.tm_year = luaL_optint(L, -1, 0);
		lua_pop(L, 1);
		lua_getfield(L, 2, "weekday");
		t.tm_wday = luaL_optint(L, -1, 0);
		lua_pop(L, 1);
		lua_getfield(L, 2, "yearday");
		t.tm_yday = luaL_optint(L, -1, 0);
		lua_pop(L, 1);
		lua_getfield(L, 2, "is_dst");
		t.tm_isdst = lua_tointeger(L, -1);
		lua_pop(L, 1);
	} else {
		time_t now = time(NULL);
		localtime_r(&now, &t);
	}

	strftime(tmp, sizeof(tmp), format, &t);
	lua_pushlstring(L, tmp, strlen(tmp));
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
static int signalno;

static const char *const Ssigmacros[] =
{
	"SIG_DFL", "SIG_ERR", "SIG_HOLD", "SIG_IGN", NULL
};

static void (*Fsigmacros[])(int) =
{
	SIG_DFL, SIG_IGN, NULL
};

static void sig_handle (lua_State *L, lua_Debug *ar) {
	(void)ar;  /* unused arg. */
	/* Get signal handlers table */
	lua_sethook(L, NULL, 0, 0);
	lua_pushlightuserdata(L, &signalL);
	lua_rawget(L, LUA_REGISTRYINDEX);

	/* Get handler */
	lua_pushinteger(L, signalno);
	lua_gettable(L, -2);

	/* Call handler with signal number */
	lua_pushinteger(L, signalno);
	lua_pcall(L, 1, 0, 0);
	/* FIXME: Deal with error */
}

static void sig_postpone (int i) {
	signalno = i;
	lua_sethook(signalL, sig_handle, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}

static int sig_action (lua_State *L)
{
	/* As newindex metamethod, we are passed (table, key, value) on Lua stack */
	struct sigaction sa;
	int sig = luaL_checkinteger(L, 2);
	void (*handler)(int) = sig_postpone;

	luaL_checktype(L, 1, LUA_TTABLE);

	/* Set Lua handler */
	if (lua_type(L, 3) == LUA_TSTRING)
		handler = Fsigmacros[luaL_checkoption(L, 3, "SIG_DFL", Ssigmacros)];
	lua_rawset(L, 1);

	/* Set up C signal handler */
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(sig, &sa, 0);         /* XXX ignores errors */

	return 0;
}


static const luaL_Reg R[] =
{
	{"_exit",		P_exit},
	{"abort",		Pabort},
	{"access",		Paccess},
	{"basename",		Pbasename},
	{"chdir",		Pchdir},
	{"chmod",		Pchmod},
	{"chown",		Pchown},
#if defined (_XOPEN_REALTIME) && _XOPEN_REALTIME != -1
	{"clock_getres",	Pclock_getres},
	{"clock_gettime",	Pclock_gettime},
#endif
	{"close",		Pclose},
#if _POSIX_VERSION >= 200112L
	{"crypt",		Pcrypt},
#endif
	{"ctermid",		Pctermid},
	{"dirname",		Pdirname},
	{"dir",			Pdir},
	{"errno",		Perrno},
	{"exec",		Pexec},
	{"execp",		Pexecp},
	{"fileno",		Pfileno},
	{"files",		Pfiles},
	{"fork",		Pfork},
	{"getcwd",		Pgetcwd},
	{"getenv",		Pgetenv},
	{"getgroup",		Pgetgroup},
#if _POSIX_VERSION >= 200112L
	{"getgroups",		Pgetgroups},
#endif
	{"getlogin",		Pgetlogin},
	{"getopt_long",		Pgetopt_long},
	{"getpasswd",		Pgetpasswd},
	{"getpid",		Pgetpid},
	{"getrlimit",		Pgetrlimit},
	{"gettimeofday",	Pgettimeofday},
	{"glob",		Pglob},
	{"gmtime",		Pgmtime},
	{"hostid",		Phostid},
	{"isgraph",		Pisgraph},
	{"isprint",		Pisprint},
	{"kill",		Pkill},
	{"link",		Plink},
	{"localtime",		Plocaltime},
	{"mkdir",		Pmkdir},
	{"mkfifo",		Pmkfifo},
	{"mkstemp",             Pmkstemp},
	{"open",		Popen},
	{"pathconf",		Ppathconf},
	{"raise",		Praise},
	{"read",		Pread},
	{"readlink",		Preadlink},
	{"rmdir",		Prmdir},
	{"rpoll",		Ppoll},
	{"set_errno",		Pset_errno},
	{"setenv",		Psetenv},
	{"setpid",		Psetpid},
	{"setrlimit",		Psetrlimit},
	{"sleep",		Psleep},
	{"nanosleep",		Pnanosleep},
	{"stat",		Pstat},
	{"strftime",		Pstrftime},
	{"sysconf",		Psysconf},
	{"time",		Ptime},
	{"times",		Ptimes},
	{"ttyname",		Pttyname},
	{"unlink",		Punlink},
	{"umask",		Pumask},
	{"uname",		Puname},
	{"utime",		Putime},
	{"wait",		Pwait},
	{"write",		Pwrite},

#if _POSIX_VERSION >= 200112L
	{"openlog",		Popenlog},
	{"syslog",		Psyslog},
	{"closelog",		Pcloselog},
	{"setlogmask",		Psetlogmask},
#endif

	{NULL,			NULL}
};

#define set_const(key, value)		\
	lua_pushnumber(L, value);	\
	lua_setfield(L, -2, key)

LUALIB_API int luaopen_posix_c (lua_State *L)
{
	luaL_register(L, MYNAME, R);

	lua_pushliteral(L, MYVERSION);
	lua_setfield(L, -2, "version");

	/* stdio.h constants */
	/* Those that are omitted already have a Lua interface, or alternative. */
	set_const("_IOFBF", _IOFBF);
	set_const("_IOLBF", _IOLBF);
	set_const("_IONBF", _IONBF);
	set_const("BUFSIZ", BUFSIZ);
	set_const("EOF", EOF);
	set_const("FOPEN_MAX", FOPEN_MAX);
	set_const("FILENAME_MAX", FILENAME_MAX);

	/* errno values */
	set_const("E2BIG", E2BIG);
	set_const("EACCES", EACCES);
	set_const("EADDRINUSE", EADDRINUSE);
	set_const("EADDRNOTAVAIL", EADDRNOTAVAIL);
	set_const("EAFNOSUPPORT", EAFNOSUPPORT);
	set_const("EAGAIN", EAGAIN);
	set_const("EALREADY", EALREADY);
	set_const("EBADF", EBADF);
	set_const("EBADMSG", EBADMSG);
	set_const("EBUSY", EBUSY);
	set_const("ECANCELED", ECANCELED);
	set_const("ECHILD", ECHILD);
	set_const("ECONNABORTED", ECONNABORTED);
	set_const("ECONNREFUSED", ECONNREFUSED);
	set_const("ECONNRESET", ECONNRESET);
	set_const("EDEADLK", EDEADLK);
	set_const("EDESTADDRREQ", EDESTADDRREQ);
	set_const("EDOM", EDOM);
	set_const("EEXIST", EEXIST);
	set_const("EFAULT", EFAULT);
	set_const("EFBIG", EFBIG);
	set_const("EHOSTUNREACH", EHOSTUNREACH);
	set_const("EIDRM", EIDRM);
	set_const("EILSEQ", EILSEQ);
	set_const("EINPROGRESS", EINPROGRESS);
	set_const("EINTR", EINTR);
	set_const("EINVAL", EINVAL);
	set_const("EIO", EIO);
	set_const("EISCONN", EISCONN);
	set_const("EISDIR", EISDIR);
	set_const("ELOOP", ELOOP);
	set_const("EMFILE", EMFILE);
	set_const("EMLINK", EMLINK);
	set_const("EMSGSIZE", EMSGSIZE);
	set_const("ENAMETOOLONG", ENAMETOOLONG);
	set_const("ENETDOWN", ENETDOWN);
	set_const("ENETRESET", ENETRESET);
	set_const("ENETUNREACH", ENETUNREACH);
	set_const("ENFILE", ENFILE);
	set_const("ENOBUFS", ENOBUFS);
	set_const("ENODATA", ENODATA);
	set_const("ENODEV", ENODEV);
	set_const("ENOENT", ENOENT);
	set_const("ENOEXEC", ENOEXEC);
	set_const("ENOLCK", ENOLCK);
	set_const("ENOMEM", ENOMEM);
	set_const("ENOMSG", ENOMSG);
	set_const("ENOPROTOOPT", ENOPROTOOPT);
	set_const("ENOSPC", ENOSPC);
	set_const("ENOSR", ENOSR);
	set_const("ENOSTR", ENOSTR);
	set_const("ENOSYS", ENOSYS);
	set_const("ENOTCONN", ENOTCONN);
	set_const("ENOTDIR", ENOTDIR);
	set_const("ENOTEMPTY", ENOTEMPTY);
	set_const("ENOTSOCK", ENOTSOCK);
	set_const("ENOTSUP", ENOTSUP);
	set_const("ENOTTY", ENOTTY);
	set_const("ENXIO", ENXIO);
	set_const("EOPNOTSUPP", EOPNOTSUPP);
	set_const("EOVERFLOW", EOVERFLOW);
	set_const("EPERM", EPERM);
	set_const("EPIPE", EPIPE);
	set_const("EPROTO", EPROTO);
	set_const("EPROTONOSUPPORT", EPROTONOSUPPORT);
	set_const("EPROTOTYPE", EPROTOTYPE);
	set_const("ERANGE", ERANGE);
	set_const("EROFS", EROFS);
	set_const("ESPIPE", ESPIPE);
	set_const("ESRCH", ESRCH);
	set_const("ETIME", ETIME);
	set_const("ETIMEDOUT", ETIMEDOUT);
	set_const("ETXTBSY", ETXTBSY);
	set_const("EWOULDBLOCK", EWOULDBLOCK);
	set_const("EXDEV", EXDEV);

	/* Signals */
	set_const("SIGABRT", SIGABRT);
	set_const("SIGALRM", SIGALRM);
	set_const("SIGBUS", SIGBUS);
	set_const("SIGCHLD", SIGCHLD);
	set_const("SIGCONT", SIGCONT);
	set_const("SIGFPE", SIGFPE);
	set_const("SIGHUP", SIGHUP);
	set_const("SIGILL", SIGILL);
	set_const("SIGINT", SIGINT);
	set_const("SIGKILL", SIGKILL);
	set_const("SIGPIPE", SIGPIPE);
	set_const("SIGQUIT", SIGQUIT);
	set_const("SIGSEGV", SIGSEGV);
	set_const("SIGSTOP", SIGSTOP);
	set_const("SIGTERM", SIGTERM);
	set_const("SIGTSTP", SIGTSTP);
	set_const("SIGTTIN", SIGTTIN);
	set_const("SIGTTOU", SIGTTOU);
	set_const("SIGUSR1", SIGUSR1);
	set_const("SIGUSR2", SIGUSR2);
	set_const("SIGPOLL", SIGPOLL);
	set_const("SIGPROF", SIGPROF);
	set_const("SIGSYS", SIGSYS);
	set_const("SIGTRAP", SIGTRAP);
	set_const("SIGURG", SIGURG );
	set_const("SIGVTALRM", SIGVTALRM);
	set_const("SIGXCPU", SIGXCPU);
	set_const("SIGXFSZ", SIGXFSZ);


#if _POSIX_VERSION >= 200112L
	set_const("LOG_AUTH", LOG_AUTH);
	set_const("LOG_AUTHPRIV", LOG_AUTHPRIV);
	set_const("LOG_CRON", LOG_CRON);
	set_const("LOG_DAEMON", LOG_DAEMON);
	set_const("LOG_FTP", LOG_FTP);
	set_const("LOG_KERN", LOG_KERN);
	set_const("LOG_LOCAL0", LOG_LOCAL0);
	set_const("LOG_LOCAL1", LOG_LOCAL1);
	set_const("LOG_LOCAL2", LOG_LOCAL2);
	set_const("LOG_LOCAL3", LOG_LOCAL3);
	set_const("LOG_LOCAL4", LOG_LOCAL4);
	set_const("LOG_LOCAL5", LOG_LOCAL5);
	set_const("LOG_LOCAL6", LOG_LOCAL6);
	set_const("LOG_LOCAL7", LOG_LOCAL7);
	set_const("LOG_LPR", LOG_LPR);
	set_const("LOG_MAIL", LOG_MAIL);
	set_const("LOG_NEWS", LOG_NEWS);
	set_const("LOG_SYSLOG", LOG_SYSLOG);
	set_const("LOG_USER", LOG_USER);
	set_const("LOG_UUCP", LOG_UUCP);

	set_const("LOG_EMERG", LOG_EMERG);
	set_const("LOG_ALERT", LOG_ALERT);
	set_const("LOG_CRIT", LOG_CRIT);
	set_const("LOG_ERR", LOG_ERR);
	set_const("LOG_WARNING", LOG_WARNING);
	set_const("LOG_NOTICE", LOG_NOTICE);
	set_const("LOG_INFO", LOG_INFO);
	set_const("LOG_DEBUG", LOG_DEBUG);
#endif

	/* Signals table */
	lua_newtable(L); /* Signals table */

	/* Signals table's metatable */
	lua_newtable(L);
	lua_pushcfunction(L, sig_action);
	lua_setfield(L, -2, "__newindex");
	lua_setmetatable(L, -2);

	/* Take a copy of the signals table to use in sig_action */
	lua_pushlightuserdata(L, &signalL);
	lua_pushvalue(L, -2);
	lua_rawset(L, LUA_REGISTRYINDEX);

	signalL = L; /* For sig_postpone */

	/* Register signals table */
	lua_setfield(L, -2, "signal");

	return 1;
}

/*EOF*/
