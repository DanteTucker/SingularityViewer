#ifndef LL_LUA_H
#define LL_LUA_H

#include "llsingleton.h"
#include "llthread.h"

#include <boost\function.hpp>
#include <map>

// we built the library in a c++ compiler we dont need these
//extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
//}

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

class LLLuaEngine : public LLSingleton<LLLuaEngine>
{
	friend class LLSingleton<LLLuaEngine>;
private:
	LLLuaState mState; //master lua_State
	LLLuaEngine();
	/*virtual*/ void initSingleton();
public:
	bool load();
	void tick(); //ticks every frame
	void doString(const std::string& s);
	void console(const std::string& s);
	const std::string getError();
};
#endif