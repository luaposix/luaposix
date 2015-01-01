--[[
 POSIX library for Lua 5.1, 5.2 & 5.3.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2014-2015
]]

-- Backwards compatibility alias.

return {
  openpty = require "posix".openpty,
}
