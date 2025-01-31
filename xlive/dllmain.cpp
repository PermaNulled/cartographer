#include "stdafx.h"

#include "cartographer/discord/discord_interface.h"
#include "interface/new_hud_draw.h"
#include "tag_files/tag_loader/tag_injection.h"

#include "H2MOD/Modules/Shell/Startup/Startup.h"
#include "H2MOD/Modules/Shell/Config.h"

#ifdef TEST_DISCORD_INSTANCE
#include "H2MOD/Modules/Shell/H2MODShell.h"
#endif


/* constants */

#define k_discord_dll_filename L"discord_game_sdk.dll"

/* globals */

HMODULE hThis = NULL;

/* externs */

extern CRITICAL_SECTION log_section;

/* prototypes */

void discord_initialize(void);
void discord_dispose(void);
void heap_debug_initialize(void);
void initialize_instance(void);
void exit_instance(void);

/* entry point */

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hThis = hModule;
		srand((unsigned int)time(NULL));
		InitH2Startup();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		exit_instance();
		break;
	}
	return TRUE;
}

/* private code */

void discord_initialize(void)
{
	HMODULE module = LoadLibraryW(k_discord_dll_filename);
	if (module && !Memory::IsDedicatedServer()
		&& H2Config_discord_enable
#ifdef TEST_DISCORD_INSTANCE
		&& _Shell::GetInstanceId() == 1
#endif
		)
	{
		discord_game_status_create(module);
	}

	return;
}

void discord_dispose(void)
{
	if (!Memory::IsDedicatedServer()
		&& H2Config_discord_enable
#ifdef TEST_DISCORD_INSTANCE
		&& _Shell::GetInstanceId() == 1
#endif
		)
	{
		discord_game_status_dispose();
	}
	return;
}

void heap_debug_initialize(void)
{
#if CARTOGRAPHER_HEAP_DEBUG
	int CurrentFlags;
	CurrentFlags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	CurrentFlags |= _CRTDBG_DELAY_FREE_MEM_DF;
	CurrentFlags |= _CRTDBG_LEAK_CHECK_DF;
	CurrentFlags |= _CRTDBG_CHECK_ALWAYS_DF;
	_CrtSetDbgFlag(CurrentFlags);
#endif
	return;
}

void initialize_instance(void)
{
	static bool init = false;

	if (!init)
	{
		init = true;

		heap_debug_initialize();
		H2DedicatedServerStartup();

		discord_initialize();
	}
	return;
}

void exit_instance(void)
{
	discord_dispose();
	tag_injection_deinitialize();
	new_hud_draw_deinitialize();

#ifndef NO_TRACE
	EnterCriticalSection(&log_section);
	delete xlive_log;
	delete h2mod_log;
	delete network_log;
	delete console_log;
	delete onscreendebug_log;

	LeaveCriticalSection(&log_section);
	DeleteCriticalSection(&log_section);
#endif
#if CARTOGRAPHER_HEAP_DEBUG
	_CrtDumpMemoryLeaks();
#endif
	TerminateProcess(GetCurrentProcess(), 0);
	return;
}
