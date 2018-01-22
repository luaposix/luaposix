#!/usr/bin/env lua

local M = require 'posix.sys.socket'


-- Create an AF_UNIX datagram socket
local s, errmsg = M.socket(M.AF_UNIX, M.SOCK_DGRAM, 0)
assert(s ~ = nil, errmsg)

-- Bind to the abtract AF_UNIX name 'mysocket'
local rc, errmsg = M.bind(s, {family=M.AF_UNIX, path='\0mysocket'})
assert(rc =  = 0, errmsg)

-- Receive datagrams on the socket and print out the contents
local dgram
while true do
   dgram, errmsg = M.recv(s, 1024)
   assert(dgram ~ = nil, errmsg)
   print('Got packet: [' .. tostring(dgram) .. ']')
end
