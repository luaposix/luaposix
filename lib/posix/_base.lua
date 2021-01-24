--[[
 POSIX library for Lua 5.1, 5.2, 5.3 & 5.4.
 Copyright (C) 2014-2021 Gary V. Vaughan
]]
--[[--
 Private argument checking helpers.

 Undocumented internal helpers for argcheck wrapping.

 @module posix._base
]]


local HAVE_TYPECHECK, typecheck = pcall(require, 'typecheck')

local HAVE_BITWISE_OPS, bitwise = pcall(require, 'posix._bitwise')

if not HAVE_BITWISE_OPS then
   bitwise = require 'bit32'
end


local _ENV = require 'posix._strict' {
   S_IRUSR = require 'posix.sys.stat'.S_IRUSR,
   S_IWUSR = require 'posix.sys.stat'.S_IWUSR,
   S_IXUSR = require 'posix.sys.stat'.S_IXUSR,
   S_IRGRP = require 'posix.sys.stat'.S_IRGRP,
   S_IWGRP = require 'posix.sys.stat'.S_IWGRP,
   S_IXGRP = require 'posix.sys.stat'.S_IXGRP,
   S_IROTH = require 'posix.sys.stat'.S_IROTH,
   S_IWOTH = require 'posix.sys.stat'.S_IWOTH,
   S_IXOTH = require 'posix.sys.stat'.S_IXOTH,
   S_ISUID = require 'posix.sys.stat'.S_ISUID,
   S_ISGID = require 'posix.sys.stat'.S_ISGID,
   band = bitwise.band,
   concat = table.concat,
   error = error,
   format = string.format,
}


local MODE_MAP = {
   {c='r', b=S_IRUSR}, {c='w', b=S_IWUSR}, {c='x', b=S_IXUSR},
   {c='r', b=S_IRGRP}, {c='w', b=S_IWGRP}, {c='x', b=S_IXGRP},
   {c='r', b=S_IROTH}, {c='w', b=S_IWOTH}, {c='x', b=S_IXOTH},
}


return {
   MODE_MAP = MODE_MAP,

   argerror = function(name, i, extramsg, level)
      level = level or 1
      local s = format("bad argument #%d to '%s'", i, name)
      if extramsg ~= nil then
         s = s .. ' (' .. extramsg .. ')'
      end
      error(s, level + 1)
   end,

   argscheck = HAVE_TYPECHECK and typecheck.argscheck or function(_, fn)
      return fn
   end,

   band = bitwise.band,

   bor = bitwise.bor,

   bnot = bitwise.bnot,

   pushmode = function(mode)
      local m = {}
      for i = 1, 9 do
         if band(mode, MODE_MAP[i].b) ~= 0 then
            m[i] = MODE_MAP[i].c
         else
            m[i] = '-'
         end
      end
      if band(mode, S_ISUID) ~= 0 then
         if band(mode, S_IXUSR) ~= 0 then
            m[3] = 's'
         else
            m[3] = 'S'
         end
      end
      if band(mode, S_ISGID) ~= 0 then
         if band(mode, S_IXGRP) ~= 0 then
            m[6] = 's'
         else
            m[6] = 'S'
         end
      end
      return concat(m)
   end,
}
