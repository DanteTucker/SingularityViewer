#ifndef LL_LUA_BINDINGS_H
#define LL_LUA_BINDINGS_H
#include "lllua.h"
#include "lluuid.h"
#include "lleasymessagesender.h"
#include "chatbar_as_cmdline.h"

#include <map>

#include <boost/signals2/connection.hpp>

#define LunaBase(T)\
	friend class Luna < T >;\
	static const char className[];\
	static const Luna < T >::FunctionType Functions[];\
	static const Luna < T >::PropertyType Properties[];\
	bool isExisting; \
	bool isPrecious;

class LunaMessageBuilder : public LLEasyMessageSender
{
	LunaBase(LunaMessageBuilder)
	
	LunaMessageBuilder(lua_State* L){}
public:
	int SendRawMessage(lua_State* L);

	int SendMessage(lua_State* L);

	int NewMessage(lua_State* L);

	int AddBlock(lua_State* L);

	int AddField(lua_State* L);
	int AddHexField(lua_State* L);
protected:

	void luaNewMessage(const std::string& message_name, const std::string& direction, bool include_agent_boilerplate=false);

	void luaAddBlock(const std::string& blockname);

	void luaAddField(const std::string& name, const std::string& value);
	void luaAddHexField(const std::string& name, const std::string& value);

	std::string mMessageBuffer;

	/*virtual*/ void printError(const std::string& error);
};

class LunaMessageHandler
{
	LunaBase(LunaMessageHandler)
	LunaMessageHandler(lua_State* L);
	~LunaMessageHandler();

	void slot_func(LLMessageSystem* msg); //internal function

	int connect(lua_State* L); //get callback function and add slot_func to signal
	int disconnect(lua_State* L); //disconnect the slot_func
	int setHandler(lua_State* L); //sets up the lua functions to be used as a callback
	
	typedef std::map<std::string, std::string> message_field_t;
	message_field_t mMessageFields;
	
	int mFuncRef;
	LLMessageTemplate* mMessageTemplate;
	boost::signals2::connection mMessageConnection;
};

class LunaChatCommand : public CmdLineChatCommand
{
public:
	/*virtual*/ void execute(std::string text);
private:
	LunaBase(LunaChatCommand)
	LunaChatCommand(lua_State* L);
	~LunaChatCommand();
	int deleteme(lua_State* L);

	/*virtual*/ bool execute(const std::string& text);
	int mFuncRef;

	bool mDisconnected;
};
/*class LunaUUID : public LLUUID
{
	LunaBase(LunaUUID)

	LunaUUID(lua_State* L);
}*/

#endif