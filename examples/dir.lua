local posix = require "posix"

files, errstr, errno = posix.dir("/var/log")
if files then
  for a,b in ipairs(files) do
    print(b)
  end
else
  print(errstr)
end
