#! /usr/bin/env lua

local glob = require 'posix.glob'.glob


for _, j in pairs(glob('/proc/[0-9]*/exe', 0)) do
   print(j)
end
