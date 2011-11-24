#ifndef LL_LUA_H
#define LL_LUA_H

#include "llsingleton.h"
#include "llthread.h"

#include <boost\function.hpp>
#include <map>

// we built the library in a c++ compiler we dont need these
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "luna.h"
#include "llluaconsole.h"

#define LUA_HOOK(hook_name, args) LLLuaEngine::callHook(hook_name, args)
#define LUA_ARGS_NONE std::vector< std::string >()

#define LUA_ERROR(s) {std::ostringstream ostr; ostr << s; \
					 LL_WARNS("Lua") << ostr.str() << LL_ENDL; \
					 LLFloaterLuaConsole::addOutput(ostr.str(), true);}
#define LUA_ERRORED(s) LUA_ERROR(s << " " << LLLuaEngine::getInstance()->getError())

class LLLuaState
{
private:
	lua_State *L;
public:
	LLLuaState();
	//LLLuaState(lua_State *new_L): L(new_L) { }
	~LLLuaState();
	// implicitly act as a lua_State pointer
	inline operator lua_State*() {
		return L;
	}
};

class LLLuaTable
{
	bool K;
	unsigned int C;
public:
	LLLuaTable(lua_State* L)
		:K(false),
		 C(1)
	{
		lua_newtable(L);
	}
	void pushkeystring(lua_State* L, const char* s)
	{
		K = true;
		lua_pushstring(L, s);
	}
	void pushvalue(lua_State* L, const char* s)
	{
		lua_pushstring(L, s);
		push(L);
	}
	void pushvalue(lua_State* L, lua_Number s)
	{
		lua_pushnumber(L, s);
		push(L);
	}
	void push(lua_State* L)
	{
		if(K)
			lua_rawset(L, -3);
		else
			lua_rawseti(L, -2, C++);
		K=false;
	}
};

class LLLuaEngine : public LLSingleton<LLLuaEngine>
{
	friend class LLSingleton<LLLuaEngine>;
private:
	bool mRegisteredBindings;

	LLLuaState mState; //master lua_State
	LLLuaEngine();
	/*virtual*/ void initSingleton();
public:	
	static void tick(); //ticks every frame
	static void callHook(const std::string& hook_name,const std::vector< std::string > &args);

	lua_State* getLuaState() { return mState; }

	void doString(const std::string& s);
	void console(const std::string& s);
	
	void registerBindings();
	const std::string getError();
};

#endif