--[[
 POSIX library for Lua 5.1, 5.2 & 5.3.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2015
 (c) Reuben Thomas <rrt@sc3d.org> 2010-2015
 (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
]]
--[[--
 Lua POSIX bindings.

 In addition to the convenience functions documented in this module, all
 APIs from submodules are copied into the return table for convenience and
 backwards compatibility.

 @module posix
]]

local bit = require "bit32"
local M = {}


-- For backwards compatibility, copy all table entries into M namespace.
for _, sub in ipairs {
  "ctype", "dirent", "errno", "fcntl", "fnmatch", "getopt", "glob", "grp",
  "libgen", "poll", "pwd", "sched", "signal", "stdio", "stdlib", "sys.msg",
  "sys.resource", "sys.socket", "sys.stat", "sys.statvfs", "sys.time",
  "sys.times", "sys.utsname", "sys.wait", "syslog", "termio", "time",
  "unistd", "utime"
} do
  local t = require ("posix." .. sub)
  for k, v in pairs (t) do
    if k ~= "version" then
      assert(M[k] == nil, "posix namespace clash: " .. sub .. "." .. k)
      M[k] = v
    end
  end
end


-- Inject deprecated APIs (overwriting submodules) for backwards compatibility.
for k, v in pairs (require "posix.deprecated") do
  M[k] = v
end
for k, v in pairs (require "posix.compat") do
  M[k] = v
end

M.version = "posix for " .. _VERSION .. " / luaposix 33.3.1"


local argerror, argtypeerror, checkstring, checktable, toomanyargerror =
  M.argerror, M.argtypeerror, M.checkstring, M.checktable, M.toomanyargerror


-- Code extracted from lua-stdlib with minimal modifications
local list = {
  sub = function (l, from, to)
    local r = {}
    local len = #l
    from = from or 1
    to = to or len
    if from < 0 then
      from = from + len + 1
    end
    if to < 0 then
      to = to + len + 1
    end
    for i = from, to do
      table.insert (r, l[i])
    end
    return r
  end
}
-- end of stdlib code



--- Check permissions like @{posix.unistd.access}, but for euid.
-- Based on the glibc function of the same name. Does not always check
-- for read-only file system, text busy, etc., and does not work with
-- ACLs &c.
-- @function euidaccess
-- @string file file to check
-- @string mode checks to perform (as for access)
-- @return 0 if access allowed; <code>nil</code> otherwise (and errno is set)

local access, set_errno, stat = M.access, M.set_errno, M.stat
local getegid, geteuid, getgid, getuid =
  M.getegid, M.geteuid, M.getgid, M.getuid

local function euidaccess (file, mode)
  local euid, egid = geteuid (), getegid ()

  if getuid () == euid and getgid () == egid then
    -- If we are not set-uid or set-gid, access does the same.
    return access (file, mode)
  end

  local stats = stat (file)
  if not stats then
    return
  end

  -- The super-user can read and write any file, and execute any file
  -- that anyone can execute.
  if euid == 0 and ((not string.match (mode, "x")) or
                    string.match (stats.st_mode, "x")) then
    return 0
  end

  -- Convert to simple list of modes.
  mode = string.gsub (mode, "[^rwx]", "")

  if mode == "" then
    return 0 -- The file exists.
  end

  -- Get the modes we need.
  local granted = stats.st_mode:sub (1, 3)
  if euid == stats.st_uid then
    granted = stats.st_mode:sub (7, 9)
  elseif egid == stats.st_gid or set.new (posix.getgroups ()):member (stats.st_gid) then
    granted = stats.st_mode:sub (4, 6)
  end
  granted = string.gsub (granted, "[^rwx]", "")

  if string.gsub ("[^" .. granted .. "]", mode) == "" then
    return 0
  end
  set_errno (EACCESS)
end

if _DEBUG ~= false then
  M.euidaccess = function (...)
    local argt = {...}
    checkstring ("euidaccess", 1, argt[1])
    checkstring ("euidaccess", 2, argt[2])
    if #argt > 2 then toomanyargerror ("euidaccess", 2, #argt) end
    return euidaccess (...)
  end
else
  M.euidaccess = euidaccess
end


--- Open a pseudo-terminal.
-- Based on the glibc function of the same name.
-- @fixme add support for term and win arguments
-- @treturn[1] int master file descriptor
-- @treturn[1] int slave file descriptor
-- @treturn[1] string slave file name
-- @return[2] nil
-- @treturn[2] string error message

local bit    = require "bit32"
local fcntl  = require "posix.fcntl"
local stdlib = require "posix.stdlib"
local unistd = require "posix.unistd"

local bor = bit.bor
local open, O_RDWR, O_NOCTTY = fcntl.open, fcntl.O_RDWR, fcntl.O_NOCTTY
local grantpt, openpt, ptsname, unlockpt =
  stdlib.grantpt, stdlib.openpt, stdlib.ptsname, stdlib.unlockpt
local close = unistd.close

local function openpty (term, win)
  local ok, errmsg, master, slave, slave_name
  master, errmsg = openpt (bor (O_RDWR, O_NOCTTY))
  if master then
    ok, errmsg = grantpt (master)
    if ok then
      ok, errmsg = unlockpt (master)
      if ok then
	slave_name, errmsg = ptsname (master)
	if slave_name then
          slave, errmsg = open (slave_name, bor (O_RDWR, O_NOCTTY))
	  if slave then
            return master, slave, slave_name
	  end
	end
      end
    end
    close (master)
  end
  return nil, errmsg
end

if _DEBUG ~= false then
  M.openpty = function (...)
    local argt = {...}
    if #argt > 0 then toomanyargerror ("openpty", 0, #argt) end
    return openpty (...)
  end
else
  M.openpty = openpty
end


--- Exec a command or Lua function.
-- @function execx
-- @param task, a table of arguments to `P.execp` or a Lua function, which
--   should read from standard input, write to standard output, and return
--   an exit code
-- @param ... positional arguments to the function
-- @treturn nil on error (normally does not return)
-- @treturn string error message

local unpack = table.unpack or unpack -- 5.3 compatibility

local errno, execp, _exit =
  M.errno, M.execp, M._exit

function execx (task, ...)
  if type (task) == "table" then
    execp (unpack (task))
    -- Only get here if there's an error; kill the fork
    local _, n = errno ()
    _exit (n)
  else
    _exit (task (...) or 0)
  end
end

if _DEBUG ~= false then
  M.execx = function (task, ...)
    local argt, typetask = {task, ...}, type (task)
    if typetask ~= "table" and typetask ~= "function" then
      argtypeerror ("execx", 1, "table or function", task)
    end
    return execx (task, ...)
  end
else
  M.execx = execx
end


--- Run a command or function in a sub-process using `P.execx`.
-- @function spawn
-- @param task, as for `P.execx`.
-- @tparam string ... as for `P.execx`
-- @return values as for `P.wait`

local unpack = table.unpack or unpack -- 5.3 compatibility

local fork, wait =
  M.fork, M.wait

local function spawn (task, ...)
  local pid, err = fork ()
  if pid == nil then
    return pid, err
  elseif pid == 0 then
    execx (task, ...)
  else
    local _, reason, status = wait (pid)
    return status, reason -- If wait failed, status is nil & reason is error
  end
end

if _DEBUG ~= false then
  M.spawn = function (task, ...)
    local argt, typetask = {task, ...}, type (task)
    if typetask ~= "table" and typetask ~= "function" then
      argtypeerror ("spawn", 1, "table or function", task)
    end
    return spawn (task, ...)
  end
else
  M.spawn = spawn
end


local close, dup2, fork, pipe, wait, _exit =
  M.close, M.dup2, M.fork, M.pipe, M.wait, M._exit
local STDIN_FILENO, STDOUT_FILENO = M.STDIN_FILENO, M.STDOUT_FILENO

--- Close a pipeline opened with popen or popen_pipeline.
-- @function pclose
-- @tparam table pfd pipeline object
-- @return values as for `P.wait`, for the last (or only) stage of the pipeline

local function pclose (pfd)
  close (pfd.fd)
  for i = 1, #pfd.pids - 1 do
    wait (pfd.pids[i])
  end
  local _, reason, status = wait (pfd.pids[#pfd.pids])
  return reason, status
end

if _DEBUG ~= false then
  M.pclose = function (...)
    local argt = {...}
    checktable ("pclose", 1, argt[1])
    if #argt > 2 then toomanyargerror ("pclose", 1, #argt) end
    return pclose (...)
  end
else
  M.pclose = pclose
end


local function move_fd (from_fd, to_fd)
  if from_fd ~= to_fd then
    if not dup2 (from_fd, to_fd) then
      error "error dup2-ing"
    end
    close (from_fd)
  end
end

--- Run a commands or Lua function in a sub-process.
-- @function popen
-- @tparam task, as for @{execx}
-- @tparam string mode `"r"` for read or `"w"` for write
-- @func[opt] pipe_fn function returning a paired read and
--   write file descriptor (*default* @{posix.unistd.pipe})
-- @treturn pfd pipeline object

local function popen (task, mode, pipe_fn)
  local read_fd, write_fd = (pipe_fn or pipe) ()
  if not read_fd then
    error "error opening pipe"
  end
  local parent_fd, child_fd, in_fd, out_fd
  if mode == "r" then
    parent_fd, child_fd, in_fd, out_fd = read_fd, write_fd, STDIN_FILENO, STDOUT_FILENO
  elseif mode == "w" then
    parent_fd, child_fd, in_fd, out_fd = write_fd, read_fd, STDOUT_FILENO, STDIN_FILENO
  else
    error "invalid mode"
  end
  local pid = fork ()
  if pid == nil then
    error "error forking"
  elseif pid == 0 then -- child process
    move_fd (child_fd, out_fd)
    close (parent_fd)
    _exit (execx (task, child_fd, in_fd, out_fd))
  end -- parent process
  close (child_fd)
  return {pids = {pid}, fd = parent_fd}
end

if _DEBUG ~= false then
  M.popen = function (task, ...)
    local argt, typetask = {task, ...}, type (task)
    if typetask ~= "table" and typetask ~= "function" then
      argtypeerror ("popen", 1, "table or function", task)
    end
    checkstring ("popen", 2, argt[2])
    if argt[3] ~= nil and type (argt[3]) ~= "function" then
      argtypeerror ("popen", 3, "function or nil", argt[3])
    end
    if #argt > 3 then toomanyargerror ("popen", 3, #argt) end
    return popen (task, ...)
  end
else
  M.popen = popen
end


--- Perform a series of commands and Lua functions as a pipeline.
-- @function popen_pipeline
-- @tparam table t tasks for @{execx}
-- @tparam string mode `"r"` for read or `"w"` for write
-- @func[opt] pipe_fn function returning a paired read and
--   write file descriptor (*default* @{posix.unistd.pipe})
-- @treturn pfd pipeline object

local close, _exit = M.close, M._exit

local function popen_pipeline (tasks, mode, pipe_fn)
  local first, from, to, inc = 1, 2, #tasks, 1
  if mode == "w" then
    first, from, to, inc = #tasks, #tasks - 1, 1, -1
  end
  local pfd = popen (tasks[first], mode, pipe_fn)
  for i = from, to, inc do
    local pfd_next = popen (function (fd, in_fd, out_fd)
                              move_fd (pfd.fd, in_fd)
                              _exit (execx (tasks[i]))
                            end,
                            mode,
                            pipe_fn)
    close (pfd.fd)
    pfd.fd = pfd_next.fd
    table.insert (pfd.pids, pfd_next.pids[1])
  end
  return pfd
end

if _DEBUG ~= false then
  M.popen_pipeline = function (...)
    local argt = {...}
    checktable ("popen_pipeline", 1, argt[1])
    checkstring ("popen_pipeline", 2, argt[2])
    if argt[3] ~= nil and type (argt[3]) ~= "function" then
      argtypeerror ("popen_pipeline", 3, "function or nil", argt[3])
    end
    if #argt > 3 then toomanyargerror ("popen_pipeline", 3, #argt) end
    return popen_pipeline (...)
  end
else
  M.popen_pipeline = popen_pipeline
end


--- Add one gettimeofday() returned timeval to another.
-- @function timeradd
-- @param x a timeval
-- @param y another timeval
-- @return x + y, adjusted for usec overflow

local function timeradd (x, y)
  local sec, usec = 0, 0
  if x.sec then sec = sec + x.sec end
  if y.sec then sec = sec + y.sec end
  if x.usec then usec = usec + x.usec end
  if y.usec then usec = usec + y.usec end
  if usec > 1000000 then
    sec = sec + 1
    usec = usec - 1000000
  end

  return { sec = sec, usec = usec }
end

if _DEBUG ~= false then
  M.timeradd = function (...)
    local argt = {...}
    checktable ("timeradd", 1, argt[1])
    checktable ("timeradd", 2, argt[2])
    if #argt > 2 then toomanyargerror ("timeradd", 2, #argt) end
    return timeradd (...)
  end
end


--- Compare one gettimeofday() returned timeval with another
-- @function timercmp
-- @param x a timeval
-- @param y another timeval
-- @return 0 if x and y are equal, >0 if x is newer, <0 if y is newer

local function timercmp (x, y)
  local x = { sec = x.sec or 0, usec = x.usec or 0 }
  local y = { sec = y.sec or 0, usec = y.usec or 0 }
  if x.sec ~= y.sec then
    return x.sec - y.sec
  else
    return x.usec - y.usec
  end
end

if _DEBUG ~= false then
  M.timercmp = function (...)
    local argt = {...}
    checktable ("timercmp", 1, argt[1])
    checktable ("timercmp", 2, argt[2])
    if #argt > 2 then toomanyargerror ("timercmp", 2, #argt) end
    return timercmp (...)
  end
end


--- Subtract one gettimeofday() returned timeval from another.
-- @function timersub
-- @param x a timeval
-- @param y another timeval
-- @return x - y, adjusted for usec underflow

local function timersub (x,y)
  local sec, usec = 0, 0
  if x.sec then sec = x.sec end
  if y.sec then sec = sec - y.sec end
  if x.usec then usec = x.usec end
  if y.usec then usec = usec - y.usec end
  if usec < 0 then
    sec = sec - 1
    usec = usec + 1000000
  end
  return { sec = sec, usec = usec }
end

if _DEBUG ~= false then
  M.timersub = function (...)
    local argt = {...}
    checktable ("timersub", 1, argt[1])
    checktable ("timersub", 2, argt[2])
    if #argt > 2 then toomanyargerror ("timersub", 2, #argt) end
    return timersub (...)
  end
end


return M
