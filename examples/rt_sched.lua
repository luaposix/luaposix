local p = require "posix"
--[[ This example tests sched_setscheduler() / sched_getscheduler()
-- The script must be run as privileged process (CAP_SYS_NICE on linux)
]]


local mypid = p.getpid'pid'

-- get sched params from ps(1)
local function get_ps_sched(pid)
	local fp = io.popen(('ps --no-headers -o "policy,rtprio" %s'):format(pid))
	local res, err = fp:read'*a'
	assert(res, err )
	fp:close()
	local policy, rtprio = res:match'([^%s]+)%s+([^%s]+)'
	assert(policy)
	assert(rtprio)
	return policy, rtprio
end


do -- Tests on own process

	-- get scheduler policy for own process
	local res, err = p.sched_getscheduler(0) -- 0 pid: own process
	assert(res == p.SCHED_OTHER)
	local res, err = p.sched_getscheduler() -- default pid: own process
	assert(res == p.SCHED_OTHER)

	local policy, rtprio = get_ps_sched(mypid)
	assert(policy== 'TS')
	assert(rtprio== '-')


	-- set realtime priority on own process : SCHED_FIFO
	local res, err = p.sched_setscheduler(0, p.SCHED_FIFO, 10 )
	assert(res, err)

	local policy, rtprio = get_ps_sched(mypid)
	assert(policy== 'FF')
	assert(rtprio== '10')

	local res, err = p.sched_getscheduler(0)
	assert(res == p.SCHED_FIFO)


	-- set realtime priority on own process SCHED_RR
	local res, err = p.sched_setscheduler(0, p.SCHED_RR, 11 )
	assert(res, err)

	local policy, rtprio = get_ps_sched(mypid)
	assert(policy== 'RR')
	assert(rtprio== '11')

	local res, err = p.sched_getscheduler(0)
	assert(res == p.SCHED_RR)


	-- no parameters: reset own process to normal priority :
	local res, err = p.sched_setscheduler()
	assert(res, err)

	local policy, rtprio = get_ps_sched(mypid)
	assert(policy== 'TS')
	assert(rtprio== '-')

	local res, err = p.sched_getscheduler(0)
	assert(res == p.SCHED_OTHER)
end


-- fork a child to check sched_setscheduler on other process
do
	local r,w = p.pipe()
	local cpid = p.fork()
	if cpid == 0 then
		-- child: block on pipe until parent is finshed
		p.close(w)  -- close unused write end
		local b = p.read(r,1)
		p._exit(0)
	end

	-- parent:
	p.close(r)

	do -- do tests
		-- get scheduler policy for child process
		local res, err = p.sched_getscheduler(cpid)
		assert(res == p.SCHED_OTHER)

		local policy, rtprio = get_ps_sched(cpid)
		assert(policy== 'TS')
		assert(rtprio== '-')


		-- set realtime priority on child process : SCHED_FIFO
		local res, err = p.sched_setscheduler(cpid, p.SCHED_FIFO, 10 )
		assert(res, err)

		local policy, rtprio = get_ps_sched(cpid)
		assert(policy== 'FF')
		assert(rtprio== '10')

		local res, err = p.sched_getscheduler(cpid)
		assert(res == p.SCHED_FIFO)


		-- set realtime priority on child process SCHED_RR
		local res, err = p.sched_setscheduler(cpid, p.SCHED_RR, 11 )
		assert(res, err)

		local policy, rtprio = get_ps_sched(cpid)
		assert(policy== 'RR')
		assert(rtprio== '11')

		local res, err = p.sched_getscheduler(cpid)
		assert(res == p.SCHED_RR)


		-- no parameters after pid: reset child process to normal priority :
		local res, err = p.sched_setscheduler(cpid)
		assert(res, err)

		local policy, rtprio = get_ps_sched(cpid)
		assert(policy== 'TS')
		assert(rtprio== '-')

		local res, err = p.sched_getscheduler(cpid)
		assert(res == p.SCHED_OTHER)
	end

	-- stop child
	p.write(w,"stop")
	p.close(w)
	p.wait(cpid)
end
