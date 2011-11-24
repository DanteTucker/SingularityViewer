#include "llviewerprecompiledheaders.h"
#include "lllua.h"
#include "llluabindings.h"
#include "llluaconsole.h"
#include <llchat.h>
#include "llfloaterchat.h"
#include "llviewercontrol.h"
#include "lleasymessagesender.h"
#include "llstartup.h"

static std::string LuaPrintStrings(lua_State* L)
{ 
	int nArgs = lua_gettop(L);
	lua_getglobal(L, "tostring");
	std::string ret = "";
	for(int i = 1; i <= nArgs; i++)
	{
		lua_pushvalue(L, -1);
		lua_pushvalue(L, i);
		lua_call(L, 1, 1);
		const char* s = lua_tostring(L, -1);
		if(s == NULL)
		{
			luaL_error(L, "\"tostring\" must return a string to \"print\"");
		}
		if(i > 1) ret+= " ";
		ret.append(s);
		lua_pop(L, 1);
	}
	return ret;
}

static int LuaPrint(lua_State* L)
{
	std::string str = LuaPrintStrings(L);
	LL_INFOS("Lua") << str << LL_ENDL;
	LLChat chat;
	chat.mFromName = "Lua";
	chat.mSourceType = CHAT_SOURCE_SYSTEM;
	chat.mText = str;
	LLFloaterChat::addChat(chat);
	LLFloaterLuaConsole::getInstance()->addOutput(str, false);
	return 0;
}

static void bind_print(lua_State* L, int (*print_fn)(lua_State*))
{
	lua_register(L, "print", print_fn);
}
// defined in llchatbar.cpp, but not declared in any header
void send_chat_from_viewer(std::string utf8_out_text, EChatType type, S32 channel);
static int LuaSay(lua_State* L)
{
	send_chat_from_viewer(std::string(lua_tostring(L, -1)), CHAT_TYPE_NORMAL, 0);
	lua_pop(L,1);
	return 0;
}
static int LuaShout(lua_State* L)
{
	send_chat_from_viewer(std::string(lua_tostring(L, -1)), CHAT_TYPE_SHOUT, 0);
	lua_pop(L,1);
	return 0;
}
static int LuaWhipser(lua_State* L)
{
	send_chat_from_viewer(std::string(lua_tostring(L, -1)), CHAT_TYPE_WHISPER, 0);
	lua_pop(L,1);
	return 0;
}
LLLuaState::LLLuaState()
{ 
	L = lua_open();
}
LLLuaState::~LLLuaState()
{ 
	lua_close(L);
}

LLLuaEngine::LLLuaEngine()
	:mState()
{
}

static int luaOnPanic(lua_State *L)
{	
	LUA_ERROR("PANIC: " << lua_tostring(L, -1));
	lua_pop(L, -1);
	return 0;
}

void LLLuaEngine::initSingleton()
{
	LL_INFOS("Lua") << "Loading Lua..." << LL_ENDL;
	lua_atpanic(mState, luaOnPanic);

	LL_INFOS("Lua") << "Load standard library with integrated bit and lfs" << LL_ENDL;
	luaL_openlibs(mState);

	//load binding
	bind_print(mState, LuaPrint);

	//hack because im lazy
	std::string version("_SL_VERSION=\"" + gCurrentVersion + "\"");
	if(luaL_dostring(mState,version.c_str()))
	{
		LUA_ERROR(getError());
		return;
	}

	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_LUA,"_init_.lua");
	if(luaL_dofile(mState, filename.c_str()))
	{
		LUA_ERROR("Failed to load: " << filename << " " << getError());
		return;
	}
}

void LLLuaEngine::doString(const std::string& s)
{
	if(luaL_dostring(mState,s.c_str()))
	{
		LUA_ERROR("FAILED to run string [" << s << "] reason: " << getError());
	}
}

void LLLuaEngine::console(const std::string & s)
{
	if(luaL_dostring(mState,s.c_str()))
	{
		LUA_ERROR("console" << getError());
	}
}

//static
void LLLuaEngine::tick()
{
	if(!LLLuaEngine::instanceExists()) return; //so the tick doesnt happen till we are up and running

	//logic that happens when idle
	if(LLStartUp::getStartupState() == STATE_STARTED && !getInstance()->mRegisteredBindings)
	{
		getInstance()->registerBindings();
		LUA_HOOK("OnAgentInit",LUA_ARGS_NONE);
	}

	LUA_HOOK("OnTick",LUA_ARGS_NONE);
}

void LLLuaEngine::registerBindings()
{
	lua_register(mState, "say", LuaSay);
	lua_register(mState, "shout", LuaShout);
	lua_register(mState, "whisper", LuaWhipser);

	//luna register
	luna_register(mState, LunaMessageBuilder);
	luna_register(mState, LunaMessageHandler);

	mRegisteredBindings = true;
}

void LLLuaEngine::callHook(const std::string &hook_name, const std::vector< std::string > &args)
{
	if(!LLLuaEngine::instanceExists()) return; //dont hook if lua isnt up and running

	LLLuaEngine &self = instance();

	lua_getglobal(self.mState, "callHook");
	lua_pushstring(self.mState, hook_name.c_str());
	for(std::vector< std::string >::const_iterator it = args.begin(); it != args.end(); it++)
	{
		lua_pushstring(self.mState, (*it).c_str());
	}
	if(lua_pcall(self.mState, args.size()+1,1,0))
	{
		LUA_ERROR(self.getError());
	}
	lua_pop(self.mState,1);
}

const std::string LLLuaEngine::getError()
{
	std::string err(lua_tostring(mState, -1));
	lua_pop(mState,1);
	return err;
}