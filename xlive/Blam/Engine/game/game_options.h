#pragma once
#include "networking/logic/life_cycle_manager.h"
#include "saved_games/game_variant.h"
#include "simulation/machine_id.h"

enum e_game_simulation : int8
{
	_game_simulation_none = 0,
	_game_simulation_local = 1,
	_game_simulation_synchronous_client = 2,
	_game_simulation_synchronous_server = 3,
	_game_simulation_distributed_client = 4,
	_game_simulation_distributed_server = 5,
	k_game_simulation_count,
};

enum e_game_mode : int32
{
	_game_mode_campaign = 1,
	_game_mode_multiplayer = 2,
	_game_mode_ui_shell = 3,
	_game_mode_mutiplayer_shared = 4,
	_game_mode_single_player_shared = 5,
	k_game_mode_count
};

#pragma pack(push, 1)
struct game_player_options
{
	bool player_valid;
	bool player_left_game;
	int16 user_index;
	e_controller_index controller_index;
	s_machine_identifier machine_identifier;
	s_player_identifier player_identifier;
	int16 field_16;
	s_player_properties properties;
	s_persistent_campaign_player player_type[2]; // index 0 is masterchief, index 1 is dervish
};
ASSERT_STRUCT_SIZE(game_player_options, 212);
#pragma pack(pop)

struct s_game_options
{
	e_game_mode game_mode;
	e_game_simulation simulation_type;
	int8 network_protocol_related;
	bool session_host_is_dedicated;
	bool scenario_custom;
	int16 game_tick_rate;
	int16 pad_C[3];
	uint8 random_data[8];
	int32 random_seed;
	bool is_custom_map;
	uint8 field_1D;
	wchar_t custom_map_name[48];
	int16 pad_7E;
	int32 campaign_id;						// This should always be 1, since there can be multiple "camapaigns". However, this isn't taken advantage of in the retail game 
	int32 map_id;
	wchar_t scenario_path[MAX_PATH];
	int16 initial_bsp_index;
	int16 field_292;
	bool load_level_only;
	uint8 local_peer_index;
	bool dump_object_log;
	bool dump_random_seeds;
	bool playtest_mode;
	uint8 pad_299;
	int16 difficulty;
	bool coop;
	uint8 player_count;
	int16 pad_29E;
	s_game_variant game_variant;
	uint32 menu_context;
	uint32 valid_machine_mask;
	s_machine_identifier machines[k_network_maximum_machines_per_session];
	bool local_machine_exists;
	s_machine_identifier local_machine_identifier;
	int8 pad_444[3];
	game_player_options players[k_maximum_players]; 
};
ASSERT_STRUCT_SIZE(s_game_options, 4488);

// Initializes game options
void game_options_new(s_game_options* game_options);
