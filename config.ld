-- -*- lua -*-
file = 'lposix.c'
project = 'luaposix'
description = 'POSIX bindings for Lua'

tparam_alias 'string'
tparam_alias 'int'
tparam_alias 'bool'

custom_see_handler('^([%w_]+)%((%d)$',function(name,section)
    local url = ("http://www.kernel.org/doc/man-pages/online/pages/man%s/%s.%s.html"):format(section,name,section)
    local name = name .. '(' ..section..')'
    return name, url
end)

examples = 'examples'

format = 'discount'

