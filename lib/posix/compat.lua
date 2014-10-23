--[[
 POSIX library for Lua 5.1/5.2.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2014
 (c) Reuben Thomas <rrt@sc3d.org> 2010-2013
 (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
]]
--[[--
 Legacy Lua POSIX bindings.

 Undocumented Legacy APIs for compatibility with previous releases.

 @module posix.compat
]]

local bit = bit32 or require "bit"
local band, bnot, bor = bit.band, bit.bnot, bit.bor


local function argerror (name, i, extramsg, level)
  level = level or 1
  local s = string.format ("bad argument #%d to '%s'", i, name)
  if extramsg ~= nil then
    s = s .. " (" .. extramsg .. ")"
  end
  error (s, level + 1)
end

local function toomanyargerror (name, expected, got, level)
  level = level or 1
  local fmt = "no more than %d argument%s expected, got %d"
  argerror (name, expected + 1,
            fmt:format (expected, expected > 1 and "s" or "", got), level + 1)
end

local function argtypeerror (name, i, expect, actual, level)
  level = level or 1
  local fmt = "%s expected, got %s"
  argerror (name, i,
            fmt:format (expect, type (actual):gsub ("nil", "no value")), level + 1)
end

local function badoption (name, i, what, option, level)
  level = level or 1
  local fmt = "invalid %s option '%s'"
  argerror (name, i, fmt:format (what, option), level + 1)
end

local function checkint (name, i, actual, level)
  level = level or 1
  if type (actual) ~= "number" then
    argtypeerror (name, i, "int", actual, level + 1)
  end
  return actual
end

local function checkstring (name, i, actual, level)
  level = level or 1
  if type (actual) ~= "string" then
    argtypeerror (name, i, "string", actual, level + 1)
  end
  return actual
end

local function checktable (name, i, actual, level)
  level = level or 1
  if type (actual) ~= "table" then
    argtypeerror (name, i, "table", actual, level + 1)
  end
  return actual
end

local function checkselection (fname, argi, fields, level)
  level = level or 1
  local field1, type1 = fields[1], type (fields[1])
  if type1 == "table" and #fields > 1 then
    toomanyargerror (fname, argi, #fields + argi - 1, level + 1)
  elseif field1 ~= nil and type1 ~= "table" and type1 ~= "string" then
    argtypeerror (fname, argi, "table, string or nil", field1, level + 1)
  end
  for i = 2, #fields do
    checkstring (fname, i + argi - 1, fields[i], level + 1)
  end
end

local function optstring (name, i, actual, def, level)
  level = level or 1
  if actual ~= nil and type (actual) ~= "string" then
    argtypeerror (name, i, "string or nil", actual, level + 1)
  end
  return actual or def
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

local function rwxrwxrwx (modestr)
  local mode = 0
  for i = 1, 9 do
    if modestr:sub (i, i) == mode_map[i].c then
      mode = bor (mode, mode_map[i].b)
    elseif modestr:sub (i, i) == "s" then
      if i == 3 then
        mode = bor (mode, S_ISUID, S_IXUSR)
      elseif i == 6 then
        mode = bor (mode, S_ISGID, S_IXGRP)
      else
	return nil  -- bad mode
      end
    end
  end
  return mode
end

local function octal_mode (modestr)
  local mode = 0
  for i = 1, #modestr do
    mode = mode * 8 + tonumber (modestr:sub (i, i))
  end
  return mode
end

local function mode_munch (mode, modestr)
  if #modestr == 9 and modestr:match "^[-rswx]+$" then
    return rwxrwxrwx (modestr)
  elseif modestr:match "^[0-7]+$" then
    return octal_mode (modestr)
  elseif modestr:match "^[ugoa]+%s*[-+=]%s*[rswx]+,*" then
    modestr:gsub ("%s*(%a+)%s*(.)%s*(%a+),*", function (who, op, what)
      local bits, bobs = 0, 0
      if who:match "[ua]" then bits = bor (bits, S_ISUID, S_IRWXU) end
      if who:match "[ga]" then bits = bor (bits, S_ISGID, S_IRWXG) end
      if who:match "[oa]" then bits = bor (bits, S_IRWXO) end
      if what:match "r" then bobs = bor (bobs, S_IRUSR, S_IRGRP, S_IROTH) end
      if what:match "w" then bobs = bor (bobs, S_IWUSR, S_IWGRP, S_IWOTH) end
      if what:match "x" then bobs = bor (bobs, S_IXUSR, S_IXGRP, S_IXOTH) end
      if what:match "s" then bobs = bor (bobs, S_ISUID, S_ISGID) end
      if op == "+" then
	-- mode |= bits & bobs
	mode = bor (mode, band (bits, bobs))
      elseif op == "-" then
	-- mode &= ~(bits & bobs)
	mode = band (mode, bnot (band (bits, bobs)))
      elseif op == "=" then
	-- mode = (mode & ~bits) | (bits & bobs)
	mode = bor (band (mode, bnot (bits)), band (bits, bobs))
      end
    end)
    return mode
  else
    return nil, "bad mode"
  end
end


local M = {
  argerror        = argerror,
  argtypeerror    = argtypeerror,
  badoption       = badoption,
  checkstring     = checkstring,
  checktable      = checktable,
  optstring       = optstring,
  toomanyargerror = toomanyargerror,
}



--- Change the mode of the path.
-- @function chmod
-- @string path existing file path
-- @string mode one of the following formats:
--
--   * "rwxrwxrwx" (e.g. "rw-rw-r--")
--   * "ugo+-=rwx" (e.g. "u+w")
--   * +-=rwx" (e.g. "+w")
--
-- @return[1] int `0`, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @see chmod(2)
-- @usage P.chmod ('bin/dof', '+x')

local bit   = bit32 or require "bit"
local st    = require "posix.sys.stat"

local _chmod, stat = st.chmod, st.stat
local RWXALL = bit.bor (st.S_IRWXU, st.S_IRWXG, st.S_IRWXO)

local function chmod (path, modestr)
  local mode = (stat (path) or {}).st_mode
  local bits, err = mode_munch (mode or 0, modestr)
  if bits == nil then
    argerror ("chmod", 2, err, 2)
  end
  return _chmod (path, band (bits, RWXALL))
end

if _DEBUG ~= false then
  M.chmod = function (...)
    local argt = {...}
    checkstring ("chmod", 1, argt[1])
    checkstring ("chmod", 2, argt[2])
    if #argt > 2 then toomanyargerror ("chmod", 2, #argt) end
    return chmod (...)
  end
else
  M.chmod = chmod
end


--- Create a file.
-- This function is obsoleted by @{posix.fcntl.open} with `posix.O_CREAT`.
-- @function creat
-- @string path name of file to create
-- @string mode permissions with which to create file
-- @treturn[1] int file descriptor of file at *path*, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @see creat(2)
-- @see posix.chmod
-- @usage
--   fd = P.creat ("data", "rw-r-----")

local bit   = bit32 or require "bit"
local fcntl = require "posix.fcntl"
local st    = require "posix.sys.stat"

local band, bor   = bit.band, bit.bor
local creat_flags = bor (fcntl.O_CREAT, fcntl.O_WRONLY, fcntl.O_TRUNC)
local RWXALL      = bor (st.S_IRWXU, st.S_IRWXG, st.S_IRWXO)

local open = fcntl.open

local function creat (path, modestr)
  local mode, err = mode_munch (0, modestr)
  if mode == nil then
    argerror ("creat", 2, err, 2)
  end
  return open (path, creat_flags, band (mode, RWXALL))
end

if _DEBUG ~= false then
  M.creat = function (...)
    local argt = {...}
    checkstring ("creat", 1, argt[1])
    checkstring ("creat", 2, argt[2])
    if #argt > 2 then toomanyargerror ("creat", 2, #argt) end
    return creat (...)
  end
else
  M.creat = creat
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


--- Get host id.
-- @function hostid
-- @treturn int unique host identifier

local unistd = require "posix.unistd"

M.hostid = unistd.gethostid


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


--- Make a directory.
-- @function mkdir
-- @string path location in file system to create directory
-- @rteturn[1] int `0`, if successful
-- @return[2] nil
-- @treturn[2] string error message

local bit = bit32 or require "bit"
local st  = require "posix.sys.stat"

local _mkdir = st.mkdir
local RWXALL = bit.bor (st.S_IRWXU, st.S_IRWXG, st.S_IRWXO)

local function mkdir (path)
  return _mkdir (path, RWXALL)
end

if _DEBUG ~= false then
  M.mkdir = function (...)
    local argt = {...}
    checkstring ("mkdir", 1, argt[1])
    if #argt > 1 then toomanyargerror ("mkdir", 1, #argt) end
    return mkdir (...)
  end
else
  M.mkdir = _mkdir
end


--- Make a FIFO pipe.
-- @function mkfifo
-- @string path location in file system to create fifo
-- @rteturn[1] int `0`, if successful
-- @return[2] nil
-- @treturn[2] string error message

local bit = bit32 or require "bit"
local st  = require "posix.sys.stat"

local _mkfifo = st.mkfifo
local RWXALL  = bit.bor (st.S_IRWXU, st.S_IRWXG, st.S_IRWXO)

local function mkfifo (path)
  return _mkfifo (path, RWXALL)
end

if _DEBUG ~= false then
  M.mkfifo = function (...)
    local argt = {...}
    checkstring ("mkfifo", 1, argt[1])
    if #argt > 1 then toomanyargerror ("mkfifo", 1, #argt) end
    return mkfifo (...)
  end
else
  M.mkfifo = mkfifo
end


--- Get a message queue identifier
-- @function msgget
-- @int key message queue id, or `IPC_PRIVATE` for a new queue
-- @int[opt=0] flags bitwise OR of zero or more from `IPC_CREAT` and `IPC_EXCL`
-- @string[opt="rw-rw-rw-"] mode execute bits are ignored
-- @treturn[1] int message queue identifier, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @see msgget(2)

local bit   = bit32 or require "bit"
local msg   = require "posix.sys.msg"
local st    = require "posix.sys.stat"

local _msgget   = msg.msgget
local band, bor = bit.band, bit.bor
local RWXALL    = bor (st.S_IRWXU, st.S_IRWXG, st.S_IRWXO)

local function msgget (key, msgflg, modestr)
  local mode, err = mode_munch (0, modestr)
  if mode == nil then
    argerror ("open", 3, err, 2)
  end
  return _msgget (key, bor (msgflg, band (mode, RWXALL)))
end

if not _msgget then
  -- Not supported by underlying system
elseif _DEBUG ~= false then
  M.msgget = function (...)
    local argt = {...}
    checkint ("msgget", 1, argt[1])
    if argt[2] ~= nil and type (argt[2]) ~= "number" then
      argtypeerror ("msgget", 2, "int or nil", argt[2])
    end
    if argt[3] ~= nil and type (argt[3]) ~= "string" then
      argtypeerror ("msgget", 3, "string or nil", argt[3])
    end
    if #argt > 3 then toomanyargerror ("msgget", 3, #argt) end
    return msgget (...)
  end
else
  M.msgget = msgget
end


--- Open a file.
-- @function open
-- @string path file to act on
-- @int oflags bitwise OR of zero or more of `O_RDONLY`, `O_WRONLY`, `O_RDWR`,
--   `O_APPEND`, `O_CREAT`, `O_DSYNC`, `O_EXCL`, `O_NOCTTY`, `O_NONBLOCK`,
--   `O_RSYNC`, `O_SYNC`, `O_TRUNC`
-- @string modestr (used with `O_CREAT`; see @{chmod} for format)
-- @treturn[1] int file descriptor for *path*, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @see open(2)
-- @usage
-- fd = P.open ("data", bit.bor (P.O_CREAT, P.O_RDWR), "rw-r-----")

local bit   = bit32 or require "bit"
local fcntl = require "posix.fcntl"

local _open, O_CREAT = fcntl.open, fcntl.O_CREAT
local band = bit.band

local function open (path, oflags, modestr)
  local mode
  if band (oflags, O_CREAT) ~= 0 then
    mode, err = mode_munch (0, modestr)
    if mode == nil then
      argerror ("open", 3, err, 2)
    end
    mode = band (mode, RWXALL)
  end
  return _open (path, oflags, mode)
end

if _DEBUG ~= false then
  M.open = function (...)
    local argt, maxt = {...}, 2
    checkstring ("open", 1, argt[1])
    local oflags = checkint ("open", 2, argt[2])
    if band (oflags, O_CREAT) ~= 0 then
      checkstring ("open", 3, argt[3])
      maxt = 3
    end
    if #argt > maxt then toomanyargerror ("open", maxt, #argt) end
    return open (...)
  end
else
  M.creat = creat
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


local _stat = st.lstat

local function stat (path, ...)
  local info = _stat (path)
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

if _DEBUG ~= false then
  M.stat = function (path, ...)
    checkstring ("stat", 1, path, 2)
    checkselection ("stat", 2, {...}, 2)
    return stat (path, ...)
  end
else
  M.lstat = lstat
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

if _DEBUG ~= false then
  M.statvfs = function (path, ...)
    checkstring ("statvfs", 1, path, 2)
    checkselection ("statvfs", 2, {...}, 2)
    return statvfs (path, ...)
  end
else
  M.statvfs = statvfs
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


--- Set file mode creation mask.
-- @function umask
-- @string[opt] mode file creation mask string
-- @treturn string previous umask
-- @see umask(2)

local st = require "posix.sys.stat"

local _umask, RWXALL = st.umask, bor (st.S_IRWXU, st.S_IRWXG, st.S_IRWXO)

local function umask (modestr)
  modestr = modestr or ""
  local mode = _umask (0)
  _umask (mode)
  mode = band (bnot (mode), RWXALL)
  if #modestr > 0 then
    local bits, err = mode_munch (mode, modestr)
    if bits == nil then
      argerror ("umask", 1, err, 2)
    end
    mode = band (bits, RWXALL)
    _umask (bnot (mode))
  end
  return pushmode (mode)
end


if _DEBUG ~= false then
  M.umask = function (modestr, ...)
    local argt = {modestr, ...}
    optstring ("umask", 1, modestr, "")
    if #argt > 1 then
      toomanyargerror ("umask", 1, #argt)
    end
    return umask (modestr)
  end
else
  M.umask = umask
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
