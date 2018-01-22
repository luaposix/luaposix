#!/usr/bin/env lua

local M = require 'posix.sys.socket'


local dgram = arg[1] or 'test data'

-- Create an AF_UNIX datagram socket
local s, errmsg = M.socket(M.AF_UNIX, M.SOCK_DGRAM, 0)
assert(s ~= nil, errmsg)

-- Sendto the abtract AF_UNIX name 'mysocket'
local rc, errmsg = M.sendto(s, dgram, {family=M.AF_UNIX, path='\0mysocket'})
assert(rc ~= nil, errmsg)
