#! /usr/bin/env lua

-- limit.lua
-- Limiting the CPU time used by a child process;
-- it will be killed and we don't get the final message

local M = require 'posix.sys.resource'


local times = require 'posix.sys.times'.times

M.setrlimit (M.RLIMIT_CPU, {rlim_cur=1, rlim_max=1})

local t = times().elapsed

local pid = require 'posix.unistd'.fork ()
if pid == 0 then -- child
   print 'start'
   for i = 1, 1e9 do
   end
   print 'finish'
else
   print (require 'posix.sys.wait'.wait (pid))
   print (times().elapsed - t)
end
