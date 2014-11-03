--[[
 POSIX library for Lua 5.1/5.2.
 (c) Gary V. Vaughan <gary@vaughan.pe>, 2014
]]

-- Backwards compatibility alias.

return {
  openpty = require "posix".openpty,
}
