#! /usr/bin/env lua

local M = require 'posix.dirent'


local ok, files = pcall(M.dir, '/var/log')
if not ok then
   print('/var/log: ' .. files)
end

for _, f in ipairs(files) do
   print(f)
end
