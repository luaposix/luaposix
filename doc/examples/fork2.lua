#! /usr/bin/env lua

local M = require 'posix.unistd'


print ('parent: my pid is: ' .. M.getpid())

local childpid = M.fork ()

if childpid == -1 then
   print('parent: The fork failed.')
elseif childpid == 0 then
   print('child: Hello World! I am pid: ' .. M.getpid())
   print("child: I'll sleep for 1 second ... ")
   M.sleep(1)
   print('child: Good bye');
else
   print("parent: While the child sleeps, I'm still running.")
   print('parent: waiting for child (pid:' .. childpid .. ') to die...')
   require 'posix.sys.wait'.wait(childpid)
   print("parent: child died, but I'm still alive.")
   print('parent: Good bye')
end
