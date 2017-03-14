local reload = {}

reload.start = function(path)
   local run = function()
       local watcher           
       
       pcall(function() watcher = require 'watcher' end)
       
       if watcher then
           print('starting watcher loop')
           
           while true do
               local change = watcher:watch(path)
               print(change)
               print(__vega.main.reload())
           end
       end
   end 
     
   flow(run) 
end

return reload

