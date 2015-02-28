--[[
 POSIX library for Lua 5.1, 5.2 & 5.3.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2014-2015
]]
--[[--
 Legacy Lua POSIX bindings.

 Undocumented Legacy APIs for compatibility with previous releases.

 @module posix.deprecated
]]

local _argcheck = require "posix._argcheck"
local bit       = require "bit32"

-- Lua 5.3 has table.unpack but not _G.unpack
-- Lua 5.2 has table.unpack and _G.unpack
-- Lua 5.1 has _G.unpack but not table.unpack
local unpack = table.unpack or unpack

local argerror, argtypeerror, badoption =
  _argcheck.argerror, _argcheck.argtypeerror, _argcheck.badoption
local band, bnot, bor = bit.band, bit.bnot, bit.bor
local checkint, checkselection, checkstring, checktable =
  _argcheck.checkint, _argcheck.checkselection, _argcheck.checkstring, _argcheck.checktable
local optint, optstring, opttable =
  _argcheck.optint, _argcheck.optstring, _argcheck.opttable
local toomanyargerror = _argcheck.toomanyargerror


-- Convert a legacy API tm table to a posix.time.PosixTm compatible table.
local function PosixTm (legacytm)
  return {
    tm_sec   = legacytm.sec,
    tm_min   = legacytm.min,
    tm_hour  = legacytm.hour,
    -- For compatibility with Lua os.date() use "day" if "monthday" is
    -- not set.
    tm_mday  = legacytm.monthday or legacytm.day,
    tm_mon   = legacytm.month - 1,
    tm_year  = legacytm.year - 1900,
    tm_wday  = legacytm.weekday,
    tm_yday  = legacytm.yearday,
    tm_isdst = legacytm.is_dst and 1 or 0,
  }
end

-- Convert a posix.time.PosixTm into a legacy API tm table.
local function LegacyTm (posixtm)
  return {
    sec      = posixtm.tm_sec,
    min      = posixtm.tm_min,
    hour     = posixtm.tm_hour,
    monthday = posixtm.tm_mday,
    day      = posixtm.tm_mday,
    month    = posixtm.tm_mon + 1,
    year     = posixtm.tm_year + 1900,
    weekday  = posixtm.tm_wday,
    yearday  = posixtm.tm_yday,
    is_dst   = posixtm.tm_isdst ~= 0,
  }
end


local function doselection (name, argoffset, fields, map)
  if #fields == 1 and type (fields[1]) == "table" then fields = fields[1] end

  if not (next (fields)) then
    return map
  else
    local r = {}
    for i, v in ipairs (fields) do
      if map[v] then
        r[#r + 1] = map[v]
      else
        argerror (name, i + argoffset, "invalid option '" .. v .. "'", 2)
      end
    end
    return unpack (r)
  end
end


local st = require "posix.sys.stat"

local S_IRUSR, S_IWUSR, S_IXUSR = st.S_IRUSR, st.S_IWUSR, st.S_IXUSR
local S_IRGRP, S_IWGRP, S_IXGRP = st.S_IRGRP, st.S_IWGRP, st.S_IXGRP
local S_IROTH, S_IWOTH, S_IXOTH = st.S_IROTH, st.S_IWOTH, st.S_IXOTH
local S_ISUID, S_ISGID, S_IRWXU, S_IRWXG, S_IRWXO =
  st.S_ISUID, st.S_ISGID, st.S_IRWXU, st.S_IRWXG, st.S_IRWXO

local mode_map = {
  { c = "r", b = S_IRUSR }, { c = "w", b = S_IWUSR }, { c = "x", b = S_IXUSR },
  { c = "r", b = S_IRGRP }, { c = "w", b = S_IWGRP }, { c = "x", b = S_IXGRP },
  { c = "r", b = S_IROTH }, { c = "w", b = S_IWOTH }, { c = "x", b = S_IXOTH },
}

local function pushmode (mode)
  local m = {}
  for i = 1, 9 do
    if band (mode, mode_map[i].b) ~= 0 then m[i] = mode_map[i].c else m[i] = "-" end
  end
  if band (mode, S_ISUID) ~= 0 then
    if band (mode, S_IXUSR) ~= 0 then m[3] = "s" else m[3] = "S" end
  end
  if band (mode, S_ISGID) ~= 0 then
    if band (mode, S_IXGRP) ~= 0 then m[6] = "s" else m[6] = "S" end
  end
  return table.concat (m)
end


local M = {}


--- Bind an address to a socket.
-- @function bind
-- @int fd socket descriptor to act on
-- @tparam PosixSockaddr addr socket address
-- @treturn[1] bool `true`, if successful
-- @return[2] nil
-- @treturn[2] string error messag
-- @treturn[2] int errnum
-- @see bind(2)

local sock = require "posix.sys.socket"

local bind = sock.bind

function M.bind (...)
  local rt = { bind (...) }
  if rt[1] == 0 then return true end
  return unpack (rt)
end


--- Find the precision of a clock.
-- @function clock_getres
-- @string[opt="realtime"] name name of clock, one of "monotonic",
--   "process\_cputime\_id", "realtime", or "thread\_cputime\_id"
-- @treturn[1] int seconds
-- @treturn[21 int nanoseconds, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @treturn[2] int errnum
-- @see clock_getres(3)

local tm = require "posix.time"

local _clock_getres = tm.clock_getres

local function get_clk_id_const (name)
  local map = {
    monotonic = tm.CLOCK_MONOTONIC,
    process_cputime_id = tm.CLOCK_PROCESS_TIME_ID,
    thread_cputime_id = tm.CLOCK_THREAD_CPUTIME_ID,
  }
  return map[name] or tm.CLOCK_REALTIME
end

local function clock_getres (name)
  local ts = _clock_getres (get_clk_id_const (name))
  return ts.tv_sec, ts.tv_nsec
end

if _clock_getres == nil then
  -- Not supported by underlying system
elseif _DEBUG ~= false then
  M.clock_getres = function (...)
    local argt = {...}
    optstring ("clock_getres", 1, argt[1], "realtime")
    if #argt > 1 then toomanyargerror ("clock_getres", 1, #argt) end
    return clock_getres (...)
  end
else
  M.clock_getres = clock_getres
end


--- Read a clock
-- @function clock_gettime
-- @string[opt="realtime"] name name of clock, one of "monotonic",
--   "process\_cputime\_id", "realtime", or "thread\_cputime\_id"
-- @treturn[1] int seconds
-- @treturn[21 int nanoseconds, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @treturn[2] int errnum
-- @see clock_gettime(3)

local tm = require "posix.time"

local _clock_gettime = tm.clock_gettime

local function clock_gettime (name)
  local ts = _clock_gettime (get_clk_id_const (name))
  return ts.tv_sec, ts.tv_nsec
end

if _clock_gettime == nil then
  -- Not supported by underlying system
elseif _DEBUG ~= false then
  M.clock_gettime = function (...)
    local argt = {...}
    optstring ("clock_gettime", 1, argt[1], "realtime")
    if #argt > 1 then toomanyargerror ("clock_gettime", 1, #argt) end
    return clock_gettime (...)
  end
else
  M.clock_gettime = clock_gettime
end


--- Initiate a connection on a socket.
-- @function connect
-- @int fd socket descriptor to act on
-- @tparam PosixSockaddr addr socket address
-- @treturn[1] bool `true`, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @treturn[2] int errnum
-- @see connect(2)

local sock = require "posix.sys.socket"

local connect = sock.connect

function M.connect (...)
  local rt = { connect (...) }
  if rt[1] == 0 then return true end
  return unpack (rt)
end


--- Execute a program without using the shell.
-- @function exec
-- @string path
-- @tparam[opt] table|strings ... table or tuple of arguments (table can include index 0)
-- @return nil
-- @treturn string error message
-- @see execve(2)

local unistd = require "posix.unistd"

local _exec = unistd.exec

local function exec (path, ...)
  local argt = {...}
  if #argt == 1 and type (argt[1]) == "table" then
    argt = argt[1]
  end
  return _exec (path, argt)
end

if _DEBUG ~= false then
  M.exec = function (...)
    local argt = {...}
    checkstring ("exec", 1, argt[1])
    if type (argt[2]) ~= "table" and type (argt[2]) ~= "string" and type (argt[2]) ~= "nil" then
      argtypeerror ("exec", 2, "string, table or nil", argt[2])
    end
    if #argt > 2 then
      if type (argt[2]) == "table" then
        toomanyargerror ("exec", 2, #argt)
      else
        for i = 3, #argt do
	  checkstring ("exec", i, argt[i])
	end
      end
    end
    return exec (...)
  end
else
  M.exec = exec
end


--- Execute a program with the shell.
-- @function execp
-- @string path
-- @tparam[opt] table|strings ... table or tuple of arguments (table can include index 0)
-- @return nil
-- @treturn string error message
-- @see execve(2)

local unistd = require "posix.unistd"

local _execp = unistd.execp

local function execp (path, ...)
  local argt = {...}
  if #argt == 1 and type (argt[1]) == "table" then
    argt = argt[1]
  end
  return _execp (path, argt)
end

if _DEBUG ~= false then
  M.execp = function (...)
    local argt = {...}
    checkstring ("execp", 1, argt[1])
    if type (argt[2]) ~= "table" and type (argt[2]) ~= "string" and type (argt[2]) ~= "nil" then
      argtypeerror ("execp", 2, "string, table or nil", argt[2])
    end
    if #argt > 2 then
      if type (argt[2]) == "table" then
        toomanyargerror ("execp", 2, #argt)
      else
        for i = 3, #argt do
	  checkstring ("execp", i, argt[i])
	end
      end
    end
    return execp (...)
  end
else
  M.execp = execp
end


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

local fc    = require "posix.fcntl"
local stdio = require "posix.stdio"

local posix_fadvise = fc.posix_fadvise
local fileno = stdio.fileno

local function fadvise (fh, ...)
  return posix_fadvise (fileno (fh), ...)
end

if posix_fadvise == nil then
  -- Not supported by underlying system
elseif _DEBUG ~= false then
  M.fadvise = function (...)
    argt = {...}
    if io.type (argt[1]) ~= "file" then
      argtypeerror ("fadvise", 1, "FILE*", argt[1])
    end
    checkint ("fadvise", 2, argt[2])
    checkint ("fadvise", 3, argt[3])
    checkint ("fadvise", 4, argt[4])
    if #argt > 4 then toomanyargerror ("fadvise", 4, #argt) end
    return fadvise (...)
  end
else
  M.fadvise = fadvise
end


--- Match a filename against a shell pattern.
-- @function fnmatch
-- @string pat shell pattern
-- @string name filename
-- @return true or false
-- @raise error if fnmatch failed
-- @see posix.fnmatch.fnmatch

local fnm = require "posix.fnmatch"

function M.fnmatch (...)
  local r = fnm.fnmatch (...)
  if r == 0 then
    return true
  elseif r == fnm.FNM_NOMATCH then
    return false
  end
  error "fnmatch failed"
end


--- Group information.
-- @table group
-- @string name name of group
-- @int gid unique group id
-- @string ... list of group members


--- Information about a group.
-- @function getgroup
-- @tparam[opt=current group] int|string group id or group name
-- @treturn group group information
-- @usage
--   print (P.getgroup (P.getgid ()).name)

local grp    = require "posix.grp"
local unistd = require "posix.unistd"

local getgrgid, getgrnam = grp.getgrgid, grp.getgrnam
local getegid = unistd.getegid

local function getgroup (grp)
  if grp == nil then grp = getegid () end

  local g
  if type (grp) == "number" then
    g = getgrgid (grp)
  elseif type (grp) == "string" then
    g = getgrnam (grp)
  else
    argtypeerror ("getgroup", 1, "string, int or nil", grp)
  end

  if g ~= nil then
    return {name=g.gr_name, gid=g.gr_gid, mem=g.gr_mem}
  end
end

if _DEBUG ~= false then
  M.getgroup = function (...)
    local argt = {...}
    if #argt > 1 then toomanyargerror ("getgroup", 1, #argt) end
    return getgroup (...)
  end
else
  M.getgroup = getgroup
end


--- Get the password entry for a user.
-- @function getpasswd
-- @tparam[opt=current user] int|string user name or id
-- @string ... field names, each one of "uid", "name", "gid", "passwd",
--   "dir" or "shell"
-- @return ... values, or a table of all fields if *user* is `nil`
-- @usage for a,b in pairs (P.getpasswd "root") do print (a, b) end
-- @usage print (P.getpasswd ("root", "shell"))

local pwd    = require "posix.pwd"
local unistd = require "posix.unistd"

local getpwnam, getpwuid = pwd.getpwnam, pwd.getpwuid
local geteuid = unistd.geteuid

local function getpasswd (user, ...)
  if user == nil then user = geteuid () end

  local p
  if type (user) == "number" then
    p = getpwuid (user)
  elseif type (user) == "string" then
    p = getpwnam (user)
  else
    argtypeerror ("getpasswd", 1, "string, int or nil", user)
  end

  if p ~= nil then
    return doselection ("getpasswd", 1, {...}, {
      dir    = p.pw_dir,
      gid    = p.pw_gid,
      name   = p.pw_name,
      passwd = p.pw_passwd,
      shell  = p.pw_shell,
      uid    = p.pw_uid,
    })
  end
end

if _DEBUG ~= false then
  M.getpasswd = function (user, ...)
    checkselection ("getpasswd", 2, {...}, 2)
    return getpasswd (user, ...)
  end
else
  M.getpasswd = getpasswd
end


--- Get process identifiers.
-- @function getpid
-- @tparam[opt] table|string type one of "egid", "euid", "gid", "uid",
--   "pgrp", "pid" or "ppid"; or a single list of the same
-- @string[opt] ... unless *type* was a table, zero or more additional
--   type strings
-- @return ... values, or a table of all fields if no option given
-- @usage for a,b in pairs (P.getpid ()) do print (a, b) end
-- @usage print (P.getpid ("uid", "euid"))

local unistd = require "posix.unistd"

local getegid, geteuid, getgid, getuid =
  unistd.getegid, unistd.geteuid, unistd.getgid, unistd.getuid
local _getpid, getpgrp, getppid =
  unistd.getpid, unistd.getpgrp, unistd.getppid

local function getpid (...)
  return doselection ("getpid", 0, {...}, {
    egid = getegid (),
    euid = geteuid (),
    gid  = getgid (),
    uid  = getuid (),
    pgrp = getpgrp (),
    pid  = _getpid (),
    ppid = getppid (),
  })
end

if _DEBUG ~= false then
  M.getpid = function (...)
    checkselection ("getpid", 1, {...}, 2)
    return getpid (...)
  end
else
  M.getpid = getpid
end


--- Get resource limits for this process.
-- @function getrlimit
-- @string resource one of "core", "cpu", "data", "fsize", "nofile",
--   "stack" or "as"
-- @treturn[1] int soft limit
-- @treturn[1] int hard limit, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @treturn[2] int errnum

local resource = require "posix.sys.resource"

local _getrlimit = resource.getrlimit

local rlimit_map = {
  core   = resource.RLIMIT_CORE,
  cpu    = resource.RLIMIT_CPU,
  data   = resource.RLIMIT_DATA,
  fsize  = resource.RLIMIT_FSIZE,
  nofile = resource.RLIMIT_NOFILE,
  stack  = resource.RLIMIT_STACK,
  as     = resource.RLIMIT_AS,
}

local function getrlimit (rcstr)
  local rc = rlimit_map[string.lower (rcstr)]
  if rc == nil then
    argerror("getrlimit", 1, "invalid option '" .. rcstr .. "'")
  end
  local rlim = _getrlimit (rc)
  return rlim.rlim_cur, rlim.rlim_max
end

if _DEBUG ~= false then
  M.getrlimit = function (...)
    local argt = {...}
    checkstring ("getrlimit", 1, argt[1])
    if #argt > 1 then toomanyargerror ("getrlimit", 1, #argt) end
    return getrlimit (...)
  end
else
  M.getrlimit = getrlimit
end


--- Get time of day.
-- @function gettimeofday
-- @treturn timeval time elapsed since *epoch*
-- @see gettimeofday(2)

local systime = require "posix.sys.time"

local gettimeofday = systime.gettimeofday

function M.gettimeofday (...)
  local tv = gettimeofday (...)
  return { sec = tv.tv_sec, usec = tv.tv_usec }
end


--- Convert epoch time value to a broken-down UTC time.
-- Here, broken-down time tables the month field is 1-based not
-- 0-based, and the year field is the full year, not years since
-- 1900.
-- @function gmtime
-- @int[opt=now] t seconds since epoch
-- @treturn table broken-down time

local tm = require "posix.time"

local _gmtime, time = tm.gmtime, tm.time

local function gmtime (epoch)
  return LegacyTm (_gmtime (epoch or time ()))
end

if _DEBUG ~= false then
  M.gmtime = function (...)
    local argt = {...}
    optint ("gmtime", 1, argt[1])
    if #argt > 1 then toomanyargerror ("gmtime", 1, #argt) end
    return gmtime (...)
  end
else
  M.gmtime = gmtime
end


--- Get host id.
-- @function hostid
-- @treturn int unique host identifier

local unistd = require "posix.unistd"

M.hostid = unistd.gethostid


--- Check for any printable character except space.
-- @function isgraph
-- @see isgraph(3)
-- @string character to act on
-- @treturn bool non-`false` if character is in the class

local ctype = require "posix.ctype"

local isgraph = ctype.isgraph

function M.isgraph (...)
  return isgraph (...) ~= 0
end


--- Check for any printable character including space.
-- @function isprint
-- @string character to act on
-- @treturn bool non-`false` if character is in the class
-- @see isprint(3)

local ctype = require "posix.ctype"

local isprint = ctype.isprint

function M.isprint (...)
  return isprint (...) ~= 0
end


--- Convert epoch time value to a broken-down local time.
-- Here, broken-down time tables the month field is 1-based not
-- 0-based, and the year field is the full year, not years since
-- 1900.
-- @function localtime
-- @int[opt=now] t seconds since epoch
-- @treturn table broken-down time

local tm = require "posix.time"

local _localtime, time = tm.localtime, tm.time

local function localtime (epoch)
  return LegacyTm (_localtime (epoch or time ()))
end

if _DEBUG ~= false then
  M.localtime = function (...)
    local argt = {...}
    optint ("localtime", 1, argt[1])
    if #argt > 1 then toomanyargerror ("localtime", 1, #argt) end
    return localtime (...)
  end
else
  M.localtime = localtime
end


--- Convert a broken-down localtime table into an epoch time.
-- @function mktime
-- @tparam tm broken-down localtime table
-- @treturn in seconds since epoch
-- @see mktime(3)
-- @see localtime

local tm = require "posix.time"

local _mktime, localtime, time = tm.mktime, tm.localtime, tm.time

local function mktime (legacytm)
  local posixtm = legacytm and PosixTm (legacytm) or localtime (time ())
  return _mktime (posixtm)
end

if _DEBUG ~= false then
  M.mktime = function (...)
    local argt = {...}
    opttable ("mktime", 1, argt[1])
    if #argt > 1 then toomanyargerror ("mktime", 1, #argt) end
    return mktime (...)
  end
else
  M.mktime = mktime
end


--- Sleep with nanosecond precision.
-- @function nanosleep
-- @int seconds requested sleep time
-- @int nanoseconds requested sleep time
-- @treturn[1] int `0` if requested time has elapsed
-- @return[2] nil
-- @treturn[2] string error message
-- @treturn[2] int errnum
-- @treturn[2] int unslept seconds remaining, if interrupted
-- @treturn[2] int unslept nanoseconds remaining, if interrupted
-- @see nanosleep(2)
-- @see posix.unistd.sleep

local tm = require "posix.time"

local _nanosleep = tm.nanosleep

local function nanosleep (sec, nsec)
  local r, errmsg, errno, timespec = _nanosleep {tv_sec = sec, tv_nsec = nsec}
  if r == 0 then return 0 end
  return r, errmsg, errno, timespec.tv_sec, timespec.tv_nsec
end

if _DEBUG ~= false then
  M.nanosleep = function (...)
    local argt = {...}
    checkint ("nanosleep", 1, argt[1])
    checkint ("nanosleep", 2, argt[2])
    if #argt > 2 then toomanyargerror ("nanosleep", 2, #argt) end
    return nanosleep (...)
  end
else
  M.nanosleep = nanosleep
end


--- Open the system logger.
-- @function openlog
-- @string ident all messages will start with this
-- @string[opt] option any combination of 'c' (directly to system console
--   if an error sending), 'n' (no delay) and 'p' (show PID)
-- @int [opt=`LOG_USER`] facility one of `LOG_AUTH`, `LOG_AUTHORITY`,
--   `LOG_CRON`, `LOG_DAEMON`, `LOG_FTP`, `LOG_KERN`, `LOG_LPR`, `LOG_MAIL`,
--   `LOG_NEWS`, `LOG_SECURITY`, `LOG_SYSLOG`, `LOG_USER`, `LOG_UUCP` or
--   `LOG_LOCAL0` through `LOG_LOCAL7`
-- @see syslog(3)

local bit = require "bit32"
local log = require "posix.syslog"

local bor = bit.bor
local _openlog = log.openlog

local optionmap = {
  [' '] = 0,
  c = log.LOG_CONS,
  n = log.LOG_NDELAY,
  p = log.LOG_PID,
}

local function openlog (ident, optstr, facility)
  local option = 0
  if optstr then
    for i = 1, #optstr do
      local c = optstr:sub (i, i)
      if optionmap[c] == nil then
	badoption ("openlog", 2, "openlog", c)
      end
      option = bor (option, optionmap[c])
    end
  end
  return _openlog (ident, option, facility)
end

if _DEBUG ~= false then
  M.openlog = function (...)
    local argt = {...}
    checkstring ("openlog", 1, argt[1])
    optstring ("openlog", 2, argt[2])
    optint ("openlog", 3, argt[3])
    if #argt > 3 then toomanyargerror ("openlog", 3, #argt) end
    return openlog (...)
  end
else
  M.openlog = openlog
end


--- Get configuration information at runtime.
-- @function pathconf
-- @string[opt="."] path file to act on
-- @tparam[opt] table|string key one of "CHOWN_RESTRICTED", "LINK_MAX",
--   "MAX_CANON", "MAX_INPUT", "NAME_MAX", "NO_TRUNC", "PATH_MAX", "PIPE_BUF"
--   or "VDISABLE"
-- @string[opt] ... unless *type* was a table, zero or more additional
--   type strings
-- @return ... values, or a table of all fields if no option given
-- @see sysconf(2)
-- @usage for a,b in pairs (P.pathconf "/dev/tty") do print (a, b) end

local unistd = require "posix.unistd"

local _pathconf = unistd.pathconf

local Spathconf = { CHOWN_RESTRICTED = 1, LINK_MAX = 1, MAX_CANON = 1,
  MAX_INPUT = 1, NAME_MAX = 1, NO_TRUNC = 1, PATH_MAX = 1, PIPE_BUF = 1,
  VDISABLE = 1 }

local function pathconf (path, ...)
  local argt, map = {...}, {}
  if path ~= nil and Spathconf[path] ~= nil then
    path, argt = ".", {path, ...}
  end
  for k in pairs (Spathconf) do
    map[k] = _pathconf (path or ".", unistd["_PC_" .. k])
  end
  return doselection ("pathconf", 1, {...}, map)
end

if _DEBUG ~= false then
  M.pathconf = function (path, ...)
    if path ~= nil and Spathconf[path] ~= nil then
      checkselection ("pathconf", 1, {path, ...}, 2)
    else
      optstring ("pathconf", 1, path, ".", 2)
      checkselection ("pathconf", 2, {...}, 2)
    end
    return pathconf (path, ...)
  end
else
  M.pathconf = pathconf
end


--- Set resource limits for this process.
-- @function setrlimit
-- @string resource one of "core", "cpu", "data", "fsize", "nofile",
--   "stack" or "as"
-- @int[opt] softlimit process may receive a signal when reached
-- @int[opt] hardlimit process may be terminated when reached
-- @treturn[1] int `0`, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @treturn[2] int errnum

local resource = require "posix.sys.resource"

local _setrlimit = resource.setrlimit

local rlimit_map = {
  core   = resource.RLIMIT_CORE,
  cpu    = resource.RLIMIT_CPU,
  data   = resource.RLIMIT_DATA,
  fsize  = resource.RLIMIT_FSIZE,
  nofile = resource.RLIMIT_NOFILE,
  stack  = resource.RLIMIT_STACK,
  as     = resource.RLIMIT_AS,
}

local function setrlimit (rcstr, cur, max)
  local rc = rlimit_map[string.lower (rcstr)]
  if rc == nil then
    argerror("setrlimit", 1, "invalid option '" .. rcstr .. "'")
  end
  local lim
  if cur == nil or max == nil then
    lim= _getrlimit (rc)
  end
  return _setrlimit (rc, {
    rlim_cur = cur or lim.rlim_cur,
    rlim_max = max or lim.rlim_max,
  })
end

if _DEBUG ~= false then
  M.setrlimit = function (...)
    local argt = {...}
    checkstring ("setrlimit", 1, argt[1])
    optint ("setrlimit", 2, argt[2])
    optint ("setrlimit", 3, argt[3])
    if #argt > 3 then toomanyargerror ("setrlimit", 3, #argt) end
    return setrlimit (...)
  end
else
  M.getrlimit = getrlimit
end


--- Information about an existing file path.
-- If the file is a symbolic link, return information about the link
-- itself.
-- @function stat
-- @string path file to act on
-- @tparam[opt] table|string field one of "dev", "ino", "mode", "nlink",
--   "uid", "gid", "rdev", "size", "atime", "mtime", "ctime" or "type"
-- @string[opt] ... unless *field* was a table, zero or more additional
--   field names
-- @return values, or table of all fields if no option given
-- @see stat(2)
-- @usage for a,b in pairs (P,stat "/etc/") do print (a, b) end

local st = require "posix.sys.stat"

local S_ISREG, S_ISLNK, S_ISDIR, S_ISCHR, S_ISBLK, S_ISFIFO, S_ISSOCK =
  st.S_ISREG, st.S_ISLNK, st.S_ISDIR, st.S_ISCHR, st.S_ISBLK, st.S_ISFIFO, st.S_ISSOCK

local function filetype (mode)
  if S_ISREG (mode) ~= 0 then
    return "regular"
  elseif S_ISLNK (mode) ~= 0 then
    return "link"
  elseif S_ISDIR (mode) ~= 0 then
    return "directory"
  elseif S_ISCHR (mode) ~= 0 then
    return "character device"
  elseif S_ISBLK (mode) ~= 0 then
    return "block device"
  elseif S_ISFIFO (mode) ~= 0 then
    return "fifo"
  elseif S_ISSOCK (mode) ~= 0 then
    return "socket"
  else
    return "?"
  end
end


local _stat = st.lstat  -- for bugwards compatibility with v<=32

local function stat (path, ...)
  local info = _stat (path)
  if info ~= nil then
    return doselection ("stat", 1, {...}, {
      dev   = info.st_dev,
      ino   = info.st_ino,
      mode  = pushmode (info.st_mode),
      nlink = info.st_nlink,
      uid   = info.st_uid,
      gid   = info.st_gid,
      size  = info.st_size,
      atime = info.st_atime,
      mtime = info.st_mtime,
      ctime = info.st_ctime,
      type  = filetype (info.st_mode),
    })
  end
end

if _DEBUG ~= false then
  M.stat = function (path, ...)
    checkstring ("stat", 1, path, 2)
    checkselection ("stat", 2, {...}, 2)
    return stat (path, ...)
  end
else
  M.stat = stat
end


--- Fetch file system statistics.
-- @function statvfs
-- @string path any path within the mounted file system
-- @tparam[opt] table|string field one of "bsize", "frsize", "blocks",
--   "bfree", "bavail", "files", "ffree", "favail", "fsid", "flag",
--   "namemax"
-- @string[opt] ... unless *field* was a table, zero or more additional
--   field names
-- @return values, or table of all fields if no option given
-- @see statvfs(2)
-- @usage for a,b in pairs (P,statvfs "/") do print (a, b) end

local sv = require "posix.sys.statvfs"

local _statvfs = sv.statvfs

local function statvfs (path, ...)
  local info = _statvfs (path)
  if info ~= nil then
    return doselection ("statvfs", 1, {...}, {
      bsize   = info.f_bsize,
      frsize  = info.f_frsize,
      blocks  = info.f_blocks,
      bfree   = info.f_bfree,
      bavail  = info.f_bavail,
      files   = info.f_files,
      ffree   = info.f_ffree,
      favail  = info.f_favail,
      fsid    = info.f_fsid,
      flag    = info.f_flag,
      namemax = info.f_namemax,
    })
  end
end

if _DEBUG ~= false then
  M.statvfs = function (path, ...)
    checkstring ("statvfs", 1, path, 2)
    checkselection ("statvfs", 2, {...}, 2)
    return statvfs (path, ...)
  end
else
  M.statvfs = statvfs
end


--- Write a time out according to a format.
-- @function strftime
-- @string format specifier with `%` place-holders
-- @tparam PosixTm tm broken-down local time
-- @treturn string *format* with place-holders plugged with *tm* values
-- @see strftime(3)

local tm = require "posix.time"

local _strftime, localtime, time = tm.strftime, tm.localtime, tm.time

local function strftime (fmt, legacytm)
  local posixtm = legacytm and PosixTm (legacytm) or localtime (time ())
  return _strftime (fmt, posixtm)
end

if _DEBUG ~= false then
  M.strftime = function (...)
    local argt = {...}
    checkstring ("strftime", 1, argt[1])
    opttable ("strftime", 2, argt[2])
    if #argt > 2 then toomanyargerror ("strftime", 2, #argt) end
    return strftime (...)
  end
else
  M.strftime = strftime
end


--- Parse a date string.
-- @function strptime
-- @string s
-- @string format same as for `strftime`
-- @usage posix.strptime('20','%d').monthday == 20
-- @treturn[1] PosixTm broken-down local time
-- @treturn[1] int next index of first character not parsed as part of the date
-- @return[2] nil
-- @see strptime(3)

local tm = require "posix.time"

local _strptime = tm.strptime

local function strptime (s, fmt)
  return _strptime (s, fmt)
end

if _DEBUG ~= false then
  M.strptime = function (...)
    local argt = {...}
    checkstring ("strptime", 1, argt[1])
    checkstring ("strptime", 2, argt[2])
    if #argt > 2 then toomanyargerror ("strptime", 2, #argt) end
    local tm, i = strptime (...)
    return LegacyTm (tm), i
  end
else
  M.strptime = strptime
end


--- Get configuration information at runtime.
-- @function sysconf
-- @tparam[opt] table|string key one of "ARG_MAX", "CHILD_MAX",
--   "CLK_TCK", "JOB_CONTROL", "NGROUPS_MAX", "OPEN_MAX", "SAVED_IDS",
--   "STREAM_MAX", "TZNAME_MAX" or "VERSION"
-- @string[opt] ... unless *type* was a table, zero or more additional
--   type strings
-- @return ... values, or a table of all fields if no option given
-- @see sysconf(2)
-- @usage for a,b in pairs (P.sysconf ()) do print (a, b) end
-- @usage print (P.sysconf ("STREAM_MAX", "ARG_MAX"))

local unistd = require "posix.unistd"

local _sysconf = unistd.sysconf

local function sysconf (...)
  return doselection ("sysconf", 0, {...}, {
    ARG_MAX     = _sysconf (unistd._SC_ARG_MAX),
    CHILD_MAX   = _sysconf (unistd._SC_CHILD_MAX),
    CLK_TCK     = _sysconf (unistd._SC_CLK_TCK),
    JOB_CONTROL = _sysconf (unistd._SC_JOB_CONTROL),
    NGROUPS_MAX = _sysconf (unistd._SC_NGROUPS_MAX),
    OPEN_MAX    = _sysconf (unistd._SC_OPEN_MAX),
    SAVED_IDS   = _sysconf (unistd._SC_SAVED_IDS),
    STREAM_MAX  = _sysconf (unistd._SC_STREAM_MAX),
    TZNAME_MAX  = _sysconf (unistd._SC_TZNAME_MAX),
    VERSION     = _sysconf (unistd._SC_VERSION),
  })
end

if _DEBUG ~= false then
  M.sysconf = function (...)
    checkselection ("sysconf", 1, {...}, 2)
    return sysconf (...)
  end
else
  M.sysconf = sysconf
end


--- Get the current process times.
-- @function times
-- @tparam[opt] table|string key one of "utime", "stime", "cutime",
--   "cstime" or "elapsed"
-- @string[opt] ... unless *key* was a table, zero or more additional
--   key strings.
-- @return values, or a table of all fields if no keys given
-- @see times(2)
-- @usage for a,b in pairs(P.times ()) do print (a, b) end
-- @usage print (P.times ("utime", "elapsed")

local tms = require "posix.sys.times"

local _times = tms.times

local function times (...)
  local info = _times ()
  return doselection ("times", 0, {...}, {
    utime   = info.tms_utime,
    stime   = info.tms_stime,
    cutime  = info.tms_cutime,
    cstime  = info.tms_cstime,
    elapsed = info.elapsed,
  })
end

if _DEBUG ~= false then
  M.times = function (...)
    checkselection ("times", 1, {...}, 2)
    return times (...)
  end
else
  M.times = times
end


--- Return information about this machine.
-- @function uname
-- @see uname(2)
-- @string[opt="%s %n %r %v %m"] format contains zero or more of:
--
-- * %m  machine name
-- * %n  node name
-- * %r  release
-- * %s  sys name
-- * %v  version
--
--@treturn[1] string filled *format* string, if successful
--@return[2] nil
--@treturn string error message

local utsname = require "posix.sys.utsname"

local _uname = utsname.uname

local function uname (spec)
  local u = _uname ()
  return optstring ("uname", 1, spec, "%s %n %r %v %m"):gsub ("%%(.)", function (s)
    if s == "%" then return "%"
    elseif s == "m" then return u.machine
    elseif s == "n" then return u.nodename
    elseif s == "r" then return u.release
    elseif s == "s" then return u.sysname
    elseif s == "v" then return u.version
    else
      badoption ("uname", 1, "format", s)
    end
  end)
end

if _DEBUG ~= false then
  M.uname = function (s, ...)
    local argt = {s, ...}
    if #argt > 1 then
      toomanyargerror ("uname", 1, #argt)
    end
    return uname (s)
  end
else
  M.uname = uname
end


return M
