#pragma once
#include "tag_files/tag_block.h"

/* enums */

enum e_sound_class_type : int32
{
	_sound_class_projectile_impact = 0,
	_sound_class_projectile_detonation = 1,
	_sound_class_projectile_flyby = 2,
	_sound_class_projectile_unused1 = 3,
	
	_sound_class_weapon_fire = 4,
	_sound_class_weapon_ready = 5,
	_sound_class_weapon_reload = 6,
	_sound_class_weapon_empty = 7,
	_sound_class_weapon_charge = 8,
	_sound_class_weapon_overheat = 9,
	_sound_class_weapon_idle = 10,
	_sound_class_weapon_melee = 11,
	_sound_class_weapon_animation = 12,

	_sound_class_object_impacts = 13,
	_sound_class_particle_impacts = 14,
	_sound_class_slow_impacts = 15,
	_sound_class_effect_unused2 = 16,
	_sound_class_effect_unused3 = 17,
	
	_sound_class_footstep = 18,
	
	_sound_class_unit_dialog = 19,
	_sound_class_unit_animation = 20,
	_sound_class_unit_unused1 = 21,
	
	_sound_class_vehicle_impact = 22,
	_sound_class_vehicle_engine = 23,
	_sound_class_vehicle_animation = 24,
	_sound_class_vehicle_unused1 = 25,
	
	_sound_class_device_door = 26,
	_sound_class_device_force_field = 27,
	_sound_class_device_machinery = 28,
	_sound_class_device_stationary = 29,
	_sound_class_device_computers = 30,
	_sound_class_device_unused1 = 31,
	
	_sound_class_music = 32,

	_sound_class_ambient_nature = 33,
	_sound_class_ambient_machinery = 34,
	_sound_class_ambient_computers = 35,
	
	_sound_class_marty_huge_ass = 36,
	_sound_class_marty_object_looping = 37,
	_sound_class_marty_cinematic_music = 38,
	
	_sound_class_player_hurt = 39,
	_sound_class_player_unused0 = 40,
	_sound_class_player_unused1 = 41,
	_sound_class_player_unused2 = 42,
	_sound_class_player_unused3 = 43,

	_sound_class_scripted_dialog_to_player = 44,
	_sound_class_scripted_cortana_mission = 45,
	_sound_class_scripted_cortana_cinematic = 46,
	_sound_class_scripted_mission_dialog = 47,
	_sound_class_scripted_cinematic_dialog = 48,
	_sound_class_scripted_cinematic_foley = 49,
	
	_sound_class_game_event = 50,
	_sound_class_ui = 51,
	_sound_class_test = 52,
	_sound_class_multilingual_test = 53,
	NUMBER_OF_SOUND_CLASSES,
};


enum e_sound_class_internal_flags : uint16
{
	_sound_class_internal_valid_bit = 0,
	_sound_class_internal_is_speech_bit = 1,
	_sound_class_internal_scripted_bit = 2,
	_sound_class_internal_stops_with_object_bit = 3,
	_sound_class_internal_unused_bit = 4,
	_sound_class_internal_valid_doppler_factor_bit = 5,
	_sound_class_internal_valid_obstruction_factor_bit = 6,
	_sound_class_internal_multilingual_bit = 7,
	k_number_of_sound_class_internal_flags
};

enum e_sound_class_flags : uint16
{
	_sound_class_plays_during_pause_bit = 0,
	_sound_class_dry_stereo_mix_bit = 1,
	_sound_class_no_object_obstruction_bit = 2,
	_sound_class_use_center_speaker_unspatialized_bit = 3,
	_sound_class_send_mono_to_lfe_bit = 4,
	_sound_class_deterministic_bit = 5,
	_sound_class_use_huge_transmission_bit = 6,
	_sound_class_always_use_speakers_bit = 7,
	_sound_class_dont_strip_from_main_menu_bit = 8,
	_sound_class_ignore_stereo_headroom_bit = 9,
	_sound_class_stop_when_object_dies_bit = 10,
	_sound_class_allow_cache_file_editing_bit = 11,
	k_number_of_sound_class_flags
};

enum e_sound_class_cache_miss_mode : int16
{
	_sound_class_cache_miss_mode_discard = 0,
	_sound_class_cache_miss_mode_postpone = 1
};

enum e_sound_class_stereo_playback_type : int8
{
	_sound_class_stereo_playback_type_first_person = 0,
	_sound_class_stereo_playback_type_ambient = 1
};

/* structures */

// max count: NUMBER_OF_SOUND_CLASSES
struct sound_class_definition
{
	int16 maximum_number_per_definition;	// maximum number of sounds playing per individual sound tag
	int16 maximum_number_per_object;		// maximum number of sounds of this type playing on an object
	int32 preemption_time;					// replaces other instances after this many milliseconds
	c_flags_no_init<e_sound_class_internal_flags, uint16, k_number_of_sound_class_internal_flags> internal_flags;
	c_flags_no_init<e_sound_class_flags, uint16, k_number_of_sound_class_flags> flags;
	int16 priority;
	e_sound_class_cache_miss_mode cache_miss_mode;
	real32 reverb_gain;								// DB
	real32 override_speaker_gain;					// DB
	real_vector2d distance_bounds;
	
	/* edit */
	real_vector2d gain_bounds;						// DB
	real32 cutscene_ducking;						// DB
	real32 cutscene_ducking_fade_in_time;			// Seconds
	
	// how long this lasts after the cutscene ends
	real32 cutscene_ducking_sustain_time;			// Seconds
	real32 cutscene_ducking_fade_out_time;			// Seconds
	real32 scripted_dialog_ducking;					// DB
	real32 scripted_dialog_ducking_fade_in_time;	// Seconds
	
	// how long this lasts after the scripted dialog ends
	real32 scripted_dialog_ducking_sustain_time;	// Seconds
	real32 scripted_dialog_ducking_fade_out_time;	// Seconds
	real32 doppler_factor;

	e_sound_class_stereo_playback_type stereo_playback_type;
	int8 pad[3];

	real32 transmission_multiplier;
	real32 obstruction_max_bend;
	real32 occlusion_max_bend;
};
ASSERT_STRUCT_SIZE(sound_class_definition, 92);

struct s_sound_classes_definition
{
	tag_block<sound_class_definition> sound_classes;
};
ASSERT_STRUCT_SIZE(s_sound_classes_definition, 8);
