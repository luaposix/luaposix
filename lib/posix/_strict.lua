--[[
 POSIX library for Lua 5.1, 5.2, 5.3 & 5.4.
 Copyright (C) 2014-2021 Gary V. Vaughan
]]
--[[--
 Private argument checking helpers.

 Undocumented internal helpers for strict environment wrapping.

 @module posix._strict
]]


local setfenv = rawget(_G, 'setfenv') or function() end


local strict
do
   local ok, _debug = pcall(require, 'std._debug.init')
   if ok and _debug.strict then
      ok, strict = pcall(require, 'std.strict.init')
   end
   if not ok then
      strict = function(env)
         return env
      end
   end
end


return function(env, level)
   env = strict(env)
   setfenv(1+(level or 1), env)
   return env
end
