-- Additions to the posix module (of luaposix).
local M = require "posix_c"
local posix = M
local bit
if _VERSION == "Lua 5.1" then bit = require "bit" else bit = require "bit32" end

--- Create a file.
-- @param file name of file to create
-- @param mode permissions with which to create file
-- @return file descriptor, or -1 on error
function M.creat (file, mode)
  return posix.open (file, bit.bor (posix.O_CREAT, posix.O_WRONLY, posix.O_TRUNC), mode)
end

--- Run a command or function in a sub-process.
-- @param command, a string to be executed as a shell command, or a
-- table of arguments to <code>posix.execp</code> or a Lua function
-- which should read from standard input, write to standard output,
-- and return an exit code
-- @param task command to run, or Lua function
-- @param ... positional arguments to the program or function
-- @return status exit code, or nil if fork or wait fails
-- @return error message, or exit type if wait succeeds
function M.spawn (task, ...)
  local pid, err = posix.fork ()
  if pid == nil then
    return pid, err
  elseif pid == 0 then
    if type (task) == "string" then
      task = {"/bin/sh", "-c", task, ...}
    end
    if type (task) == "table" then
      posix.execp (unpack (task))
      -- Only get here if there's an error; kill the fork
      local _, no = posix.errno ()
      posix._exit (no)
    else
      posix._exit (task (...) or 0)
    end
  else
    local _, reason, status = posix.wait (pid)
    return status, reason -- If wait failed, status is nil & reason is error
  end
end
M.system = M.spawn -- OBSOLETE alias

--- Perform a series of commands and Lua functions as a pipeline.
-- @param {t1 ...} tasks for <code>posix.spawn</code>
-- @param pipe_fn function returning a paired read and write file
-- descriptor
-- (default: <code>posix.pipe</code>)
-- @return exit code of the chain
local function pipeline (t, pipe_fn)
  local pipe_fn = pipe_fn or posix.pipe
  assert (type (t) == "table",
          "bad argument #1 to 'pipeline' (table expected, got " .. type (t1) .. ")")

  local pid, read_fd, write_fd, save_stdout
  if #t > 1 then
    read_fd, write_fd = pipe_fn ()
    if not read_fd then
      die ("error opening pipe")
    end
    pid = posix.fork ()
    if pid == nil then
      die ("error forking")
    elseif pid == 0 then -- child process
      if not posix.dup2 (read_fd, posix.STDIN_FILENO) then
        die ("error dup2-ing")
      end
      posix.close (read_fd)
      posix.close (write_fd)
      os.exit (pipeline (list.sub (t, 2), pipe_fn)) -- recurse with remaining arguments
    else -- parent process
      save_stdout = posix.dup (posix.STDOUT_FILENO)
      if not save_stdout then
        die ("error dup-ing")
      end
      if not posix.dup2 (write_fd, posix.STDOUT_FILENO) then
        die ("error dup2-ing")
      end
      posix.close (read_fd)
      posix.close (write_fd)
    end
  end

  local ret = M.spawn (t[1])
  if not ret then
    die ("error in fork or wait")
  end
  posix.close (posix.STDOUT_FILENO)
  
  if #t > 1 then
    posix.close (write_fd)
    posix.wait (pid)
    if not posix.dup2 (save_stdout, posix.STDOUT_FILENO) then
      die ("error dup2-ing")
    end
    posix.close (save_stdout)
  end

  return ret
end
M.pipeline = pipeline

--- Perform a series of commands and Lua functions as a pipeline,
-- returning the output of the last stage's <code>stdout</code> as
-- the output of an iterator.
-- @param t as for <code>posix.pipeline</code>
-- @param pipe_fn as for <code>posix.pipeline</code>
-- @return iterator function returning a chunk of output on each call
function M.pipeline_iterator (t, pipe_fn)
  local read_fd, write_fd = posix.pipe ()
  if not read_fd then
    die ("error opening pipe")
  end
  table.insert (t, function ()
                     local s
                     repeat
                       s = posix.read (posix.STDIN_FILENO, posix.BUFSIZ)
                       if s and #s > 0 then
                         posix.write (write_fd, s)
                       else
                         break
                       end
                       posix.close (write_fd)
                     until false
                   end)

  local ret
  local exit = false
  local pid = posix.fork ()
  if pid == nil then
    die ("error forking")
  elseif pid == 0 then -- child process
    os.exit (M.pipeline (t, pipe_fn))
  else -- parent process
    posix.signal (posix.SIGCHLD,
                  function ()
                    posix.fcntl (read_fd, posix.F_SETFL, posix.O_NONBLOCK)
                    exit = true
                  end,
                  posix.SA_NOCLDSTOP)
    return function ()
      local s = posix.read (read_fd, posix.BUFSIZ)
      if (not s or #s == 0) and exit == true then
        local _
        _, _, ret = posix.wait (pid)
        s = posix.read (read_fd, posix.BUFSIZ)
        return s
      end
      return s or ""
    end
  end
end

--- Perform a series of commands and Lua functions as a pipeline,
-- returning the output of the last stage's <code>stdout</code>.
-- @param t as for <code>posix.pipeline</code>
-- @param pipe_fn as for <code>posix.pipeline</code>
-- @return output of the pipeline
function M.pipeline_slurp (t, pipe_fn)
  local out = ""
  for s in M.pipeline_iterator (t, pipe_fn) do
    out = out .. s
  end
  return out
end

--- Check permissions like <code>access</code>, but for euid.
-- Based on the glibc function of the same name. Does not always check
-- for read-only file system, text busy, etc., and does not work with
-- ACLs &c.
-- @param file file to check
-- @param mode checks to perform (as for access)
-- @return 0 if access allowed; <code>nil</code> otherwise (and errno is set)
function M.euidaccess (file, mode)
  local pid = posix.getpid ()

  if pid.uid == pid.euid and pid.gid == pid.egid then
    -- If we are not set-uid or set-gid, access does the same.
    return posix.access (file, mode)
  end

  local stats = posix.stat (file)
  if not stats then
    return
  end

  -- The super-user can read and write any file, and execute any file
  -- that anyone can execute.
  if pid.euid == 0 and ((not string.match (mode, "x")) or
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
  if pid.euid == stats.st_uid then
    granted = stats.st_mode:sub (7, 9)
  elseif pid.egid == stats.st_gid or set.new (posix.getgroups ()):member (stats.st_gid) then
    granted = stats.st_mode:sub (4, 6)
  end
  granted = string.gsub (granted, "[^rwx]", "")

  if string.gsub ("[^" .. granted .. "]", mode) == "" then
    return 0
  end
  posix.set_errno (EACCESS)
end

--- Open a pseudo-terminal.
-- Based on the glibc function of the same name.
-- FIXME: add support for term and win arguments.
-- @return master file descriptor, or nil on error
-- @return slave file descriptor, or message on error
-- @return file name, or nothing on error
function M.openpty (term, win)
  local master, reason = posix.openpt (bit.bor (posix.O_RDWR, posix.O_NOCTTY))
  if master then
    local ok
    ok, reason = posix.grantpt (master)
    if ok then
      ok, reason = posix.unlockpt (master)
      if ok then
        local slave_name
        slave_name, reason = posix.ptsname (master)
        if slave_name then
          local slave
          slave, reason = posix.open (slave_name, bit.bor (posix.O_RDWR, posix.O_NOCTTY))
          if slave then
            return master, slave, slave_name
          end
        end
      end
    end
    close (master)
  end
  return nil, reason
end

--- Add one gettimeofday() returned timeval to another.
-- @param x a timeval
-- @param y another timeval
-- @return x + y, adjusted for usec overflow
function M.timeradd (x,y)
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

--- Compare one gettimeofday() returned timeval with another
-- @param x a timeval
-- @param y another timeval
-- @return 0 if x and y are equal, >0 if x is newer, <0 if y is newer
function M.timercmp (x, y)
  local x = { sec = x.sec or 0, usec = x.usec or 0 }
  local y = { sec = y.sec or 0, usec = y.usec or 0 }
  if x.sec ~= y.sec then
    return x.sec - y.sec
  else
    return x.usec - y.usec
  end
end

--- Subtract one gettimeofday() returned timeval from another.
-- @param x a timeval
-- @param y another timeval
-- @return x - y, adjusted for usec underflow
function M.timersub (x,y)
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

return M
