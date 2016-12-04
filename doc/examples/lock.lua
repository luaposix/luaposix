local p = require "posix"
local fd = p.creat("file.txt", "rw-r--r--")

-- Set lock on file
local lock = {
    l_type = p.F_WRLCK;     -- Exclusive lock
    l_whence = p.SEEK_SET;  -- Relative to beginning of file
    l_start = 0;            -- Start from 1st byte
    l_len = 0;              -- Lock whole file
  }
local result = p.fcntl(fd, p.F_SETLK, lock)
if result == -1 then
  error("file locked by another process")
end

-- Do something with file while it's locked
p.write(fd, "Lorem ipsum\n")

-- Release the lock
lock.l_type = p.F_UNLCK
p.fcntl(fd, p.F_SETLK, lock)
