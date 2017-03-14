local common = {}

function common.append(object, what)
    local target = object
    local mt = getmetatable(object)
    if mt then target = mt.__index end 

    for name, method in pairs(target) do
        local overload = function(...)
            return what(method, ...)
        end
    
        target[name] = overload
    end 
    
    if mt then setmetatable(object, mt) end
    
    return object
end

function common.wrap(object)
    local check = function(method, ...)
        local result = method(...)
        
        if type(result) == 'table' and result.error then
            assert(false, result.error)
        end
        
        return result
    end
    
    return common.append(object, check)
end

common.class = require('vega.class')

return common