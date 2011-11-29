--[[
            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
                    Version 2, December 2004 

 Copyright (C) 2004 Sam Hocevar <sam@hocevar.net> 

 Everyone is permitted to copy and distribute verbatim or modified 
 copies of this license document, and changing it is allowed as long 
 as the name is changed. 

            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE 
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION 

  0. You just DO WHAT THE FUCK YOU WANT TO. 
--]]

--load lua library files for scripts to use.
require "libhook"

print "Registering Hooks"
registerHook("OnAgentInit", "This is called when the Agent and World has been loaded. This also indicates when World related bindings have been loaded.")

local function pack2(...) return {n=select('#', ...), ...} end
local function unpack2(t) return unpack(t, 1, t.n) end

local chat_lua_dostring = ChatCommand("/lua",function(...) loadstring( unpack2(pack2(...)) )() end)


function printMessage(x)
	for k,v in pairs(x) do
		print('['..k..']')
		for _,y in ipairs(v) do
			for a,b in pairs(y) do
				print(a..' = '..b)
			end
		end
	end
end
--setHook("OnAgentInit", function() print "OnAgentInit" end)

print "Initialization done."