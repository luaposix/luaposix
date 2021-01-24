--[[
 POSIX library for Lua 5.1, 5.2, 5.3 & 5.4.
 Copyright (C) 2014-2021 Gary V. Vaughan
]]
--[[--
 Legacy Lua POSIX bindings.

 Undocumented Legacy APIs for compatibility with previous releases.

 @module posix.deprecated
]]

local HAVE_DEBUG, _debug = pcall(require, 'std._debug')

local argerror = require 'posix._base'.argerror

local _ENV = require 'posix._strict' {
   CLOCK_MONOTONIC = require 'posix.time'.CLOCK_MONOTONIC,
   CLOCK_PROCESS_CPUTIME_ID = require 'posix.time'.CLOCK_PROCESS_CPUTIME_ID,
   CLOCK_REALTIME = require 'posix.time'.CLOCK_REALTIME,
   CLOCK_THREAD_CPUTIME_ID = require 'posix.time'.CLOCK_THREAD_CPUTIME_ID,
   FNM_NOMATCH = require 'posix.fnmatch'.FNM_NOMATCH,
   LOG_CONS = require 'posix.syslog'.LOG_CONS,
   LOG_NDELAY = require 'posix.syslog'.LOG_NDELAY,
   LOG_PID = require 'posix.syslog'.LOG_PID,
   RLIMIT_AS = require 'posix.sys.resource'.RLIMIT_AS,
   RLIMIT_CORE = require 'posix.sys.resource'.RLIMIT_CORE,
   RLIMIT_CPU = require 'posix.sys.resource'.RLIMIT_CPU,
   RLIMIT_DATA = require 'posix.sys.resource'.RLIMIT_DATA,
   RLIMIT_FSIZE = require 'posix.sys.resource'.RLIMIT_FSIZE,
   RLIMIT_NOFILE = require 'posix.sys.resource'.RLIMIT_NOFILE,
   RLIMIT_STACK = require 'posix.sys.resource'.RLIMIT_STACK,
   S_ISBLK = require 'posix.sys.stat'.S_ISBLK,
   S_ISCHR = require 'posix.sys.stat'.S_ISCHR,
   S_ISDIR = require 'posix.sys.stat'.S_ISDIR,
   S_ISFIFO = require 'posix.sys.stat'.S_ISFIFO,
   S_ISLNK = require 'posix.sys.stat'.S_ISLNK,
   S_ISREG = require 'posix.sys.stat'.S_ISREG,
   S_ISSOCK = require 'posix.sys.stat'.S_ISSOCK,
   _PC_CHOWN_RESTRICTED = require 'posix.unistd'._PC_CHOWN_RESTRICTED,
   _PC_LINK_MAX = require 'posix.unistd'._PC_LINK_MAX,
   _PC_MAX_CANON = require 'posix.unistd'._PC_MAX_CANON,
   _PC_MAX_INPUT = require 'posix.unistd'._PC_MAX_INPUT,
   _PC_NAME_MAX = require 'posix.unistd'._PC_NAME_MAX,
   _PC_NO_TRUNC = require 'posix.unistd'._PC_NO_TRUNC,
   _PC_PATH_MAX = require 'posix.unistd'._PC_PATH_MAX,
   _PC_PIPE_BUF = require 'posix.unistd'._PC_PIPE_BUF,
   _PC_VDISABLE = require 'posix.unistd'._PC_VDISABLE,
   _SC_ARG_MAX = require 'posix.unistd'._SC_ARG_MAX,
   _SC_CHILD_MAX = require 'posix.unistd'._SC_CHILD_MAX,
   _SC_CLK_TCK = require 'posix.unistd'._SC_CLK_TCK,
   _SC_JOB_CONTROL = require 'posix.unistd'._SC_JOB_CONTROL,
   _SC_NGROUPS_MAX = require 'posix.unistd'._SC_NGROUPS_MAX,
   _SC_OPEN_MAX = require 'posix.unistd'._SC_OPEN_MAX,
   _SC_PAGESIZE = require 'posix.unistd'._SC_PAGESIZE,
   _SC_SAVED_IDS = require 'posix.unistd'._SC_SAVED_IDS,
   _SC_STREAM_MAX = require 'posix.unistd'._SC_STREAM_MAX,
   _SC_TZNAME_MAX = require 'posix.unistd'._SC_TZNAME_MAX,
   _SC_VERSION = require 'posix.unistd'._SC_VERSION,
   _debug = HAVE_DEBUG and _debug or {},
   argscheck = require 'posix._base'.argscheck,
   bind = require 'posix.sys.socket'.bind,
   bor = require 'posix._base'.bor,
   clock_getres = require 'posix.time'.clock_getres or false,
   clock_gettime = require 'posix.time'.clock_gettime or false,
   connect = require 'posix.sys.socket'.connect,
   error = error,
   exec = require 'posix.unistd'.exec,
   execp = require 'posix.unistd'.execp,
   fileno = require 'posix.stdio'.fileno,
   fnmatch = require 'posix.fnmatch'.fnmatch,
   format = string.format,
   getegid = require 'posix.unistd'.getegid,
   geteuid = require 'posix.unistd'.geteuid,
   getgid = require 'posix.unistd'.getgid,
   getgrgid = require 'posix.grp'.getgrgid,
   getgrnam = require 'posix.grp'.getgrnam,
   gethostid = require 'posix.unistd'.gethostid,
   getpgrp = require 'posix.unistd'.getpgrp,
   getppid = require 'posix.unistd'.getppid,
   getpwnam = require 'posix.pwd'.getpwnam,
   getpwuid = require 'posix.pwd'.getpwuid,
   getrlimit = require 'posix.sys.resource'.getrlimit,
   gettimeofday = require 'posix.sys.time'.gettimeofday,
   getuid = require 'posix.unistd'.getuid,
   getpid = require 'posix.unistd'.getpid,
   gmtime = require 'posix.time'.gmtime,
   gsub = string.gsub,
   isgraph = require 'posix.ctype'.isgraph,
   isprint = require 'posix.ctype'.isprint,
   localtime = require 'posix.time'.localtime,
   lower = string.lower,
   lstat = require 'posix.sys.stat'.lstat,
   mktime = require 'posix.time'.mktime,
   nanosleep = require 'posix.time'.nanosleep,
   next = next,
   nonempty = next,
   openlog = require 'posix.syslog'.openlog,
   pathconf = require 'posix.unistd'.pathconf,
   posix_fadvise = require 'posix.fcntl'.posix_fadvise or false,
   pushmode = require 'posix._base'.pushmode,
   require = require,
   select = select,
   setrlimit = require 'posix.sys.resource'.setrlimit,
   statvfs = require 'posix.sys.statvfs'.statvfs,
   strftime = require 'posix.time'.strftime,
   strptime = require 'posix.time'.strptime,
   sub = string.sub,
   sysconf = require 'posix.unistd'.sysconf,
   time = require 'posix.time'.time,
   times = require 'posix.sys.times'.times,
   type = type,
   uname = require 'posix.sys.utsname'.uname,
   unpack = table.unpack or unpack,
}



-- FIXME: specl-14.x breaks function environments here :(
local bor, exec, getegid, getgrgid, getgrnam, getrlimit, gmtime, localtime, lower, mktime, nanosleep, strftime, strptime, time, uname =
   bor, exec, getegid, getgrgid, getgrnam, getrlimit, gmtime, localtime, lower, mktime, nanosleep, strftime, strptime, time, uname


local OPENLOG_MAP = {
   [' '] = 0,
   c = LOG_CONS,
   n = LOG_NDELAY,
   p = LOG_PID,
}

local RLIMIT_MAP = {
   core = RLIMIT_CORE,
   cpu = RLIMIT_CPU,
   data = RLIMIT_DATA,
   fsize = RLIMIT_FSIZE,
   nofile = RLIMIT_NOFILE,
   stack = RLIMIT_STACK,
   as = RLIMIT_AS,
}

-- Convert a legacy API tm table to a posix.time.PosixTm compatible table.
local function PosixTm(legacytm)
   return {
      tm_sec = legacytm.sec,
      tm_min = legacytm.min,
      tm_hour = legacytm.hour,
      -- For compatibility with Lua os.date() use 'day' if 'monthday' is
      -- not set.
      tm_mday = legacytm.monthday or legacytm.day,
      tm_mon = legacytm.month - 1,
      tm_year = legacytm.year - 1900,
      tm_wday = legacytm.weekday,
      tm_yday = legacytm.yearday,
      tm_isdst = legacytm.is_dst and 1 or 0,
   }
end

-- Convert a posix.time.PosixTm into a legacy API tm table.
local function LegacyTm(posixtm)
   return {
      sec = posixtm.tm_sec,
      min = posixtm.tm_min,
      hour = posixtm.tm_hour,
      monthday = posixtm.tm_mday,
      day = posixtm.tm_mday,
      month = posixtm.tm_mon + 1,
      year = posixtm.tm_year + 1900,
      weekday = posixtm.tm_wday,
      yearday = posixtm.tm_yday,
      is_dst = posixtm.tm_isdst ~= 0,
   }
end


local function argtypeerror(name, i, expect, actual, level)
   level = level or 1
   local fmt = '%s expected, got %s'
   argerror(
      name,
      i,
      format(fmt, expect, gsub(type(actual), 'nil', 'no value')),
      level + 1
   )
end


local function badoption(name, i, what, option, level)
   level = level or 1
   local fmt = "invalid %s option '%s'"
   argerror(name, i, format(fmt, what, option), level + 1)
end


local function checkstring(name, i, actual, level)
   level = level or 1
   if type(actual) ~= 'string' then
      argtypeerror(name, i, 'string', actual, level + 1)
   end
   return actual
end


local function optstring(name, i, actual, def, level)
   level = level or 1
   if actual ~= nil and type(actual) ~= 'string' then
      argtypeerror(name, i, 'string or nil', actual, level + 1)
   end
   return actual or def
end


local function toomanyargerror(name, expected, got, level)
   level = level or 1
   local fmt = 'no more than %d argument%s expected, got %d'
   argerror(
      name,
      expected + 1,
      format(fmt, expected, expected == 1 and '' or 's', got),
      level + 1
   )
end


local function checkselection(fname, argi, fields, level)
   level = level or 1
   local field1, type1 = fields[1], type(fields[1])
   if type1 == 'table' and #fields > 1 then
      toomanyargerror(fname, argi, #fields + argi - 1, level + 1)
   elseif field1 ~= nil and type1 ~= 'table' and type1 ~= 'string' then
      argtypeerror(fname, argi, 'string, table or nil', field1, level + 1)
   end
   for i = 2, #fields do
      checkstring(fname, i + argi -1, fields[i], level + 1)
   end
end


local function doselection(name, argoffset, fields, map)
   if #fields == 1 and type(fields[1]) == 'table' then
      fields = fields[1]
   end

   if nonempty(fields) then
      local r = {}
      for i = 1, #fields do
         local v = fields[i]
         if map[v] then
            r[#r + 1] = map[v]
         else
            argerror(name, i + argoffset, "invalid option '" .. v .. "'", 2)
         end
      end
      return unpack(r)
   else
      return map
   end
end


local get_clk_id_const

local Pclock_getres
if clock_getres then
   -- When supported by underlying system
   get_clk_id_const = function(name)
      local map = {
         monotonic = CLOCK_MONOTONIC,
         process_cputime_id = CLOCK_PROCESS_CPUTIME_ID,
         thread_cputime_id = CLOCK_THREAD_CPUTIME_ID,
      }
      return map[name] or CLOCK_REALTIME
   end

   Pclock_getres = argscheck('clock_getres(?string)', function(name)
      local ts = require 'posix.time'.clock_getres(get_clk_id_const(name))
      return ts.tv_sec, ts.tv_nsec
   end)
end


local Pclock_gettime
if clock_gettime then
   -- When supported by underlying system
   Pclock_gettime = argscheck('clock_gettime(?string)', function(name)
      local ts = require 'posix.time'.clock_gettime(get_clk_id_const(name))
      return ts.tv_sec, ts.tv_nsec
   end)
end


local function Pexec(path, ...)
   local argt = (...)
   if type(argt) ~= 'table' then
      argt = {...}
   end
   return exec(path, argt)
end

if _debug.argcheck then
   Pexec = function(path, ...)
      checkstring('exec', 1, path)
      local argt = (...)
      if type(argt) == 'string' then
         argt = {...}
         for i = 1, #argt do
            checkstring('exec', i + 1, argt[i])
         end
      elseif type(argt) == 'table' then
         local n = select('#', ...)
         if n > 1 then
            toomanyargerror('exec', 2, n + 1)
         end
      else
         argtypeerror('exec', 2, 'string, table or nil', argt)
      end
      return exec(path, argt)
   end
end


local function Pexecp(path, ...)
   local argt = (...)
   if type(argt) ~= 'table' then
      argt = {...}
   end
   return execp(path, argt)
end

if _debug.argcheck then
   Pexecp = function(path, ...)
      checkstring('execp', 1, path)
      local argt = (...)
      if type(argt) == 'string' then
         argt = {...}
         for i = 1, #argt do
            checkstring('execp', i + 1, argt[i])
         end
      elseif type(argt) == 'table' then
         local n = select('#', ...)
         if n > 1 then
            toomanyargerror('execp', 2, n + 1)
         end
      else
         argtypeerror('execp', 2, 'string, table or nil', argt)
      end
      return execp(path, argt)
   end
end


local Pfadvise
if posix_fadvise then
   -- When supported by underlying system
   Pfadvise = argscheck('fadvise(FILE*, int, int, int)', function(fh, ...)
      return posix_fadvise(fileno(fh), ...)
   end)
end


local function Pgetpasswd(user, ...)
   user = user or geteuid()

   local p
   if type(user) == 'number' then
      p = getpwuid(user)
   elseif type(user) == 'string' then
      p = getpwnam(user)
   end

   if p ~= nil then
      return doselection('getpasswd', 1, {...}, {
         dir = p.pw_dir,
         gid = p.pw_gid,
         name = p.pw_name,
         passwd = p.pw_passwd,
         shell = p.pw_shell,
         uid = p.pw_uid,
      })
   end
end

if _debug.argcheck then
   local _getpasswd = Pgetpasswd

   Pgetpasswd = function(user, ...)
      checkselection('getpasswd', 2, {...}, 2)
      return _getpasswd(user, ...)
   end
end


local function Pgetpid(...)
   return doselection('getpid', 0, {...}, {
      egid = getegid(),
      euid = geteuid(),
      gid = getgid(),
      uid = getuid(),
      pgrp = getpgrp(),
      pid = getpid(),
      ppid = getppid(),
   })
end

if _debug.argcheck then
   local _getpid = Pgetpid

   Pgetpid = function(...)
      checkselection('getpid', 1, {...}, 2)
      return _getpid(...)
   end
end


local Spathconf = {
   CHOWN_RESTRICTED = _PC_CHOWN_RESTRICTED,
   LINK_MAX = _PC_LINK_MAX,
   MAX_CANON = _PC_MAX_CANON,
   MAX_INPUT = _PC_MAX_INPUT,
   NAME_MAX = _PC_NAME_MAX,
   NO_TRUNC = _PC_NO_TRUNC,
   PATH_MAX = _PC_PATH_MAX,
   PIPE_BUF = _PC_PIPE_BUF,
   VDISABLE = _PC_VDISABLE,
}

local function Ppathconf(path, ...)
   local argt, map = {...}, {}
   if path ~= nil and Spathconf[path] ~= nil then
      path, argt = '.', {path, ...}
   end
   for k, v in next, Spathconf do
      map[k] = pathconf(path or '.', v)
   end
   return doselection('pathconf', 1, {...}, map)
end

if _debug.argcheck then
   local _pathconf = Ppathconf

   Ppathconf = function(path, ...)
      if path ~= nil and Spathconf[path] ~= nil then
         checkselection('pathconf', 1, {path, ...}, 2)
      else
         optstring('pathconf', 1, path, '.', 2)
         checkselection('pathconf', 2, {...}, 2)
      end
      return _pathconf(path, ...)
   end
end


local function filetype(mode)
   if S_ISREG(mode) ~= 0 then
      return 'regular'
   elseif S_ISLNK(mode) ~= 0 then
      return 'link'
   elseif S_ISDIR(mode) ~= 0 then
      return 'directory'
   elseif S_ISCHR(mode) ~= 0 then
      return 'character device'
   elseif S_ISBLK(mode) ~= 0 then
      return 'block device'
   elseif S_ISFIFO(mode) ~= 0 then
      return 'fifo'
   elseif S_ISSOCK(mode) ~= 0 then
      return 'socket'
   else
      return '?'
   end
end


local function Pstat(path, ...)
   -- for bugwards compatibility with v<=32
   local info = lstat(path)
   if info ~= nil then
      return doselection('stat', 1, {...}, {
         dev = info.st_dev,
         ino = info.st_ino,
         mode = pushmode(info.st_mode),
         nlink = info.st_nlink,
         uid = info.st_uid,
         gid = info.st_gid,
         size = info.st_size,
         atime = info.st_atime,
         mtime = info.st_mtime,
         ctime = info.st_ctime,
         type = filetype(info.st_mode),
      })
   end
end

if _debug.argcheck then
   local _stat = Pstat

   Pstat = function(path, ...)
      checkstring('stat', 1, path, 2)
      checkselection('stat', 2, {...}, 2)
      return _stat(path, ...)
   end
end


local function Pstatvfs(path, ...)
   local info = statvfs(path)
   if info ~= nil then
      return doselection('statvfs', 1, {...}, {
         bsize = info.f_bsize,
         frsize = info.f_frsize,
         blocks = info.f_blocks,
         bfree = info.f_bfree,
         bavail = info.f_bavail,
         files = info.f_files,
         ffree = info.f_ffree,
         favail = info.f_favail,
         fsid = info.f_fsid,
         flag = info.f_flag,
         namemax = info.f_namemax,
      })
   end
end

if _debug.argcheck then
   local _statvfs = Pstatvfs

   Pstatvfs = function(path, ...)
      checkstring('statvfs', 1, path, 2)
      checkselection('statvfs', 2, {...}, 2)
      return _statvfs(path, ...)
   end
end


local function Psysconf(...)
   return doselection('sysconf', 0, {...}, {
      ARG_MAX = sysconf(_SC_ARG_MAX),
      CHILD_MAX = sysconf(_SC_CHILD_MAX),
      CLK_TCK = sysconf(_SC_CLK_TCK),
      JOB_CONTROL = sysconf(_SC_JOB_CONTROL),
      NGROUPS_MAX = sysconf(_SC_NGROUPS_MAX),
      OPEN_MAX = sysconf(_SC_OPEN_MAX),
      PAGESIZE = sysconf(_SC_PAGESIZE),
      SAVED_IDS = sysconf(_SC_SAVED_IDS),
      STREAM_MAX = sysconf(_SC_STREAM_MAX),
      TZNAME_MAX = sysconf(_SC_TZNAME_MAX),
      VERSION = sysconf(_SC_VERSION),
   })
end

if _debug.argcheck then
   local _sysconf = Psysconf

   Psysconf = function(...)
      checkselection('sysconf', 1, {...}, 2)
      return _sysconf(...)
   end
end


local function Ptimes(...)
   local info = times()
   return doselection('times', 0, {...}, {
      utime = info.tms_utime,
      stime = info.tms_stime,
      cutime = info.tms_cutime,
      cstime = info.tms_cstime,
      elapsed = info.elapsed,
   })
end

if _debug.argcheck then
   local _times = Ptimes

   Ptimes = function(...)
      checkselection('times', 1, {...}, 2)
      return _times(...)
   end
end



return {
   --- Bind an address to a socket.
   -- @function bind
   -- @int fd socket descriptor to act on
   -- @tparam PosixSockaddr addr socket address
   -- @treturn[1] bool `true`, if successful
   -- @return[2] nil
   -- @treturn[2] string error messag
   -- @treturn[2] int errnum
   -- @see bind(2)
   bind = function(...)
      local rt = {bind(...)}
      if rt[1] == 0 then
         return true
      end
      return unpack(rt)
   end,

   --- Find the precision of a clock.
   -- @function clock_getres
   -- @string[opt='realtime'] name name of clock, one of 'monotonic',
   --    'process\_cputime\_id', 'realtime', or 'thread\_cputime\_id'
   -- @treturn[1] int seconds
   -- @treturn[1] int nanoseconds, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   -- @see clock_getres(3)
   clock_getres = Pclock_getres,

   --- Read a clock
   -- @function clock_gettime
   -- @string[opt='realtime'] name name of clock, one of 'monotonic',
   --    'process\_cputime\_id', 'realtime', or 'thread\_cputime\_id'
   -- @treturn[1] int seconds
   -- @treturn[1] int nanoseconds, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   -- @see clock_gettime(3)
   clock_gettime = Pclock_gettime,

   --- Initiate a connection on a socket.
   -- @function connect
   -- @int fd socket descriptor to act on
   -- @tparam PosixSockaddr addr socket address
   -- @treturn[1] bool `true`, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   -- @see connect(2)
   connect = function(...)
      local rt = {connect(...)}
      if rt[1] == 0 then
         return true
      end
      return unpack(rt)
   end,

   --- Execute a program without using the shell.
   -- @function exec
   -- @string path
   -- @tparam[opt] table|strings ... table or tuple of arguments(table can include index 0)
   -- @return nil
   -- @treturn string error message
   -- @treturn int errnum
   -- @see execve(2)
   exec = Pexec,

   --- Execute a program with the shell.
   -- @function execp
   -- @string path
   -- @tparam[opt] table|strings ... table or tuple of arguments(table can include index 0)
   -- @return nil
   -- @treturn string error message
   -- @treturn int errnum
   -- @see execve(2)
   execp = Pexecp,

   --- Instruct kernel on appropriate cache behaviour for a file or file segment.
   -- @function fadvise
   -- @tparam file fh Lua file object
   -- @int offset start of region
   -- @int len number of bytes in region
   -- @int advice one of `POSIX_FADV_NORMAL, `POSIX_FADV_SEQUENTIAL,
   -- `POSIX_FADV_RANDOM`, `POSIX_FADV_\NOREUSE`, `POSIX_FADV_WILLNEED` or
   -- `POSIX_FADV_DONTNEED`
   -- @treturn[1] int `0`, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   -- @see posix_fadvise(2)
   fadvise = Pfadvise,

   --- Match a filename against a shell pattern.
   -- @function fnmatch
   -- @string pat shell pattern
   -- @string name filename
   -- @treturn bool `true` if *pat* matched *name*, otherwise `false`
   -- @raise error if fnmatch failed
   -- @see posix.fnmatch.fnmatch
   fnmatch = function(...)
      local r = fnmatch(...)
      if r == 0 then
         return true
      elseif r == FNM_NOMATCH then
         return false
      end
      error 'fnmatch failed'
   end,

   --- Information about a group.
   -- @function getgroup
   -- @tparam[opt=current group] int|string group id or group name
   -- @treturn group group information
   -- @usage
   --    print(posix.getgroup(posix.getgid()).name)
   getgroup = argscheck('getgroup(?int|string)', function(grp)
      grp = grp or getegid()

      local g
      if type(grp) == 'number' then
         g = getgrgid(grp)
      elseif type(grp) == 'string' then
         g = getgrnam(grp)
      end

      if g ~= nil then
         return {name=g.gr_name, gid=g.gr_gid, mem=g.gr_mem}
      end
   end),

   --- Get the password entry for a user.
   -- @function getpasswd
   -- @tparam[opt=current user] int|string user name or id
   -- @string ... field names, each one of 'uid', 'name', 'gid', 'passwd',
   --    'dir' or 'shell'
   -- @return ... values, or a table of all fields if *user* is `nil`
   -- @usage for a,b in pairs(posix.getpasswd 'root') do print(a, b) end
   -- @usage print(posix.getpasswd('root', 'shell'))
   getpasswd = argscheck('getpasswd(?int|string, ?string...)', Pgetpasswd),

   --- Get process identifiers.
   -- @function getpid
   -- @tparam[opt] table|string type one of 'egid', 'euid', 'gid', 'uid',
   --    'pgrp', 'pid' or 'ppid'; or a single list of the same
   -- @string[opt] ... unless *type* was a table, zero or more additional
   --    type strings
   -- @return ... values, or a table of all fields if no option given
   -- @usage for a,b in pairs(posix.getpid()) do print(a, b) end
   -- @usage print(posix.getpid('uid', 'euid'))
   getpid = Pgetpid,

   --- Get resource limits for this process.
   -- @function getrlimit
   -- @string resource one of 'core', 'cpu', 'data', 'fsize', 'nofile',
   --    'stack' or 'as'
   -- @treturn[1] int soft limit
   -- @treturn[1] int hard limit, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   getrlimit = argscheck('getrlimit(string)', function (rcstr)
      local rc = RLIMIT_MAP[lower(rcstr)]
      if rc == nil then
         argerror('getrlimit', 1, "invalid option '" .. rcstr .. "'")
      end
      local rlim = getrlimit(rc)
      return rlim.rlim_cur, rlim.rlim_max
   end),

   --- Get time of day.
   -- @function gettimeofday
   -- @treturn timeval time elapsed since *epoch*
   -- @see gettimeofday(2)
   gettimeofday = function(...)
      local tv = gettimeofday(...)
      return {sec=tv.tv_sec, usec=tv.tv_usec}
   end,

   --- Convert epoch time value to a broken-down UTC time.
   -- Here, broken-down time tables the month field is 1-based not
   -- 0-based, and the year field is the full year, not years since
   -- 1900.
   -- @function gmtime
   -- @int[opt=now] t seconds since epoch
   -- @treturn table broken-down time
   gmtime = argscheck('gmtime(?int)', function(epoch)
      return LegacyTm(gmtime(epoch or time()))
   end),

   --- Get host id.
   -- @function hostid
   -- @treturn int unique host identifier
   hostid = gethostid,

   --- Check for any printable character except space.
   -- @function isgraph
   -- @string character to act on
   -- @treturn bool non-`false` if character is in the class
   -- @see isgraph(3)
   isgraph = function(...)
      return isgraph(...) ~= 0
   end,

   --- Check for any printable character including space.
   -- @function isprint
   -- @string character to act on
   -- @treturn bool non-`false` if character is in the class
   -- @see isprint(3)
   isprint = function(...)
      return isprint(...) ~= 0
   end,

   --- Convert epoch time value to a broken-down local time.
   -- Here, broken-down time tables the month field is 1-based not
   -- 0-based, and the year field is the full year, not years since
   -- 1900.
   -- @function localtime
   -- @int[opt=now] t seconds since epoch
   -- @treturn table broken-down time
   localtime = argscheck('localtime(?int)', function(epoch)
      return LegacyTm(localtime(epoch or time()))
   end),

   --- Convert a broken-down localtime table into an epoch time.
   -- @function mktime
   -- @tparam tm broken-down localtime table
   -- @treturn in seconds since epoch
   -- @see mktime(3)
   -- @see localtime
   mktime = argscheck('mktime(?table)', function(legacytm)
      local posixtm = legacytm and PosixTm(legacytm) or localtime(time())
      return mktime(posixtm)
   end),

   --- Sleep with nanosecond precision.
   -- @function nanosleep
   -- @int seconds requested sleep time
   -- @int nanoseconds requested sleep time
   -- @treturn[1] int `0` if requested time has elapsed, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   -- @treturn[2] int unslept seconds remaining, if interrupted
   -- @treturn[2] int unslept nanoseconds remaining, if interrupted
   -- @see nanosleep(2)
   -- @see posix.unistd.sleep
   nanosleep = argscheck('nanosleep(int, int)', function(sec, nsec)
      local r, errmsg, errno, timespec = nanosleep {tv_sec=sec, tv_nsec=nsec}
      if r == 0 then
         return 0
      end
      return r, errmsg, errno, timespec.tv_sec, timespec.tv_nsec
   end),

   --- Open the system logger.
   -- @function openlog
   -- @string ident all messages will start with this
   -- @string[opt] option any combination of 'c'(directly to system console
   --    if an error sending), 'n'(no delay) and 'p'(show PID)
   -- @int [opt=`LOG_USER`] facility one of `LOG_AUTH`, `LOG_AUTHORITY`,
   --    `LOG_CRON`, `LOG_DAEMON`, `LOG_KERN`, `LOG_LPR`, `LOG_MAIL`,
   --    `LOG_NEWS`, `LOG_SECURITY`, `LOG_USER`, `LOG_UUCP` or `LOG_LOCAL0`
   --    through `LOG_LOCAL7`
   -- @see syslog(3)
   openlog = argscheck('openlog(string, ?string, ?int)',
      function(ident, optstr, facility)
         local option = 0
         if optstr then
            for i = 1, #optstr do
               local c = sub(optstr, i, i)
               if OPENLOG_MAP[c] == nil then
                  badoption('openlog', 2, 'openlog', c)
               end
               option = bor(option, OPENLOG_MAP[c])
            end
         end
         return openlog(ident, option, facility)
      end
   ),

   --- Get configuration information at runtime.
   -- @function pathconf
   -- @string[opt='.'] path file to act on
   -- @tparam[opt] table|string key one of 'CHOWN_RESTRICTED', 'LINK_MAX',
   --    'MAX_CANON', 'MAX_INPUT', 'NAME_MAX', 'NO_TRUNC', 'PATH_MAX', 'PIPE_BUF'
   --    or 'VDISABLE'
   -- @string[opt] ... unless *type* was a table, zero or more additional
   --    type strings
   -- @return ... values, or a table of all fields if no option given
   -- @see sysconf(2)
   -- @usage for a,b in pairs(posix.pathconf '/dev/tty') do print(a, b) end
   pathconf = Ppathconf,

   --- Set resource limits for this process.
   -- @function setrlimit
   -- @string resource one of 'core', 'cpu', 'data', 'fsize', 'nofile',
   --    'stack' or 'as'
   -- @int[opt] softlimit process may receive a signal when reached
   -- @int[opt] hardlimit process may be terminated when reached
   -- @treturn[1] int `0`, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   setrlimit = argscheck('setrlimit(string, ?int, ?int)', function(rcstr, cur, max)
      local rc = RLIMIT_MAP[lower(rcstr)]
      if rc == nil then
         argerror('setrlimit', 1, "invalid option '" .. rcstr .. "'")
      end
      local lim
      if cur == nil or max == nil then
         lim = getrlimit(rc)
      end
      return setrlimit(rc, {
         rlim_cur = cur or lim.rlim_cur,
         rlim_max = max or lim.rlim_max,
      })
   end),

   --- Information about an existing file path.
   -- If the file is a symbolic link, return information about the link
   -- itself.
   -- @function stat
   -- @string path file to act on
   -- @tparam[opt] table|string field one of 'dev', 'ino', 'mode', 'nlink',
   --    'uid', 'gid', 'rdev', 'size', 'atime', 'mtime', 'ctime' or 'type'
   -- @string[opt] ... unless *field* was a table, zero or more additional
   --    field names
   -- @return values, or table of all fields if no option given
   -- @see stat(2)
   -- @usage for a,b in pairs(P,stat '/etc/') do print(a, b) end
   stat = Pstat,

   --- Fetch file system statistics.
   -- @function statvfs
   -- @string path any path within the mounted file system
   -- @tparam[opt] table|string field one of 'bsize', 'frsize', 'blocks',
   --    'bfree', 'bavail', 'files', 'ffree', 'favail', 'fsid', 'flag',
   --    'namemax'
   -- @string[opt] ... unless *field* was a table, zero or more additional
   --    field names
   -- @return values, or table of all fields if no option given
   -- @see statvfs(2)
   -- @usage for a,b in pairs(P,statvfs '/') do print(a, b) end
   statvfs = Pstatvfs,

   --- Write a time out according to a format.
   -- @function strftime
   -- @string format specifier with `%` place-holders
   -- @tparam[opt] PosixTm tm broken-down local time
   -- @treturn string *format* with place-holders plugged with *tm* values
   -- @see strftime(3)
   strftime = argscheck('strftime(string, ?table)', function(fmt, legacytm)
      local posixtm = legacytm and PosixTm(legacytm) or localtime(time())
      return strftime(fmt, posixtm)
   end),

   --- Parse a date string.
   -- @function strptime
   -- @string s
   -- @string format same as for `strftime`
   -- @usage posix.strptime('20','%d').monthday == 20
   -- @treturn[1] PosixTm broken-down local time
   -- @treturn[1] int next index of first character not parsed as part of the date, if successful
   -- @return[2] nil
   -- @see strptime(3)
   strptime = argscheck('strptime(string, string)', function(s, fmt)
      local tm, i = strptime(s, fmt)
      return LegacyTm(tm), i
   end),

   --- Get configuration information at runtime.
   -- @function sysconf
   -- @tparam[opt] table|string key one of 'ARG_MAX', 'CHILD_MAX',
   --    'CLK_TCK', 'JOB_CONTROL', 'NGROUPS_MAX', 'OPEN_MAX', 'SAVED_IDS',
   --    'STREAM_MAX', 'TZNAME_MAX' or 'VERSION'
   -- @string[opt] ... unless *type* was a table, zero or more additional
   --    type strings
   -- @return ... values, or a table of all fields if no option given
   -- @see sysconf(2)
   -- @usage for a,b in pairs(posix.sysconf()) do print(a, b) end
   -- @usage print(posix.sysconf('STREAM_MAX', 'ARG_MAX'))
   sysconf = Psysconf,

   --- Get the current process times.
   -- @function times
   -- @tparam[opt] table|string key one of 'utime', 'stime', 'cutime',
   --    'cstime' or 'elapsed'
   -- @string[opt] ... unless *key* was a table, zero or more additional
   --    key strings.
   -- @return values, or a table of all fields if no keys given
   -- @see times(2)
   -- @usage for a,b in pairs(posix.times()) do print(a, b) end
   -- @usage print(posix.times('utime', 'elapsed')
   times = Ptimes,

   --- Return information about this machine.
   -- @function uname
   -- @see uname(2)
   -- @string[opt='%s %n %r %v %m'] format contains zero or more of:
   --
   -- * %m   machine name
   -- * %n   node name
   -- * %r   release
   -- * %s   sys name
   -- * %v   version
   --
   -- @treturn[1] string filled *format* string, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   uname = argscheck('uname(?string)', function(spec)
      local u = uname()
      return gsub(optstring('uname', 1, spec, '%s %n %r %v %m'), '%%(.)', function(s)
         if s == '%' then
            return '%'
         elseif s == 'm' then
            return u.machine
         elseif s == 'n' then
            return u.nodename
         elseif s == 'r' then
            return u.release
         elseif s == 's' then
            return u.sysname
         elseif s == 'v' then
            return u.version
         else
            badoption('uname', 1, 'format', s)
         end
      end)
   end),
}


--- Group information.
-- @table group
-- @string name name of group
-- @int gid unique group id
-- @string ... list of group members
