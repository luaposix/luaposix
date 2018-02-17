--[[
 POSIX library for Lua 5.1, 5.2 & 5.3.
 Copyright (C) 2014-2018 Gary V. Vaughan
]]
--[[--
 Legacy Lua POSIX bindings.

 APIs for maintaining compatibility with previous releases.

 @module posix
]]



local _ENV = require 'std.normalize' {
   'bit32.band',
   'bit32.bnot',
   'bit32.bor',
   'posix._base.MODE_MAP',
   'posix._base.argscheck',
   'posix._base.pushmode',
   'posix.fcntl.O_CREAT',
   'posix.fcntl.O_TRUNC',
   'posix.fcntl.O_WRONLY',
   'posix.fcntl.open',
   'posix.syslog.LOG_MASK',
   'posix.syslog.setlogmask',
   'posix.sys.msg.msgget',
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
   'posix.sys.stat.S_IRWXU',
   'posix.sys.stat.S_IRWXG',
   'posix.sys.stat.S_IRWXO',
   'posix.sys.stat.chmod',
   'posix.sys.stat.mkdir',
   'posix.sys.stat.mkfifo',
   'posix.sys.stat.stat',
   'posix.sys.stat.umask',
   'string.gsub',
   'string.match',
   'string.sub',
   'table.concat',
}


local RWXALL = bor(S_IRWXU, S_IRWXG, S_IRWXO)
local FCREAT = bor(O_CREAT, O_WRONLY, O_TRUNC)


-- FIXME: specl-14.x breaks function environments here :(
local pushmode = pushmode


local function rwxrwxrwx(modestr)
   local mode = 0
   for i = 1, 9 do
      if sub(modestr, i, i) == MODE_MAP[i].c then
         mode = bor(mode, MODE_MAP[i].b)
      elseif sub(modestr, i, i) == 's' then
         if i == 3 then
            mode = bor(mode, S_ISUID, S_IXUSR)
         elseif i == 6 then
            mode = bor(mode, S_ISGID, S_IXGRP)
         else
            return nil   -- bad mode
         end
      end
   end
   return mode
end


local function octal_mode(modestr)
   local mode = 0
   for i = 1, #modestr do
      mode = mode * 8 + tonumber(sub(modestr, i, i))
   end
   return mode
end


local function mode_munch(mode, modestr)
   if modestr == nil then
      return nil, 'string expected, got no value'
   elseif #modestr == 9 and match(modestr, '^[-rswx]+$') then
      return rwxrwxrwx(modestr)
   elseif match(modestr, '^[0-7]+$') then
      return octal_mode(modestr)
   elseif match(modestr, '^[ugoa]+%s*[-+=]%s*[rswx]+,*') then
      gsub(modestr, '%s*(%a+)%s*(.)%s*(%a+),*', function(who, op, what)
         local bits, bobs = 0, 0
         if match(who, '[ua]') then
            bits = bor(bits, S_ISUID, S_IRWXU)
         end
         if match(who, '[ga]') then
            bits = bor(bits, S_ISGID, S_IRWXG)
         end
         if match(who, '[oa]') then
            bits = bor(bits, S_IRWXO)
         end
         if match(what, 'r') then
            bobs = bor(bobs, S_IRUSR, S_IRGRP, S_IROTH)
         end
         if match(what, 'w') then
            bobs = bor(bobs, S_IWUSR, S_IWGRP, S_IWOTH)
         end
         if match(what, 'x') then
            bobs = bor(bobs, S_IXUSR, S_IXGRP, S_IXOTH)
         end
         if match(what, 's') then
            bobs = bor(bobs, S_ISUID, S_ISGID)
         end
         if op == '+' then
            -- mode |= bits & bobs
            mode = bor(mode, band(bits, bobs))
         elseif op == '-' then
            -- mode &= ~(bits & bobs)
            mode = band(mode, bnot(band(bits, bobs)))
         elseif op == '=' then
            -- mode =(mode & ~bits) |(bits & bobs)
            mode = bor(band(mode, bnot(bits)), band(bits, bobs))
         end
      end)
      return mode
   else
      return nil, 'bad mode'
   end
end


-- FIXME: specl-14.x breaks function environments here :(
local O_CREAT, stat, band, bnot, bor, chmod, mkdir, mkfifo, msgget, open, umask =
   O_CREAT, stat, band, bnot, bor, chmod, mkdir, mkfifo, msgget, open, umask

return {
   --- Change the mode of the path.
   -- @function chmod
   -- @string path existing file path
   -- @string mode one of the following formats:
   --
   --    * 'rwxrwxrwx' (e.g. 'rw-rw-r--')
   --    * 'ugo+-=rwx' (e.g. 'u+w')
   --    * '+-=rwx'    (e.g. '+w')
   --
   -- @return[1] int `0`, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   -- @see chmod(2)
   -- @usage chmod('bin/dof', '+x')
   chmod = argscheck('chmod(string, string)', function(path, modestr)
      local mode = (stat(path) or {}).st_mode
      local bits, err = mode_munch(mode or 0, modestr)
      if bits == nil then
         argerror('chmod', 2, err, 2)
      end
      return chmod(path, band(bits, RWXALL))
   end),

   --- Create a file.
   -- This function is obsoleted by @{posix.fcntl.open} with `posix.O_CREAT`.
   -- @function creat
   -- @string path name of file to create
   -- @string mode permissions with which to create file
   -- @treturn[1] int file descriptor of file at *path*, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   -- @see creat(2)
   -- @see posix.chmod
   -- @usage
   --    fd = creat('data', 'rw-r-----')
   creat = argscheck('creat(string, string)', function(path, modestr)
      local bits, err = mode_munch(0, modestr)
      if bits == nil then
         argerror('creat', 2, err, 2)
      end
      return open(path, FCREAT, band(bits, RWXALL))
   end),

   --- Make a directory.
   -- @function mkdir
   -- @string path location in file system to create directory
   -- @treturn[1] int `0`, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   mkdir = argscheck('mkdir(string)', function(path)
      return mkdir(path, RWXALL)
   end),

   --- Make a FIFO pipe.
   -- @function mkfifo
   -- @string path location in file system to create fifo
   -- @treturn[1] int `0`, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   mkfifo = argscheck('mkfifo(string)', function(path)
      return mkfifo(path, RWXALL)
   end),

   --- Get a message queue identifier
   -- @function msgget
   -- @int key message queue id, or `IPC_PRIVATE` for a new queue
   -- @int[opt=0] flags bitwise OR of zero or more from `IPC_CREAT` and `IPC_EXCL`
   -- @string[opt='rw-rw-rw-'] mode execute bits are ignored
   -- @treturn[1] int message queue identifier, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   -- @see msgget(2)
   msgget = argscheck('msgget(int, ?int, ?string)', function(key, msgflg, modestr)
      local bits, err = mode_munch(0, modestr)
      if bits == nil then
         argerror('msgget', 3, err, 2)
      end
      return msgget(key, bor(msgflg, band(bits, RWXALL)))
   end),

   --- Open a file.
   -- @function open
   -- @string path file to act on
   -- @int oflags bitwise OR of zero or more of `O_RDONLY`, `O_WRONLY`, `O_RDWR`,
   --    `O_APPEND`, `O_CREAT`, `O_DSYNC`, `O_EXCL`, `O_NOCTTY`, `O_NONBLOCK`,
   --    `O_RSYNC`, `O_SYNC`, `O_TRUNC`
   -- @string modestr(used with `O_CREAT`; see @{chmod} for format)
   -- @treturn[1] int file descriptor for *path*, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   -- @see open(2)
   -- @usage
   -- fd = posix.open('data', bit.bor(posix.O_CREAT, posix.O_RDWR), 'rw-r-----')
   open = argscheck('open(string, int, [string])', function(path, oflags, modestr)
      local bits
      if band(oflags, O_CREAT) ~= 0 then
         bits, err = mode_munch(0, modestr)
         if bits == nil then
            argerror('open', 3, err, 2)
         end
         bits = band(bits, RWXALL)
      end
      return open(path, oflags, bits)
   end),

   --- Set log priority mask
   -- @function setlogmask
   -- @int ... zero or more of `LOG_EMERG`, `LOG_ALERT`, `LOG_CRIT`,
   --    `LOG_WARNING`, `LOG_NOTICE`, `LOG_INFO` and `LOG_DEBUG`
   -- @treturn[1] int `0`, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   setlogmask = argscheck('setlogmask(?int...)', function(...)
      local mask = 0
      for _, v in ipairs {...} do
         mask = bor(mask, LOG_MASK(v))
      end
      return setlogmask(mask)
   end),

   --- Set file mode creation mask.
   -- @function umask
   -- @string[opt] mode file creation mask string
   -- @treturn string previous umask
   -- @see umask(2)
   -- @see posix.sys.stat.umask
   umask = argscheck('umask(?string)', function(modestr)
      modestr = modestr or ''
      local mode = umask(0)
      umask(mode)
      mode = band(bnot(mode), RWXALL)
      if modestr ~= '' then
         local bits, err = mode_munch(mode, modestr)
         if bits == nil then
            argerror('umask', 1, err, 2)
         end
         mode = band(bits, RWXALL)
         umask(bnot(mode))
      end
      return pushmode(mode)
   end),
}
