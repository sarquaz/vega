require 'std'
local common = require 'vega.common'

 __vega = __vega or {}

class = common.class

local wrapper = function(method, ...)
    local object = method(...)
    if  object.errors then object = common.wrap(object) end
    return object
end

common.append(__vega.get, wrapper)

run = function(what) 
    if type(what) == 'function' then return  __vega.jet.start(what) else return __vega.get.runner() end
end
    
event = function()
    return common.wrap(__vega.get.event())
end    

sleep = function(what) 
    run():wait(what)
end    

local _require = require
require = function(name)
    local loaded = __vega.set.require(name)
    if not loaded then return _require(name) end
    
    return loaded    
end

