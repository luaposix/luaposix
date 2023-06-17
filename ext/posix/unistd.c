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
 Unix Standard APIs.

 Where the underlying system does not support one of these functions, it
 will have a `nil` value in the module table.

@module posix.unistd
*/

#if HAVE_CRYPT_H
#  include <crypt.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>

#include "_helpers.c"

static uid_t
mygetuid(lua_State *L, int i)
{
	if (lua_isnoneornil(L, i))
		return (uid_t)-1;
	else if (lua_isinteger(L, i))
		return (uid_t) lua_tointeger(L, i);
	else if (lua_isstring(L, i))
	{
		struct passwd *p = getpwnam(lua_tostring(L, i));
		return (p == NULL) ? (uid_t) -1 : p->pw_uid;
	}
	else
		return argtypeerror(L, i, "integer, nil or string");
}

static gid_t
mygetgid(lua_State *L, int i)
{
	if (lua_isnoneornil(L, i))
		return (gid_t)-1;
	else if (lua_isinteger(L, i))
		return (gid_t) lua_tointeger(L, i);
	else if (lua_isstring(L, i))
	{
		struct group *g = getgrnam(lua_tostring(L, i));
		return (g == NULL) ? (uid_t) -1 : g->gr_gid;
	}
	else
		return argtypeerror(L, i, "integer, nil or string");
}

/***
Terminate the calling process.
@function _exit
@int status process exit status
@see _exit(2)
*/
static int
P_exit(lua_State *L)
{
	pid_t ret = (pid_t)checkinteger(L, 1);
	checknargs(L, 1);
	_exit(ret);
	return 0; /* Avoid a compiler warning (or possibly cause one
		     if the compiler's too clever, sigh). */
}


/***
Check real user's permissions for a file.
@function access
@string path file to act on
@string[opt="f"] mode can contain 'r','w','x' and 'f'
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see access(2)
@usage
  local unistd = require "posix.unistd"
  status, errstr, errno = unistd.access("/etc/passwd", "rw")
*/
static int
Paccess(lua_State *L)
{
	int mode=F_OK;
	const char *path=luaL_checkstring(L, 1);
	const char *s;
	checknargs(L, 2);
	for (s=optstring(L, 2, "f"); *s!=0 ; s++)
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


/***
Schedule an alarm signal.
@function alarm
@int seconds number of seconds to send SIGALRM in
@return int number of seconds remaining in previous alarm or `0`
@see alarm(2)
@usage
  local unistd = require "posix.unistd"
  seconds = unistd.alarm(10)
*/
static int
Palarm(lua_State *L)
{
	int seconds = checkint(L, 1);
	checknargs(L, 1);
	return pushintegerresult(alarm(seconds));
}
/***
Set the working directory.
@function chdir
@string path file to act on
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see chdir(2)
@usage
  local unistd = require "posix.unistd"
  status, errstr, errno = unistd.chdir ("/var/tmp")
*/
static int
Pchdir(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 1);
	return pushresult(L, chdir(path), path);
}


/***
Change ownership of a file.
@function chown
@string path existing file path
@tparam string|int uid new owner user id
@tparam string|int gid new owner group id
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error messoge
@treturn[2] int errnum
@see chown(2)
@usage
  local unistd = require "posix.unistd"
  -- will fail for a normal user, and print an error
  print(unistd.chown ("/etc/passwd", 100, 200))
*/
static int
Pchown(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	uid_t uid = mygetuid(L, 2);
	gid_t gid = mygetgid(L, 3);
	checknargs(L, 3);
	return pushresult(L, chown(path, uid, gid), path);
}


/***
Close an open file descriptor.
@function close
@int fd file descriptor to act on
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see close(2)
@usage
  local unistd = require "posix.unistd"
  local ok, errmsg = unistd.close (log)
  if not ok then error (errmsg) end
*/
static int
Pclose(lua_State *L)
{
	int fd = checkint(L, 1);
	checknargs(L, 1);
	return pushresult(L, close(fd), NULL);
}


#if defined HAVE_CRYPT
/***
Encrypt a password.
Not recommended for general encryption purposes.
@function crypt
@string trypass string to hash
@string salt two-character string from [a-zA-Z0-9./]
@return encrypted string
@see crypt(3)
@usage
  local pwd = require "posix.pwd"
  local unistd = require "posix.unistd"

  local salt, hash = pwd.pwent:match ":$6$(.-)$([^:]+)"
  if unistd.crypt (trypass, salt) ~= hash then
    error "wrong password"
  end
*/
static int
Pcrypt(lua_State *L)
{
	const char *str, *salt;
	char *r;

	str = luaL_checkstring(L, 1);
	salt = luaL_checkstring(L, 2);
	if (strlen(salt) < 2)
		luaL_error(L, "not enough salt");
	checknargs(L, 2);

	r = crypt(str, salt);
	return pushstringresult(r);
}
#endif


/***
Duplicate an open file descriptor.
@function dup
@int fd file descriptor to act on
@treturn[1] int new file descriptor duplicating *fd*, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see dup(2)
@usage
  local stdio = require "posix.stdio"
  local unistd = require "posix.unistd"

  outfd = unistd.dup (stdio.fileno (io.stdout))
*/
static int
Pdup(lua_State *L)
{
	int fd = checkint(L, 1);
	checknargs(L, 1);
	return pushresult(L, dup(fd), NULL);
}


/***
Duplicate one open file descriptor to another.
If *newfd* references an open file already, it is closed before being
reallocated to *fd*.
@function dup2
@int fd an open file descriptor to act on
@int newfd new descriptor to duplicate *fd*
@treturn[1] int new file descriptor, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see dup2(2)
*/
static int
Pdup2(lua_State *L)
{
	int fd = checkint(L, 1);
	int newfd = checkint(L, 2);
	checknargs(L, 2);
	return pushresult(L, dup2(fd, newfd), NULL);
}


static int
runexec(lua_State *L, int use_shell)
{
	char **argv;
	const char *path = luaL_checkstring(L, 1);
	int i, n;
	checknargs(L, 2);

	if (lua_type(L, 2) != LUA_TTABLE)
		argtypeerror(L, 2, "table");

	n = lua_objlen(L, 2);
	argv = lua_newuserdata(L, (n + 2) * sizeof(char*));

	/* Set argv[0], defaulting to command */
	argv[0] = (char*) path;
	lua_pushinteger(L, 0);
	lua_gettable(L, 2);
	if (lua_type(L, -1) == LUA_TSTRING)
		argv[0] = (char*)lua_tostring(L, -1);
	else
		lua_pop(L, 1);

	/* Read argv[1..n] from table. */
	for (i=1; i<=n; i++)
	{
		lua_pushinteger(L, i);
		lua_gettable(L, 2);
		argv[i] = (char*)lua_tostring(L, -1);
	}
	argv[n+1] = NULL;

	(use_shell ? execvp : execv) (path, argv);
	return pusherror(L, path);
}


/***
Execute a program at exactly *path*.
@function exec
@string path
@tparam table argt arguments (table can include index 0)
@return nil
@treturn string error message
@treturn int errnum
@see execve(2)
@usage exec ("/bin/bash", {[0] = "-sh", "--norc"})
*/
static int
Pexec(lua_State *L)
{
	return runexec(L, 0);
}


/***
Execute a program found using command PATH search, like the shell.
@function execp
@string path
@tparam table argt arguments (table can include index 0)
@return nil
@treturn string error message
@treturn int errnum
@see execve(2)
*/
static int
Pexecp(lua_State *L)
{
	return runexec(L, 1);
}


#if HAVE_FDATASYNC

#if !HAVE_DECL_FDATASYNC
extern int fdatasync ();
#endif

/***
Synchronize a file's in-core state with storage device without metadata.
@function fdatasync
@int fd
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see fdatasync(2)
*/
static int
Pfdatasync(lua_State *L)
{
  int fd = checkint(L, 1);
  checknargs(L, 1);
  return pushresult(L, fdatasync(fd), NULL);
}
#endif


/***
Fork this program.
@function fork
@treturn[1] int `0` in the resulting child process
@treturn[2] int process id of child, in the calling process
@return[3] nil
@treturn[3] string error message
@treturn[3] int errnum
@see fork(2)
@see fork.lua
@see fork2.lua
@usage
  local unistd = require "posix.unistd"

  local pid, errmsg = unistd.fork ()
  if pid == nil then
    error (errmsg)
  elseif pid == 0 then
    print ("in child:", unistd.getpid "pid")
  else
    print (require "posix.sys.wait".wait (pid))
  end
  os.exit ()
*/
static int
Pfork(lua_State *L)
{
	checknargs(L, 0);
	return pushresult(L, fork(), NULL);
}


/***
Synchronize a file's in-core state with storage device.
@function fsync
@int fd
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see fsync(2)
@see sync
*/
static int
Pfsync(lua_State *L)
{
  int fd = checkint(L, 1);
  checknargs(L, 1);
  return pushresult(L, fsync(fd), NULL);
}


/***
Current working directory for this process.
@function getcwd
@treturn[1] string path of current working directory, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see getcwd(3)
*/
static int
Pgetcwd(lua_State *L)
{
#ifdef __GNU__
	char *r = get_current_dir_name();
	checknargs(L, 0);
	if (r != NULL)
        lua_pushstring(L, r);
    free(r);
#else
	long size = pathconf(".", _PC_PATH_MAX);
	void *ud;
	lua_Alloc lalloc;
	char *b, *r;
	checknargs(L, 0);
	lalloc = lua_getallocf(L, &ud);
	if (size == -1)
		size = _POSIX_PATH_MAX; /* FIXME: Retry if this is not long enough */
	if ((b = lalloc(ud, NULL, 0, (size_t)size + 1)) == NULL)
		return pusherror(L, "lalloc");
	r = getcwd(b, (size_t)size);
	if (r != NULL)
		lua_pushstring(L, b);
	lalloc(ud, b, (size_t)size + 1, 0);
#endif
	return (r == NULL) ? pusherror(L, ".") : 1;
}


/***
Return effective group id of calling process.
@function getegid
@treturn int effective group id of calling process
@see getgid
*/
static int
Pgetegid(lua_State *L)
{
	checknargs(L, 0);
	return pushintegerresult(getegid());
}


/***
Return effective user id of calling process.
@function geteuid
@treturn int effective user id of calling process
@see getuid
*/
static int
Pgeteuid(lua_State *L)
{
	checknargs(L, 0);
	return pushintegerresult(geteuid());
}


/***
Return group id of calling process.
@function getgid
@treturn int group id of calling process
@see getegid
*/
static int
Pgetgid(lua_State *L)
{
	checknargs(L, 0);
	return pushintegerresult(getgid());
}


#if LPOSIX_2001_COMPLIANT
/***
Get list of supplementary group ids.
@function getgroups
@treturn[1] table group id, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see getgroups(2)
*/
static int
Pgetgroups(lua_State *L)
{
	int n_group_slots = getgroups(0, NULL);
	checknargs(L, 0);

	if (n_group_slots < 0)
		return pusherror(L, NULL);
	else if (n_group_slots == 0)
		lua_newtable(L);
	else
	{
		gid_t  *group;
		int     n_groups;
		int     i;

		group = lua_newuserdata(L, sizeof(*group) * n_group_slots);

		n_groups = getgroups(n_group_slots, group);
		if (n_groups < 0)
			return pusherror(L, NULL);

		lua_createtable(L, n_groups, 0);
		for (i = 0; i < n_groups; i++)
		{
			lua_pushinteger(L, group[i]);
			lua_rawseti(L, -2, i + 1);
		}
	}

	return 1;
}
#endif


/***
Current logged-in user.
@treturn[1] string username, if successful
@return[2] nil
@see getlogin(3)
*/
static int
Pgetlogin(lua_State *L)
{
	checknargs(L, 0);
	return pushstringresult(getlogin());
}


static int
iter_getopt(lua_State *L)
{
	int r, argc = lua_tointeger(L, lua_upvalueindex(1));
	char **argv = (char **)lua_touserdata(L, lua_upvalueindex(3));
	char c;

	if (argv == NULL) /* If we have already completed, return now. */
		return 0;

	/* Fetch upvalues to pass to getopt. */
	r = getopt(argc, argv, lua_tostring(L, lua_upvalueindex(2)));
	if (r == -1)
		return 0;

	c = (char) r;
	lua_pushlstring(L, &c, 1);
	lua_pushstring(L, optarg);
	lua_pushinteger(L, optind);
	return 3;
}


/***
Parse command-line options.
@function getopt
@param arg command line arguments
@string opts short option specifier
@int[opt=0] opterr index of the option with an error
@int[opt=1] optind index of the next unprocessed option
@treturn option iterator, returning 3 values
@see getopt(3)
@see getopt.lua
@usage
local getopt = require "posix.getopt".getopt
for opt, opterr, i in getopt (arg, "ho:v", opterr, i) do
  process (arg, opterr, i)
end
*/
static int
Pgetopt(lua_State *L)
{
	int argc, i;
	const char *optstring;
	char **argv;

	checknargs(L, 4);
	checktype(L, 1, LUA_TTABLE, "list");
	optstring = luaL_checkstring(L, 2);
	opterr = optinteger(L, 3, 0);
	optind = optinteger(L, 4, 1);

	argc = (int)lua_objlen(L, 1) + 1;

	lua_pushinteger(L, argc);
	lua_pushstring(L, optstring);

	argv = lua_newuserdata(L, (argc + 1) * sizeof(char *));
	argv[argc] = NULL;
	for (i = 0; i < argc; i++)
	{
		lua_pushinteger(L, i);
		lua_gettable(L, 1);
		argv[i] = (char *)luaL_checkstring(L, -1);
	}

	/* Push remaining upvalues, and make and push closure. */
	lua_pushcclosure(L, iter_getopt, 3 + argc);

	return 1;
}


/***
Return process group id of calling process.
@function getpgrp
@treturn int process group id of calling process
@see getpid
*/
static int
Pgetpgrp(lua_State *L)
{
	checknargs(L, 0);
	return pushintegerresult(getpgrp());
}


/***
Return process id of calling process.
@function getpid
@treturn int process id of calling process
*/
static int
Pgetpid(lua_State *L)
{
	checknargs(L, 0);
	return pushintegerresult(getpid());
}


/***
Return parent process id of calling process.
@function getppid
@treturn int parent process id of calling process
@see getpid
*/
static int
Pgetppid(lua_State *L)
{
	checknargs(L, 0);
	return pushintegerresult(getppid());
}


/***
Return user id of calling process.
@function getuid
@treturn int user id of calling process
@see geteuid
*/
static int
Pgetuid(lua_State *L)
{
	checknargs(L, 0);
	return pushintegerresult(getuid());
}


/***
Get host id.
@function gethostid
@treturn[1] int host id, if successful
@return[2] nil
@treturn[2] string error message
@see gethostid(3)
*/
static int
Pgethostid(lua_State *L)
{
	checknargs(L, 0);
#if HAVE_GETHOSTID
	return pushintegerresult(gethostid());
#else
	lua_pushnil(L);
	lua_pushliteral(L, "unsupported by this host");
	return 2;
#endif
}


/***
Test whether a file descriptor refers to a terminal.
@function isatty
@int fd file descriptor to act on
@treturn[1] int `1` if *fd* is open and refers to a terminal, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see isatty(3)
*/
static int
Pisatty(lua_State *L)
{
	int fd = checkint(L, 1);
	checknargs(L, 1);
	return pushresult(L, isatty(fd) == 0 ? -1 : 1, "isatty");
}


#if LPOSIX_2001_COMPLIANT
/***
This is like `chown`, but does not dereference symbolic links.
In other words, if a file is a symlink, then it changes ownership of the
symlink itself.
@function lchown
@string path existing file path
@tparam string|int uid new owner user id
@tparam string|int gid new owner group id
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error messoge
@treturn[2] int errnum
@see lchown(2)
@usage
  local unistd = require "posix.unistd"
  -- will fail for a normal user, and print an error
  print(unistd.lchown ("/etc/passwd", 100, 200))
*/
static int
Plchown(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	uid_t uid = mygetuid(L, 2);
	gid_t gid = mygetgid(L, 3);
	checknargs(L, 3);
	return pushresult(L, lchown(path, uid, gid), path);
}
#endif


/***
Create a link.
@function link
@string target name
@string link name
@bool[opt=false] soft link
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see link(2)
@see symlink(2)
*/
static int
Plink(lua_State *L)
{
	const char *oldpath = luaL_checkstring(L, 1);
	const char *newpath = luaL_checkstring(L, 2);
	int symbolicp = optboolean(L, 3, 0);
	checknargs(L, 3);
	return pushresult(L, (symbolicp ? symlink : link)(oldpath, newpath), NULL);
}


/***
Create a link at specified directory.
@function linkat
@int targetdir fd
@string target name
@int linkdir fd
@string link name
@int flags
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see linkat(2)
*/
static int
Plinkat(lua_State *L)
{
	int olddirfd = checkint(L, 1);
	const char *oldpath = luaL_checkstring(L, 2);
	int newdirfd = checkint(L, 3);
	const char *newpath = luaL_checkstring(L, 4);
	int flags = checkint(L, 5);
	checknargs(L, 5);
	return pushresult(L, linkat(olddirfd, oldpath, newdirfd, newpath, flags), NULL);
}


/***
reposition read/write file offset
@function lseek
@int fd open file descriptor to act on
@int offset bytes to seek
@int whence one of `SEEK_SET`, `SEEK_CUR` or `SEEK_END`
@treturn[1] int new offset, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see lseek(2)
*/
static int
Plseek(lua_State *L)
{
	int fd = checkint(L, 1);
	off_t offset = (off_t)checkinteger(L, 2);
	int whence = checkint(L, 3);
	checknargs(L, 3);
	return pushresult(L, lseek(fd, offset, whence), NULL);
}


/***
change process priority
@function nice
@int inc adds inc to the nice value for the calling process
@treturn[1] int new nice value, if successful
@return[2] nil
@return[2] string error message
@treturn[2] int errnum
@see nice(2)
*/
static int
Pnice(lua_State *L)
{
	int inc = checkint(L, 1);
	checknargs(L, 1);
	return pushresult(L, nice(inc), "nice");
}


/***
Get a value for a configuration option for a filename.
@function pathconf
@string path optional
@int key one of `_PC_LINK_MAX`, `_PC_MAX_CANON`, `_PC_NAME_MAX`,
  `_PC_PIPE_BUF`, `_PC_CHOWN_RESTRICTED`, `_PC_NO_TRUNC` or
  `_PC_VDISABLE`
@treturn int associated path configuration value
@see pathconf(3)
@usage
  local unistd = require "posix.unistd"
  for a, b in pairs (unistd.pathconf "/dev/tty") do print(a, b) end
*/
static int
Ppathconf(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 2);
	return pushintegerresult(pathconf(path, checkint(L, 2)));
}


/***
Creates a pipe.
@function pipe
@treturn[1] int read end file descriptor
@treturn[1] int write end file descriptor, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see pipe(2)
@see fork.lua
*/
static int
Ppipe(lua_State *L)
{
	int pipefd[2];
	int rc;
	checknargs(L, 0);
	rc = pipe(pipefd);
	if (rc < 0)
		return pusherror(L, "pipe");
	lua_pushinteger(L, pipefd[0]);
	lua_pushinteger(L, pipefd[1]);
	return 2;
}


/***
Read bytes from a file.
@function read
@int fd the file descriptor to act on
@int count maximum number of bytes to read
@treturn[1] string string from *fd* with at most *count* bytes, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see read(2)
*/
static int
Pread(lua_State *L)
{
	int fd = checkint(L, 1);
	size_t count = (size_t)checkinteger(L, 2);
	ssize_t ret;
	void *ud, *buf;
	lua_Alloc lalloc;

	checknargs(L, 2);
	lalloc = lua_getallocf(L, &ud);

	/* Reset errno in case lalloc doesn't set it */
	errno = 0;
	if ((buf = lalloc(ud, NULL, 0, count)) == NULL && count > 0)
		return pusherror(L, "lalloc");

	ret = read(fd, buf, count);
	if (ret >= 0)
		lua_pushlstring(L, buf, ret);
	lalloc(ud, buf, count, 0);
	return (ret < 0) ? pusherror(L, NULL) : 1;
}


/***
Read value of a symbolic link.
@function readlink
@string path file to act on
@treturn[1] string link target, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see readlink(2)
*/
static int
Preadlink(lua_State *L)
{
	char b[PATH_MAX];
	struct stat s;
	const char *path = luaL_checkstring(L, 1);
	void *ud;
	lua_Alloc lalloc;
	ssize_t n;
	checknargs(L, 1);

	errno = 0; /* ignore outstanding unreported errors */

	/* s.st_size is length of linkname, with no trailing \0 */
	if (lstat(path, &s) < 0)
		return pusherror(L, path);

	if (!S_ISLNK(s.st_mode))
	{
		lua_pushnil(L);
		lua_pushfstring(L, "%s: not a symbolic link", path);
		lua_pushinteger(L, EINVAL);
		return 3;
	}

	n = readlink(path, b, PATH_MAX);
	if (n < 0)
		/* report new errors from this function */
		return pusherror(L, "readlink");

	lua_pushlstring(L, b, n);
	return 1;
}


/***
Remove a directory.
@function rmdir
@string path file to act on
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see rmdir(2)
*/
static int
Prmdir(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 1);
	return pushresult(L, rmdir(path), path);
}


/***
Set the uid, euid, gid, egid, sid or pid & gid.
@function setpid
@string what one of 'u', 'U', 'g', 'G', 's', 'p' (upper-case means "effective")
@int id (uid, gid or pid for every value of `what` except 's')
@int[opt] gid (only for `what` value 'p')
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see setuid(2)
@see seteuid(2)
@see setgid(2)
@see setegid(2)
@see setsid(2)
@see setpgid(2)
*/
static int
Psetpid(lua_State *L)
{
	const char *what=luaL_checkstring(L, 1);
	checknargs(L, *what == 'p' ? 3 : 2);
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
			pid_t pid  = (pid_t)checkinteger(L, 2);
			pid_t pgid = (pid_t)checkinteger(L, 3);
			return pushresult(L, setpgid(pid,pgid), NULL);
		}
		default:
			badoption(L, 1, "id", *what);
			return 0;
	}
}


/***
Sleep for a number of seconds.
@function sleep
@int seconds minimum numebr of seconds to sleep
@treturn[1] int `0` if the requested time has elapsed
@treturn[2] int unslept seconds remaining, if interrupted
@see sleep(3)
@see posix.time.nanosleep
*/
static int
Psleep(lua_State *L)
{
	unsigned int seconds = checkint(L, 1);
	checknargs(L, 1);
	return pushintegerresult(sleep(seconds));
}


/***
Commit buffer cache to disk.
@function sync
@see fsync
@see sync(2)
*/
static int
Psync(lua_State *L)
{
	checknargs(L, 0);
	sync();
	return 0;
}


/***
Get configuration information at runtime.
@function sysconf
@int key one of `_SC_ARG_MAX`, `_SC_CHILD_MAX`, `_SC_CLK_TCK`, `_SC_JOB_CONTROL`,
  `_SC_OPEN_MAX`, `_SC_NGROUPS_MAX`, `_SC_SAVED_IDS`, `_SC_STREAM_MAX`,
  `_SC_PAGESIZE`, `_SC_TZNAME_MAX` or `_SC_VERSION`,
@treturn int associated system configuration value
@see sysconf(3)
*/
static int
Psysconf(lua_State *L)
{
	checknargs(L, 1);
	return pushintegerresult(sysconf(checkint(L, 1)));
}


/***
Name of a terminal device.
@function ttyname
@int[opt=0] fd file descriptor to process
@treturn[1] string name, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see ttyname(3)
*/
static int
Pttyname(lua_State *L)
{
	int fd = optint(L, 1, 0);
	char *name;
	checknargs(L, 1);
	name = ttyname(fd);
	if (name != NULL)
		return pushstringresult(name);
	if (errno != 0)
		return pusherror(L, "ttyname");
	lua_pushnil(L);
	lua_pushliteral(L, "not a tty");
	return 2;
}


#if LPOSIX_2001_COMPLIANT
/***
Get id of foreground process group of terminal *fd*.
@function tcgetpgrp
@int fd the file descriptor of the controlling terminal of the current process
@treturn[1] int id of foreground process group, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see tcgetpgrp(2)
*/
static int
Ptcgetpgrp(lua_State *L)
{
	int fd = checkint(L, 1);
	return pushresult(L, tcgetpgrp(fd), NULL);
}


/***
Make process group *pgid* the foreground process group of terminal *fd*.
@function tcsetpgrp
@int fd the file descriptor of the controlling terminal of the current process
@int pgid id of the process group to make foreground process group
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see tcsetpgrp(2)
*/
static int
Ptcsetpgrp(lua_State *L)
{
	int fd = checkint(L, 1);
	pid_t pgid = (pid_t)checkinteger(L, 2);
	return pushresult(L, tcsetpgrp(fd, pgid), NULL);
}
#endif


/***
Unlink a file.
@function unlink
@string path
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see unlink(2)
*/
static int
Punlink(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	checknargs(L, 1);
	return pushresult(L, unlink(path), path);
}


/***
Write bytes to a file.
If *nbytes* is `nil` or omitted, write all bytes from *offset*
through to the end of *buf*.  If *offset* is `nil` or omitted,
start writing bytes from the beginning of *buf*.
Bounds checks are enforced, returning `posix.errno.EINVAL`
before attempting to write any bytes, if the requested
parameters would access bytes outside *buf*.
@function write
@int fd the file descriptor to act on
@string buf containing bytes to write
@int[opt=#buf] nbytes number of bytes to write
@int[opt=0] offset skip the first offset bytes of buf
@treturn[1] int number of bytes written, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see write(2)
*/
static int
Pwrite(lua_State *L)
{
	const int fd = checkint(L, 1);
	const char *buf = luaL_checkstring(L, 2);
	const int buflen = lua_objlen(L, 2);
	ssize_t nbytes = (ssize_t)optinteger(L, 3, buflen);
	const off_t offset = (off_t)optinteger(L, 4, 0);
	off_t invalid_offset = offset;

	checknargs(L, 4);

	/* `write(fd, buf, nil, #buf - 3)` -> write the last 3 bytes in buf */
	if (offset && lua_type(L, 3) == LUA_TNIL)
		nbytes = buflen - offset;

	/* calling write with nbytes `0` may cause unspecified behaviour */
	if (nbytes == 0)
		return pushintegerresult(0);

	if (offset >= 0 && nbytes > 0 && offset + nbytes <= buflen)
		return pushresult(L, write(fd, buf + offset, nbytes), NULL);

	if (offset + nbytes < 0 || offset + nbytes > buflen)
		invalid_offset += nbytes;

	errno = EINVAL;
	lua_pushnil(L);
	lua_pushfstring(L, "write: invalid attempt to access offset %d in a buffer of length %d", invalid_offset, buflen);
	lua_pushinteger(L, errno);
	return 3;
}


/***
Truncate a file to a specified length.
@function ftruncate
@int fd the file descriptor to act on
@int length the length to truncate to
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see ftruncate(2)
*/
static int
Pftruncate(lua_State *L)
{
	int fd = checkint(L, 1);
	off_t length = (off_t)checkinteger(L, 2);
	checknargs(L, 2);
	return pushresult(L, ftruncate(fd, length), NULL);
}

/***
Truncate a file to a specified length.
@function truncate
@string path file to act on
@int length the length to truncate to
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@treturn[2] int errnum
@see ftruncate(2)
*/
static int
Ptruncate(lua_State *L)
{
	const char *path = luaL_checkstring(L, 1);
	off_t length = (off_t)checkinteger(L, 2);
	checknargs(L, 2);
	return pushresult(L, truncate(path, length), NULL);
}

static const luaL_Reg posix_unistd_fns[] =
{
	LPOSIX_FUNC( P_exit		),
	LPOSIX_FUNC( Paccess		),
	LPOSIX_FUNC( Palarm		),
	LPOSIX_FUNC( Pchdir		),
	LPOSIX_FUNC( Pchown		),
	LPOSIX_FUNC( Pclose		),
#if defined HAVE_CRYPT
	LPOSIX_FUNC( Pcrypt		),
#endif
	LPOSIX_FUNC( Pdup		),
	LPOSIX_FUNC( Pdup2		),
	LPOSIX_FUNC( Pexec		),
	LPOSIX_FUNC( Pexecp		),
#if HAVE_FDATASYNC
	LPOSIX_FUNC( Pfdatasync		),
#endif
	LPOSIX_FUNC( Pfork		),
	LPOSIX_FUNC( Pfsync		),
	LPOSIX_FUNC( Pgetcwd		),
#if LPOSIX_2001_COMPLIANT
	LPOSIX_FUNC( Pgetgroups		),
#endif
	LPOSIX_FUNC( Pgetegid		),
	LPOSIX_FUNC( Pgeteuid		),
	LPOSIX_FUNC( Pgetgid		),
	LPOSIX_FUNC( Pgetlogin		),
	LPOSIX_FUNC( Pgetopt		),
	LPOSIX_FUNC( Pgetpgrp		),
	LPOSIX_FUNC( Pgetpid		),
	LPOSIX_FUNC( Pgetppid		),
	LPOSIX_FUNC( Pgetuid		),
	LPOSIX_FUNC( Pgethostid		),
	LPOSIX_FUNC( Pisatty		),
#if LPOSIX_2001_COMPLIANT
	LPOSIX_FUNC( Plchown		),
#endif
	LPOSIX_FUNC( Plink		),
	LPOSIX_FUNC( Plinkat		),
	LPOSIX_FUNC( Plseek		),
	LPOSIX_FUNC( Pnice		),
	LPOSIX_FUNC( Ppathconf		),
	LPOSIX_FUNC( Ppipe		),
	LPOSIX_FUNC( Pread		),
	LPOSIX_FUNC( Preadlink		),
	LPOSIX_FUNC( Prmdir		),
	LPOSIX_FUNC( Psetpid		),
	LPOSIX_FUNC( Psleep		),
	LPOSIX_FUNC( Psync		),
	LPOSIX_FUNC( Psysconf		),
	LPOSIX_FUNC( Pttyname		),
#if LPOSIX_2001_COMPLIANT
	LPOSIX_FUNC( Ptcgetpgrp		),
	LPOSIX_FUNC( Ptcsetpgrp		),
#endif
	LPOSIX_FUNC( Punlink		),
	LPOSIX_FUNC( Pwrite		),
	LPOSIX_FUNC( Pftruncate		),
	LPOSIX_FUNC( Ptruncate		),
	{NULL, NULL}
};


/***
Constants.
@section constants
*/

/***
Standard constants.
Any constants not available in the underlying system will be `nil` valued.
@table posix.unistd
@int _PC_CHOWN_RESTRICTED return 1 if chown requires appropriate privileges, 0 otherwise
@int _PC_LINK_MAX maximum file link count
@int _PC_MAX_CANON maximum bytes in terminal canonical input line
@int _PC_MAX_INPUT maximum number of bytes in a terminal input queue
@int _PC_NAME_MAX maximum number of bytes in a file name
@int _PC_NO_TRUNC return 1 if over-long file names are truncated
@int _PC_PATH_MAXmaximum number of bytes in a pathname
@int _PC_PIPE_BUF maximum number of bytes in an atomic pipe write
@int _PC_VDISABLE terminal character disabling value
@int _SC_ARG_MAX maximum bytes of argument to @{posix.unistd.execp}
@int _SC_CHILD_MAX maximum number of processes per user
@int _SC_CLK_TCK statistics clock frequency
@int _SC_JOB_CONTROL return 1 if system has job control, -1 otherwise
@int _SC_NGROUPS_MAX maximum number of supplemental groups
@int _SC_OPEN_MAX maximum number of open files per user
@int _SC_SAVED_IDS return 1 if system supports saved user and group ids, -1 otherwise
@int _SC_STREAM_MAX maximum number of streams per process
@int _SC_TZNAME_MAX maximum number of timezone types
@int _SC_VERSION POSIX.1 compliance version
@int SEEK_CUR relative file pointer position
@int SEEK_END set file pointer to the end of file
@int SEEK_SET absolute file pointer position
@int STDERR_FILENO standard error file descriptor
@int STDIN_FILENO standard input file descriptor
@int STDOUT_FILENO standard output file descriptor
@usage
  -- Print unistd constants supported on this host.
  for name, value in pairs (require "posix.unistd") do
    if type (value) == "number" then
      print (name, value)
     end
  end
*/

LUALIB_API int
luaopen_posix_unistd(lua_State *L)
{
	luaL_newlib(L, posix_unistd_fns);
	lua_pushstring(L, LPOSIX_VERSION_STRING("unistd"));
	lua_setfield(L, -2, "version");

	/* pathconf arguments */
	LPOSIX_CONST( _PC_CHOWN_RESTRICTED	);
	LPOSIX_CONST( _PC_LINK_MAX		);
	LPOSIX_CONST( _PC_MAX_CANON		);
	LPOSIX_CONST( _PC_MAX_INPUT		);
	LPOSIX_CONST( _PC_NAME_MAX		);
	LPOSIX_CONST( _PC_NO_TRUNC		);
	LPOSIX_CONST( _PC_PATH_MAX		);
	LPOSIX_CONST( _PC_PIPE_BUF		);
	LPOSIX_CONST( _PC_VDISABLE		);

	/* sysconf arguments */
	LPOSIX_CONST( _SC_ARG_MAX	);
	LPOSIX_CONST( _SC_CHILD_MAX	);
	LPOSIX_CONST( _SC_CLK_TCK	);
	LPOSIX_CONST( _SC_JOB_CONTROL	);
	LPOSIX_CONST( _SC_NGROUPS_MAX	);
	LPOSIX_CONST( _SC_OPEN_MAX	);
	LPOSIX_CONST( _SC_PAGESIZE	);
	LPOSIX_CONST( _SC_SAVED_IDS	);
	LPOSIX_CONST( _SC_STREAM_MAX	);
	LPOSIX_CONST( _SC_TZNAME_MAX	);
	LPOSIX_CONST( _SC_VERSION	);

	/* lseek arguments */
	LPOSIX_CONST( SEEK_CUR		);
	LPOSIX_CONST( SEEK_END		);
	LPOSIX_CONST( SEEK_SET		);

	/* Miscellaneous */
	LPOSIX_CONST( STDERR_FILENO	);
	LPOSIX_CONST( STDIN_FILENO	);
	LPOSIX_CONST( STDOUT_FILENO	);

	return 1;
}
