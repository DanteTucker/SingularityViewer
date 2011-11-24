#ifndef LUNA_H
#define LUNA_H

/*****************************************************************************
 *     .:: Luna V5.1 ::.                                                     *
 *                                                                           *
 *  C++ library for binding classes into Lua.     By miningold               *
 *                                                                           *
 *  Credits: nornagon  (http://lua-users.org/wiki/LunaWrapper)               *
 *           'unknown' (http://lua-users.org/wiki/LunaFour)                  *
 *                                                                           *
 ****************************************************************************/

// convenience macros
#define luna_register(L, klass) (Luna<klass>::Register((L)))
#define luna_registermetatable(L, klass) (Luna<klass>::RegisterMetatable((L)))
#define luna_inject(L, klass, t) (Luna<klass>::inject((L), (t)))

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

template<class T> 
class Luna 
{
public:
    static void Register(lua_State *L) 
    {
        lua_pushcfunction(L, &Luna<T>::constructor);
        lua_setglobal(L, T::className); // T() in lua will make a new instance.

        RegisterMetatable(L);
    }

    // register the metatable without registering the class constructor
    static void RegisterMetatable(lua_State *L) 
    {
        // create a metatable in the registry
        luaL_newmetatable(L, T::className); 
        int metatable = lua_gettop(L);
    
        lua_pushstring(L, "__gc");
        lua_pushcfunction(L, &Luna<T>::gc_obj);
        // metatable["__gc"] = Luna<T>::gc_obj
        lua_settable(L, metatable); 
            
        lua_pushstring(L, "__index");
        lua_pushcfunction(L, &Luna<T>::property_getter);
        // mettable["__index"] = Luna<T>::property_getter
        lua_settable(L, metatable); 
        
        lua_pushstring(L, "__newindex");
        lua_pushcfunction(L, &Luna<T>::property_setter);
        // mettable["__newindex"] = Luna<T>::property_setter
        lua_settable(L, metatable); 
            
        lua_pop(L, 1);
    }

    static int constructor(lua_State *L) 
    {
        return inject(L, new T(L));
    }

    static int inject(lua_State *L, T* obj) 
    {
        lua_newtable(L); // create a new table for the class object ('self')
            
        int newtable = lua_gettop(L);

        lua_pushnumber(L, 0);

        // store a ptr to the ptr
        T** a = static_cast<T**>(lua_newuserdata(L, sizeof(T*))); 
        *a = obj; // set the ptr to the ptr to point to the ptr... >.>
        luaL_newmetatable(L, T::className);//get (or create) the metatable
        lua_setmetatable(L, -2); // self.metatable = uniqe_metatable

        lua_settable(L, -3); // self[0] = obj;
        
        luaL_getmetatable(L, T::className);
        lua_setmetatable(L, newtable);
        
        luaL_getmetatable(L, T::className);
        
        for (int i = 0; T::Properties[i].name; i++) 
        {
            lua_pushstring(L, T::Properties[i].name);
            lua_pushnumber(L, i);
            lua_settable(L, -3);
        }
        
        lua_pop(L, 1);

        for (int i = 0; T::Functions[i].name; i++) 
        { // register the functions
            lua_pushstring(L, T::Functions[i].name);
            lua_pushnumber(L, i); // let the thunk know which method we mean
            lua_pushcclosure(L, &Luna<T>::thunk, 1);
            lua_settable(L, newtable); // self["function"] = thunk("function")
        }
        return 1;
    }

    static int thunk(lua_State *L) 
    {
        // redirect method call to the real thing
        int i = (int)lua_tonumber(L, lua_upvalueindex(1)); // which function?
        lua_pushnumber(L, 0);
        lua_gettable(L, 1); // get the class table (i.e, self)

        T** obj = static_cast<T**>(luaL_checkudata(L, -1, T::className));
        lua_remove(L, -1); // remove the userdata from the stack

        return ((*obj)->*(T::Functions[i].function))(L); // execute the thunk
    }
    
    static int property_getter(lua_State * L) 
    {
        lua_pushvalue(L, 2);

        lua_getmetatable(L, 1);

        lua_pushvalue(L, 2);
        lua_rawget(L, -2);

        if (lua_isnumber(L, -1)) {

            int _index = lua_tonumber(L, -1);

            lua_pushnumber(L, 0);
            lua_rawget(L, 1);

            T** obj = static_cast <T**>(lua_touserdata(L, -1));

            lua_pushvalue(L, 3);

            //const PropertyType* _properties = (*obj)->T::Properties;

            int result = ((*obj)->*(T::Properties[_index].getter)) (L);

            return result;
        }
        // PUSH NIL 
        lua_pushnil(L);

        return 1;
    }
    
    static int property_setter(lua_State * L) 
    {
        
        lua_getmetatable(L, 1);

        lua_pushvalue(L, 2);
        lua_rawget(L, -2);

        if (lua_isnil(L, -1)) 
        {
            lua_pop(L, 2);

            lua_rawset(L, 1);

            return 0;
        } 
        else 
        {
            int _index = lua_tonumber(L, -1);

            lua_pushnumber(L, 0);
            lua_rawget(L, 1);

            T** obj = static_cast <T**>(lua_touserdata(L, -1));

            lua_pushvalue(L, 3);

            //const PropertyType *_properties = (*obj)->T::Properties;

            return ((*obj)->*(T::Properties[_index].setter)) (L);
        }
    }
    

    static int gc_obj(lua_State *L) 
    {
        // clean up
        //printf("GC called: %s\n", T::className);
        T** obj = static_cast<T**>(luaL_checkudata(L, -1, T::className));
        delete (*obj);
        return 0;
    }
    
    struct PropertyType 
    {
        const char *name;
        int (T::*getter) (lua_State *);
        int (T::*setter) (lua_State *);
    };

    struct FunctionType 
    {
        const char *name;
        int (T::*function) (lua_State *);
    };
};


#endif /* LUNA_H */
