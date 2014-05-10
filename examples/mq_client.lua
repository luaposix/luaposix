
--
-- Example for POSIX message queues. Put messages in a queue that will be
-- read by mq_server.lua
--

local posix = require("posix")

local q, errmsg, errno = posix.mq_open("/luaposix-ex-server",
    posix.O_WRONLY + posix.O_CREAT, "rw-------")
assert(q, errmsg)

local att, errmsg, errno = posix.mq_getattr(q)
assert(att, errmsg)

print("Message queue " .. tostring(q) .. " opened as O_WRONLY, properties:")
for k, v in pairs(att) do
    print(k, v)
end

print('Type any message to send, ".quit" to finish, ".stop" to stop server')
while true do
    io.write("> ")
    local msg = io.stdin:read("*l")
    if not msg or msg == ".quit" then
        break
    end
    local ret, err = posix.mq_send(q, msg, 0)
    if ret ~= 0 then
        print("Error sending the message: " .. err)
    end
    if msg == ".stop" then
        break
    end
end

print("mq_close", posix.mq_close(q))
