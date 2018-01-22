#! /usr/bin/env lua

local M = require 'posix.sys.socket'


if M.AF_NETLINK ~= nil then
   local getpid = require 'posix.unistd'.getpid

   local fd, err = M.socket(M.AF_NETLINK, M.SOCK_DGRAM, M.NETLINK_KOBJECT_UEVENT)
   assert(fd, err)

   local ok, err = M.bind(fd, {family=M.AF_NETLINK, pid=getpid(), groups=-1})
   assert(ok, err)

   while true do
      local data, err = M.recv(fd, 16384)
      assert(data, err)
      for k, v in string.gmatch(data, '%z(%u+)=([^%z]+)') do
         print(k, v)
      end
      print '\n'
   end
end
