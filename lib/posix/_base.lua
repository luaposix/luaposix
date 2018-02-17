--[[
 POSIX library for Lua 5.1, 5.2 & 5.3.
 Copyright (C) 2014-2018 Gary V. Vaughan
]]
--[[--
 Private argument checking helpers.

 Undocumented internal helpers for argcheck wrapping.

 @module posix._base
]]



local HAVE_TYPECHECK, typecheck = pcall(require, 'typecheck')


local _ENV = require 'std.normalize' {
   'bit32.band',
   'posix.sys.stat.S_IRUSR',
   'posix.sys.stat.S_IWUSR',
   'posix.sys.stat.S_IXUSR',
   'posix.sys.stat.S_IRGRP',
   'posix.sys.stat.S_IWGRP',
   'posix.sys.stat.S_IXGRP',
   'posix.sys.stat.S_IROTH',
   'posix.sys.stat.S_IWOTH',
   'posix.sys.stat.S_IXOTH',
   'posix.sys.stat.S_ISUID',
   'posix.sys.stat.S_ISGID',
   'table.concat',
}


local MODE_MAP = {
   {c='r', b=S_IRUSR}, {c='w', b=S_IWUSR}, {c='x', b=S_IXUSR},
   {c='r', b=S_IRGRP}, {c='w', b=S_IWGRP}, {c='x', b=S_IXGRP},
   {c='r', b=S_IROTH}, {c='w', b=S_IWOTH}, {c='x', b=S_IXOTH},
}


return {
   MODE_MAP = MODE_MAP,

   argscheck = HAVE_TYPECHECK and typecheck.argscheck or function(_, fn)
      return fn
   end,

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
