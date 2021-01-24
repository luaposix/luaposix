--[[
 POSIX library for Lua 5.1, 5.2, 5.3 & 5.4.
 Copyright (C) 2013-2021 Gary V. Vaughan
 Copyright (C) 2010-2013 Reuben Thomas <rrt@sc3d.org>
 Copyright (C) 2008-2010 Natanael Copa <natanael.copa@gmail.com>
]]
--[[--
 Lua POSIX bindings.

 In addition to the convenience functions documented in this module, many
 APIs from submodules are copied into the return table for backwards
 compatibility and, if necessary, wrapped to match the deprecated API.

 This means that your old code will continue to work, but that you can
 use the improved documented APIs in new code.

 @module posix
]]



local _ENV = require 'posix._strict' {
   GLOB_MARK = require 'posix.glob'.GLOB_MARK,
   O_NOCTTY = require 'posix.fcntl'.O_NOCTTY,
   O_RDWR = require 'posix.fcntl'.O_RDWR,
   STDIN_FILENO = require 'posix.unistd'.STDIN_FILENO,
   STDOUT_FILENO = require 'posix.unistd'.STDOUT_FILENO,
   _exit = require 'posix.unistd'._exit,
   access = require 'posix.unistd'.access,
   argscheck = require 'posix._base'.argscheck,
   assert = assert,
   bor = require 'posix._base'.bor,
   close = require 'posix.unistd'.close,
   dup2 = require 'posix.unistd'.dup2,
   errno = require 'posix.errno'.errno,
   error = error,
   execp = require 'posix.unistd'.execp,
   exit = os.exit,
   fork = require 'posix.unistd'.fork,
   getegid = require 'posix.unistd'.getegid,
   geteuid = require 'posix.unistd'.geteuid,
   getgid = require 'posix.unistd'.getgid,
   getuid= require 'posix.unistd'.getuid,
   glob = require 'posix.glob'.glob,
   grantpt = require 'posix.stdlib'.grantpt,
   gsub = string.gsub,
   insert = table.insert,
   match = string.match,
   next = next,
   open = require 'posix.fcntl'.open,
   openpt = require 'posix.stdlib'.openpt,
   pcall = pcall,
   pipe = require 'posix.unistd'.pipe,
   ptsname = require 'posix.stdlib'.ptsname,
   rawset = rawset,
   remove = table.remove,
   require = require,
   set_errno = require 'posix.errno'.set_errno,
   setmetatable = setmetatable,
   stat = require 'posix.sys.stat'.stat,
   sub = string.sub,
   tonumber = tonumber,
   type = type,
   unlockpt = require 'posix.stdlib'.unlockpt,
   wait = require 'posix.sys.wait'.wait,
}


-- FIXME: specl-14.x breaks function environments here :(
local GLOB_MARK, STDIN_FILENO, STDOUT_FILENO, _exit, close, errno, execp, exit, fork, glob, pipe, wait =
   GLOB_MARK, STDIN_FILENO, STDOUT_FILENO, _exit, close, errno, execp, exit, fork, glob, pipe, wait


local function Peuidaccess(file, mode)
   local euid, egid = geteuid(), getegid()

   if getuid() == euid and getgid() == egid then
      -- If we are not set-uid or set-gid, access does the same.
      return access(file, mode)
   end

   local stats = stat(file)
   if not stats then
      return
   end

   -- The super-user can read and write any file, and execute any file
   -- that anyone can execute.
   if euid == 0 and ((not match(mode, 'x')) or match(stats.st_mode, 'x')) then
      return 0
   end

   -- Convert to simple list of modes.
   mode = gsub(mode, '[^rwx]', '')

   if mode == '' then
      return 0 -- The file exists.
   end

   -- Get the modes we need.
   local granted = sub(stats.st_mode, 1, 3)
   if euid == stats.st_uid then
      granted = sub(stats.st_mode, 7, 9)
   elseif egid == stats.st_gid or set.new(posix.getgroups()):member(stats.st_gid) then
      granted = sub(stats.st_mode, 4, 6)
   end
   granted = gsub(granted, '[^rwx]', '')

   if gsub('[^' .. granted .. ']', mode) == '' then
      return 0
   end
   set_errno(EACCESS)
end


local function Pexecx(task, ...)
   if type(task) == 'table' then
      local path = remove(task, 1)
      execp(path, task)
      -- Only get here if there's an error; kill the fork
      local _, n = errno()
      _exit(n)
   else
      -- Function call must flush open file descriptors et.al. with `os.exit()`
      exit(task(...) or 0)
   end
end


local function Pglob(args)
   -- Support previous `glob '.*'` style calls.
   if type(args) == 'string' then
      args = {pattern=args}
   elseif type(args) ~= 'table' then
      args = {}
   end
   local flags = 0
   if args.MARK then
      flags = GLOB_MARK
   end
   return glob(args.pattern, flags)
end


local function Popenpty(term, win)
   local ok, slave, slave_name
   local master, errmsg, errnum = openpt(bor(O_RDWR, O_NOCTTY))
   if master then
      ok, errmsg, errnum = grantpt(master)
      if ok then
         ok, errmsg, errnum = unlockpt(master)
         if ok then
            slave_name, errmsg, errnum = ptsname(master)
            if slave_name then
               slave, errmsg, errnum = open(slave_name, bor(O_RDWR, O_NOCTTY))
               if slave then
                  return master, slave, slave_name
               end
            end
         end
      end
      close(master)
   end
   return nil, errmsg, errnum
end


local function Ppclose(pfd)
   close(pfd.fd)
   for i = 1, #pfd.pids - 1 do
      wait(pfd.pids[i])
   end
   local _, reason, status = wait(pfd.pids[#pfd.pids])
   return reason, status
end


local function move_fd(from_fd, to_fd)
   if from_fd ~= to_fd then
      if not dup2(from_fd, to_fd) then
         error 'error dup2-ing'
      end
      close(from_fd)
   end
end


local function Ppopen(task, mode, pipe_fn)
   local read_fd, write_fd = (pipe_fn or pipe)()
   if not read_fd then
      error 'error opening pipe'
   end
   local parent_fd, child_fd, in_fd, out_fd
   if mode == 'r' then
      parent_fd, child_fd, in_fd, out_fd = read_fd, write_fd, STDIN_FILENO, STDOUT_FILENO
   elseif mode == 'w' then
      parent_fd, child_fd, in_fd, out_fd = write_fd, read_fd, STDOUT_FILENO, STDIN_FILENO
   else
      error 'invalid mode'
   end
   local pid = fork()
   if pid == nil then
      error 'error forking'
   elseif pid == 0 then -- child process
      move_fd(child_fd, out_fd)
      close(parent_fd)
      _exit(Pexecx(task, child_fd, in_fd, out_fd))
   end -- parent process
   close(child_fd)
   return {pids={pid}, fd=parent_fd}
end


local function Ppopen_pipeline(tasks, mode, pipe_fn)
   local first, from, to, inc = 1, 2, #tasks, 1
   if mode == 'w' then
      first, from, to, inc = #tasks, #tasks - 1, 1, -1
   end
   local pfd = Ppopen(tasks[first], mode, pipe_fn)
   for i = from, to, inc do
      local pfd_next = Ppopen(function(fd, in_fd, out_fd)
         move_fd(pfd.fd, in_fd)
         _exit(Pexecx(tasks[i]))
      end,
      mode,
      pipe_fn)
      close(pfd.fd)
      pfd.fd = pfd_next.fd
      insert(pfd.pids, pfd_next.pids[1])
   end
   return pfd
end


local function Pspawn(task, ...)
   local pid, err = fork()
   if pid == nil then
      return pid, err
   elseif pid == 0 then
      Pexecx(task, ...)
   else
      local _, reason, status = wait(pid)
      return status, reason -- If wait failed, status is nil & reason is error
   end
end


local function Ptimeradd(x, y)
   local sec, usec = 0, 0
   if x.tv_sec or x.tv_usec then
      sec = sec + (tonumber(x.tv_sec) or 0)
      usec = usec + (tonumber(x.tv_usec) or 0)
   else
      sec = sec + (tonumber(x.sec) or 0)
      usec = usec + (tonumber(x.usec) or 0)
   end
   if y.tv_sec or y.tv_usec then
      sec = sec + (tonumber(y.tv_sec) or 0)
      usec = usec + (tonumber(y.tv_usec) or 0)
   else
      sec = sec + (tonumber(y.sec) or 0)
      usec = usec + (tonumber(y.usec) or 0)
   end
   while usec > 1000000 do
      sec = sec + 1
      usec = usec - 1000000
   end

   return {sec=sec, usec=usec}
end


local function Ptimercmp(x, y)
   local x = {sec=x.tv_sec or x.sec or 0, usec=x.tv_usec or x.usec or 0}
   local y = {sec=y.tv_sec or y.sec or 0, usec=y.tv_usec or y.usec or 0}
   if x.sec ~= y.sec then
      return x.sec - y.sec
   else
      return x.usec - y.usec
   end
end


local function Ptimersub(x,y)
   local sec, usec = 0, 0
   if x.tv_sec or x.tv_usec then
      sec = (tonumber(x.tv_sec) or 0)
      usec = (tonumber(x.tv_usec) or 0)
   else
      sec = (tonumber(x.sec) or 0)
      usec = (tonumber(x.usec) or 0)
   end
   if y.tv_sec or y.tv_usec then
      sec = sec - (tonumber(y.tv_sec) or 0)
      usec = usec - (tonumber(y.tv_usec) or 0)
   else
      sec = sec - (tonumber(y.sec) or 0)
      usec = usec - (tonumber(y.usec) or 0)
   end
   while usec < 0 do
      sec = sec - 1
      usec = usec + 1000000
   end
   return { sec = sec, usec = usec }
end


-- For backwards compatibility, copy all table entries into M namespace.
local M = {}
do
   local names = {
      'ctype', 'dirent', 'errno', 'fcntl', 'fnmatch', 'glob', 'grp',
      'libgen', 'poll', 'pwd', 'sched', 'signal', 'stdio', 'stdlib', 'sys.msg',
      'sys.resource', 'sys.socket', 'sys.stat', 'sys.statvfs', 'sys.time',
      'sys.times', 'sys.utsname', 'sys.wait', 'syslog', 'termio', 'time',
      'unistd', 'utime'
   }
   for i = 1, #names do
      local name = names[i]
      local t = require('posix.' .. name)
      for k, v in next, t do
         if k ~= 'version' then
            assert(M[k] == nil, 'posix namespace clash: ' .. name .. '.' .. k)
            M[k] = v
         end
      end
   end

   -- Inject deprecated APIs (overwriting submodules) for backwards compatibility.
   for k, v in next, (require 'posix.deprecated') do
      M[k] = v
   end
   for k, v in next, (require 'posix.compat') do
      M[k] = v
   end
end


local function merge(t, r)
   for k, v in next, t do
      r[k] = r[k] or v
   end
   return r
end


return setmetatable(merge(M, {
   --- Close a pipeline opened with popen or popen_pipeline.
   -- @function pclose
   -- @tparam table pfd pipeline object
   -- @return values as for @{posix.sys.wait.wait}, for the last (or only)
   --  stage of the pipeline
   pclose = argscheck('pclose(table)', Ppclose),

   --- Check permissions like @{posix.unistd.access}, but for euid.
   -- Based on the glibc function of the same name. Does not always check
   -- for read-only file system, text busy, etc., and does not work with
   -- ACLs &c.
   -- @function euidaccess
   -- @string file file to check
   -- @string mode checks to perform (as for access)
   -- @treturn[1] int `0`, if access allowed
   -- @return[2] nil (and errno is set)
   euidaccess = argscheck('euidaccess(string, string)', Peuidaccess),

   --- Exec a command or Lua function.
   -- @function execx
   -- @tparam table|function task argument list for @{posix.unistd.execp}
   --  or a Lua function, which should read from standard input, write to
   --  standard output, and return an exit code
   -- @param ... positional arguments to the function
   -- @treturn nil on error (normally does not return)
   -- @treturn string error message
   execx = argscheck('execx(function|table, ?any...)', Pexecx),

   --- Find all files in this directory matching a shell pattern.
   -- The flag MARK appends a trailing slash to matches that are directories.
   -- @function glob
   -- @tparam table|string|nil args the three possible parameters can be used to
   -- override the default values for `pattern` and `MARK`, which are `"*"` and
   -- `false` respectively. A table will be checked for optional keys `pattern`
   -- and `MARK`. A string will be used as the glob pattern.
   -- @treturn[1] table matching files and directories, if successful
   -- @return[2] nil
   -- @treturn[2] one of `GLOB_BORTED`, `GLOB_NOMATCH` or `GLOB_NOSPACE`
   -- @see posix.glob.glob
   glob = argscheck('glob(?string|table)', Pglob),

   --- Open a pseudo-terminal.
   -- Based on the glibc function of the same name.
   -- @function openpty
   -- @fixme add support for term and win arguments
   -- @treturn[1] int master file descriptor
   -- @treturn[1] int slave file descriptor
   -- @treturn[1] string slave file name, if successful
   -- @return[2] nil
   -- @treturn[2] string error message
   -- @treturn[2] int errnum
   openpty = argscheck('openpty()', Popenpty),

   --- Run a command or Lua function in a sub-process.
   -- @function popen
   -- @tparam table task argument list for @{posix.unistd.execp} or a Lua
   --  function, which should read from standard input, write to standard
   --  output, and return an exit code
   -- @tparam string mode `"r"` for read or `"w"` for write
   -- @func[opt] pipe_fn function returning a paired read and
   --  write file descriptor (*default* @{posix.unistd.pipe})
   -- @treturn pfd pipeline object
   -- @see posix.execx
   -- @see posix.spawn
   popen = argscheck('popen(function|table, string, ?function)', Ppopen),

   --- Perform a series of commands and Lua functions as a pipeline.
   -- @function popen_pipeline
   -- @tparam table t tasks for @{posix.execx}
   -- @tparam string mode `"r"` for read or `"w"` for write
   -- @func[opt] pipe_fn function returning a paired read and
   --  write file descriptor (*default* @{posix.unistd.pipe})
   -- @treturn pfd pipeline object
   popen_pipeline = argscheck('popen_pipeline(function|table, string, ?function)',
      Ppopen_pipeline),

   --- Run a command or function in a sub-process using @{posix.execx}.
   -- @function spawn
   -- @tparam table task argument list for @{posix.unistd.execp} or a Lua
   --  function, which should read from standard input, write to standard
   --  output, and return an exit code
   -- @param ... as for @{posix.execx}
   -- @return values as for @{posix.sys.wait.wait}
   spawn = argscheck('spawn(function|table, ?any...)', Pspawn),

   --- Add one gettimeofday() returned timeval to another.
   -- @function timeradd
   -- @param x a timeval
   -- @param y another timeval
   -- @return x + y, adjusted for usec overflow
   timeradd = argscheck('timeradd(table, table)', Ptimeradd),

   --- Compare one gettimeofday() returned timeval with another
   -- @function timercmp
   -- @param x a timeval
   -- @param y another timeval
   -- @return 0 if x and y are equal, >0 if x is newer, <0 if y is newer
   timercmp = argscheck('timercmp(table, table)', Ptimercmp),

   --- Subtract one gettimeofday() returned timeval from another.
   -- @function timersub
   -- @param x a timeval
   -- @param y another timeval
   -- @return x - y, adjusted for usec underflow
   timersub = argscheck('timersub(table, table)', Ptimersub),

}), {
   --- Metamethods
   -- @section metamethods

   --- Lazy loading of luaposix modules.
   -- Don't load everything on initial startup, wait until first attempt
   -- to access a submodule, and then load it on demand.
   -- @function __index
   -- @string name submodule name
   -- @treturn table|nil the submodule that was loaded to satisfy the missing
   --  `name`, otherise `nil` if nothing was found
   -- @usage
   -- local version = require 'posix'.version
   __index = function(self, name)
      local ok, t = pcall(require, 'posix.' ..name)
      if ok then
         rawset(self, name, t)
         return t
      end
   end,
})

