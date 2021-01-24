--[[
 POSIX library for Lua 5.1, 5.2, 5.3 & 5.4.
 Copyright (C) 2014-2021 Gary V. Vaughan
]]

-- Backwards compatibility alias.

return {
   openpty = require 'posix'.openpty,
}
