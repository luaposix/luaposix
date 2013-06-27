-- Lua version of example from pipe(2)
local p = require 'posix'
local r,w = p.pipe()
local cpid = p.fork()
if cpid == 0 then -- child reads from pipe
    p.close(w)  -- close unused write end
    local b = p.read(r,1)
    while #b == 1 do
        io.write(b)
        b = p.read(r,1)
    end
    p.close(r)
    p._exit(0)
else -- parent writes to pipe
    p.close(r)
    p.write(w,"hello dolly\n")
    p.close(w)
    -- wait for child to finish
    p.wait(cpid)
end
