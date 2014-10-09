/***
@module posix.unistd
*/
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

#include <config.h>

#if HAVE_CRYPT_H
#  include <crypt.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "_helpers.c"


/***
Terminate the calling process.
@function _exit
@int status process exit status
@see _exit(2)
*/
static int
P_exit(lua_State *L)
{
	pid_t ret = checkint(L, 1);
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
@see access(2)
@usage status, errstr, errno = P.access("/etc/passwd", "rw")
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
Set the working directory.
@function chdir
@string path file to act on
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
@see chdir(2)
@usage status, errstr, errno = P.chdir("/var/tmp")
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
@see chown(2)
@usage
-- will fail for a normal user, and print an error
print(P.chown("/etc/passwd",100,200))
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
@treturn[1] int `0` if successful
@return[2] nil
@treturn[2] string error message
@see close(2)
@usage
local ok, errmsg = P.close (log)
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
local salt, hash = pwent:match ":$6$(.-)$([^:]+)"
if P.crypt (trypass, salt) ~= hash then
  error "wrong password"
end
*/
static int
Pcrypt(lua_State *L)
{
	const char *str, *salt;
	char *res;

	str = luaL_checkstring(L, 1);
	salt = luaL_checkstring(L, 2);
	if (strlen(salt) < 2)
		luaL_error(L, "not enough salt");
	checknargs(L, 2);

	res = crypt(str, salt);
	lua_pushstring(L, res);

	return 1;
}
#endif


/***
Duplicate an open file descriptor.
@function dup
@int fd file descriptor to act on
@treturn[1] int new file descriptor duplicating *fd*, if successful
@return[2] nil
@treturn[2] string error message
@see dup(2)
@usage
local outfd = P.dup (P.fileno (io.stdout))
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
	int i,n=lua_gettop(L), table = 0;
	if (n >= 1 && lua_type(L, 2) == LUA_TTABLE)
	{
		checknargs(L, 2);
		n = lua_objlen(L, 2);
		table = 1;
	}
	else if (lua_type(L, 2) == LUA_TSTRING)
		n--;
	else
		argtypeerror(L, 2, "table or string");

	argv = lua_newuserdata(L,(n+2)*sizeof(char*));

	/* Set argv[0], defaulting to command */
	argv[0] = (char*)path;
	if (table)
	{
		lua_pushinteger(L, 0);
		lua_gettable(L, 2);
		if (lua_type(L, -1) == LUA_TSTRING)
			argv[0] = (char*)lua_tostring(L, -1);
		else
			lua_pop(L, 1);
	}

	/* Read argv[1..n] from arguments or table. */
	for (i=1; i<=n; i++)
	{
		if (table)
		{
			lua_pushinteger(L, i);
			lua_gettable(L, 2);
			argv[i] = (char*)lua_tostring(L, -1);
		}
		else
			argv[i] = (char*)luaL_checkstring(L, i+1);
	}
	argv[n+1] = NULL;

	(use_shell?execvp:execv)(path, argv);
	return pusherror(L, path);
}


/***
Execute a program without using the shell.
@function exec
@string path
@tparam table|strings ... table or tuple of arguments (table can include
  index 0)
@return nil
@return string error message
@see execve(2)
*/
static int
Pexec(lua_State *L)
{
	return runexec(L, 0);
}


/***
Execute a program using the shell.
@function execp
@string path
@tparam table|strings ... table or tuple of arguments (table can include
  index 0)
@return nil
@return string error message
@see execve(2)
*/
static int
Pexecp(lua_State *L)
{
	return runexec(L, 1);
}


#if _POSIX_VERSION >= 200112L

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
@see fork(2)
@see fork.lua
@see fork2.lua
@usage
local pid, errmsg = P.fork ()
if pid == nil then
  error (errmsg)
elseif pid == 0 then
  print ("in child:", P.getpid "pid")
else
  print (P.wait (pid))
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
@see getcwd(3)
*/
static int
Pgetcwd(lua_State *L)
{
#ifdef __GNU__
	char *b = get_current_dir_name();
	checknargs(L, 0);
	if (b == NULL)
		/* we return the same error as below */
		return pusherror(L, ".");
	lua_pushstring(L, b);
	return 1;
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
	return (r == NULL) ? pusherror(L, ".") : 1;
#endif
}


#if _POSIX_VERSION >= 200112L
/***
Get list of supplementary group ids.
@function getgroups
@see getgroups(2)
@treturn table group id
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
	lua_pushstring(L, getlogin());
	return 1;
}


static void
FgetID(lua_State *L, int i, const void *LPOSIX_UNUSED (data))
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


/***
Get process identifiers.
@function getpid
@string ... types, each one of "egid", "euid", "gid", "uid", "pgrp", "pid", "ppid"
@return ... values, or table of all ids if no option given
@usage P.getpid 'pid' -- PID of current process
*/
static int
Pgetpid(lua_State *L)
{
	return doselection(L, 1, SgetID, FgetID, NULL);
}


/***
Get host id.
@function hostid
@see gethostid(3)
@return host id
*/
static int
Phostid(lua_State *L)
{
	checknargs(L, 0);
	lua_pushinteger(L, gethostid());
	return 1;
}


/***
Test whether a file descriptor refers to a terminal.
@function isatty
@int fd file descriptor to act on
@treturn[1] int `1` if *fd* is open and refers to a terminal, if successful
@return[2] nil
@treturn[2] string error message
@see isatty(3)
*/
static int
Pisatty(lua_State *L)
{
	int fd = checkint(L, 1);
	checknargs(L, 1);
	return pushresult(L, isatty(fd) == 0 ? -1 : 1, "isatty");
}


/***
Create a link.
@function link
@string target name
@string link name
@bool[opt=false] soft link
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
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
reposition read/write file offset
@function lseek
@see lseek(2)
@int fd open file descriptor to act on
@int offset bytes to seek
@int whence one of `SEEK_SET`, `SEEK_CUR` or `SEEK_END`
@treturn[1] int new offset, if successful
@return[2] nil
@treturn[2] string error message
*/
static int
Plseek(lua_State *L)
{
	int fd = checkint(L, 1);
	int offset = checkint(L, 2);
	int whence = checkint(L, 3);
	checknargs(L, 3);
	return pushresult(L, lseek(fd, offset, whence), NULL);
}


static const char *
filetype(mode_t m)
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


static void
Fstat(lua_State *L, int i, const void *data)
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


/***
Information about an existing file path.
If file is a symbolic link return information about the link itself.
@function lstat
@string path file to act on
@string ... field names, each one of "mode", "ino", "dev", "nlink", "uid", "gid",
"size", "atime", "mtime", "ctime", "type"
@return ... values, or table of all fields if no option given
@see lstat(2)
@see stat
@usage for a, b in pairs(P.lstat("/etc/")) do print(a, b) end
*/
static int
Plstat(lua_State *L)
{
	struct stat s;
	const char *path=luaL_checkstring(L, 1);
	if (lstat(path,&s)==-1)
		return pusherror(L, path);
	return doselection(L, 2, Sstat, Fstat, &s);
}


/***
change process priority
@function nice
@int inc adds inc to the nice value for the calling process
@treturn[1] int new nice value, if successful
@return[2] nil
@return[2] string error message
@see nice(2)
*/
static int
Pnice(lua_State *L)
{
	int inc = checkint(L, 1);
	checknargs(L, 1);
	return pushresult(L, nice(inc), "nice");
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


static void
Fpathconf(lua_State *L, int i, const void *data)
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


/***
Get a value for a configuration option for a filename.
@function pathconf
@string path optional (default ".")
@string ... field names, each one of "LINK\_MAX", "MAX\_CANON", "NAME\_MAX", "PIPE\_BUF",
"CHOWN\_RESTRICTED", "NO\_TRUNC", "VDISABLE"
@return ... values, or table of all fields if no option given
@see pathconf(3)
@usage for a, b in pairs(P.pathconf("/dev/tty")) do print(a, b) end
*/
static int
Ppathconf(lua_State *L)
{
	const char *path = optstring(L, 1, ".");
	return doselection(L, 2, Spathconf, Fpathconf, path);
}


/***
Creates a pipe.
@function pipe
@treturn[1] int read end file descriptor
@treturn[1] int write end file descriptor
@return[2] nil
@treturn[2] string error message
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
@see read(2)
*/
static int
Pread(lua_State *L)
{
	int fd = checkint(L, 1);
	int count = checkint(L, 2), ret;
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
@see readlink(2)
*/
static int
Preadlink(lua_State *L)
{
	char *b;
	struct stat s;
	const char *path = luaL_checkstring(L, 1);
	void *ud;
	lua_Alloc lalloc;
	ssize_t n;
	int err;
	checknargs(L, 1);
	lalloc = lua_getallocf(L, &ud);

	errno = 0; /* ignore outstanding unreported errors */

	/* s.st_size is length of linkname, with no trailing \0 */
	if (lstat(path, &s) < 0)
		return pusherror(L, path);

	/* diagnose non-symlinks */
	if (!S_ISLNK(s.st_mode))
	{
		lua_pushnil(L);
		lua_pushfstring(L, "%s: not a symbolic link", path);
		lua_pushinteger(L, EINVAL);
		return 3;
	}

	/* allocate a buffer for linkname, with no trailing \0 */
	if ((b = lalloc(ud, NULL, 0, s.st_size)) == NULL)
		return pusherror(L, "lalloc");

	n = readlink(path, b, s.st_size);
	err = errno; /* save readlink error code, if any */
	if (n != -1)
		lua_pushlstring(L, b, s.st_size);
	lalloc(ud, b, s.st_size, 0);

	/* report new errors from this function */
	if (n < 0)
	{
		errno = err; /* restore readlink error code */
		return pusherror(L, "readlink");
	}
	else if (n < s.st_size)
	{
		lua_pushnil(L);
		lua_pushfstring(L, "%s: readlink wrote only %d of %d bytes", path, n, s.st_size);
		return 2;
	}

	return 1;
}


/***
Remove a directory.
@function rmdir
@string path file to act on
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
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
			pid_t pid  = checkint(L, 2);
			pid_t pgid = checkint(L, 3);
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
	lua_pushinteger(L, sleep(seconds));
	return 1;
}


/***
Information about an existing file path.
If file is a symbolic link return information about the file the link points to.
@function stat
@string path file to act on
@string ... field names, each one of "mode", "ino", "dev", "nlink", "uid", "gid",
"size", "atime", "mtime", "ctime", "type"
@return ... values, or table of all fields if no option given
@see stat(2)
@see lstat
@usage for a, b in pairs(P.stat("/etc/")) do print(a, b) end
*/
static int
Pstat(lua_State *L)
{
	struct stat s;
	const char *path=luaL_checkstring(L, 1);
	if (stat(path,&s)==-1)
		return pusherror(L, path);
	return doselection(L, 2, Sstat, Fstat, &s);
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

static void
Fsysconf(lua_State *L, int i, const void *LPOSIX_UNUSED (data))
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

/***
Get configuration information at runtime.
@function sysconf
@string ... field names, each one of `ARG_MAX`, `CHILD_MAX`, `CLK_TCK`, `NGROUPS_MAX`,
`STREAM_MAX`, `TZNAME_MAX`, `OPEN_MAX`, `JOB_CONTROL`, `VERSION`
@return ... values, or table of all fields no option
@see sysconf(3)
*/
static int
Psysconf(lua_State *L)
{
	return doselection(L, 1, Ssysconf, Fsysconf, NULL);
}


/***
Name of a terminal device.
@function ttyname
@see ttyname(3)
@int[opt=0] fd file descriptor to process
@return string name
*/
static int
Pttyname(lua_State *L)
{
	int fd=optint(L, 1, 0);
	checknargs(L, 1);
	lua_pushstring(L, ttyname(fd));
	return 1;
}


/***
Unlink a file.
@function unlink
@string path
@treturn[1] int `0`, if successful
@return[2] nil
@treturn[2] string error message
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
@function write
@int fd the file descriptor to act on
@string buf containing bytes to write
@treturn[1] int number of bytes written, if successful
@return[2] nil
@treturn[2] string error message
@see write(2)
*/
static int
Pwrite(lua_State *L)
{
	int fd = checkint(L, 1);
	const char *buf = luaL_checkstring(L, 2);
	checknargs(L, 2);
	return pushresult(L, write(fd, buf, lua_objlen(L, 2)), NULL);
}


static const luaL_Reg posix_unistd_fns[] =
{
	LPOSIX_FUNC( P_exit		),
	LPOSIX_FUNC( Paccess		),
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
#if _POSIX_VERSION >= 200112L
	LPOSIX_FUNC( Pfdatasync		),
#endif
	LPOSIX_FUNC( Pfork		),
	LPOSIX_FUNC( Pfsync		),
	LPOSIX_FUNC( Pgetcwd		),
#if _POSIX_VERSION >= 200112L
	LPOSIX_FUNC( Pgetgroups		),
#endif
	LPOSIX_FUNC( Pgetgroups		),
	LPOSIX_FUNC( Pgetlogin		),
	LPOSIX_FUNC( Pgetpid		),
	LPOSIX_FUNC( Phostid		),
	LPOSIX_FUNC( Pisatty		),
	LPOSIX_FUNC( Plink		),
	LPOSIX_FUNC( Plseek		),
	LPOSIX_FUNC( Plstat		),
	LPOSIX_FUNC( Pnice		),
	LPOSIX_FUNC( Ppathconf		),
	LPOSIX_FUNC( Ppipe		),
	LPOSIX_FUNC( Pread		),
	LPOSIX_FUNC( Preadlink		),
	LPOSIX_FUNC( Prmdir		),
	LPOSIX_FUNC( Psetpid		),
	LPOSIX_FUNC( Psleep		),
	LPOSIX_FUNC( Pstat		),
	LPOSIX_FUNC( Psync		),
	LPOSIX_FUNC( Psysconf		),
	LPOSIX_FUNC( Pttyname		),
	LPOSIX_FUNC( Punlink		),
	LPOSIX_FUNC( Pwrite		),
	{NULL, NULL}
};


LUALIB_API int
luaopen_posix_unistd(lua_State *L)
{
	luaL_register(L, "posix.unistd", posix_unistd_fns);
	lua_pushliteral(L, "posix.unistd for " LUA_VERSION " / " PACKAGE_STRING);
	lua_setfield(L, -2, "version");

	/* Miscellaneous */
	LPOSIX_CONST( STDIN_FILENO	);
	LPOSIX_CONST( STDOUT_FILENO	);
	LPOSIX_CONST( STDERR_FILENO	);

	/* lseek arguments */
	LPOSIX_CONST( SEEK_SET		);
	LPOSIX_CONST( SEEK_CUR		);
	LPOSIX_CONST( SEEK_END		);

	return 1;
}
