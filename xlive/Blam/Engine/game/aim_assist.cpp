#include "stdafx.h"
#include "aim_assist.h"

#include "H2MOD.h"
#include "Blam/Engine/game/game_globals.h"

// sword-flying target clear patch
void __cdecl aim_assist_targeting_clear_hook(s_aim_assist_targetting_data* target_data)
{
	if (!s_game_globals::game_is_campaign() && !get_xbox_tickrate_bool())
	{
		target_data->primary_auto_aim_level = 0.0;
		target_data->secondary_auto_aim_level = 0.0;
		target_data->target_object = -1;
		target_data->model_target = -1;
		target_data->target_player = -1;
		target_data->auto_aim_flags = 0;
	}
}