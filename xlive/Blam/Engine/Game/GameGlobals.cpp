#include "stdafx.h"

#include "GameGlobals.h"

s_game_globals* s_game_globals::get()
{
	return *Memory::GetAddress<s_game_globals**>(0x482D3C, 0x4CB520);
}

bool s_game_globals::map_initialized()
{
	return get() && get()->map_active && get()->sbsp_index != 0xFFFF;
}

s_game_options* s_game_globals::get_game_options()
{
	return &get()->options;
}

s_game_variant* s_game_globals::get_game_variant()
{
	return &get_game_options()->game_variant;
}

bool s_game_globals::game_is_campaign()
{
	return get_game_options()->engine_type == e_engine_type::_single_player;
}

bool s_game_globals::game_is_multiplayer()
{
	return get_game_options()->engine_type == e_engine_type::_multiplayer;
}

bool s_game_globals::game_is_mainmenu()
{
	return get_game_options()->engine_type == e_engine_type::_main_menu;
}

bool s_game_globals::game_is_predicted()
{
	return get_game_options()->simulation_type == _game_simulation_distributed_client;
}

bool s_game_globals::game_is_in_progress()
{
	return get() && get()->game_in_progress;
}

s_game_cluster_bit_vectors* s_game_globals::game_get_cluster_activation()
{
	return get()->cluster_activation;
}
