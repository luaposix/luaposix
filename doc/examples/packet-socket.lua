#! /usr/bin/env lua

local M = require 'posix.sys.socket'

-- Packet socket loopback test
-- Need CAP_NET_RAW, otherwise get "Operation not permitted"
if M.AF_PACKET ~= nil then
    local fd = assert(M.socket(M.AF_PACKET, M.SOCK_RAW, 0))
    assert(M.bind(fd, {family=M.AF_PACKET, ifindex=M.if_nametoindex("lo")}))
    assert(M.send(fd, string.rep("1", 64)) == 64)
    require 'posix.unistd'.close(fd)
end
