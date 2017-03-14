local can = {}

function can.pile()
    return __vega.mall.pile()
end

for _, module in ipairs{'io', 'misc', 'sync'} do
    table.merge(can, require('vega.can.' .. module))    
end


return can