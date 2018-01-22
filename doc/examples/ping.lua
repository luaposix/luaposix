#! /usr/bin/env lua

local M = require 'posix.sys.socket'


if M.SOCK_RAW and M.SO_BINDTODEVICE then
   -- Open raw socket

   local fd, err = M.socket(M.AF_INET, M.SOCK_RAW, M.IPPROTO_ICMP)
   assert(fd, err)

   -- Optionally, bind to specific device

   local ok, err = M.setsockopt(fd, M.SOL_SOCKET, M.SO_BINDTODEVICE, 'wlan0')
   assert(ok, err)

   -- Create raw ICMP echo (ping) message

   local data = string.char(0x08, 0x00, 0x89, 0x98, 0x6e, 0x63, 0x00, 0x04, 0x00)

   -- Send message

   local ok, err = M.sendto(fd, data, {family=M.AF_INET, addr='8.8.8.8', port=0})
   assert(ok, err)

   -- Read reply

   local data, sa = M.recvfrom(fd, 1024)
   assert(data, sa)

   if data then
      print('Received ICMP message from ' .. sa.addr)
   end
end
