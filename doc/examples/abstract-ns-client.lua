#!/usr/bin/env lua

local socket = require "posix.sys.socket"

local AF_UNIX, SOCK_DGRAM = socket.AF_UNIX, socket.SOCK_DGRAM

local sendto = socket.sendto
local socket = socket.socket


local dgram = arg[1] or "test data"

-- Create an AF_UNIX datagram socket
local s, errmsg = socket (AF_UNIX, SOCK_DGRAM, 0)
assert (s ~= nil, errmsg)

-- Sendto the abtract AF_UNIX name "mysocket"
local rc, errmsg = sendto (s, dgram, { family = AF_UNIX, path = "\0mysocket" })
assert (rc ~= nil, errmsg)
