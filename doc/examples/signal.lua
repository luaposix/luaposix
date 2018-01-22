#! /usr/bin/env lua

local M = require 'posix.signal'


local unistd = require 'posix.unistd'
local syswait = require 'posix.sys.wait'


local function go(fn, ...)
   local childpid = unistd.fork()
   if childpid == 0 then -- run function as child
      unistd._exit(fn(...) or 0)
   else
      return childpid
   end
end

local verbose = #arg > 0

local function sleepx(secs)
   while true do
      secs = unistd.sleep(secs)
      if verbose then
         print('sleep', secs)
      end
      if secs == 0 then
         return
      end
   end
end

local nchild, nsig = 0, 0

M.signal(M.SIGCHLD, function()
   local pid, status, code = syswait.wait(-1, syswait.WNOHANG)
   while pid do
      if pid ~= 0 then
         if verbose then
            print('wait', pid, status, code)
         end
         nchild = nchild + 1
      end
      pid, status, code = syswait.wait(-1, syswait.WNOHANG)
   end
end)

local function handler(signo)
   if verbose then
      print('handled', signo)
   end
   nsig = nsig + 1
end

M.signal(M.SIGUSR1, handler)
M.signal(M.SIGUSR2, handler)
M.signal(60, handler)

local function killp(nsig)
   return M.kill(unistd.getppid(), nsig)
end

c1 = go(function() unistd.sleep(1); killp(M.SIGUSR1); killp(M.SIGUSR2) end)
c2 = go(function() unistd.sleep(2); killp(M.SIGUSR2); end)
c3 = go(function() unistd.sleep(2); killp(M.SIGUSR1) end)

sleepx(3)

if verbose then
   print('children', nchild, 'signals', nsig)
else
   assert(nchild == 3)
   assert(nsig ==   4)
   print '+++ tests OK +++'
end
