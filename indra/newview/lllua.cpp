#include "llviewerprecompiledheaders.h"
#include "lllua.h"
#include "llluabindings.h"
#include "llluaconsole.h"
#include <llchat.h>
#include "llfloaterchat.h"
#include "llviewercontrol.h"
#include "lleasymessagesender.h"
#include "llstartup.h"
#include "llcallbacklist.h"

void dump_stack (lua_State *L) {
    int i;
    int top = lua_gettop(L);
    for (i = 1; i <= top; i++) {  /* repeat for each level */
    int t = lua_type(L, i);
    switch (t) {
    
        case LUA_TSTRING:  /* strings */
        LL_INFOS("STACK") << llformat("`%s'", lua_tostring(L, i)) << LL_ENDL;
        break;
    
        case LUA_TBOOLEAN:  /* booleans */
        LL_INFOS("STACK") << llformat(lua_toboolean(L, i) ? "true" : "false") << LL_ENDL;
        break;
    
        case LUA_TNUMBER:  /* numbers */
        LL_INFOS("STACK") << llformat("%g", lua_tonumber(L, i)) << LL_ENDL;
        break;
    
        default:  /* other values */
        LL_INFOS("STACK") << llformat("%s", lua_typename(L, t)) << LL_ENDL;
        break;
    
			}
    }
}


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

LLLuaTable::LLLuaTable(lua_State* L)
	:K(false),
	 C(1)
{
	lua_newtable(L);
}

void LLLuaTable::pushkeystring(lua_State* L, const char* s)
{
	K = true;
	lua_pushstring(L, s);
}
void LLLuaTable::pushvalue(lua_State* L, const char* s)
{
	lua_pushstring(L, s);
	push(L);
}
void LLLuaTable::pushvalue(lua_State* L, lua_Number s)
{
	lua_pushnumber(L, s);
	push(L);
}
void LLLuaTable::push(lua_State* L)
{
	if(K)
		lua_rawset(L, -3);
	else
		lua_rawseti(L, -2, C++);
	K=false;
}

//static
void LLLuaTable::make_table(lua_State* L, const std::vector< std::string > vec)
{
	LLLuaTable table(L);
	for(auto itr = vec.begin(); itr != vec.end(); itr++)
		table.pushvalue(L, (*itr).c_str());
}

//static
void LLLuaTable::make_table(lua_State* L, const std::vector< lua_Number > vec)
{
	LLLuaTable table(L);
	for(auto itr = vec.begin(); itr != vec.end(); itr++)
		table.pushvalue(L, (*itr));
}

//static
void LLLuaTable::make_table(lua_State* L, const std::map< std::string, std::string > map)
{
	LLLuaTable table(L);
	for(auto itr = map.begin(); itr != map.end(); itr++)
	{
		table.pushkeystring(L, itr->first.c_str());
		table.pushvalue(L, itr->second.c_str());
	}
}

//static
void LLLuaTable::make_table(lua_State* L, const std::map< std::string, lua_Number > map)
{
	LLLuaTable table(L);
	for(auto itr = map.begin(); itr != map.end(); itr++)
	{
		table.pushkeystring(L, itr->first.c_str());
		table.pushvalue(L, itr->second);
	}
}

static int luaOnPanic(lua_State *L)
{	
	LUA_ERROR("PANIC: " << lua_tostring(L, -1));
	lua_pop(L, -1);
	return 0;
}
LLLuaEngine::LLLuaEngine()
	:mState()
{
	gIdleCallbacks.addFunction(LLLuaEngine::tick);
}
LLLuaEngine::~LLLuaEngine()
{
	gIdleCallbacks.deleteFunction(&LLLuaEngine::tick);
}
void LLLuaEngine::initSingleton()
{
	LL_INFOS("Lua") << "Loading Lua..." << LL_ENDL;
	lua_atpanic(mState, luaOnPanic);

	LL_INFOS("Lua") << "Load standard library with integrated bit and lfs" << LL_ENDL;
	luaL_openlibs(mState);

	//load binding
	bind_print(mState, LuaPrint);
	luna_register(mState, LunaChatCommand); //no message depend

	//logic that happens when idle
	if(LLStartUp::getStartupState() == STATE_STARTED)
	{
		registerBindings();
		LUA_HOOK("OnAgentInit",LUA_ARGS_NONE);
	}

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

	mLoadedFully = true;

	if(mRegisteredBindings)
	{
		LUA_HOOK("OnAgentInit",LUA_ARGS_NONE);
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
void LLLuaEngine::tick(void* userdata)
{
	if(!LLLuaEngine::instanceExists()) return; //so the tick doesnt happen till we are up and running

	//logic that happens when idle
	if(LLStartUp::getStartupState() == STATE_STARTED && !getInstance()->mRegisteredBindings)
	{
		getInstance()->registerBindings();
		LUA_HOOK("OnAgentInit",LUA_ARGS_NONE);
	}
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

	if(!self.mLoadedFully) return;

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