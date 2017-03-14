local util = {}

-- format time 
function util.formatTime(timestamp, format, tzoffset, tzname)
    format = format or "!%c %z"
    tzoffset = tzoffset or  "GMT"
    
    if tzoffset == "local" then  -- calculate local time zone (for the server)
       local now = os.time()
       local local_t = os.date("*t", now)
       local utc_t = os.date("!*t", now)
       local delta = (local_t.hour - utc_t.hour)*60 + (local_t.min - utc_t.min)
       local h, m = math.modf( delta / 60)
       tzoffset = string.format("%+.4d", 100 * h + 60 * m)
    end
   
    tzoffset = tzoffset or "GMT"
    format = format:gsub("%%z", tzname or tzoffset)
    if tzoffset == "GMT" then
       tzoffset = "+0000"
    end
   
    tzoffset = tzoffset:gsub(":", "")

    local sign = 1
    if tzoffset:sub(1,1) == "-" then
       sign = -1
       tzoffset = tzoffset:sub(2)
    elseif tzoffset:sub(1,1) == "+" then
       tzoffset = tzoffset:sub(2)
    end
   
    tzoffset = sign * (tonumber(tzoffset:sub(1,2))*60 +  tonumber(tzoffset:sub(3,4))) * 60
   
    return os.date(format, timestamp + tzoffset)
end

function util.parseQuery(query)
    
	local parsed = {}
	local pos = 0

	query = string.gsub(query, "&amp;", "&")
	query = string.gsub(query, "&lt;", "<")
	query = string.gsub(query, "&gt;", ">")

	local function ginsert(qstr)
		local first, last = string.find(qstr, "=")
		if first then
			parsed[string.sub(qstr, 0, first-1)] = string.sub(qstr, first+1)
		end
	end

	while true do
		local first, last = string.find(query, "&", pos)
		if first then
			ginsert(string.sub(query, pos, first-1));
			pos = last+1
            
		else
			ginsert(string.sub(query, pos));
			break;
		end
	end
	return parsed
end

function util.parseUrl(url)
    local result = {}

    local slashes = url:find("//")
    local scheme
    if slashes then
         scheme = url:sub(1, slashes - 2) 
         url = url:sub(slashes + 2)
     else
         scheme = 'http'
    end        
    
    local slash = url:find("/")
    local path
    if slash then 
        path = url:sub(slash)

        url = url:sub(1, slash - 1)
    end
    
    local ports = {https = 443, http = 80}
    
    local host, port = url:match("(.*):(%d+)")
    
    if not host then host = url end
    if not port then port = ports[scheme] end

    result.query = ""
    result.params = {}
    
    
    local sep
    if path then sep = path:find("?") end
     
    if  sep then 
        result.query = path:sub(sep)
        result.params = parseQuery(result.query:sub(2))
        path = path:sub(1, sep - 1)
    end
    

    result.host = host
    result.port = tonumber(port)
    
    result.path = path
    result.scheme = scheme
    
    return result    
end

return util