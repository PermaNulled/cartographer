#pragma once
#include "damage.h"
#include "object_placement.h"
#include "object_types.h"

#include "Blam/Math/BlamMath.h"
#include "Blam/Engine/DataArray/DataArray.h"
#include "Blam/Engine/memory/data.h"
#include "Blam/Engine/Players/PlayerActions.h"
#include <wtypes.h>

enum e_object_team : BYTE
{
	// MP
	_object_team_red = 0,
	_object_team_blue = 1,
	_object_team_yellow = 2,
	_object_team_green = 3,
	_object_team_purple = 4,
	_object_team_orange = 5,
	_object_team_brown = 6,
	_object_team_pink = 7,
	_object_team_end = 8,

	// SP
	Default = 0,
	player = 1,
	Human = 2,
	Covenant = 3,
	Flood = 4,
	Sentinel = 5,
	Heretic = 6,
	Prophet = 7,

	// unasigned team ids
	Unused8 = 8,
	Unused9 = 9,
	Unused10 = 10,
	Unused11 = 11,
	Unused12 = 12,
	Unused13 = 13,
	Unused14 = 14,
	Unused15 = 15,

	// Shared
	None = 255
};

enum e_unit_weapons
{
	PrimaryWeapon,
	SecondaryWeapon,
	DualWeildWeapon
};

enum e_grenades : BYTE
{
	Fragmentation,
	Plasma
};

enum e_weapon_index : WORD
{
	Primary = 0xFF00,
	Secondary = 0xFF01,
	DualWeild = 0x0201
};

enum e_biped_physics_mode : BYTE
{
	mode_ground = 1,
	mode_flying,
	mode_dead,
	mode_posture,
	mode_climbing,
	mode_melee
};

enum e_object_data_flags : int
{
	object_data_flag_0x1 = FLAG(0),
	_object_has_attached_lights_bit = FLAG(5),
	_object_has_attached_looping_sounds_bit = FLAG(5),
	_object_has_unattached_lights_bit = FLAG(6),
	_object_in_limbo_bit = FLAG(7),
	_object_connected_to_map_bit = FLAG(8),
	_object_has_collision_bit = FLAG(9),
	object_data_flag_0x400 = FLAG(10),
	object_data_flag_0x800 = FLAG(11),
	_object_garbage_bit = FLAG(14),
	_object_does_not_cast_shadow_bit = FLAG(16),
	object_data_flag_0x20000 = FLAG(17),
	object_data_flag_0x40000 = FLAG(18),
	object_data_flag_0x200000 = FLAG(21),
	_object_has_override_bit = FLAG(25),
	_object_is_child_object = FLAG(26),
	_object_has_prt_or_lighting_info = FLAG(31)
};
ENUM_OPERATORS(e_object_data_flags, DWORD)

enum e_object_physics_flags : WORD
{
	_object_allocated_havok_component_bit = FLAG(0),
	_object_physics_flag_0x2 = FLAG(1),
	_object_is_early_mover_bit = FLAG(3),
	_object_is_early_mover_child_bit = FLAG(4),
	_object_connected_to_physics_bit = FLAG(6),
	_object_physics_flag_0x100 = FLAG(8),
};
ENUM_OPERATORS(e_object_physics_flags, WORD)

enum e_object_damage_flags : WORD
{
	_object_is_dead_bit = FLAG(2),
};
ENUM_OPERATORS(e_object_damage_flags, WORD)


#pragma pack(push, 1)
struct s_object_data_definition
{
	datum tag_definition_index;
	e_object_data_flags object_flags;
	void* object_header_block;
	datum next_index;
	datum current_weapon_datum;		//maybe attachment or child
	datum parent_datum;
	WORD unit_in_vehicle_flag;
	INT16 placement_index;
	BYTE gap_1C[8];
	DWORD foreground_emblem;
	s_location location;
	real_point3d center;
	float radius;
	real_point3d object_origin_point;
	float shadow_sphere_radius;
	real_point3d dynamic_light_sphere_offset;
	float dynamic_light_sphere_radius;
	DWORD first_cluster_reference;
	real_point3d position;
	real_vector3d orientation;
	real_vector3d up;
	real_point3d translational_velocity;
	real_vector3d angular_velocity;
	float scale;
	datum unique_id;
	s_placement_info placement_info;
	__int16 name_list_index;
	byte structure_bsp_index;
	char netgame_equipment_index;
	byte placement_policy;
	byte field_b1;
	byte field_b2;
	byte field_b3;
	datum havok_datum;
	char gap_B8[8];
	e_object_physics_flags physics_flags;
	WORD damage_owner_unk3;
	DWORD damage_owner_unk1;
	DWORD damage_owner_unk2;
	unsigned int field_CC;
	__int16 field_D0;
	BYTE model_variant_id;//hlmt variant tag_block index
	char gap_D3;
	unsigned int simulation_entity_index;
	bool b_attached_to_simulation;
	char gap_D9[3];
	datum object_projectile_datum;
	WORD destroyed_constraints_flag;
	WORD loosened_constraints_flag;
	float body_max_vitality;
	float shield_max_vitality;
	float body_current_vitality;
	float shield_current_vitality;
	DWORD gap_F4[4];
	WORD shield_stun_ticks;
	WORD body_stun_ticks;
	byte byte_108;
	byte byte_109;
	e_object_damage_flags object_damage_flags;		//(object_damage_flags & 4) != 0 -- > object_is_dead
	__int16 original_orientation_size;
	__int16 original_orientation_offset;
	__int16 node_orientation_size;
	__int16 node_orientation_offset;
	__int16 node_buffer_size;
	__int16 nodes_offset;
	__int16 collision_regions_size;
	__int16 collision_regions_offset;
	__int16 object_attachments_size;
	__int16 object_attachments_offset;
	__int16 damage_sections_size;
	__int16 damage_sections_offset;
	__int16 change_color_size;
	__int16 change_color_offset;
	__int16 animation_manager_size;
	__int16 animation_manager_offset;
};
#pragma pack(pop)
CHECK_STRUCT_OFFSET(s_object_data_definition, node_buffer_size, 0x114);
CHECK_STRUCT_OFFSET(s_object_data_definition, nodes_offset, 0x116);
CHECK_STRUCT_SIZE(s_object_data_definition, 0x12C);

struct s_unit_data_definition : s_object_data_definition
{
	char gap_12C[4];
	DWORD field_130;
	datum simulation_actor_index;
	DWORD unit_flags;		  //(unit_data->unit_flags & 8) != 0   -->active_camo_active
							  //unit_data->unit_flags |= 2         -->unit_is_alive
	e_object_team unit_team;
	char pad[3];
	datum controlling_player_index;
	char gap_142[12];
	DWORD control_flags;
	DWORD control_flags_2;
	DWORD animation_state;
	real_vector3d desired_facing;
	real_vector3d desired_aiming;
	real_vector3d aiming_vector;
	real_vector3d aiming_vector_velocity;
	real_vector3d desired_looking;
	real_vector3d looking_vector;
	real_vector3d looking_vector_velocity;
	DWORD field_1B0;
	DWORD field_1B4;
	DWORD field_1B8;
	real_vector3d throttle;
	char aiming_speed;			//might not be char
	char gap_1C9[3];
	float trigger;
	float secondary_trigger;
	s_aim_assist_targetting_data target_info;
	char gap_1F8[24];
	DWORD parent_seat_index;
	char gap_214[20];
	WORD weapon_set_identifier;
	char gap_22A[39];
	char current_grenade_index;
	WORD grenade_counts_mask;
	char gap_254;
	char zoom_level;
	char gap_256[110];
	float active_camo_power;
	char gap_2C8[4];
	float active_camo_regrowth;
	PAD(144);
};
CHECK_STRUCT_SIZE(s_unit_data_definition, 0x360);

struct s_biped_data_definition : s_unit_data_definition
{
	byte gap0[148];
	e_biped_physics_mode biped_mode; //0x3F4
	byte gap1[139];

	// NEW DATA
	string_id variant_name;
};
CHECK_STRUCT_SIZE(s_biped_data_definition, sizeof(s_unit_data_definition) + 288 + 4);

struct s_weapon_data_definition : s_object_data_definition
{
	byte gap[304];
};
CHECK_STRUCT_SIZE(s_weapon_data_definition, 0x25C);



enum e_object_header_flag : BYTE
{
	_object_header_active_bit = FLAG(0),
	_object_header_awake_bit = FLAG(1),
	_object_header_requires_motion_bit = FLAG(2),
	_object_header_being_deleted_bit = FLAG(3),
	_object_header_flags_10 = FLAG(4),
	_object_header_connected_to_map_bit = FLAG(5),
	_object_header_child_bit = FLAG(6)
};
ENUM_OPERATORS(e_object_header_flag, BYTE)

struct s_object_header {
	__int16 datum_salt;
	e_object_header_flag flags;
	e_object_type object_type;
	__int16 cluster_index;
	__int16 object_data_size;
	void* object;
};
CHECK_STRUCT_SIZE(s_object_header, 0xC);

struct s_object_payload
{
	WORD object_type;
	WORD object_collision_cull_flags;
	real_point3d origin_point;
	float bounding_sphere_radius;
};
CHECK_STRUCT_SIZE(s_object_payload, 0x14);

static s_data_array* get_object_data_array()
{
	return *Memory::GetAddress<s_data_array**>(0x4E461C, 0x50C8EC);
};

// Gets the header of the object, containing some details
static s_object_header* get_object_header(datum object_idx)
{
	s_data_array* objects_header = get_object_data_array();
	return (s_object_header*)(&objects_header->data[objects_header->single_element_size * DATUM_INDEX_TO_ABSOLUTE_INDEX(object_idx)]);
}

// Get the object fast, with no validation from datum index
template<typename T = s_object_data_definition>
static T* object_get_fast_unsafe(datum object_idx)
{
	return (T*)get_object_header(object_idx)->object;
}

static real_matrix4x3* get_object_nodes(datum object_idx, int* out_node_count)
{
	auto object = object_get_fast_unsafe(object_idx);
	*out_node_count = object->node_buffer_size / sizeof(real_matrix4x3);
	return (real_matrix4x3*)((char*)object + object->nodes_offset);
}

// Gets the object and verifies the type, returns NULL if object doesn't match object type flags
template<typename T = s_object_data_definition>
static T* object_try_and_get_and_verify_type(datum object_idx, int object_type_flags)
{
	auto p_object_try_and_get_and_verify_type = Memory::GetAddress<char* (__cdecl*)(datum, int)>(0x1304E3, 0x11F3A6);
	return (T*)p_object_try_and_get_and_verify_type(object_idx, object_type_flags);
}

void create_new_placement_data(object_placement_data* object_placement_data, datum object_definition_idx, datum object_owner_idx, int a4);
datum object_new(object_placement_data* object_placement_data);
void apply_biped_object_definition_patches();
void simulation_action_object_create(datum object_idx);
void object_delete(const datum object_idx);
void object_wake(const datum object_datum);
void __cdecl object_disconnect_from_map(const datum object_index);
void __cdecl object_reconnect_to_map(const s_location* location_struct, const datum object_index);
void object_compute_node_matrices_with_children(const datum object_datum);

int object_get_count();
int object_count_from_iter();
