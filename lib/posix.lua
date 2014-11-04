--[[
 POSIX library for Lua 5.1/5.2.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2014
 (c) Reuben Thomas <rrt@sc3d.org> 2010-2013
 (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
]]
--[[--
 Lua POSIX bindings.

 In addition to the convenience functions documented in this module, all
 APIs from submodules are copied into the return table for convenience and
 backwards compatibility.

 @module posix
]]

local bit = bit32 or require "bit"
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

M.version = "posix for " .. _VERSION .. " / luaposix 33.0.0"


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

local bit    = bit32 or require "bit"
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


--- Perform a series of commands and Lua functions as a pipeline.
-- @function pipeline
-- @tparam table t tasks for @{spawn}
-- @func[opt=@{posix.unistd.pipe} pipe_fn function returning a paired read and
--   write file descriptor
-- @treturn int exit code of the chain

local close, dup, dup2, fork, pipe, wait =
  M.close, M.dup, M.dup2, M.fork, M.pipe, M.wait
local STDIN_FILENO, STDOUT_FILENO = M.STDIN_FILENO, M.STDOUT_FILENO

local function pipeline (t, pipe_fn)
  pipe_fn = pipe_fn or pipe

  local pid, read_fd, write_fd, save_stdout
  if #t > 1 then
    read_fd, write_fd = pipe_fn ()
    if not read_fd then
      die ("error opening pipe")
    end
    pid = fork ()
    if pid == nil then
      die ("error forking")
    elseif pid == 0 then -- child process
      if not dup2 (read_fd, STDIN_FILENO) then
        die ("error dup2-ing")
      end
      close (read_fd)
      close (write_fd)
      os.exit (pipeline (list.sub (t, 2), pipe_fn)) -- recurse with remaining arguments
    else -- parent process
      save_stdout = dup (STDOUT_FILENO)
      if not save_stdout then
        die ("error dup-ing")
      end
      if not dup2 (write_fd, STDOUT_FILENO) then
        die ("error dup2-ing")
      end
      close (read_fd)
      close (write_fd)
    end
  end

  local r = spawn (t[1])
  if not r then
    die ("error in fork or wait")
  end
  close (STDOUT_FILENO)

  if #t > 1 then
    close (write_fd)
    wait (pid)
    if not dup2 (save_stdout, STDOUT_FILENO) then
      die ("error dup2-ing")
    end
    close (save_stdout)
  end

  return r
end

if _DEBUG ~= false then
  M.pipeline = function (...)
    local argt = {...}
    checktable ("pipeline", 1, argt[1])
    local typepipefn = type (argt[2])
    if argt[2] ~= nil and type (argt[2]) ~= "function" then
      argtypeerror ("pipeline", 2, "function or nil", argt[2])
    end
    if #argt > 2 then toomanyargerror ("pipeline", 2, #argt) end
    return pipeline (...)
  end
else
  M.pipeline = pipeline
end


--- Perform a series of commands and Lua functions as a pipeline,
-- returning the output of the last stage's <code>stdout</code> as
-- the values of an iterator.
-- @function pipeline_iterator
-- @tparam table t tasks for @{spawn}
-- @func[opt=@{posix.unistd.pipe} pipe_fn function returning a paired read and
--   write file descriptor
-- @return iterator function returning a chunk of output on each call

local close, fork, pipe, read, wait, write =
  M.close, M.fork, M.pipe, M.read, M.wait, M.write
local BUFSIZ, STDIN_FILENO = M.BUFSIZ, M.STDIN_FILENO

function pipeline_iterator (t, pipe_fn)
  local read_fd, write_fd = pipe ()
  if not read_fd then
    die ("error opening pipe")
  end
  table.insert (t, function ()
                     while true do
                       local s = read (STDIN_FILENO, BUFSIZ)
                       if not s or #s == 0 then break end
                       write (write_fd, s)
                     end
                     close (write_fd)
                   end)

  local pid = fork ()
  if pid == nil then
    die ("error forking")
  elseif pid == 0 then -- child process
    os.exit (pipeline (t, pipe_fn))
  else -- parent process
    close (write_fd)
    return function ()
      local s = read (read_fd, BUFSIZ)
      if not s or #s == 0 then
        wait (pid)
        return nil
      end
      return s
    end
  end
end

if _DEBUG ~= false then
  M.pipeline_iterator = function (...)
    local argt = {...}
    checktable ("pipeline_iterator", 1, argt[1])
    local typepipefn = type (argt[2])
    if argt[2] ~= nil and type (argt[2]) ~= "function" then
      argtypeerror ("pipeline_iterator", 2, "function or nil", argt[2])
    end
    if #argt > 2 then toomanyargerror ("pipeline_iterator", 2, #argt) end
    return pipeline_iterator (...)
  end
end


--- Perform a series of commands and Lua functions as a pipeline.
-- Return the output of the last stage's `stdout`.
-- @function pipeline_slurp
-- @tparam table t tasks for @{spawn}
-- @func[opt=@{posix.unistd.pipe} pipe_fn function returning a paired read and
--   write file descriptor
-- @treturn string output of the pipeline

local function pipeline_slurp (t, pipe_fn)
  local out = ""
  for s in pipeline_iterator (t, pipe_fn) do
    out = out .. s
  end
  return out
end

if _DEBUG ~= false then
  M.pipeline_slurp = function (...)
    local argt = {...}
    checktable ("pipeline_slurp", 1, argt[1])
    local typepipefn = type (argt[2])
    if argt[2] ~= nil and type (argt[2]) ~= "function" then
      argtypeerror ("pipeline_slurp", 2, "function or nil", argt[2])
    end
    if #argt > 2 then toomanyargerror ("pipeline_slurp", 2, #argt) end
    return pipeline_slurp (...)
  end
end


--- Run a command or function in a sub-process.
-- @function spawn
-- @param task, a string to be executed as a shell command, or a
--   table of arguments to `P.execp` or a Lua function, which should read
--   from standard input, write to standard output, and return an exit code
-- @tparam string ... positional arguments to the program or function
-- @treturn[1] int status exit code, if successful
-- @treturn[1] string exit type, if wait succeeds
-- @treturn[2] nil
-- @treturn[2] string error message

local _exit, errno, execp, fork, wait =
  M._exit, M.errno, M.execp, M.fork, M.wait

local function spawn (task, ...)
  local pid, err = fork ()
  if pid == nil then
    return pid, err
  elseif pid == 0 then
    if type (task) == "string" then
      task = {"/bin/sh", "-c", task, ...}
    end
    if type (task) == "table" then
      execp (unpack (task))
      -- Only get here if there's an error; kill the fork
      local _, no = errno ()
      _exit (no)
    else
      _exit (task (...) or 0)
    end
  else
    local _, reason, status = wait (pid)
    return status, reason -- If wait failed, status is nil & reason is error
  end
end

if _DEBUG ~= false then
  M.spawn = function (task, ...)
    local argt, typetask = {task, ...}, type (task)
    if typetask ~= "string" and typetask ~= "table" and typetask ~= "function" then
      argtypeerror ("spawn", 1, "string, table or function", task)
    end
    local i = 2
    repeat
      checkstring ("spawn", i, argt[i])
      i = i + 1
    until i > #argt
    return spawn (task, ...)
  end
else
  M.spawn = spawn
end

M.system = M.spawn -- OBSOLETE alias


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
