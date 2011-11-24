#include "llviewerprecompiledheaders.h"
#include "llmessagetemplate.h"
#include "lltemplatemessagebuilder.h"
#include "lltemplatemessagereader.h"
#include "llfloatermessagelog.h"
#include "llluabindings.h" //includes all lua headers needed.
#include "llagent.h"

static void dump_stack (lua_State *L) {
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

// implementation of Luna stuff
const char LunaMessageBuilder::className[] = "MessageBuilder";
const Luna < LunaMessageBuilder >::PropertyType LunaMessageBuilder::Properties[] = {
	{0}
};
const Luna < LunaMessageBuilder >::FunctionType LunaMessageBuilder::Functions[] = {
	{"AddBlock", &LunaMessageBuilder::AddBlock},
	{"AddField", &LunaMessageBuilder::AddField},
	{"AddHexField", &LunaMessageBuilder::AddHexField},
	{"NewMessage", &LunaMessageBuilder::NewMessage},
	{"SendMessage", &LunaMessageBuilder::SendMessage},
	{"SendRawMessage", &LunaMessageBuilder::SendRawMessage},
	{0}
};

int LunaMessageBuilder::SendRawMessage(lua_State* L)
{
	int nArgs = lua_gettop(L)-1; //element 1 is the table we are in.
	std::string region_host = gAgent.getRegionHost().getString();
	int i = 2; //arg magic
	bool ret = false;
	if(nArgs >= 2)
	{
		if(!lua_isnil(L,i) && lua_isstring(L,i))
		{
			region_host.assign(lua_tostring(L, i));
		}
		i++;
	}
	std::string message(luaL_checklstring(L,i, NULL));
	mMessageBuffer = "";

	LLHost proper_region_host = LLHost(region_host);

	//check that this was a valid host
	if(proper_region_host.isOk())
		ret = sendMessage(proper_region_host, message);
	lua_pushboolean(L, ret);
	return 1;
}
int LunaMessageBuilder::SendMessage(lua_State* L)
{
	bool ret = false;
	size_t len = 0;
	const char* arg = luaL_optlstring(L, 2, gAgent.getRegionHost().getString().c_str(), &len);
	std::string region_host(arg, len);
	LLHost proper_region_host = LLHost(region_host);

	//check that this was a valid host
	if(proper_region_host.isOk())
		ret = sendMessage(proper_region_host, mMessageBuffer);
	mMessageBuffer = "";
	lua_pushboolean(L, ret);
	return 1;
}

int LunaMessageBuilder::NewMessage(lua_State* L)
{
	int nArgs = lua_gettop(L)-1; //element 1 is the table we are in.
	bool boiler_plate = false; //default is false anyways.
	switch(nArgs)
	{
	case 3:
		if(!lua_isnil(L,4))
		{
			boiler_plate = lua_toboolean(L,4);
		}
	case 2:
		if(!lua_isnil(L,2) && !lua_isnil(L,3))
			luaNewMessage(std::string(luaL_checklstring(L,2,NULL)),std::string(luaL_checklstring(L,3,NULL)), boiler_plate);
		else
			LUA_ERRORED("NewMessage");
		break;
	default:
		LUA_ERRORED("NewMessage");
	}
	return 0;
}

int LunaMessageBuilder::AddBlock(lua_State* L)
{
	int nArgs = lua_gettop(L)-1; //element 1 is the table we are in.
	switch(nArgs)
	{
	case 1:
		if(!lua_isnil(L,2))
			luaAddBlock(std::string(luaL_checklstring(L,2,NULL)));
		else
			LUA_ERRORED("AddField");
		break;
	default:
		LUA_ERRORED("AddField");
	}
	return 0;
}

int LunaMessageBuilder::AddField(lua_State* L)
{
	int nArgs = lua_gettop(L)-1; //element 1 is the table we are in.
	switch(nArgs)
	{
	case 2:
		if(!lua_isnil(L,2) && !lua_isnil(L,3))
			luaAddField(std::string(luaL_checklstring(L,2,NULL)),std::string(luaL_checklstring(L,3,NULL)));
		else
			LUA_ERRORED("AddField");
		break;
	default:
		LUA_ERRORED("AddField");
	}
	return 0;
}

int LunaMessageBuilder::AddHexField(lua_State* L)
{
	int nArgs = lua_gettop(L)-1; //element 1 is the table we are in.
	switch(nArgs)
	{
	case 2:
		if(!lua_isnil(L,2) && !lua_isnil(L,3))
			luaAddHexField(std::string(luaL_checklstring(L,2,NULL)),std::string(luaL_checklstring(L,3,NULL)));
		else
			LUA_ERRORED("AddHexField");
		break;
	default:
		LUA_ERRORED("AddHexField");
	}
	return 0;
}

void LunaMessageBuilder::luaNewMessage(const std::string& message_name, const std::string& direction, bool include_agent_boilerplate)
{
	//clear out any message that may be in the buffer
	mMessageBuffer = "";

	mMessageBuffer = direction + " " + message_name + "\n";

	//include the agentdata block with our agentid and sessionid automagically
	if(include_agent_boilerplate)
		mMessageBuffer += "[AgentData]\nAgentID = $AgentID\nSessionID = $SessionID\n";
}

void LunaMessageBuilder::luaAddBlock(const std::string& blockname)
{
	mMessageBuffer += "[" + blockname + "]\n";
}

void LunaMessageBuilder::luaAddField(const std::string& name, const std::string& value)
{
	mMessageBuffer += name + " = " + value + "\n";
}

void LunaMessageBuilder::luaAddHexField(const std::string& name, const std::string& value)
{
	mMessageBuffer += name + " =| " + value + "\n";
}

/*virtual*/
void LunaMessageBuilder::printError(const std::string& error)
{
	LUA_ERROR(error);
}

// implementation of Luna stuff
const char LunaMessageHandler::className[] = "MessageHandler";
const Luna < LunaMessageHandler >::PropertyType LunaMessageHandler::Properties[] = {
	{0}
};
const Luna < LunaMessageHandler >::FunctionType LunaMessageHandler::Functions[] = {
	{"connect", &LunaMessageHandler::connect},
	{"disconnect", &LunaMessageHandler::disconnect},
	{"setHandler", &LunaMessageHandler::setHandler},
	{0}
};

LunaMessageHandler::LunaMessageHandler(lua_State* L)
{
	//TODO: connect on creation if there are the right parameters
	setHandler(L);
}

LunaMessageHandler::~LunaMessageHandler()
{
}

int LunaMessageHandler::setHandler(lua_State* L)
{
	if(mFuncRef != LUA_NOREF)
		luaL_unref(L, LUA_REGISTRYINDEX, mFuncRef);
	if(lua_isfunction(L, 2))
	{
		lua_pushvalue(L, 2);
		mFuncRef = luaL_ref(L, LUA_REGISTRYINDEX);
	}
	return 0;
}

void LunaMessageHandler::slot_func(LLMessageSystem* msg)
{
	if(!LLLuaEngine::instanceExists())
		mMessageConnection.disconnect();
	if(mFuncRef == LUA_NOREF) return; //shouldnt happen ever.

	lua_State* L = LLLuaEngine::getInstance()->getLuaState();
	//get all fields and make them a lua table.
	
	//get function pointer
	lua_rawgeti(L, LUA_REGISTRYINDEX, mFuncRef);

	//create table for arg
	LLLuaTable main_table(L); //magic util function.

	LLMessageTemplate::message_block_map_t::iterator blocks_end = mMessageTemplate->mMemberBlocks.end();
	for (LLMessageTemplate::message_block_map_t::iterator blocks_iter = mMessageTemplate->mMemberBlocks.begin();
			blocks_iter != blocks_end; ++blocks_iter)
	{
		LLMessageBlock* block = (*blocks_iter);
		if(block->mType != MBT_NULL)
		{
			const char* block_name = block->mName;
			S32 num_blocks = msg->getNumberOfBlocks(block_name);
			if(num_blocks) //only if we have the blocks.
			{
				main_table.pushkeystring(L, block_name);
				LLLuaTable block_table(L);
				for(S32 block_num = 0; block_num < num_blocks; block_num++)
				{
					LLLuaTable block_variable_table(L);

					LLMessageBlock::message_variable_map_t::iterator var_end = block->mMemberVariables.end();
					for (LLMessageBlock::message_variable_map_t::iterator var_iter = block->mMemberVariables.begin();
							var_iter != var_end; ++var_iter)
					{
						LLMessageVariable* variable = (*var_iter);
						const char* var_name = variable->getName();
						block_variable_table.pushkeystring(L, var_name);

						switch(variable->getType())
						{
						case MVT_U8:
							{
								U8 value;
								msg->getU8Fast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_U16:
							{
								U16 value;
								msg->getU16Fast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_U32:
							{
								U32 value;
								msg->getU32Fast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_U64:
							{
								U64 value;
								msg->getU64Fast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_S8:
							{
								S8 value;
								msg->getS8Fast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_S16:
							{
								S16 value;
								msg->getS16Fast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_S32:
							{
								S32 value;
								msg->getS32Fast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_F32:
							{
								F32 value;
								msg->getF32Fast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_F64:
							{
								F64 value;
								msg->getF64Fast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_LLVector3:
							{
								LLVector3 value;
								msg->getVector3Fast(block_name, var_name, value, block_num);
								LLLuaTable vector(L);
								for(int i = 0; i < 3; i++)
								{
									vector.pushvalue(L, value.mV[i]);
								}
								block_variable_table.push(L);
							}
							break;
						case MVT_LLVector3d:
							{
								LLVector3d value;
								msg->getVector3dFast(block_name, var_name, value, block_num);
								LLLuaTable vector(L);
								for(int i = 0; i < 3; i++)
								{
									vector.pushvalue(L, value.mdV[i]);
								}
								block_variable_table.push(L);
							}
							break;
						case MVT_LLVector4:
							{
								LLVector4 value;
								msg->getVector4Fast(block_name, var_name, value, block_num);
								LLLuaTable vector(L);
								for(int i = 0; i < 4; i++)
								{
									vector.pushvalue(L, value.mV[i]);
								}
								block_variable_table.push(L);
							}
							break;
						case MVT_LLQuaternion:
							{
								LLQuaternion value;
								msg->getQuatFast(block_name, var_name, value, block_num);
								LLLuaTable quat(L);
								for(int i = 0; i < 4; i++)
								{
									quat.pushvalue(L, value.mQ[i]);
								}
								block_variable_table.push(L);
							}
							break;
						case MVT_LLUUID:
							{
								LLUUID value;
								msg->getUUIDFast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value.asString().c_str());
							}
							break;
						case MVT_BOOL:
							{
								BOOL value;
								msg->getBOOLFast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
							break;
						case MVT_IP_ADDR:
							{
								U32 value;
								msg->getIPAddrFast(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, LLHost(value, 0).getIPString().c_str());
							}
							break;
						case MVT_IP_PORT:
							{
								U16 value;
								msg->getIPPort(block_name, var_name, value, block_num);
								block_variable_table.pushvalue(L, value);
							}
						case MVT_VARIABLE:
						case MVT_FIXED:
						default:
							{
								S32 size = msg->getSize(block_name, block_num, var_name);
								if(size)
								{
									std::ostringstream stream;
									char* value = new char[size + 1];
									msg->getBinaryDataFast(block_name, var_name, value, size, block_num);
									value[size] = '\0';
									S32 readable = 0;
									S32 unreadable = 0;
									for(S32 i = 0; i < size; i++)
									{
										if(!value[i])
										{
											if(i != (size - 1))
											{ // don't want null terminator hiding data
												unreadable = S32_MAX;
												break;
											}
										}
										else if(value[i] < 0x20 || value[i] >= 0x7F)
										{
											unreadable = S32_MAX;
											break;
										}
										else readable++;
									}
									if(readable >= unreadable)
									{
										stream << value;
				
										delete[] value;
									}
									else
									{
										for(S32 i = 0; i < size; i++)
											stream << llformat("%02X ", (U8)value[i]);
									}
									block_variable_table.pushvalue(L, stream.str().c_str());
								}
								else
								{
									block_variable_table.pushvalue(L,""); //empty string
								}
							}
							break;
						} //end switch
					}
					block_table.push(L);
				}
				main_table.push(L);
			} //end block num check
		} //NULL check.
	} // blocks_iter
	dump_stack(L);

	//call lua function
	if(lua_pcall(L, 1, 0, 0))
	{
		LUA_ERRORED("LunaMessageHandler");
	}
	LL_INFOS(LunaMessageHandler::className) << "called message handler bootstrap" << LL_ENDL;
}

int LunaMessageHandler::connect(lua_State* L)
{
	const char* lua_name = luaL_checkstring(L, 2); //do not delete.
	std::map<const char *, LLMessageTemplate*>::iterator template_iter;
	const char* message_name = LLMessageStringTable::getInstance()->getString(lua_name);
	template_iter = gMessageSystem->mMessageTemplates.find( message_name );
	if(template_iter == gMessageSystem->mMessageTemplates.end())
	{
		LUA_ERROR(llformat("Don't know how to handle a '%s' message", message_name));
		return 0;
	}
	mMessageTemplate = template_iter->second;
	mMessageConnection = gMessageSystem->addHandlerFuncFast(message_name, boost::bind(&LunaMessageHandler::slot_func,this,_1));
	return 0;
}

int LunaMessageHandler::disconnect(lua_State* L)
{
	mMessageConnection.disconnect();
	return 0;
}