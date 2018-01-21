--[[
 POSIX library for Lua 5.1, 5.2 & 5.3.
 Copyright (C) 2014-2018 Gary V. Vaughan
]]

-- Backwards compatibility alias.

return {
   openpty = require 'posix'.openpty,
}
