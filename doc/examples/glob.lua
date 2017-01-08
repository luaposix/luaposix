local posix = require "posix"

for i, j in pairs (posix.glob ("/proc/[0-9]*/exe")) do
  print(j)
end
