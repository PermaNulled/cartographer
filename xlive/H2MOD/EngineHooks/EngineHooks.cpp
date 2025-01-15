#include "stdafx.h"

#include "EngineHooks.h"
#include "networking/logic/life_cycle_manager.h"
#include "H2MOD/Modules/EventHandler/EventHandler.hpp"


namespace EngineHooks
{
	static e_game_life_cycle previousGamestate = _life_cycle_none;

	game_life_cycle_update p_game_life_cycle_update;
	void GameLifeCycleUpdate()
	{
		auto gameLifeCycleData = Memory::GetAddress<BYTE*>(0x420FC4, 0x3C40AC);
		p_game_life_cycle_update(gameLifeCycleData);

		e_game_life_cycle currentLifeCycle = *(e_game_life_cycle*)(gameLifeCycleData);
		if (previousGamestate != currentLifeCycle) {
			previousGamestate = currentLifeCycle;
			EventHandler::GameLifeCycleEventExecute(EventExecutionType::execute_after, currentLifeCycle);
		}
	}

	main_game_reset_map* p_main_game_reset_map;
	void __cdecl main_game_reset_map_blue_screen_detection()
	{
		p_main_game_reset_map();
		EventHandler::BlueScreenEventExecute(EventExecutionType::execute_after);
		LOG_TRACE_GAME("[{}] Bluescreen Detected.", __FUNCTION__);
	}

#pragma region Game Version hooks
	bool __cdecl verify_game_version_on_join_hook(uint8 executable_type, uint16 executable_version, uint16 compatible_version)
	{
		return executable_type == EXECUTABLE_TYPE && executable_version >= EXECUTABLE_VERSION && compatible_version <= COMPATIBLE_VERSION;
	}

	bool __cdecl verify_executable_type_hook(BYTE executable_type)
	{
		// will not display servers that don't match this in server list
		return executable_type == EXECUTABLE_TYPE;
	}

	void __cdecl get_game_version_hook(BYTE* executable_type, uint16* executable_version, uint16* compatible_version)
	{
		*executable_type = EXECUTABLE_TYPE;
		*executable_version = EXECUTABLE_VERSION;
		*compatible_version = COMPATIBLE_VERSION;
	}

	struct s_session_live_result
	{
		uint8 gap_0[116];
		int16 executable_type;
		int32 executable_version;
		int32 compatible_version;
	};
	ASSERT_STRUCT_OFFSET(s_session_live_result, executable_type, 116);

	typedef bool(__stdcall* t_xlocator_parse_search_result)(void* thisx, int a2, s_session_live_result* session_out);
	t_xlocator_parse_search_result p_xlocator_parse_search_result;

	bool __stdcall xlocator_parse_search_result(void* thisx, int a2, s_session_live_result* session_out)
	{
		bool result = p_xlocator_parse_search_result(thisx, a2, session_out);
		return result && verify_game_version_on_join_hook((uint8)session_out->executable_type, (uint16)session_out->executable_version, (uint16)session_out->compatible_version);
	}
#pragma endregion

	void ApplyHooks()
	{
		/*Game Life Cycle Change Detection*/
		p_game_life_cycle_update = Memory::GetAddress<game_life_cycle_update>(0x1d7738, 0x1BCDA8);
		PatchCall(Memory::GetAddress(0x1AD84D, 0x1A67CA), GameLifeCycleUpdate);

		/*Blue Screen detection*/
		p_main_game_reset_map = Memory::GetAddress<main_game_reset_map*>(0x9763, 0x1FA4E);
		PatchCall(Memory::GetAddress(0x1AE82F, 0x1A8A89), main_game_reset_map_blue_screen_detection);

		/*Game Version Hooks*/
		DetourFunc(Memory::GetAddress<BYTE*>(0x1B4BF5, 0x1B0043), (BYTE*)get_game_version_hook, 8);
		if (!Memory::IsDedicatedServer()) {
			DetourFunc(Memory::GetAddress<BYTE*>(0x1B4C14), (BYTE*)verify_game_version_on_join_hook, 5);
			DetourFunc(Memory::GetAddress<BYTE*>(0x1B4C32), (BYTE*)verify_executable_type_hook, 8);
			p_xlocator_parse_search_result = (t_xlocator_parse_search_result)DetourClassFunc(Memory::GetAddress<BYTE*>(0x1DA8ED), (BYTE*)xlocator_parse_search_result, 8);
		}
	}
}
