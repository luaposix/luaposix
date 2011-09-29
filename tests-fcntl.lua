require "lunit"
require "bit"

module("test_fcntl", lunit.testcase, package.seeall)

function test_sane_getfl()
   require "posix"
   local fd = posix.fileno(io.stdin)
   local flags = posix.getfl(fd)
   assert_number(flags)
   assert(flags >= 0, "returned flags are negative")
end

function test_setfl_works()
   require "posix"
   local fd = posix.fileno(io.stdin)
   local flags = posix.getfl(fd)
   -- Remove NONBLOCK, if any
   posix.setfl(fd, bit.band(flags, bit.bnot(posix.O_NONBLOCK)))
   flags = posix.getfl(fd)
   assert(bit.band(flags, posix.O_NONBLOCK) == 0, "Removal of O_NONBLOCK failed")
   posix.setfl(fd, bit.bor(flags, posix.O_NONBLOCK))
   flags = posix.getfl(fd)
   assert(bit.band(flags, posix.O_NONBLOCK) ~= 0, "Set of O_NONBLOCK failed")
end

function test_negative_fd_fails()
   require "posix"
   ret, msg, errno = posix.getfl(-7)
   assert_nil(ret)
end

function test_nonopen_fd_fails()
   require "posix"
   ret, msg, errno = posix.getfl(666)
   assert_nil(ret)
end

function test_wrong_userdata_fails()
   require "posix"
   assert_error("Passing wrong type instead of fd does not bomb out",
      function()
         posix.getfl("foobar")
      end)
end
