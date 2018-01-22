#! /usr/bin/env lua

local M = require 'posix.fcntl'


local S = require 'posix.sys.stat'


local fd = M.open(
   'file.txt',
   M.O_CREAT + M.O_WRONLY + M.O_TRUNC,
   S.S_IRUSR + S.S_IWUSR + S.S_IRGRP + S.S_IROTH
)

-- Set lock on file
local lock = {
   l_type = M.F_WRLCK;     -- Exclusive lock
   l_whence = M.SEEK_SET;  -- Relative to beginning of file
   l_start = 0;            -- Start from 1st byte
   l_len = 0;              -- Lock whole file
}

if M.fcntl(fd, M.F_SETLK, lock) == -1 then
   error('file locked by another process')
end

-- Do something with file while it's locked
require 'posix.unistd'.write(fd, 'Lorem ipsum\n')

-- Release the lock
lock.l_type = M.F_UNLCK
M.fcntl(fd, M.F_SETLK, lock)
