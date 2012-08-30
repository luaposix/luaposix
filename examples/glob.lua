require "posix"

for i, j in pairs (posix.glob ("/proc/[0-9]*/exe")) do
  local f = posix.readlink (j)
  if f then
    print (f)
  end
end
