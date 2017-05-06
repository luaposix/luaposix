local dirent = require "posix.dirent"

local ok, files = pcall (dirent.dir "/var/log")
if not ok then
  print ("/var/log: " .. files)
end

for _, f in ipairs (files) do
  print (f)
end
