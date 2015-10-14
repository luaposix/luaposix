#!/usr/bin/env lua

local socket = require "posix.sys.socket"

local AF_UNIX, SOCK_DGRAM = socket.AF_UNIX, socket.SOCK_DGRAM

local bind   = socket.bind
local recv   = socket.recv
local socket = socket.socket


-- Create an AF_UNIX datagram socket
local s, errmsg = socket (AF_UNIX, SOCK_DGRAM, 0)
assert (s ~= nil, errmsg)

-- Bind to the abtract AF_UNIX name "mysocket"
local rc, errmsg = bind (s, { family = AF_UNIX, path = "\0mysocket" })
assert (rc == 0, errmsg)

-- Receive datagrams on the socket and print out the contents
local dgram
while true do
  dgram, errmsg = recv (s, 1024)
  assert (dgram ~= nil, errmsg)
  print ("Got packet: [" .. tostring (dgram) .. "]")
end
