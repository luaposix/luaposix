--[[
 POSIX library for Lua 5.1/5.2.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2013-2014
 (c) Reuben Thomas <rrt@sc3d.org> 2010-2013
 (c) Natanael Copa <natanael.copa@gmail.com> 2008-2010
]]
------
-- @module posix.util

local M = {}


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


return M
