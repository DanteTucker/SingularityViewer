#include "llviewerprecompiledheaders.h"
#include "llluaconsole.h"
#include "lluictrlfactory.h"
#include "llviewertexteditor.h"
#include "llcallbacklist.h"
#include "llviewercontrol.h"
#include "llkeyboard.h"

LLFloaterLuaConsole::LLFloaterLuaConsole(const LLSD& key) :
LLFloater(std::string("Lua Console"))
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_lua_console.xml");
}

LLFloaterLuaConsole::~LLFloaterLuaConsole()
{
}

void LLFloaterLuaConsole::onClose(bool app_quitting)
{
	setVisible(FALSE);
}

BOOL LLFloaterLuaConsole::postBuild()
{
	childSetAction("Send", onClickSend, this);
	childSetAction("Clear", onClickClear, this);
	childSetAction("Abort", onClickAbort, this);
	childSetAction("Reset", onClickReset, this);

	//childDisable("Send");
	LLButton * sendp = getChild<LLButton>("Send");
	LLPanel * luap = getChild<LLPanel>("lua_panel");
	if(sendp && luap)
	{
		luap->setDefaultBtn(sendp);
	}

	LLLineEditor * editorp = getChild<LLLineEditor>("Lua Editor", TRUE);
	if(editorp)
	{
		editorp->setCommitOnFocusLost(FALSE);
		editorp->setRevertOnEsc(FALSE);
		editorp->setEnableLineHistory(TRUE);
		editorp->setFocusReceivedCallback( &onInputEditorGainFocus, this );
	}
	getChild<LLViewerTextEditor>("Lua Output Editor",TRUE);
	return TRUE;
}

void LLFloaterLuaConsole::draw()
{
	LLFloater::draw();
}
void LLFloaterLuaConsole::onInputEditorGainFocus( LLFocusableElement* caller, void* userdata )
{
	LLFloaterLuaConsole *self = (LLFloaterLuaConsole *)userdata;
	LLViewerTextEditor *editor = self->getChild<LLViewerTextEditor>("Lua Output Editor");
	editor->setCursorAndScrollToEnd();
}
void LLFloaterLuaConsole::onClickSend(void *data)
{
	LLFloaterLuaConsole *self = (LLFloaterLuaConsole *)data;
	LLLineEditor *editor = self->getChild<LLLineEditor>("Lua Editor", TRUE);

	if(editor->getLength()) 
	{
		LLColor4 text_color = gSavedSettings.getColor4("llOwnerSayChatColor");
		LLViewerTextEditor *out = self->getChild<LLViewerTextEditor>("Lua Output Editor");
		out->appendColoredText("] "+editor->getText(), false, true, text_color); //echo command, like a proper console.
		LLLuaEngine::getInstance()->console(editor->getText());
		editor->updateHistory();
		editor->clear();
	}
}

void LLFloaterLuaConsole::onClickAbort(void *data)
{
	LLLuaEngine::getInstance()->deleteSingleton(); //Unloads Lua
}

void LLFloaterLuaConsole::onClickReset(void *data)
{
    LLFloaterLuaConsole *self = (LLFloaterLuaConsole *)data;

	LLLuaEngine::getInstance()->deleteSingleton();
	LLLuaEngine::getInstance()->load();

    LLLineEditor *editor = self->getChild<LLLineEditor>("Lua Editor", TRUE);

    if(editor->getLength()) 
	{
		editor->updateHistory();
		editor->clear();
	}
}

void LLFloaterLuaConsole::onClickClear(void *data)
{
	LLFloaterLuaConsole *self = (LLFloaterLuaConsole *)data;
	LLViewerTextEditor *editor = self->getChild<LLViewerTextEditor>("Lua Output Editor");
	editor->removeTextFromEnd(editor->getWText().length());
	editor->makePristine();
}

void LLFloaterLuaConsole::addOutput(std::string output, bool error)
{
	if(!instanceVisible())
	{
		showInstance();
	}
	LLColor4 text_color;
	if(error)
		text_color = gSavedSettings.getColor4("ScriptErrorColor");
	else
		text_color = gSavedSettings.getColor4("ObjectChatColor");

	LLViewerTextEditor *editor = getChild<LLViewerTextEditor>("Lua Output Editor");
	editor->setParseHTML(TRUE);
	editor->setParseHighlights(TRUE);
	editor->appendColoredText(output, false, true, text_color);
}
