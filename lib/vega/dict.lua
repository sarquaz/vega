--- Key value store shared between all threads
-- @usage local dict = require('vega.dict')
-- dict.set('key', 'value')
-- local value = dict.get('key') 
-- @module leda.dict

local dict = {}

--- set key to value
-- @param key key string
-- @param value value string
dict.set = function(key, value)
    assert(key, "no key provided")
    assert(type(value) == 'string', 'can only store strings')
    __vega.dictionary.set(key, value)
end

--- get value 
-- @param key key string
-- @return value or nil if not found
dict.get = function(key)    
    assert(key, "no key provided")
    return __vega.dictionary.get(key)
end

--- delete key 
-- @param key key string
dict.remove = function(key)
    assert(key, "no key provided")
    __vega.dictionary.remove(key)
end

--- get all keys stored in the dictionary
-- @return table with keys 
dict.keys = function()
    return __vega.dictionary.keys()
end

return dict