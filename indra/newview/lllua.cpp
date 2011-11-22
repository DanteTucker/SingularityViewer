#include "llviewerprecompiledheaders.h"
#include "lllua.h"
#include "llluaconsole.h"
#include <llchat.h>
#include "llviewercontrol.h"
#define LUA_ERROR(s) LL_WARNS("Lua") << s << LL_ENDL
#define LUA_ERRORED() LUA_ERROR(getError())

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
	LL_INFOS("Lua") << LuaPrintStrings(L) << LL_ENDL;
	return 0;
}
static int LuaPrintConsole(lua_State* L)
{
	LLFloaterLuaConsole::getInstance()->addOutput(LuaPrintStrings(L), false);
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
	return 0;
}
static int LuaShout(lua_State* L)
{
	send_chat_from_viewer(std::string(lua_tostring(L, -1)), CHAT_TYPE_SHOUT, 0);
	return 0;
}
static int LuaWhipser(lua_State* L)
{
	send_chat_from_viewer(std::string(lua_tostring(L, -1)), CHAT_TYPE_WHISPER, 0);
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

void LLLuaEngine::initSingleton()
{
	
}
static int luaOnPanic(lua_State *L)
{	
	LUA_ERROR("PANIC: " << lua_tostring(L, -1));
	lua_pop(L, -1);
	return 0;
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
	//hook print like commands.
	bind_print(mState, LuaPrintConsole);

	if(luaL_dostring(mState,s.c_str()))
	{
		LLFloaterLuaConsole::getInstance()->addOutput(getError(), true);
	}

	//unhook print like commands.
	bind_print(mState, LuaPrint);
}

bool LLLuaEngine::load()
{ 
	LL_INFOS("Lua") << "Loading Lua..." << LL_ENDL;
	lua_atpanic(mState, luaOnPanic);

	LL_INFOS("Lua") << "Load standard library" << LL_ENDL;
	luaL_openlibs(mState);

	//load binding
	bind_print(mState, LuaPrint);
	lua_register(mState, "say", LuaSay);
	lua_register(mState, "shout", LuaShout);
	lua_register(mState, "whisper", LuaWhipser);

	//hack because im lazy
	std::string version("_SL_VERSION=\"" + gCurrentVersion + "\"");
	if(luaL_dostring(mState,version.c_str()))
	{
		LUA_ERRORED();
		return false;
	}

	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_LUA,"_init_.lua");
	if(luaL_dofile(mState, filename.c_str()))
	{
		LUA_ERROR("Failed to load: " << filename << " " << getError());
		return false;
	}
	return true;
}

const std::string LLLuaEngine::getError()
{
	std::string err(lua_tostring(mState, -1));
	lua_pop(mState,1);
	return err;
}