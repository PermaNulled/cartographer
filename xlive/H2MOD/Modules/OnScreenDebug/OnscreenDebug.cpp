#include "stdafx.h"

#include "H2MOD/Modules/Shell/Startup/Startup.h"
#include "H2MOD/GUI/ImGui_Integration/Console/ImGui_ConsoleImpl.h"

bool initialisedDebugText = false;

// we change global variables, async debug text could result in hazzard
std::recursive_mutex addTextMutex;

/*
//
// TODO remove entirely
// and replace with console code
//
*/

void addDebugTextInternal(char* text) {

	if (!initialisedDebugText) return;

	int lenInput = strlen(text);

	char* endChar = strchr(text, '\n');
	if (endChar) {
		lenInput = endChar - text;
	}

	std::lock_guard lg(addTextMutex);

	CircularStringBuffer* output = GetMainConsoleInstance()->GetTabOutput(_console_tab_logs);
	output->AddString(StringFlag_None, text, lenInput);
	onscreendebug_log->debug(text);

	if (endChar) {
		return addDebugTextInternal(endChar + 1);
	}
}

void addDebugText(const wchar_t* format, ...)
{
	va_list valist;
	va_start(valist, format);

	/* get the formatted buffer size */
	int stringLength = _vscwprintf(format, valist) + 1; // +1 adds null characeter, "_vscwprintf" doesn't add it

	if (stringLength == -1)
	{
		LOG_TRACE_GAME("{} - error trying to get string length size", __FUNCTION__);
		return;
	}

	wchar_t* textBufferW = (wchar_t*)calloc(stringLength, sizeof(wchar_t));
	_vswprintf(textBufferW, format, valist);

	char* textBufferA = (char*)calloc(stringLength, sizeof(char));
	_snprintf(textBufferA, stringLength, "%ls", textBufferW);

	addDebugTextInternal(textBufferA);

	free(textBufferW);
	free(textBufferA);
	va_end(valist);
}

void addDebugText(const char* format, ...)
{
	va_list valist;
	va_start(valist, format);

	/* get the formatted buffer size */
	int stringLength = _vscprintf(format, valist) + 1; // +1 adds null characeter, "_vscwprintf" doesn't add it

	if (stringLength == -1)
	{
		LOG_TRACE_GAME("{} - error trying to get string length size", __FUNCTION__);
		return;
	}

	char* textBufferA = (char*)calloc(stringLength, sizeof(char));
	vsprintf(textBufferA, format, valist);

	addDebugTextInternal(textBufferA);

	free(textBufferA);
	va_end(valist);
}

void InitOnScreenDebugText() {
	initialisedDebugText = true;

	c_static_wchar_string<MAX_PATH> path;
	prepareLogFileName(L"h2onscreendebug", &path);
	onscreendebug_log = h2log::create("OnScreenDebug", path.get_string(), true, 0); // we always create onscreendebuglog, which logs everything (log level 0)
	addDebugText("Initialized onscreendebug log");
}