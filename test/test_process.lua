local can = require 'vega.can'
local common = require 'common'

local Process = class(common.Test)

function Process:testAsync()
    local process = can.process(">&2 echo stderr; echo stdout")
    process:start()
    flow():sleep{msec=100}
    assert(process:read(can.Process.Out):find('stdout'))
    assert(process:read(can.Process.Err):find('stderr'))
    process:join()
end

function Process:testWrite()
    local process = can.process("string=`cat`; echo $string")
    local pile = can.pile()
    pile:write("string")
    process:write(pile)
    assert(process:read():find('string'))
    process:join()
end

function Process:testRead()
    local process = can.process("echo string")
    assert(process:read():find('string'))
    process:join()
end

Process()

