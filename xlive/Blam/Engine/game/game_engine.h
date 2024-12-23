#pragma once

#include "game/game_allegiance.h"
#include "game_statborg.h"
#include "math/color_math.h"
#include "saved_games/game_variant.h"
#include "tag_files/string_id.h"
#include "tag_files/tag_block.h"
#include "tag_files/tag_reference.h"

/* constants */

#define k_maximum_game_engine_event_responses_per_type 128

/* enums */

enum e_valid_multiplayer_games : short
{
	valid_multiplayer_game_capture_the_flag = FLAG(0),
	valid_multiplayer_game_slayer = FLAG(1),
	valid_multiplayer_game_oddball = FLAG(2),
	valid_multiplayer_game_king_of_the_hill = FLAG(3),
	valid_multiplayer_game_juggernaut = FLAG(4),
	valid_multiplayer_game_territories = FLAG(5),
	valid_multiplayer_game_assault = FLAG(6),
};

enum e_relevant_multiplayer_games : int
{
	relevant_multiplayer_game_capture_the_flag = FLAG(0),
	relevant_multiplayer_game_slayer = FLAG(1),
	relevant_multiplayer_game_oddball = FLAG(2),
	relevant_multiplayer_game_king_of_the_hill = FLAG(3),
	relevant_multiplayer_game_juggernaut = FLAG(4),
	relevant_multiplayer_game_territories = FLAG(5),
	relevant_multiplayer_game_assault = FLAG(6)
};

enum e_multiplayer_event_response_definition_flags : int16
{
	_multiplayer_event_response_definition_flag_quantity_message = FLAG(0)
};

enum e_multiplayer_event : int16
{
	_multiplayer_event_game_start = 0,
	_multiplayer_event_hill_controlled = 1,
	_multiplayer_event_hill_contested = 2,
	_multiplayer_event_hill_tick = 3,
	_multiplayer_event_hill_move = 4,
	_multiplayer_event_hill_controlled_team = 5,
	_multiplayer_event_hill_contested_team = 6
};

enum e_multiplayer_event_audience : int16
{
	_multiplayer_event_audience_cause_player = 0,
	_multiplayer_event_audience_cause_team = 1,
	_multiplayer_event_audience_effect_player = 2,
	_multiplayer_event_audience_effect_team = 3,
	_multiplayer_event_audience_all = 4
};

enum e_multiplayer_event_audience_type : int16
{
	_multiplayer_event_audience_type_none = 0,
	_multiplayer_event_audience_type_cause_player = 1,
	_multiplayer_event_audience_type_cause_team = 2,
	_multiplayer_event_audience_type_effect_player = 3,
	_multiplayer_event_audience_type_effect_team = 4
};

enum e_multiplayer_event_sound_flags : int16
{
	_multiplayer_event_sound_flag_announcer_sound = FLAG(0),
};

/* structures */

class c_game_engine
{
public:
	virtual e_game_engine_type get_type() = 0;

	/*
		Called on scenario load, returns success
		game engine will be disabled on failure
	*/
	virtual bool setup() = 0;

	/* Called on scenario cleanup/exit */
	virtual void cleanup() = 0;
	virtual bool unk_function_4() = 0;
	virtual bool unk_function_5(int arg1) = 0;
	virtual void unk_function_players_1(datum player_index) = 0;
	virtual void update_player__maybe__maybe_spawn(int event_type) = 0;
	virtual void unk_function_8(int arg1) = 0;
	virtual void unk_function_9(int arg1) = 0;
	virtual void unk_function_players_2(int arg1, int arg2) = 0;
	virtual void unk_function_11(int arg1) = 0;
	virtual void unk_function_12() = 0;
	virtual void unk_function_13() = 0;
	virtual void unk_function_14(int arg1) = 0;

	/* Render 3d objects (e.g hologram around hill in KotH) */
	virtual void render_in_world(int32 user_index) = 0;
	virtual void unk_function_16(int arg1) = 0;
	virtual int unk_function_17(int arg1, int arg2) = 0;
	virtual int unk_function_18(int arg1, int arg2) = 0;
	virtual void unk_function_19() = 0;
	virtual float player_speed_multiplier(datum player_index) = 0;
	virtual int unk_function_21(int arg1) = 0;
	virtual void unk_function_22(int arg1) = 0;
	virtual void unk_function_23(int arg1) = 0;
	virtual void unk_function_24(int arg1, int arg2) = 0;
	virtual int unk_function_25(int arg1, int arg2) = 0;
	virtual int unk_function_26(int arg1, int arg2, int arg3) = 0;
	virtual int unk_function_27() = 0;
	virtual bool is_team_enemy(e_game_team team_a, e_game_team team_b) = 0;
	virtual void unk_function_29(int arg1) = 0;
	virtual void unk_function_30(int arg1, int arg2, int arg3) = 0;
	virtual void unk_function_31(int arg1, int arg2, char arg3, int arg4) = 0;
	virtual int unk_function_32(int arg1, int arg2, int arg3) = 0;
	virtual void unk_function_33(int arg1, int arg2) = 0;
	virtual int unk_function_34(int arg1, int arg2) = 0;
	virtual bool unk_function_35(int arg1) = 0;
	virtual int unk_function_36(int arg1, int arg2) = 0;
	virtual void unk_function_37(int arg1) = 0;
	virtual int unk_function_38(int arg1, int arg2) = 0;
	virtual bool should_garbage_collect(int arg1) = 0;
	virtual void unk_function_40() = 0;
	virtual void unk_function_41() = 0;
	virtual void* unk_function_42(void* arg1) = 0;
	virtual void* unk_function_43(char a1, size_t* a2, void* a3) = 0;
	virtual bool unk_function_44(char a1, void* a2) = 0;
	virtual int unk_function_45(char a1, int a2) = 0;
	virtual int unk_function_46(char a1, int a2, int a3) = 0;
	virtual bool unk_function_47(char a1, int a2, int a3) = 0;
	virtual void* unk_function_48(int a1, int a2, void* a3) = 0;
	virtual void unk_function_49(int arg1, int arg2, int arg3, int arg4) = 0;
	virtual bool unk_function_50(__int16 a1, __int16 a2, int a3, int a4) = 0;
	virtual int unk_function_51(int arg1, int arg2, int arg3, int arg4, int arg5) = 0;
private:
};

struct s_game_engine_global_player_info
{
	bool valid;
	uint8 pad_1[3];
	real_point3d point;
	short field_10;
	short field_12;
	uint8 field_14[4];
};
ASSERT_STRUCT_SIZE(s_game_engine_global_player_info, 24);

struct s_simulation_player_netdebug_data
{
	int32 field_0;
	int32 field_4;
	int16 client_rtt_msec;
	int16 client_packet_rate;
	int16 client_throughput;
	int16 client_packet_loss_percentage;
};
ASSERT_STRUCT_SIZE(s_simulation_player_netdebug_data, 16);

struct s_game_engine_globals
{
	uint32 flags;
	int16 team_flags;
	uint16 field_6;
	uint16 field_8;
	uint16 team_bitmask;
	uint16 field_C;
	int16 field_E;
	uint16 field_10;
	uint16 field_12[8];
	uint32 field_24;
	uint32 field_28;
	int32 player_entity_index[k_maximum_players];
	int16 field_6C;
	uint32 field_70;
	uint32 gap_74[28];
	real32 unk_local_player_hud_field[k_number_of_users];
	uint8 field_F4;
	uint8 pad_F5[4];
	uint8 gapF9[523];
	c_game_statborg game_statborg;
	s_game_engine_global_player_info player_info[k_maximum_players];
	uint32 ticks;
	s_simulation_player_netdebug_data netdebug_data[k_maximum_players];
	uint8 gap81C[1064];
	int32 field_C44;
	uint8 gap_C48[12];
	int32 game_engine_index;
	uint8 gapC58[132];
};
ASSERT_STRUCT_SIZE(s_game_engine_globals, 0xCDC);

// max count: 1
struct s_sound_response_extra_sounds
{
	tag_reference japanese_sound;	// snd!
	tag_reference german_sound;		// snd!
	tag_reference french_sound;		// snd!
	tag_reference spanish_sound;	// snd!
	tag_reference italian_sound;	// snd!
	tag_reference korean_sound;		// snd!
	tag_reference chinese_sound;	// snd!
	tag_reference portuguese_sound;	// snd!
};
ASSERT_STRUCT_SIZE(s_sound_response_extra_sounds, 64);


// max count: 10
struct s_multiplayer_event_sound_response_definition
{
	e_multiplayer_event_sound_flags sound_flags;
	int16 pad;
	tag_reference english_sound;
	s_sound_response_extra_sounds extra_sounds;
	real32 probability;
};
ASSERT_STRUCT_SIZE(s_multiplayer_event_sound_response_definition, 80);

// max count: k_maximum_game_engine_event_responses_per_type
struct s_multiplayer_event_response_definition
{
    e_multiplayer_event_response_definition_flags flags;
    int16 pad;
    e_multiplayer_event event;
    e_multiplayer_event_audience audience;

    int16 pad1;
    int16 pad2;
    string_id display_string;
	e_multiplayer_event_audience_type required_field;
	e_multiplayer_event_audience_type excluded_audience;
    string_id primary_string;
    int32 primary_string_duration_seconds;

	string_id display_string_plural;
	int32 pad3[7];
    real32 sound_delay_announcer_only;

	e_multiplayer_event_sound_flags sound_flags;
	int16 pad4;
	tag_reference sound;    // snd!
	s_sound_response_extra_sounds extra_sounds;

	int32 pad5;
	int32 pad6[4];
    tag_block<s_multiplayer_event_sound_response_definition> sound_permutations;
};
ASSERT_STRUCT_SIZE(s_multiplayer_event_response_definition, 0xA8);

/* prototypes */

c_game_engine* current_game_engine();

s_game_engine_globals* game_engine_globals_get(void);

s_simulation_player_netdebug_data* game_engine_get_netdebug_data(datum player_index);

bool __cdecl game_engine_get_change_colors(s_player_profile* player_profile, e_game_team team_index, real_rgb_color* change_colors);

bool __cdecl game_engine_variant_cleanup(uint16* flags);

void __cdecl game_engine_player_activated(datum player_index);

bool __cdecl game_engine_team_is_enemy(e_game_team a, e_game_team b);

void __cdecl game_engine_render(void);

bool game_engine_in_round();

c_game_engine** get_game_mode_engines();
