--[[
 POSIX library for Lua 5.1, 5.2 & 5.3.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2014-2015
]]
--[[--
 Legacy Lua POSIX bindings.

 APIs for maintaining compatibility with previous releases.

 @module posix
]]

local _argcheck = require "posix._argcheck"
local bit       = require "bit32"

local argerror, argtypeerror, badoption =
  _argcheck.argerror, _argcheck.argtypeerror, _argcheck.badoption
local band, bnot, bor = bit.band, bit.bnot, bit.bor
local checkint, checkselection, checkstring, checktable =
  _argcheck.checkint, _argcheck.checkselection, _argcheck.checkstring, _argcheck.checktable
local optint, optstring, opttable =
  _argcheck.optint, _argcheck.optstring, _argcheck.opttable
local toomanyargerror = _argcheck.toomanyargerror


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
-- @treturn[2] int errnum
-- @see chmod(2)
-- @usage P.chmod ('bin/dof', '+x')

local bit   = require "bit32"
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
-- @treturn[2] int errnum
-- @see creat(2)
-- @see posix.chmod
-- @usage
--   fd = P.creat ("data", "rw-r-----")

local bit   = require "bit32"
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


--- Make a directory.
-- @function mkdir
-- @string path location in file system to create directory
-- @treturn[1] int `0`, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @treturn[2] int errnum

local bit = require "bit32"
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
  M.mkdir = mkdir
end


--- Make a FIFO pipe.
-- @function mkfifo
-- @string path location in file system to create fifo
-- @treturn[1] int `0`, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @treturn[2] int errnum

local bit = require "bit32"
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
-- @treturn[2] int errnum
-- @see msgget(2)

local bit   = require "bit32"
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
-- @treturn[2] int errnum
-- @see open(2)
-- @usage
-- fd = P.open ("data", bit.bor (P.O_CREAT, P.O_RDWR), "rw-r-----")

local bit   = require "bit32"
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


--- Set log priority mask
-- @function setlogmask
-- @int ... zero or more of `LOG_EMERG`, `LOG_ALERT`, `LOG_CRIT`,
--   `LOG_WARNING`, `LOG_NOTICE`, `LOG_INFO` and `LOG_DEBUG`
-- @treturn[1] int `0`, if successful
-- @return[2] nil
-- @treturn[2] string error message
-- @treturn[2] int errnum

local bit = require "bit32"
local log = require "posix.syslog"

local bor = bit.bor
local _setlogmask, LOG_MASK = log.setlogmask, log.LOG_MASK

local function setlogmask (...)
  local mask = 0
  for _, v in ipairs {...} do
    mask = bor (mask, LOG_MASK (v))
  end
  return _setlogmask (mask)
end

if _DEBUG ~= false then
  M.setlogmask = function (...)
    for i, v in ipairs {...} do
      optint ("setlogmask", i, v, 0) -- for "int or nil" error
    end
    return setlogmask (...)
  end
else
  M.setlogmask = setlogmask
end


--- Set file mode creation mask.
-- @function umask
-- @string[opt] mode file creation mask string
-- @treturn string previous umask
-- @see umask(2)
-- @see posix.sys.stat.umask

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


return M
