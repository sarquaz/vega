local can = {}

local Event = class()

function Event:new(options)
    self.runners = {}
    self.options = options or {}
    assert(type(self.options) == 'table', 'expecting a passed table')
end

function Event:wait()
    local runner = __runner()
    table.insert(self.runners, runner)
    print("yielding", runner)
    coroutine.yield()
end

function Event:post()
    print(self.runners)
    
    for _, runner in ipairs(self.runners) do
        coroutine.resume(runner)
    end
end

function can.event(options)
    return Event(options)
end

return can