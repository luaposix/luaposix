lunit = require "lunit"
bit = bit32 or require "bit"

module("test_fcntl", lunit.testcase, package.seeall)

function test_sane_getfl()
   local posix = require "posix"
   local fd = posix.fileno(io.stdin)
   local flags = posix.fcntl(fd, posix.F_GETFL)
   assert_number(flags)
   assert(flags >= 0, "returned flags are negative")
end

function test_setfl_works()
   local posix = require "posix"
   local fd = posix.fileno(io.stdin)
   local flags = posix.fcntl(fd, posix.F_GETFL)
   -- Remove NONBLOCK, if any
   posix.fcntl(fd, posix.F_SETFL, bit.band(flags, bit.bnot(posix.O_NONBLOCK)))
   flags = posix.fcntl(fd, posix.F_GETFL)
   assert(bit.band(flags, posix.O_NONBLOCK) == 0, "Removal of O_NONBLOCK failed")
   posix.fcntl(fd, posix.F_SETFL, bit.bor(flags, posix.O_NONBLOCK))
   flags = posix.fcntl(fd, posix.F_GETFL)
   assert(bit.band(flags, posix.O_NONBLOCK) ~= 0, "Set of O_NONBLOCK failed")
end

function test_negative_fd_fails()
   local posix = require "posix"
   ret, msg, errno = posix.fcntl(-7, posix.F_GETFL)
   assert_nil(ret)
end

function test_nonopen_fd_fails()
   local posix = require "posix"
   ret, msg, errno = posix.fcntl(666, posix.F_GETFL)
   assert_nil(ret)
end

function test_wrong_userdata_fails()
   local posix = require "posix"
   assert_error("Passing wrong type instead of fd does not bomb out",
      function()
         posix.fcntl("foobar", posix.F_GETFL)
      end)
end
