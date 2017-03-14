--local can = require 'vega.can'

-- self.event = flow.event()
--
-- flow(function()

-- local flow = run(function()
--
--     sleep{msec=10}
--     print('slept')
--      end)
--
-- print(flow:id())
-- run():terminate()
-- print(flow:id())


local a = __vega.set.dump('s')
print(a)






-- --
-- -- local t={a='b'}
-- -- t.f = function() end
-- --
-- local self = {}
--
-- -- local runner = flow(function()
-- --      flow():wait{msec=10}
-- --      print('slept')
-- -- end)
-- --
-- -- runner:wait{msec=2}
-- flow():wait()
-- local test = function()
--
--     -- local run = flow(function()
-- --             flow():wait{msec=10}
-- --             local _, error = pcall(function() self.event:set() end)
-- --         -- assert(error)
-- --             print('slept')
-- --             end)
--
--
--     self.event:wait{msec=10}
-- end
--
-- flow(test):wait{msec=20}
--
-- -- local event = flow.event()
-- --
-- -- flow(function()
-- --     flow():wait{msec=10}
-- --
-- --      event:set()
-- --
-- --      end)
-- --
-- -- event:wait()
-- -- print('waited')
--
-- --print(can.compare(a, a))
--
-- -- local pile = __vega.mall.pile()
-- -- print(pile)
-- --print(process:read(can.Process.Out))
-- -- local pile = process:data(can.Process.Out)
-- -- print(pile:read())
-- --
-- -- pile = process:data(can.Process.Err)
-- -- print(pile:read())
-- -- --
-- --
-- --
-- -- local run = flow()
-- -- while true do
-- --     channel:send('line ' .. can.info().line)
-- --     print(channel:receive())
-- --     run:sleep{msec=100}
-- -- end
--
--
--
--
