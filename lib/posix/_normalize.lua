--[[
 Core functions for Lua 5.1, 5.2, 5.3, 5.4 & 5.5
 Copyright (C) 2011-2026 std.normalize authors
]]
--[[--
 The functions that std.normalize shares with the modules that sit
 beneath it, and so cannot depend on it.

 This file requires nothing when it loads, and is the same wherever it is:
 std.normalize requires it as `std.normalize._core`, and typecheck, std.strict and
 optparse vendor a byte for byte copy as their own `_normalize` module,
 so that there is only one copy to maintain.  Change it here, and let
 `luke release` copy it to them, rather than change a copy.

 @module std.normalize._core
]]


local debug_getinfo = debug.getinfo
local debug_getupvalue = debug.getupvalue
local debug_setupvalue = debug.setupvalue
local debug_upvaluejoin = debug.upvaluejoin
-- Lua 5.1 has debug.getfenv and debug.setfenv, which work on C functions
-- too.  An environment that smooths over Lua differences (like specl's
-- sandbox) may hide them and supply a global getfenv and setfenv of its
-- own, probably this module's; so use the global ones only when nothing
-- else will do.
local debug_getfenv = debug.getfenv
local debug_setfenv = debug.setfenv
local host_getfenv = rawget(_G, 'getfenv') or false
local host_setfenv = rawget(_G, 'setfenv') or false


-- The field *n* of the metatable of *x*, or `nil` if there is no such
-- field.  The metatable that getmetatable returns for a protected object
-- is its `__metatable` field, which need not be a table, such as `true`.
local function getmetafield(x, n)
   local mt = getmetatable(x)
   if type(mt) == 'table' then
      return mt[n]
   end
end


-- Whether *x* can be called, and so what it is if it can: *x* itself,
-- rather than its `__call` metamethod, which needs `self` too.
local function callable(x)
   -- Careful here!
   -- Most versions of Lua don't recurse functables, so make sure you
   -- always put a real function in __call metamethods.  Consequently,
   -- no reason to recurse here.
   -- func=function() print 'called' end
   -- func() --> 'called'
   -- functable=setmetatable({}, {__call=func})
   -- functable() --> 'called'
   -- nested=setmetatable({}, {__call=function(self, ...) return functable(...)end})
   -- nested() -> 'called'
   -- notnested=setmetatable({}, {__call=functable})
   -- notnested()
   -- --> stdin:1: attempt to call global 'nested' (a table value)
   -- --> stack traceback:
   -- -->	stdin:1: in main chunk
   -- -->		[C]: in ?
   if type(x) == 'function' then
      return x
   end
   if getmetafield(x, '__call') then
      return x
   end
end


-- Whether *x* is callable, as true or false.
local function iscallable(x)
   return callable(x) ~= nil
end


local function getmetamethod(x, n)
   return callable(getmetafield(x, n))
end


local function rawlen(x)
   -- Lua 5.1 does not implement rawlen, and while # operator ignores
   -- __len metamethod, `nil` in sequence is handled inconsistently.
   if type(x) ~= 'table' then
      return #x
   end

   local n = #x
   for i = 1, n do
      if x[i] == nil then
         return i -1
      end
   end
   return n
end


local function len(x)
   return (getmetamethod(x, '__len') or rawlen)(x)
end


local tointeger = (function(f)
   if f == nil then
      -- No host tointeger implementationm use our own.
      local floor = math.floor
      return function(x)
         if type(x) == 'number' and x - floor(x) == 0.0 then
            return x
         end
      end

   elseif f '1' ~= nil then
      -- Don't perform implicit string-to-number conversion!
      return function(x)
         if type(x) == 'number' then
            return f(x)
         end
      end
   end

   -- Host tointeger is good!
   return f
end)(math.tointeger)


local pack = (function(f)
   local pack_mt = {
      __len = function(self)
         return self.n
      end,
   }

   local pack_fn = f or function(...)
      return {n=select('#', ...), ...}
   end

   return function(...)
      return setmetatable(pack_fn(...), pack_mt)
   end
end)(table.pack)


local pairs = (function(b)
   if b then
      -- Add support for __pairs when missing.
      return function (t)
         return (getmetamethod(t, '__pairs') or pairs)(t)
      end
   end

   return _G.pairs
end)(not not pairs(setmetatable({},{__pairs=function() return false end})))


-- The function that a functor *fn* runs: its own `call` field, like the
-- functables that share one `__call` dispatcher, or else its `__call`
-- metamethod.  No need to recurse, because Lua doesn't support nested
-- functors.
local function unwrap(fn)
   local call = type(fn) == 'table' and rawget(fn, 'call')
   if type(call) == 'function' then
      return call
   end
   return getmetafield(fn, '__call') or fn
end


-- A getfenv that works on every Lua, for a caller that is *extra* stack
-- frames beneath the function, such as one that checks its arguments.
-- Choose by version: an environment that smooths over Lua differences
-- may not provide the 5.1 debug.[gs]etfenv.
local function make_getfenv(extra)
   if _VERSION == 'Lua 5.1' then
      return function(fn)
         local n = tointeger(fn or 1)
         if n then
            if n == 0 then
               return _G
            end
            if n > 0 then
               -- Adjust for this function's stack frame.
               n = n + 1 + extra
               if debug_getfenv then
                  -- Never the global getfenv, which may not be ours.
                  return (debug_getfenv(debug_getinfo(n, 'f').func))
               end
            end

            -- The parentheses return only the environment, and keep this
            -- from being a tail call, which would remove a stack frame and
            -- break the numeric *fn* count.
            return (host_getfenv(n))
         end

         if type(fn) ~= 'function' then
            -- Unwrapping a functor doesn't change the stack frame offset.
            fn = unwrap(fn)
         end

         -- In Lua 5.1, only debug.getfenv works on C functions; but it
         -- does not work on stack counts.
         return (debug_getfenv or host_getfenv)(fn)
      end
   end

   -- Thanks to http://lua-users.org/lists/lua-l/2010-06/msg00313.html
   return function(fn)
      if fn == 0 then
         return _G
      end
      local n = tointeger(fn or 1)
      if n then
         fn = debug_getinfo(n + 1 + extra, 'f').func
      elseif type(fn) ~= 'function' then
         fn = unwrap(fn)
      end

      local name, env
      local up = 0
      repeat
         up = up + 1
         name, env = debug_getupvalue(fn, up)
      until name == '_ENV' or name == nil
      return env
   end
end


-- A setfenv that works on every Lua, with *extra* stack frames, as for
-- make_getfenv.
local function make_setfenv(extra)
   if _VERSION == 'Lua 5.1' then
      return function(fn, env)
         local n = tointeger(fn or 1)
         if n then
            if n == 0 then
               error('cannot set the global environment', 2)
            end
            if n > 0 then
               n = n + 1 + extra
               if debug_setfenv then
                  local f = debug_getinfo(n, 'f').func
                  debug_setfenv(f, env)
                  return f
               end
            end
            -- One value, and not a tail call, as in getfenv.
            return (host_setfenv(n, env))
         end
         if type(fn) ~= 'function' then
            fn = unwrap(fn)
         end
         return (debug_setfenv or host_setfenv)(fn, env)
      end
   end

   -- Thanks to http://lua-users.org/lists/lua-l/2010-06/msg00313.html
   return function(fn, env)
      local n = tointeger(fn or 1)
      if n then
         if n == 0 then
            error('cannot set the global environment', 2)
         end
         if n > 0 then
            n = n + 1 + extra
         end
         fn = debug_getinfo(n, 'f').func
      elseif type(fn) ~= 'function' then
         fn = unwrap(fn)
      end

      local up, name = 0
      repeat
         up = up + 1
         name = debug_getupvalue(fn, up)
      until name == '_ENV' or name == nil
      if name then
         debug_upvaluejoin(fn, up, function() return name end, 1)
         debug_setupvalue(fn, up, env)
      end
      return fn
   end
end


-- The hints that std._debug holds, or if it is not installed, the
-- defaults for all of the libraries that use this file, so that they
-- agree: argument checking and strict variable declarations are both on,
-- for as long as typecheck and std.strict, respectively, are installed.
-- Installing std._debug does not change that, until a hint is set.
local DEFAULTS = {argcheck = true, strict = true}
local hints_cache


local function hints()
   if hints_cache == nil then
      local ok, r = pcall(require, 'std._debug')
      if not ok then
         r = setmetatable({argcheck = true, strict = true}, {
            -- Like std._debug: nil or true for the defaults, false for none.
            __call = function(self, x)
               for k, v in pairs(DEFAULTS) do
                  self[k] = (x ~= false) and v
               end
               return self
            end,
         })
      end
      hints_cache = r
   end
   return hints_cache
end


-- The function that applies strict variable declarations to the
-- environment *env* of the function *level* frames above its caller, and
-- gives *env* back: std.strict, if it is installed and the hints want it,
-- otherwise only a function that sets the environment, in Lua 5.1.  Ask
-- for it once, when the library that wants it loads, and not from
-- std.strict itself, which has a copy of this file too.
local function strictness()
   local setfenv = rawget(_G, 'setfenv') or function() end
   local r = function(env, level)
      setfenv(1 + (level or 1), env)
      return env
   end

   if hints().strict then
      -- Specify `.init` submodule to make sure we only accept
      -- lua-stdlib/strict, and not the old strict module from
      -- lua-stdlib/lua-stdlib.
      local ok, m = pcall(require, 'std.strict.init')
      if ok then
         r = m
      end
   end
   return r
end


return {
   callable = callable,
   getfenv = make_getfenv(0),
   getmetafield = getmetafield,
   getmetamethod = getmetamethod,
   hints = hints,
   iscallable = iscallable,
   len = len,
   pack = pack,
   pairs = pairs,
   rawlen = rawlen,
   setfenv = make_setfenv(0),
   strictness = strictness,
   tointeger = tointeger,

   -- The getfenv and setfenv for a function that is *extra* stack frames
   -- beneath its caller, as std.normalize's are when typecheck checks
   -- their arguments.
   with_frames = function(extra)
      return {getfenv = make_getfenv(extra), setfenv = make_setfenv(extra)}
   end,
}
