local can = {}

function can.listener(options)
    options = options or {}
    assert(type(options) == 'table', 'expecting passed table')
    options.host = options.host or 'localhost'
    options.port = options.port or 12000
    
    return __vega.main.listener(options)
end


function can.udp(options)
    return __vega.main.udp(parse(options))
end

function can.tcp(options, init)
    return __vega.main.tcp(parse(options))
end

function can.channel(name)
    assert(type(name) == 'string', 'expecting a passed string')
    local channel = __vega.main.channel(name)
    channel.name = name
    return channel
end

function can.process(command)
    assert(type(command) == 'string', 'expecting a passed string')
    return __vega.main.process({command=command})
end

can.Process = {Out = 1, Err = 2}



return can