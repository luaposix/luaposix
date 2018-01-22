#! /usr/bin/env lua

--[[
 This example tests sched_setscheduler() / sched_getscheduler()
 The script must be run as privileged process (CAP_SYS_NICE on linux)
]]

local M = require 'posix.sched'


local mypid = require 'posix.unistd'.getpid()

-- get sched params from ps(1)
local function get_ps_sched(pid)
   local fp = io.popen(string.format('ps --no-headers -o "policy,rtprio" %s', pid))
   local res, err = fp:read'*a'
   assert(res, err)
   fp:close()
   local policy, rtprio = string.match(res, '([^%s]+)%s+([^%s]+)')
   assert(policy)
   assert(rtprio)
   return policy, rtprio
end


do -- Tests on own process

   -- get scheduler policy for own process
   local res, err = M.sched_getscheduler(0) -- 0 pid: own process
   assert(res == M.SCHED_OTHER)
   local res, err = M.sched_getscheduler() -- default pid: own process
   assert(res == M.SCHED_OTHER)

   local policy, rtprio = get_ps_sched(mypid)
   assert(policy== 'TS')
   assert(rtprio== '-')


   -- set realtime priority on own process : SCHED_FIFO
   local res, err = M.sched_setscheduler(0, p.SCHED_FIFO, 10 )
   assert(res, err)

   local policy, rtprio = get_ps_sched(mypid)
   assert(policy == 'FF')
   assert(rtprio == '10')

   local res, err = M.sched_getscheduler(0)
   assert(res == M.SCHED_FIFO)


   -- set realtime priority on own process SCHED_RR
   local res, err = M.sched_setscheduler(0, M.SCHED_RR, 11 )
   assert(res, err)

   local policy, rtprio = get_ps_sched(mypid)
   assert(policy == 'RR')
   assert(rtprio == '11')

   local res, err = M.sched_getscheduler(0)
   assert(res == M.SCHED_RR)


   -- no parameters: reset own process to normal priority :
   local res, err = M.sched_setscheduler()
   assert(res, err)

   local policy, rtprio = get_ps_sched(mypid)
   assert(policy == 'TS')
   assert(rtprio == '-')

   local res, err = M.sched_getscheduler(0)
   assert(res == M.SCHED_OTHER)
end


-- fork a child to check sched_setscheduler on other process
do
   local U = require 'posix.unistd'

   local r, w = U.pipe()
   local childpid = U.fork()
   if childpid == 0 then
      -- child: block on pipe until parent is finshed
      U.close(w)  -- close unused write end
      local b = U.read(r,1)
      U._exit(0)
   end

   -- parent:
   U.close(r)

   do -- do tests
      -- get scheduler policy for child process
      local res, err = M.sched_getscheduler(childpid)
      assert(res == M.SCHED_OTHER)

      local policy, rtprio = get_ps_sched(childpid)
      assert(policy == 'TS')
      assert(rtprio == '-')


      -- set realtime priority on child process : SCHED_FIFO
      local res, err = M.sched_setscheduler(childpid, M.SCHED_FIFO, 10 )
      assert(res, err)

      local policy, rtprio = get_ps_sched(childpid)
      assert(policy == 'FF')
      assert(rtprio == '10')

      local res, err = M.sched_getscheduler(childpid)
      assert(res == M.SCHED_FIFO)


      -- set realtime priority on child process SCHED_RR
      local res, err = M.sched_setscheduler(childpid, M.SCHED_RR, 11 )
      assert(res, err)

      local policy, rtprio = get_ps_sched(childpid)
      assert(policy == 'RR')
      assert(rtprio == '11')

      local res, err = M.sched_getscheduler(childpid)
      assert(res == M.SCHED_RR)


      -- no parameters after pid: reset child process to normal priority :
      local res, err = M.sched_setscheduler(childpid)
      assert(res, err)

      local policy, rtprio = get_ps_sched(childpid)
      assert(policy == 'TS')
      assert(rtprio == '-')

      local res, err = M.sched_getscheduler(childpid)
      assert(res == M.SCHED_OTHER)
   end

   -- stop child
   U.write(w, 'stop')
   U.close(w)
   require 'posix.sys.wait'.wait(childpid)
end
