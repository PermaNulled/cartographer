#include "stdafx.h"
#include "discord_interface.h"

#include <discord_game_sdk.h>

#include "cseries/cseries_strings.h"
#include "main/main_game.h"
#include "game/game.h"
#include "networking/logic/life_cycle_manager.h" 

/* constants */

#define k_discord_client_id 379371722685808641
#define k_thread_close_wait_time_ms 2000

const char* k_discord_difficulty_names[k_campaign_difficulty_levels_count] = { "Easy", "Normal", "Heroic", "Legendary" };
const char* k_discord_gamemode_names[] = { "ctf", "slayer", "oddball", "koth", "juggernaut", "territories", "assault" };
const char* k_valid_scenario_names[] = {
	// singleplayer maps
	"00a_introduction",
	"01a_tutorial",
	"01b_spacestation",
	"03a_oldmombasa",
	"03b_newmombasa",
	"04a_gasgiant",
	"04b_floodlab",
	"05a_deltaapproach",
	"05b_deltatowers",
	"06a_sentinelwalls",
	"06b_floodzone",
	"07a_highcharity",
	"07b_forerunnership",
	"08a_deltacliffs",
	"08b_deltacontrol",

	// multiplayer maps
	"ascension",
	"backwash",
	"beavercreek",
	"burial_mounds",
	"coagulation",
	"colossus",
	"containment",
	"cyclotron",
	"deltatap",
	"dune",
	"elongation",
	"foundation",
	"gemini",
	"headlong",
	"highplains",
	"lockout",
	"midship",
	"needle",
	"street_sweeper",
	"triplicate",
	"tombstone",
	"turf",
	"warlock",
	"waterworks",
	"zanzibar",

	// custom maps
	"salvation"
};

#define k_valid_scenario_name_count NUMBEROF(k_valid_scenario_names)

/* structures */

struct s_discord_data
{
	IDiscordCore* core;
	IDiscordUserManager* users;
	IDiscordActivityManager* activities;
	IDiscordLobbyManager* lobbies;
	DiscordUserId user_id;
};

struct s_discord_globals
{
	DiscordActivity activity;
	HANDLE thread;
	HMODULE dll_module;
	bool update_rich_presence;
	volatile LONG should_stop;
};

s_discord_globals g_discord_globals =
{
	{},
	INVALID_HANDLE_VALUE,
	0,
	true,
	FALSE
};

/* prototypes */

// Entry point for discord thread
unsigned __stdcall discord_thread_proc(void* pArguments);

// Main loop for discord logic
uint32 discord_update(s_discord_data* discord);

// Update function for rich presence
void discord_rich_presence_update(s_discord_data* discord);

// Returns true if the scenario name has a valid discord image associated with it
bool discord_interface_scenario_name_vaild(const utf8* scnr_name);

// Encode xsession info as a string
void discord_interface_encode_xsession_info(XSESSION_INFO* session_info);

void discord_interface_update_details(void);

void DISCORD_CALLBACK on_user_updated(void* data);
void DISCORD_CALLBACK on_activity_join(void* event_data, const char* secret);
void DISCORD_CALLBACK on_discord_log_print(void* hook_data, enum EDiscordLogLevel level, const char* message);
void DISCORD_CALLBACK on_rich_presence_updated(void* data, EDiscordResult res);


/* public code */

void discord_game_status_create(HMODULE module)
{
	g_discord_globals.dll_module = module;
	g_discord_globals.thread = (HANDLE)_beginthreadex(NULL, 0, &discord_thread_proc, NULL, 0, NULL);
	return;
}

void discord_game_status_dispose(void)
{
	InterlockedAdd(&g_discord_globals.should_stop, 1);
	WaitForSingleObject(g_discord_globals.thread, k_thread_close_wait_time_ms);
	CloseHandle(g_discord_globals.thread);
	FreeLibrary(g_discord_globals.dll_module);
	return;
}

void discord_interface_set_details(const char* details)
{
	csstrnzcpy(g_discord_globals.activity.details, details, sizeof(g_discord_globals.activity.details));
	g_discord_globals.update_rich_presence = true;
	return;
}

void discord_interface_set_state(const char* state)
{
	discord_interface_set_player_counts();
	csstrnzcpy(g_discord_globals.activity.state, state, sizeof(g_discord_globals.activity.state));
	g_discord_globals.update_rich_presence = true;
	return;
}

void discord_interface_set_small_image(const char* small_image, const char* small_text)
{
	csstrnzcpy(g_discord_globals.activity.assets.small_image, small_image, sizeof(g_discord_globals.activity.assets.small_image));
	csstrnzcpy(g_discord_globals.activity.assets.small_text, small_text, sizeof(g_discord_globals.activity.assets.small_text));
	g_discord_globals.update_rich_presence = true;
	return;
}

void discord_interface_set_large_image(const char* large_image, const char* large_text)
{
	csstrnzcpy(g_discord_globals.activity.assets.large_image, large_image, sizeof(g_discord_globals.activity.assets.large_image));
	csstrnzcpy(g_discord_globals.activity.assets.large_text, large_text, sizeof(g_discord_globals.activity.assets.large_text));
	g_discord_globals.update_rich_presence = true;
	return;
}

void discord_interface_set_map_name(const utf8* scenario_name, const utf8* map_name)
{
	bool valid_scnr = discord_interface_scenario_name_vaild(scenario_name);

	// If the scenario has a valid image set it to that name
	// If not set it to "unknown_map" to display the proper image
	const utf8* name = (valid_scnr ? scenario_name : "unknown_map");

	discord_interface_set_large_image(name, map_name);
	discord_interface_update_details();
	return;
}

void discord_interface_set_variant(e_context_variant variant, const utf8* variant_name)
{
	bool valid_variant = VALID_INDEX(variant, k_context_variant_count);

	// If the variant is valid set the name
	// If not set it to "unknown" to display the proper image
	const utf8* name = (valid_variant ? k_discord_gamemode_names[variant] : "unknown");


	// Create image name we select for the difficulty
	c_static_string<32> variant_image_name;
	variant_image_name.set("gamemode_");
	variant_image_name.append(name);

	// Set image name and text
	discord_interface_set_small_image(variant_image_name.get_string(), variant_name);
	discord_interface_update_details();
	return;
}

void discord_interface_set_difficulty(int16 difficulty)
{
	// Make sure we don't go out of bounds
	difficulty = PIN(difficulty, 0, k_campaign_difficulty_levels_count - 1);
	
	// Convert difficulty to string
	char number_string[2];
	snprintf(number_string, sizeof(number_string), "%hd", difficulty);

	// Create image name we select for the difficulty
	c_static_string<16> difficulty_image_name;
	difficulty_image_name.set("diff_");
	difficulty_image_name.append(number_string);

	// Set image name and text
	csstrnzcpy(g_discord_globals.activity.assets.small_image, difficulty_image_name.get_string(), difficulty_image_name.max_length());
	csstrnzcpy(g_discord_globals.activity.assets.small_text, k_discord_difficulty_names[difficulty], 16);
	discord_interface_update_details();
	return;
}

void discord_interface_zero_player_count(void)
{
	g_discord_globals.activity.party.size.current_size = 0;
	g_discord_globals.activity.party.size.max_size = 0;
	return;
}

void discord_interface_set_player_counts(void)
{
	c_network_session* session = NULL;
	if (network_life_cycle_in_squad_session(&session))
	{
		g_discord_globals.activity.party.size.current_size = session->m_session_membership.player_count;
		g_discord_globals.activity.party.size.max_size = session->m_session_parameters.max_party_players;
	}
	else
	{
		discord_interface_zero_player_count();
	}
	g_discord_globals.update_rich_presence = true;
	return;
}


/* private code */

unsigned __stdcall discord_thread_proc(void* pArguments)
{
	// Set discord instance based on instance ID
#ifdef TEST_DISCORD_INSTANCE
	int32 instance_id = _Shell::GetInstanceId();
	wchar_t instance[2]{};
	swprintf(instance, 2, L"%d", instance_id - 1);
	SetEnvironmentVariableW(L"DISCORD_INSTANCE_ID", instance);
#endif

	uint32 result = 0;

	IDiscordUserEvents users_events = {};
	IDiscordActivityEvents activities_events = {};
	s_discord_data discord = {};

	users_events.on_current_user_update = on_user_updated;
	activities_events.on_activity_join = on_activity_join;

	DiscordCreateParams params = {};
	DiscordCreateParamsSetDefault(&params);
	params.client_id = k_discord_client_id;
	params.flags = DiscordCreateFlags_NoRequireDiscord;
	params.event_data = &discord;
	params.user_events = &users_events;
	params.activity_events = &activities_events;

	typedef int(__cdecl* MYPROC)(LPCWSTR);
	decltype(DiscordCreate)* create = (decltype(DiscordCreate)*)GetProcAddress(g_discord_globals.dll_module, "DiscordCreate");
	EDiscordResult res = create(DISCORD_VERSION, &params, &discord.core);

	if (res == DiscordResult_Ok)
	{
		discord.core->set_log_hook(discord.core, DiscordLogLevel_Debug, &discord, on_discord_log_print);
		discord.users = discord.core->get_user_manager(discord.core);
		discord.activities = discord.core->get_activity_manager(discord.core);
		discord.lobbies = discord.core->get_lobby_manager(discord.core);
		discord.activities->register_command(discord.activities, "halo2.exe");

		// Main loop
		while (!g_discord_globals.should_stop)
		{
			Sleep(discord_update(&discord));
		}

		// Cleanup
		discord.activities->clear_activity(discord.activities, &discord, on_rich_presence_updated);
		discord.core->destroy(discord.core);
	}
	else
	{
		error(2, "Failed to create Discord Game SDK instance: %d\n", res);
		result = 1;
	}

	return result;
}

uint32 discord_update(s_discord_data* discord)
{
	discord->core->run_callbacks(discord->core);

	if (g_discord_globals.update_rich_presence)
	{
		discord_rich_presence_update(discord);
		g_discord_globals.update_rich_presence = false;
	}

	return 50;
}

void discord_rich_presence_update(s_discord_data* discord)
{
	g_discord_globals.activity.application_id = k_discord_client_id;
	g_discord_globals.activity.supported_platforms = DiscordActivitySupportedPlatformFlags_Desktop;

	c_network_session* network_session = NULL;
	if (network_life_cycle_in_squad_session(&network_session) 
		&& (network_session->m_session_host_peer_index != NONE 
		&& network_session->m_local_state != _network_session_state_peer_leaving))
	{
		bool session_host = network_session->is_host();

		XSESSION_INFO session;
		const int32 observer_index = network_session->get_session_peer(network_session->m_session_host_peer_index)->observer_channel_index;

		session.sessionID = network_session->session_id;
		session.keyExchangeKey = network_session->xnkey;
		session.hostAddress = 
			(session_host ? network_session->m_session_virtual_couch.xsession_info.hostAddress : network_session->m_network_observer->m_observer_channels[observer_index].xnaddr);

		XUID host;
		XUserGetXUID(0, &host);

		snprintf(g_discord_globals.activity.party.id, sizeof(g_discord_globals.activity.party.id), "%016llx", host);
		g_discord_globals.activity.party.privacy = DiscordActivityPartyPrivacy_Public;
		discord_interface_encode_xsession_info(&session);
	}
	else
	{
		csmemset(g_discord_globals.activity.secrets.join, 0, sizeof(g_discord_globals.activity.secrets.join));
	}


	discord->activities->update_activity(discord->activities, &g_discord_globals.activity, discord, on_rich_presence_updated);
	return;
}

bool discord_interface_scenario_name_vaild(const utf8* scnr_name)
{
	bool result = false;
	for (size_t i = 0; i < k_valid_scenario_name_count; i++)
	{
		if (!csstricmp(scnr_name, k_valid_scenario_names[i]))
		{
			result = true;
			break;
		}
	}

	return result;
}

void discord_interface_encode_xsession_info(XSESSION_INFO* session_info)
{
	uint8* session_bytes = (uint8*)session_info;

	// Encode the data into hex string
	for (uint32 i = 0; i < sizeof(XSESSION_INFO); i++)
	{
		sprintf(&g_discord_globals.activity.secrets.join[2 * i], "%02hhX", session_bytes[i]);
	}

	error(0, "Encoded join secret: %s", g_discord_globals.activity.secrets.join);
	return;
}

void discord_interface_update_details(void)
{
	c_static_string<128> details;
	details.set(g_discord_globals.activity.assets.small_text);
	details.append(" on ");
	details.append(g_discord_globals.activity.assets.large_text);

	discord_interface_set_details(details.get_string());
	g_discord_globals.update_rich_presence = true;
	return;
}

void DISCORD_CALLBACK on_user_updated(void* data)
{
	s_discord_data* discord = (s_discord_data*)data;
	DiscordUser user;
	discord->users->get_current_user(discord->users, &user);
	discord->user_id = user.id;
	return;
}

void DISCORD_CALLBACK on_activity_join(void* event_data, const char* secret)
{
	XSESSION_INFO session;
	uint8* session_bytes = (uint8*)&session;
	error(0, "Join secret: %s", secret);

	// Decode the data from hex string
	for (uint32 i = 0; i < sizeof(XSESSION_INFO); i++)
	{
		(void)sscanf(&secret[2 * i], "%02hhX", &session_bytes[i]);
	}

	game_direct_connect_to_session(session.sessionID, session.keyExchangeKey, &session.hostAddress, EXECUTABLE_TYPE, EXECUTABLE_VERSION, COMPATIBLE_VERSION);
	return;
}

void DISCORD_CALLBACK on_discord_log_print(void* hook_data, enum EDiscordLogLevel level, const char* message)
{
	error(0, message);
	return;
}

void DISCORD_CALLBACK on_rich_presence_updated(void* data, EDiscordResult res)
{
	ASSERT(res == DiscordResult_Ok);
	return;
}