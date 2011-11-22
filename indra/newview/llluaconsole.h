#ifndef LL_LUA_CONSOLE_H
#define LL_LUA_CONSOLE_H

#include "llfloater.h"
#include "llthread.h"
#include "lllineeditor.h"
#include "lllua.h"

#include <queue>

class LLFloaterLuaConsole : public LLFloater, public LLFloaterSingleton<LLFloaterLuaConsole>
{
	friend class LLUISingleton<LLFloaterLuaConsole, VisibilityPolicy<LLFloater> >;
public:
        LLFloaterLuaConsole(const LLSD& key);
        ~LLFloaterLuaConsole();

        /*virtual*/ BOOL postBuild();
        /*virtual*/ void draw();
        /*virtual*/ void onClose(bool app_quitting = false);

		void addOutput(std::string output, bool error);

private:
		static void	onInputEditorGainFocus(LLFocusableElement* caller,void* userdata);
        static void onClickSend(void*);
        static void onClickClear(void*);
        static void onClickAbort(void*);
        static void onClickReset(void*);
};

#endif