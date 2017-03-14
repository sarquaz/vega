local can = require 'vega.can'
local common = require 'common'

local Dump = class(common.Test)

function Dump:value()
    return true
end

function Dump:testLoad()
    self.test = ""
    local s = can.dump(self)
    local instance = can.load(s)
    assert(instance.test == self.test)
    assert(instance:__id())
    
    local test = {a='v'}
    test.loop = test
    s = can.dump(test)
    
    test = can.load(s)
    assert(not test.loop)
    
    local _, error = pcall(function() can.load(s:sub(1, #s - 1)) end)
 
    assert(error)
    assert(can.compare(self, self))
end

Dump()
