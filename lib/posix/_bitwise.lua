--[[
 POSIX library for Lua 5.1, 5.2 & 5.3.
 Copyright (C) 2021 Gary V. Vaughan
]]
--[[--
 Private argument checking helpers.

 Undocumented internal helpers for bitwise operations on Lua 5.3+.

 @module posix._bitwise
]]


return {
   band = function(a, ...)
      for _, v in next, {...} do
        a = a & v
      end
      return a
   end,

   bor = function(a, ...)
      for _, v in next, {...} do
        a = a | v
      end
      return a
   end,

   bnot = function(a) return ~a end,
}
