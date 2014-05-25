------
-- @module posix

-- Additions to the posix module (of luaposix).
local M = require "posix_c"
local posix = M
local bit
if _VERSION == "Lua 5.1" then bit = require "bit" else bit = require "bit32" end

------
-- Lazy load of available submodules.
-- @function __index
-- @string name submodule name
-- @return the submodule that was loaded to satisfy the missing `name`


--- File descriptors.
-- @section filedescriptors

--- Create a file.
-- @param file name of file to create
-- @param mode permissions with which to create file
-- @return file descriptor, or -1 on error
function M.creat (file, mode)
  return posix.open (file, bit.bor (posix.O_CREAT, posix.O_WRONLY, posix.O_TRUNC), mode)
end


--- Terminal handling.
-- @section terminalhandling

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
    posix.close (master)
  end
  return nil, reason
end


return setmetatable (M, {
  __index = function (self, name)
              local ok, t = pcall (require, "posix." .. name)
	      if ok then
                rawset (self, name, t)
		return t
	      end
	      return nil
	    end,
})
