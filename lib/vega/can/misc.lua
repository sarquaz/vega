local can = {}

function can.number (max)
    return __vega.set.random(max)
end


function can.string(length)
    local s = ""
    length = length or 10
    
    local chars = {}
    local s = "ABCDEGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

    for i = 1, #s do
        chars[i] = s:sub(i, i)
    end
    
    s = ""
    while #s < length  do
        local r = chars[__vega.set.random(#chars) + 1] 
        s = s .. r
    end
    
    return s
end

function can.dump(object)
    assert(object, "expecting passed object")
    return __vega.set.dump(object)
end

function can.load(data)
    assert(type(data) == 'string', "expecting passed data to be string")
    
    return __vega.set.load(data)
end

function can.compare(left, right)
    assert(left and right, "expecting passed operands")
    
    return __vega.set.compare(left, right)
end

function can.info()
    return __vega.set.info()
end




return can
