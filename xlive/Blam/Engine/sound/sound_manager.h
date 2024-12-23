#pragma once
#include "networking/logic/life_cycle_manager.h"

#define MAXIMUM_SOUND_INSTANCES_PER_DEFINITION 16
#define MAXIMUM_SOUND_INSTANCES_PER_OBJECT_PER_DEFINITION MAXIMUM_SOUND_INSTANCES_PER_DEFINITION

struct s_sound_global_mix_configuration
{
	real32 mono_upsatialized_gain_db;
	real32 stereo_to_3d_gain_db;
	real32 rear_surround_to_front_stereo_gain_db;

	// Explaination("surround center mix", "for sounds that use the center speaker")

	real32 surround_center_front_speaker_gain_db;
	real32 surround_center_rear_speaker_gain_db;

	// Explaination("stereo center mix", "for sounds that use the center speaker")

	real32 stereo_center_front_speaker_gain_db;
	real32 stereo_center_rear_speaker_gain_db;

	// Explaination("more sound lovin'", "for sounds that use the center speaker")

	real32 stereo_unspatialized_gain_db;

	// Explaination("last minute values", "")

	real32 solo_player_fade_out_delay_seconds;
	real32 solo_player_fade_out_time_seconds;
	real32 solo_player_fade_in_time_seconds;
	real32 game_music_fade_out_time_seconds;
};
ASSERT_STRUCT_SIZE(s_sound_global_mix_configuration, 48);

struct s_sound_mix_configuration_definition
{
	// Explaination("first person left side mix", "for first person sounds to the left of you")

	real32 first_person_left_side_left_stereo_gain_db;
	real32 first_person_left_side_right_stereo_gain_db;

	// Explaination("first person middle mix", "for first person sounds between your ears")

	real32 first_person_middle_left_stereo_gain_db;
	real32 first_person_middle_right_stereo_gain_db;

	// Explaination("first person right side mix", "for first person sounds to the right of you")

	real32 first_person_right_side_left_stereo_gain_db;
	real32 first_person_right_side_right_stereo_gain_db;

	// Explaination("first person stereo mix", "")

	real32 first_person_front_speaker_gain_db;
	real32 first_person_rear_speaker_gain_db;

	// Explaination("ambient stereo mix", "")

	real32 ambient_front_speaker_gain_db;
	real32 ambient_rear_speaker_gain_db;

	// Explaination("global mix", "")

	s_sound_global_mix_configuration global_mix;
};
ASSERT_STRUCT_SIZE(s_sound_mix_configuration_definition, 88);

struct s_sound_manager_local_listener
{
	int8 data[72];
};
ASSERT_STRUCT_SIZE(s_sound_manager_local_listener, 72);

struct s_sound_manager_globals
{
	real32 unk_float;
	int8 gap_1[90];
	bool unk_bool;
	int8 unk_flags;
	int32 system_time;
	int8 gap_2[3];
	s_sound_manager_local_listener local_listeners[k_number_of_users];
	int8 gap_3[69];

	// from snmx global mix configuration
	real32 solo_player_fade_out_delay_seconds;
	real32 solo_player_fade_out_time_seconds;
	real32 solo_player_fade_in_time_seconds;

	int32 unk_index;
	real32 unk_float_2;
	int32 unk_index_2;
	real32 unk_float_3;
	int16 channel_count;
	int8 gap_4[2];
};
ASSERT_STRUCT_SIZE(s_sound_manager_globals, 492);

void __cdecl sound_initialize(void);

void __cdecl sound_idle();

void __cdecl sound_pause(bool a1);
