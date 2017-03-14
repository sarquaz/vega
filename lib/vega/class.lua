local export = {}

local Object = {}

function Object:new()
end

function Object:__id()
    if not self.__id then
        local id
        if _tostring then id = _tostring else id = tostring end
        self.__id = id(self):match("(0x%x+)"):sub(3)
      end
    
    return self.__id
end


local function instance(class, ...)
    local new = {}
    new.__class = class
    
    local mt = {
        __index = function(table, key) 
             if key == "__base" then return table.__class.__base end
                
             local found = rawget(table, key) 
             if found then return found end
             
             return table.__class[key]
        end
    }
    
    setmetatable(new, mt)    
    new:new(...)
    
    return new
end

local function class(base)
    local class = {}
    base = base or Object
    

    class.__base = base
    
    
    for key, value in pairs(base) do
        if type(value) == 'function' then class[key] = value end 
    end
    
    local mt = {       
        __index = function(table, key)
            local found = rawget(table, key) 
            if found then return found end
            
            return table.__base[key]
        end, 
        __call = function(table, ...)
            return instance(table, ...)
        end
    }
    
    return setmetatable(class, mt)
end

return class
