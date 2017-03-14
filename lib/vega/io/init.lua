local io = {}

local function parse(options)
    assert(type(options) == 'string' or type(options) == 'table', "expecting passed options as string or table")
    
    if type(options)  == 'string' then
        local host, port = options:match("(.*):(%d+)")
        options = {host = host, port=port}
    end
    
    options.host = options.host or ""
    options.port = options.port or 0
    
    return options 
end

function io.udp(options)
    return __vega.main.udp(parse(options))
end

function io.tcp(options, init)
    return __vega.main.tcp(parse(options))
end


return io
     