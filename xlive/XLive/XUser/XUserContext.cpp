#include "stdafx.h"
#include "XUserContext.h"

#include "cartographer/discord/discord_interface.h"
#include "cseries/cseries_strings.h"
#include "game/game.h"
#include "networking/logic/life_cycle_manager.h"
#include "text/unicode.h"

#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Modules/Shell/H2MODShell.h"
#include "XLive/xbox/xbox.h"

enum e_context_id : uint32
{
	_context_id_variant = 2,
	_context_id_difficulty = 3,
	_context_id_map = 5,
	_context_id_presence = X_CONTEXT_PRESENCE,
	_context_id_game_type = X_CONTEXT_GAME_TYPE,
	_context_id_game_mode = X_CONTEXT_GAME_MODE
};

struct s_context_campaign_map_info
{
	int32 map_id;
	char map_name[32];
};

const s_context_campaign_map_info k_context_campaign_map_info[] = {
	1, "The Heretic",
	101, "Armory",
	105, "Cairo Station",
	301, "Outskirts",
	305, "Metropolis",
	401, "The Arbiter",
	405, "Oracle",
	501, "Delta Halo",
	505, "Regret",
	601, "Sacred Icon",
	605, "Quarantine Zone",
	701, "Gravemind",
	801, "High Charity",
	705, "Uprising",
	805, "The Great Journey"
};

extern XSESSION_LOCAL_DETAILS sessionDetails;

// 5289: XUserGetContext
DWORD WINAPI XUserGetContext(DWORD dwUserIndex, XUSER_CONTEXT* pContext, PXOVERLAPPED  pOverlapped)
{
	return S_OK;
}

void context_update_map_info_multiplayer(void)
{
	// Get map name and convert to utf8
	wchar_t* wide_map_name = Memory::GetAddress<wchar_t*>(0x97737C);
	utf8 map_name[512];
	wchar_string_to_utf8_string(wide_map_name, map_name, sizeof(map_name));

	// Get scenario name and convert to utf8
	c_static_wchar_string<260> scenario_path(game_options_get()->scenario_path);
	int32 index = scenario_path.last_index_of(L"\\");
	const wchar_t* scenario_name_wide = &scenario_path.get_string()[index + 1];
	utf8 scenario_name[MAX_PATH];
	wchar_string_to_utf8_string(scenario_name_wide, scenario_name, sizeof(scenario_name));

	// Update interface
	discord_interface_set_map_name(scenario_name, map_name);
}

void context_update_map_info_campaign(uint32 map_id, const utf8* scenario_name)
{
	// Get map name
	const char* map_name = NULL;
	for (uint32 i = 0; i < NUMBEROF(k_context_campaign_map_info); ++i)
	{
		if (k_context_campaign_map_info[i].map_id == map_id)
		{
			map_name = k_context_campaign_map_info[i].map_name;
			break;
		}
	}

	// Convert map_name to utf8
	if (map_name == NULL)
	{
		map_name = scenario_name;
	}

	// Update interface
	discord_interface_set_map_name(scenario_name, map_name);
}

// #5277: XUserSetContext
DWORD WINAPI XUserSetContext(DWORD dwUserIndex, DWORD dwContextId, DWORD dwContextValue)
{
	LOG_TRACE_XLIVE("XUserSetContext  (userIndex = {0}, contextId = {1}, contextValue = {2})",
		dwUserIndex, dwContextId, dwContextValue);

	if (Memory::IsDedicatedServer() || !H2Config_discord_enable || _Shell::GetInstanceId() > 1)
	{
		return ERROR_SUCCESS;
	}

	switch ((e_context_id)dwContextId)
	{
	case _context_id_variant:
	{
		utf8 variant_name[32];
		wchar_string_to_utf8_string(NetworkSession::GetGameVariantName(), variant_name, sizeof(variant_name));
		discord_interface_set_variant((e_context_variant)dwContextValue, variant_name);
		break;
	}
	/* The way hired gun calls XUserSetContext in this situation is bad, don't do this here
	* Instead we set difficulty on map load with the singleplayer map name
	case _context_id_difficulty:
	{
		discord_interface_set_difficulty(dwContextValue);
		break;
	}*/
	case _context_id_map:
	{
		context_update_map_info_multiplayer();
		break;
	}
	case _context_id_presence:
	{
		switch ((e_context_presence)dwContextValue)
		{
		case _context_presence_singleplayer:
		{
			discord_interface_set_state("Singleplayer");
			break;
		}
		case _context_presence_public_game:
		case _context_presence_invite_only_game:
		case _context_presence_network_in_game:
		case _context_presence_live_in_game:
		{
			discord_interface_set_state("Multiplayer");
			break;
		}
		case _context_presence_mainmenu:
		{
			discord_interface_set_state("In main menu");
			discord_interface_set_details("");
			discord_interface_set_large_image("default", "");
			discord_interface_set_small_image("", "");
			break;
		}
		case _context_presence_lobby:
		{
			discord_interface_set_state("In Lobby");
			discord_interface_set_details("");
			discord_interface_set_large_image("default", "");
			discord_interface_set_small_image("", "");
			break;
		}
		case _context_presence_server_browser:
		{
			discord_interface_set_state("Browsing server list");
			
			// Zero player counts here as the game still thinks we are in a network session after leaving a match
			discord_interface_zero_player_count();

			discord_interface_set_details("");
			discord_interface_set_large_image("default", "");
			discord_interface_set_small_image("", "");
			break;
		}
		case _context_presence_results:
		{
			discord_interface_set_state("Reading carnage report");
			discord_interface_set_details("");
			discord_interface_set_large_image("default", "");
			discord_interface_set_small_image("", "");
			break;
		}
		case _context_presence_settings:
			discord_interface_set_state("Adjusting settings");
			break;
		}
		break;
	}
	case _context_id_game_type:
		LOG_TRACE_XLIVE("- X_CONTEXT_GAME_TYPE = {}", dwContextValue);
		sessionDetails.dwGameType = dwContextValue;
		break;
	case _context_id_game_mode:
		LOG_TRACE_XLIVE("- X_CONTEXT_GAME_MODE = {:x}", dwContextValue);
		sessionDetails.dwGameMode = dwContextValue;
		break;
	default:
		break;
	}

	return ERROR_SUCCESS;
}


// #5292: XUserSetContextEx
int WINAPI XUserSetContextEx(DWORD dwUserIndex, DWORD dwContextId, DWORD dwContextValue, PXOVERLAPPED pOverlapped)
{
	LOG_TRACE_XLIVE("XUserSetContextEx  (userIndex = {0}, contextId = {1}, contextValue = {2:x}, pOverlapped = {3:p})",
		dwUserIndex, dwContextId, dwContextValue, (void*)pOverlapped);
	return ERROR_SUCCESS;
}
