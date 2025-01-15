#pragma once
#include "game/aim_assist.h"
#include "game/game_allegiance.h"
#include "objects/objects.h"

#define MAXIMUM_NUMBER_OF_UNIT_CAMERA_TRACKS 2
#define NUMBER_OF_UNIT_HUD_TYPES 2
#define k_powered_seats_count 2
#define MAXIMUM_INITIAL_WEAPONS_PER_UNIT 4

enum e_weapon_addition_method : int16
{
	_weapon_addition_method_zero = 0,
	_weapon_addition_method_one = 1,
	_weapon_addition_method_two = 2,
	_weapon_addition_method_three = 3,
	_weapon_addition_method_four = 4,
	_weapon_addition_method_five = 5,
	_weapon_addition_method_six = 6,
	_weapon_addition_method_seven = 7,
};

enum e_unit_grenade_type : int16
{
	_unit_grenade_human_fragmentation = 0,
	_unit_grenade_covenant_plasma = 1,
	k_unit_grenade_types_count
};

enum e_unit_weapons
{
	unit_weapons_primary_weapon,
	unit_weapons_secondary_weapon,
	unit_weapons_dual_weild_weapon
};

enum e_unit_data_flags : uint32
{
	_unit_is_actively_controlled = FLAG(1),
	_unit_is_alive = FLAG(2),
	_unit_active_camo_active = FLAG(3),
	_unit_is_enterable_by_player = FLAG(12),
	_unit_desires_tight_camera_track = FLAG(17),
	_unit_is_impervious = FLAG(19),
	_unit_suspended = FLAG(20),
	_unit_enable_eye_tracking = FLAG(24),
	_unit_only_takes_damage_from_players_team = FLAG(31)
};

enum e_unit_estimate_mode : int16
{
	_unit_estimate_none = 0,
	_unit_estimate_1 = 1,
	_unit_estimate_2 = 2,
	_unit_estimate_gun_position = 3,
	k_unit_estimate_position_modes_count
};

enum e_unit_flags : uint32
{
	_unit_is_crouching_bit = 23
};

enum e_unit_control_flags : uint32
{
	_unit_control_bit_1 = 1,
	_unit_control_bit_2 = 2,

	k_unit_control_flags_count
};

struct s_unit_304
{
	int8 gap_0[16];
};

struct unit_control_data
{
	string_id animation_state;
	uint16 aiming_speed;
	uint16 weapon_set_identifier;
	uint8 field_8;
	uint8 field_9;
	uint16 grenade_index;
	uint16 zoom_level;
	char gap_E[2];
	int64 control_flags;
	real_vector3d throttle;
	float trigger;
	float secondary_trigger;
	real_vector3d desired_facing;
	real_vector3d desired_aiming;
	real_vector3d desired_looking;
	int32 field_50;
	int32 field_54;
	int32 field_58;
	s_aim_assist_targeting_result target_info;

};
ASSERT_STRUCT_SIZE(unit_control_data, 0x80);

struct unit_datum
{
	object_datum object;
	int8 gap_12C[4];
	datum actor_datum;
	datum simulation_actor_index;
	uint32 unit_flags;
	e_game_team unit_team;
	int8 pad[2];
	datum controlling_player_index;
	datum player_datum;
	int8 gap_148[8];
	c_flags_no_init<e_unit_control_flags, uint32, k_unit_control_flags_count> control_flags;
	uint32 control_flags_2;
	string_id animation_state;
	real_vector3d desired_facing;
	real_vector3d desired_aiming;
	real_vector3d aiming_vector;
	real_vector3d aiming_vector_velocity;
	real_vector3d desired_looking;
	real_vector3d looking_vector;
	real_vector3d looking_vector_velocity;
	int32 field_1B0;
	int32 field_1B4;
	int32 field_1B8;
	real_vector3d vector_1BC;
	int8 aiming_speed;
	int8 gap_1C9[3];
	real32 trigger;
	real32 secondary_trigger;
	s_aim_assist_targeting_result target_info;
	int32 field_1F8;
	int32 field_1FC;
	int32 flags_200;
	int32 flags_204;
	int8 gap_1F8[2];
	uint8 left_eye_node_index;
	uint8 right_eye_node_index;
	uint8 horizontal_aiming_change;
	int8 gap_20D;
	int8 tick_count_20E;
	bool unk_bool_20F;
	int16 parent_seat_index;
	int8 gap_212[10];
	real32 mouth_aperture;
	int8 gap_220[4];
	uint16 field_224;
	int8 weapon_indices[2];
	uint16 weapon_set_identifier;
	int8 weapon_slots[2];
	datum inventory[4];
	int8 gap_23C[16];
	datum item_index;
	int8 grenade_type;
	int8 current_grenade_index;
	int8 grenade_counts[k_unit_grenade_types_count];
	int8 zoom_level;
	int8 gap_255;
	int8 field_256;
	uint8 aiming_change;
	int8 gap_256[4];
	datum driver_index;
	datum gunner_index;
	datum field_264;
	datum field_268;
	bool unk_bool_26C;
	int8 gap_260;
	uint16 squad_index;
	real32 driver_seat_power;
	real32 gunner_seat_power;
	real32 integrated_light_power;
	int8 gap_27C[4];
	uint32 field_280;
	real_point3d position_284;
	real_point3d position_290;
	int8 gap_290[24];
	datum simulation_field_2B4;
	uint16 simulation_field_2B8;
	datum simulation_player_index;
	datum simulation_field_2C0;
	real32 active_camo_power;
	real32 field_2C8;
	real32 active_camo_regrowth;
	int8 active_camo_mode;
	int8 field_2D1;
	uint16 active_camo_time_ticks;
	uint8 gap_2D2[4];
	real32 crouching;
	int16 field_2DC;
	int16 field_2DE;
	int32 field_2E0;
	datum field_2E4;
	datum field_2E8;
	uint8 gap_2E4[24];
	s_unit_304 field_304[4];
	int32 field_344;
	real32 boost_2D4;
	real32 boost_dead_time;
	object_header_block_reference weapon_raised_block;
	object_header_block_reference udlg_block;
	object_header_block_reference object_header_358;
	int32 field_35C;
};
ASSERT_STRUCT_SIZE(unit_datum, 864);

void __cdecl unit_delete_all_weapons(datum unit_datum_index);
datum __cdecl unit_inventory_next_weapon(datum unit_datum_index);
bool __cdecl unit_add_weapon_to_inventory(datum unit_datum_index, datum weapon_datum_index, e_weapon_addition_method weapon_addition_method);
float __cdecl unit_get_field_of_view(datum unit_datum_index, real32 unit_camera_field_of_view, int16 zoom_level);
bool unit_is_dual_wielding(datum unit_index);
datum __cdecl unit_inventory_get_weapon(datum unit_index, int16 weapon_slot);

datum player_index_from_unit_index(datum unit_index);

void __cdecl unit_get_head_position_interpolated(datum unit_index, real_point3d* position);

void __cdecl unit_get_camera_position(datum unit_index, real_point3d* out_point);

void __cdecl unit_control(datum unit_index, unit_control_data* control_data);

e_game_team unit_get_team_index(datum unit_index);

bool __cdecl unit_desires_tight_camera_track(datum unit_index);

void unit_apply_patches(void);
