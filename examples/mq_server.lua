
--
-- Example for POSIX message queues. Print messages send by mq_client.lua
--

local posix = require("posix")

local q, errmsg, errno = posix.mq_open("/luaposix-ex-server",
    posix.O_RDONLY + posix.O_CREAT, "rw-------")
assert(q, errmsg)

local att, errmsg, errno = posix.mq_getattr(q)
assert(att, errmsg)

print("Message queue " .. tostring(q) .. " opened as O_RDONLY, properties:")
for k, v in pairs(att) do
    print(k, v)
end

print("Waiting for messages")
while true do
    local msg, prio_err, errno = posix.mq_receive(q, att.mq_msgsize)
    -- For a timed wait, use the following example.
    --local msg, prio_err, errno = posix.mq_timedreceive(q, att.mq_msgsize, os.time()+5, 0)
    if msg == ".stop" then
        print('".stop" received, finishing server')
        break
    elseif msg then
        print("Message received with priority " .. tostring(prio_err) .. ": ")
        print(msg)
    else
        print("Error received: " .. prio_err)
    end
end

print("mq_close", posix.mq_close(q))
print("mq_unlink", posix.mq_unlink("/luaposix-ex-server"))
