#include "stdafx.h"
#include "game_engine.h"

#include "game.h"
#include "game_engine_util.h"

/* public code */

c_game_engine* current_game_engine()
{
	return get_game_mode_engines()[game_engine_globals_get()->game_engine_index];
}

s_game_engine_globals* game_engine_globals_get(void)
{
	return *Memory::GetAddress<s_game_engine_globals**>(0x4BF8F8, 0x4EA028);
}

s_simulation_player_netdebug_data* game_engine_get_netdebug_data(datum player_index)
{
	return &game_engine_globals_get()->netdebug_data[DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index)];
}

c_game_engine** get_game_mode_engines()
{
	return Memory::GetAddress<c_game_engine**>(0x4D8548, 0x4F3CE4);
}

c_game_engine* get_slayer_engine()
{
	return get_game_mode_engines()[_game_engine_type_slayer];
}

bool __cdecl game_engine_get_change_colors(s_player_profile* player_profile, e_game_team team_index, real_rgb_color* change_colors)
{
	return INVOKE(0x6E5C3, 0x6D1BF, game_engine_get_change_colors, player_profile, team_index, change_colors);
}

bool __cdecl game_engine_variant_cleanup(uint16* flags)
{
	return INVOKE(0x5B720, 0x3D380, game_engine_variant_cleanup, flags);
}

void __cdecl game_engine_player_activated(datum player_index)
{
	INVOKE(0x6A29E, 0x69CB6, game_engine_player_activated, player_index);
	return;
}

bool __cdecl game_engine_team_is_enemy(e_game_team a, e_game_team b)
{
	return INVOKE(0x6ADA3, 0x6A5DE, game_engine_team_is_enemy, a, b);
}

void __cdecl game_engine_render(void)
{
	INVOKE(0x6A60F, 0x0, game_engine_render);
	return;
}
