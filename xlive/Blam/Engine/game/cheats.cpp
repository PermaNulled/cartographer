#include "stdafx.h"
#include "cheats.h"

#include "game/game.h"
#include "game/game_globals.h"
#include "tag_files/global_string_ids.h"


typedef bool(__cdecl* ice_cream_flavor_available_t)(unsigned int skull);
ice_cream_flavor_available_t p_ice_cream_flavor_available;

typedef void(__cdecl* ice_cream_activation_t)(unsigned int skull);
ice_cream_activation_t p_ice_cream_activation;

bool skull_enabled[k_skull_count] = { };
const string_id skull_string_ids[k_skull_count] =
{
	_string_id_unlocked_extra_1,	// Envy
	_string_id_unlocked_extra_2,	// Grunt Birthday Party
	_string_id_unlocked_extra_3,	// Assassins
	_string_id_unlocked_extra_4,	// Thunderstorm
	_string_id_unlocked_extra_5,	// Famine
	_string_id_unlocked_extra_6,	// I Would Have Been Your Daddy
	_string_id_unlocked_extra_7,	// Blind
	_string_id_unlocked_extra_8,	// Ghost
	_string_id_unlocked_extra_9,	// Black Eye
	_string_id_unlocked_extra_10,	// Catch
	_string_id_unlocked_extra_11,	// Sputnik
	_string_id_unlocked_extra_12,	// Iron
	_string_id_unlocked_extra_13,	// Mythic
	_string_id_unlocked_extra_14,	// Angry
	_string_id_unlocked_extra_16,	// That's Just... Wrong
	_string_id_unlocked_extra_15,	// Cow Bell
};



bool __cdecl ice_cream_flavor_available(const e_skull_type skull)
{
	return skull < k_skull_count && game_is_campaign() && skull_enabled[skull];
}

void __cdecl ice_cream_flavor_stock(const e_skull_type skull)
{
	typedef void(__cdecl* hud_clear_messages_t)();
	auto p_hud_clear_messages = Memory::GetAddress<hud_clear_messages_t>(0x22CE83, 0x206863);
	typedef int(__cdecl* players_first_active_user_t)();
	auto p_players_first_active_user = Memory::GetAddress<players_first_active_user_t>(0x5343F, 0x5B973);
	typedef int(__cdecl* display_generic_hud_string_t)(int controller_index, string_id string_id);
	auto p_display_generic_hud_string = Memory::GetAddress<display_generic_hud_string_t>(0x22DEA4, 0x206BB7);
	typedef int(__cdecl* scripted_player_effect_screen_fade_in_t)(float r, float g, float b, __int16 ticks);
	auto p_scripted_player_effect_screen_fade_in = Memory::GetAddress<scripted_player_effect_screen_fade_in_t>(0xA402C, 0x9628C);
	typedef int(__cdecl* unspatialized_impulse_sound_new_t)(datum sound_datum, float scale);
	auto p_unspatialized_impulse_sound_new = Memory::GetAddress<unspatialized_impulse_sound_new_t>(0x8836C, 0x7F173);

	s_game_globals* g_globals_tag = scenario_get_game_globals();
	const s_game_globals_player_information* player_information = g_globals_tag->player_information[0];

	if (skull < k_skull_count && !skull_enabled[skull])
	{
		skull_enabled[skull] = true;
		p_hud_clear_messages();
		
		string_id skull_string_id = skull_string_ids[skull];
		int user_index = p_players_first_active_user();

		p_display_generic_hud_string(user_index, skull_string_id);
		p_scripted_player_effect_screen_fade_in(1.0, 1.0, 1.0, 20);

		// The below should never return true, if that's the case we've got bigger issues
		if (g_globals_tag->player_information.count == 0)
		{
			LOG_CRITICAL_GAME("g_globals_tag->player_information.size == 0, something is wrong with the globals tag on this map");
			return; 
		}

		datum sound_datum = player_information->ice_cream.index;
		if (sound_datum != NONE)
		{
			p_unspatialized_impulse_sound_new(sound_datum, 1.0);
		}
	}
}

bool* get_ice_cream_activation()
{
	return skull_enabled;
}

void cheats_apply_patches()
{
	DETOUR_ATTACH(p_ice_cream_flavor_available, Memory::GetAddress<ice_cream_flavor_available_t>(0xBD114, 0xAC2A2), ice_cream_flavor_available);
	DETOUR_ATTACH(p_ice_cream_activation, Memory::GetAddress<ice_cream_activation_t>(0xBD137, 0xAC2C5), ice_cream_flavor_stock);
}
