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

--- Run a program like <code>os.execute</code>, but without a shell.
-- @param file filename of program to run
-- @param ... arguments to the program
-- @return status exit code, or nil if fork or wait fails
-- @return error message, or exit type if wait succeeds
function M.system (file, ...)
  local pid = posix.fork ()
  if pid == 0 then
    posix.execp (file, ...)
    -- Only get here if there's an error; kill the fork
    local _, no = posix.errno ()
    posix._exit (no)
  else
    local pid, reason, status = posix.wait (pid)
    return status, reason -- If wait failed, status is nil & reason is error
  end
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
