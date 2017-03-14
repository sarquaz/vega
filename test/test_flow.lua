local can = require 'vega.can'
local common = require 'common'

local Flow = class(common.Test)

function Flow:testFlow()
    local runner = run(function() sleep{msec=10} self.slept = true end)
    runner:wait()
    assert(self.slept)
end
--
function Flow:testEvent()
    self.event = event()

    run(function()
        sleep{msec=10}

         self.event:set()
         self.set = true
         end)

    self.event:wait()
    assert(self.set)
end

function Flow:testTimeout()
    self.event = event()

    run(function()
        sleep{msec=100}
        self.event:set()
    end)

    local _, error = pcall(function() self.event:wait{msec=10} end)
    assert(error)
end

Flow()
