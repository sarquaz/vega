local can = require 'vega.can'

local common = {}

local Test = class()
common.Test = Test

function Test:new(options)
    options = options or {}
    
    self.options = options
    self.success = true
    
    self.timeout = self.options.timeout or 2
    
    self:_check()
end

function Test:_check()
    for name, value in pairs(self.__class) do
        if name:find("test") and type(value) == 'function' then
            print(string.format("%s", name))
            run(function() value(self) end):wait(self.timeout)
        end
    end
end    

function Test:run()
end


return common
