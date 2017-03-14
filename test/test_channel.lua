local can = require 'vega.can'
local common = require 'common'

local Udp = class(common.Test)

function Udp:new(options)
    self.strings = {}
    options = options or {}
    self.count = options.count or 1
    
    self.__base.new(self)
end

local Pair = class()

function Pair:new()
    self.host = 'localhost'
    self.port = can.number(1000) + 11000
    
    self.event = flow.event()
    self.result = false
    
    self.listener = can.listener({type='udp', host=self.host, port=self.port})
    flow(function()
         local data = self.listener:accept()
         self.result = data:data()
        self.event:set()
    end)
    
     local udp = can.udp(self.host .. ':' .. self.port)
     local string = can.string()
     udp:send(string)
    
    self.event:wait()
    assert(self.result == string)
end

function Udp:testSend()
    local runner = flow()
    repeat
         Pair()
         self.count = self.count - 1
         runner:sleep({msec=5})
    until self.count == 0
end

Udp({count=10})



