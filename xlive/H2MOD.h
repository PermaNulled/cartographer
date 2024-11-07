#pragma once

enum e_sound_type
{
	_sound_type_team_change,
	_sound_type_gained_the_lead,
	_sound_type_lost_the_lead,
	_sound_type_tied_leader,
	_sound_type_slayer,
};
#define ALL_SOUNDS_NO_SLAYER (FLAG(_sound_type_team_change) | FLAG(_sound_type_gained_the_lead) | FLAG(_sound_type_lost_the_lead) | FLAG(_sound_type_tied_leader))


bool xbox_tickrate_is_enabled();
void call_give_player_weapon(int playerIndex, datum weaponId, bool resetLoadout);

class H2MOD
{
public:
	static void Initialize();
	static void Deinitialize();
	static void ApplyHooks();
	static void RegisterEvents();
	static void RefreshTogglexDelay();

	static void player_position_increase_client_position_margin_of_error(bool enable);
	int get_player_index_from_unit_datum_index(datum unit_datum_index);
	void set_unit_speed_patch(bool hackit);
	void disable_score_announcer_sounds(int sound);
	void custom_sound_play(const wchar_t* soundName, int delay);

	void toggle_ai_multiplayer(bool toggle);
	bool drawTeamIndicators = true;
};

extern std::unique_ptr<H2MOD> h2mod;