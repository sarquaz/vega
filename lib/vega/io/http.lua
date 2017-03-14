--- Asynchronous HTTP client
-- @usage local http = require('vega.client.http')
-- http('www.google.com'):get('/', function(response)
--  print(response.body) 
--end)
-- @module leda.Http.http

local io = require 'vega.io'
local util = require 'vega.util'

--- http Http class
--- @type Http
local Http = class('Http')

---  Create HTTP connection. Asynchronously tries to connect to url specified
-- @param url url to connect to 
-- @return a new Http
-- @usage local Http = client.Http('www.google.com')
-- Http.get(function(response) print(response.body) end)
-- @name Http()
function Http:initialize(url)
    self.url = util.parseUrl(url)   
    self.type = self.url.scheme 
    self.url.path = self.url.path or '/'
    
    for _, method in ipairs{'POST', 'GET', 'PUT', 'DELETE'} do
        self[method:lower()] = function(self, path, headers, body, callback)
            return self:_prepareRequest(method, path, headers, body, callback)
        end
    end
    
    self.version =  __vega.util.version()
    
    self._tcp = io.tcp{host = self.url.host, port = self.url.port, secure = self.type == 'https'}
    
    -- self._tcp.data = function(data)
    --     print("tcp callback")
    --     print(data)
    --     if data then self:_data(data) end
    -- end
end


--- perform a GET request asynchronously. Function can receive callback function as any argument
-- @param path request path or callback
-- @param[opt] headers additional headers to send with request
-- @param[opt] callback callback function that is run when the response has been received. It receives a table with fields: headers, status, body
-- @usage Http:get(function(response)
--  print(response.body)
--      end)
function Http:get(path, headers, callback)    
end

--- perform a POST request asynchronously. Function can receive callback function as any argument
-- @param path request path 
-- @param headers additional headers to send with request
-- @param body additional headers to send with request
-- @param callback callback function that is run when the response has been received. It receives a table with fields: headers, status, body
-- @usage local headers = {}
--      headers['content-type'] = 'application/x-www-form-urlencoded'
--      Http:post('/form', headers, "name=value", function(response)
--      print(response.body)
--    end)
function Http:post(path, headers, body, callback)    
end

--- perform a PUT request asynchronously. Function can receive callback function as any argument
-- @param path request path or callback
-- @param[opt] headers additional headers to send with request
-- @param[opt] body request body
-- @param[opt] callback callback function that is run when the response has been received. It receives a table with fields: headers, status, body
-- @usage Http:put('/path', function(response)
--  print(response.body)
--      end)
function Http:put(path, headers, body, callback)    
end

--- perform a DELETE request asynchronously. Function can receive callback function as any argument
-- @param path request path or callback
-- @param[opt] headers additional headers to send with request
-- @param[opt] callback callback function that is run when the response has been received. It receives a table with fields: headers, status, body
-- @usage Http:delete('/path', function(response)
--  print(response.body)
--      end)
function Http:delete(path, headers, callback)    
end

function Http:_send()
    if self._request then
        print(self._tcp)
        self._tcp:send(self._request)
        self._request = nil
    end
end

Http.Parser = class('Http.Parser')

function Http.Parser:initialize(connection)
    self.connection = connection
    self.data = ''
end

function Http.Parser:_newResponse()
    return {headers = {},  body = ""}
end

function Http.Parser:_error(...)
    self.connection:_error()
end

function Http.Parser:headers()
    local headers = nil
    local length = 0
    local status = 0
        -- parse headers
    local s, e = self.data:find('\r\n\r\n') 
    if s then
        local header = self.data:sub(1, s)
        length = e
        headers = {}
        for i, line in ipairs(header:split('\r\n'))  do
            if i == 1 then
                -- status line
                local parts = line:split(' ')
                status = tonumber(parts[2])
            else
                -- header line
                local colon = line:find(':')
                if not colon then
                    self:_error("error parsing response")
                    return
                end
                
                local name = line:sub(1, colon - 1)
                local value = line:sub(colon + 2)
                headers[name:lower()] = value:trim()
            end
        end
    end
    
    return headers, length, status
end

function Http.Parser:add(data)
    self.response = nil
    
    print(data)
    

    
    if data then self.data = table.concat({self.data, data}) end
    
    local headers, headersLength, status = self:headers()
    local full = false
    
    if headers then
        if headers['content-length'] then
            self.length = tonumber(headers['content-length'])

            full = (#self.data - headersLength) >= self.length
            
            if full then
                local offset = headersLength + 1
                self.body = self.data:sub(offset, offset + self.length)
                self.length = self.length + offset
            end

            
        elseif headers['transfer-encoding'] == 'chunked'  then
            local s, e = self.data:find("0\r\n\r\n", headersLength) 
            if e then
                self.length = e
                self.body = self.data:sub(headersLength + 1, self.length)
                self.body =  self.body:gsub("%x+\r\n", "")
                full = true
                self.length = self.length + 2   
                headers['transfer-encoding'] = nil
            end
        end
    end
    
    if full then
        self.data = self.data:sub(self.length)
        
        -- response callback
        self.response = {body = self.body, headers = headers, status=status}
        if type(self.Http.responseCallback) == 'function' 
        then
            self.Http.responseCallback(self.response)  
        else
            -- if #self.data > 0 then
--                 local util = require 'vega.util'
--                 local Http = self.Http._Http
--                 local data = self.data
--                 self.data = ''
--
--                 util.timeout(0, function()
--                     __api.HttpResumeSend(Http, data)
--                 end)
--             end

            self.connection._tcp:read(self.length)
            return self.response
        end
    else
        if self.sync then return false end
    end
    
    -- if more responses left call self
    if (#self.data > 0) and full then 
        self:add()
    end
end

function Http:_prepareRequest(method, path, headers, body, callback)
    if type(path) == 'function' then 
        self.responseCallback = path 
        path = self.url.path 
    end
    
    if type(headers) == 'function' then 
        self.responseCallback = headers
        headers = {}
    end
    
    if type(body) == 'function' then
         self.responseCallback = body 
         body = nil
    end
    
    if type(callback) == 'function' then self.responseCallback = callback end
    self.parser = self.parser or Http.Parser(self)
    
    local request = string.format("%s %s HTTP/1.1\r\n", method, path)
    headers = headers or {}

    headers['User-Agent'] = self.version 
    headers['Host'] = string.format("%s:%d", self.url.host, self.url.port)
    headers['Accept'] = "*/*"
    
    if body then
       headers['Content-Length'] = #body
    end
    
    for key, value in pairs(headers) do
        request = request .. string.format("%s: %s\r\n", key, value)
    end
    
    request = request .. "\r\n"
    
    if body then request = request .. body end
    self._request = request
    
    self:_send()
    
    print(self._tcp.thread)
    if self._tcp.thread then
        self.parser.sync = true
        return self:_wait()
    end
end

function Http:_wait()
    local data = self._tcp:data()
    if data then
        local parsed =  self.parser:add(data)
        if parsed then return parsed end
    end
        
    return self:_wait()
end

function Http:_data(data)
    return self.parser:add(data)
end

--- close the Http
function Http:close()
    self._tcp:close()
end

--- error callback. runs when Http error occurs
-- @param callback function
-- @usage Http.error = function(Http, error)
--    print(error)
-- end
Http.error = nil

return Http