#! /usr/bin/env lua

local M = require 'posix.unistd'


local function pipe()
   local r, w = M.pipe()
   assert(r ~= nil, w)
   return r, w
end

local stdout_r, stdout_w = pipe()
local stderr_r, stderr_w = pipe()

local pid, errmsg = M.fork()
assert(pid ~= nil, errmsg)

if pid == 0 then
   -- Child Process:

   M.close(stdout_r)
   M.close(stderr_r)

   M.dup2(stdout_w, M.STDOUT_FILENO)
   M.dup2(stderr_w, M.STDERR_FILENO)

   -- Exec() a subprocess here instead if you like --

   io.stdout:write 'output string'
   io.stderr:write 'oh noes!'
   os.exit(42)

else
   -- Parent Process:

   M.close(stdout_w)
   M.close(stderr_w)

   local function read(msg, fd)
      local outs, errmsg = M.read(fd, 1024)
      assert(outs ~= nil, errmsg)
      print(msg, outs)
   end

   read('STDOUT:', stdout_r)
   read('STDERR:', stderr_r)


   local childpid, reason, status = require 'posix.sys.wait'.wait(pid)
   assert(childpid ~= nil, reason)
   print('child ' .. reason, status)
end
