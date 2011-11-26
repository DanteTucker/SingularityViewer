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

local Hooks={}
local HooksDesc={}
local function addIfNotThere(HookName)
	if Hooks[HookName] == nil then
		registerHook(HookName,"Registered by addIfNotThere")
	end
end

function setHook(HookName,Function)
	addIfNotThere(HookName)
	table.insert(Hooks[HookName],Function)
end

function callHook(HookName,...)
	if Hooks[HookName] == nil then return 0 end -- no hooks
	local calls=0 -- for debugging
	for _,hfunc in pairs(Hooks[HookName]) do
		hfunc(...)
		calls=calls+1
	end
	return calls
end

function registerHook(HookName, Desc)
	if Desc == nil then Desc = "" end -- make empty string since nil is used for sanity checks
	if Hooks[HookName] == nil then
		Hooks[HookName]={}
		HooksDesc[HookName]=Desc
		return
	end
	error(HookName.." re-registered attempt.")
end

function printHooks()
	for k,v in pairs(HooksDesc) do
		print(k,v)
	end
end
