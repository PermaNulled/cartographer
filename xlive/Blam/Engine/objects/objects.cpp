#include "stdafx.h"
#include "objects.h"

#include "object_definition.h"
#include "object_early_movers.h"
#include "object_globals.h"
#include "object_types.h"

#include "animations/animation_manager.h"
#include "cache/cache_files.h"
#include "devices/devices.h"
#include "effects/effects.h"
#include "game/game.h"
#include "items/weapons.h"
#include "main/interpolator.h"
#include "memory/memory_pool.h"
#include "models/models.h"
#include "objects/widgets/widgets.h"
#include "physics/collisions.h"
#include "physics/havok.h"
#include "physics/havok_memory.h"
#include "scenario/scenario.h"
#include "simulation/simulation.h"
#include "structures/cluster_partitions.h"
#include "tag_files/tag_files.h"


/* prototypes */

s_memory_pool* get_object_table(void);

/* public code */

s_data_array* object_header_data_get(void)
{
	return *Memory::GetAddress<s_data_array**>(0x4E461C, 0x50C8EC);
};

void* __cdecl object_try_and_get_and_verify_type(datum object_index, int32 object_type_flags)
{
	return INVOKE(0x1304E3, 0x11F3A6, object_try_and_get_and_verify_type, object_index, object_type_flags);
}

void* object_header_block_get(datum object_datum, const object_header_block_reference* reference)
{
	return (void*)((char*)object_get_fast_unsafe(object_datum) + reference->offset);
}

void* object_header_block_get_with_count(datum object_index, const object_header_block_reference* reference, uint32 element_size, int32* element_count)
{
	void* block;
	if (reference->offset == NONE)
	{
		block = NULL;
		*element_count = 0;
	}
	else
	{
		block = (void*)object_header_block_get(object_index, reference);
		*element_count = reference->size / element_size;
	}

	return block;
}

int32 object_get_entity_index(datum object_idx)
{
	return object_get_fast_unsafe(object_idx)->simulation_entity_index;
}

datum __cdecl object_header_new(int16 object_data_size)
{
	return INVOKE(0x130AF6, 0x11F9B9, object_header_new, object_data_size);
}

void __cdecl object_placement_data_new(object_placement_data* object_placement_data, datum object_definition_idx, datum object_owner_idx, s_damage_owner* damage_owner)
{
	INVOKE(0x132163, 0x121033, object_placement_data_new, object_placement_data, object_definition_idx, object_owner_idx, damage_owner);
	return;
}

bool __cdecl object_is_connected_to_map(datum object_index)
{
	return INVOKE(0x132922, 0x1217F2, object_is_connected_to_map, object_index);
}

void __cdecl object_connect_lights_recursive(datum object_datum, bool disconnect_this_object, bool reconnect_this_object, bool reconnect_child_objects, bool show_this_object)
{
	INVOKE(0x134A78, 0x123948, object_connect_lights_recursive, object_datum, disconnect_this_object, reconnect_this_object, reconnect_child_objects, show_this_object);
	return;
}

void __cdecl object_update_collision_culling(datum object_datum)
{
	INVOKE(0x1324E9, 0x1213B9, object_update_collision_culling, object_datum);
	return;
}

bool __cdecl object_header_block_allocate(datum object_datum, int16 offset, int16 padded_size, int16 alignment_bits)
{
	return INVOKE(0x130BC6, 0x11FA89, object_header_block_allocate, object_datum, offset, padded_size, alignment_bits);
}

// Sets the mode_tag_index and coll_tag_index parameters (if they exist)
// Returns whether or not the render model has PRT or Lighting Info
bool __cdecl object_is_prt_and_lightmapped(datum object_datum, datum* mode_tag_index, datum* coll_tag_index)
{
	return INVOKE(0x131B0E, 0x1209DE, object_is_prt_and_lightmapped, object_datum, mode_tag_index, coll_tag_index);
}

// Returns the index of the variant block inside the model tag of the passed object_index
int8 __cdecl object_lookup_variant_index_from_name(datum object_index, string_id variant)
{
	return INVOKE(0x12FE84, 0x11ED47, object_lookup_variant_index_from_name, object_index, variant);
}

// Updates variant index from variant string id
void update_object_variant_index(datum object_index, string_id variant)
{
	object_datum* object = object_get_fast_unsafe(object_index);
	object->model_variant_id = object_lookup_variant_index_from_name(object_index, variant);
	return;
}

void __cdecl update_object_region_information(datum object_datum, int32 region_index)
{
	INVOKE(0x132F07, 0x121DD7, update_object_region_information, object_datum, region_index);
	return;
}

pixel32 __cdecl object_set_initial_change_colors(datum object_index, uint32 change_color_override_mask, real_rgb_color* colour)
{
	return INVOKE(0x1310F9, 0x11FFBD, object_set_initial_change_colors, object_index, change_color_override_mask, colour);
}

// Initializes health and shields for the given object based on two floats passed to it
// If the floats for vitality are NULL we just initialize them to the default values in the damage info block in the model tag
void object_initialize_vitality(datum object_index, const real32* new_vitality, const real32* new_shield_vitality)
{
	INVOKE(0x175A62, 0x14D2C5, object_initialize_vitality, object_index, new_vitality, new_shield_vitality);
	return;
}

bool __cdecl object_compute_change_colors(datum object_index)
{
	return INVOKE(0x13456E, 0x12343E, object_compute_change_colors, object_index);
}

// Wakes object by adding the _object_header_awake_bit to every object and all parents of that object
void __cdecl object_wake(datum object_index) 
{
	return INVOKE(0x12FA1E, 0x11E8E1, object_wake, object_index);
}

void __cdecl object_activate(datum object_index)
{
	INVOKE(0x13204A, 0x120F1A, object_activate, object_index);
	return;
}

void __cdecl object_deactivate(datum object_index)
{
	INVOKE(0x1320B8, 0x120F88, object_deactivate, object_index);
	return;
}

void __cdecl object_cleanup_havok(datum object_index)
{
	INVOKE(0xA2778, 0x949D8, object_cleanup_havok, object_index);
	return;
}

void object_reset_interpolation(datum object_index)
{
	const object_datum* object = object_get_fast_unsafe(object_index);

	ASSERT(object);

	c_animation_manager* animation_manager = (c_animation_manager*)object_header_block_get(object_index, &object->animation_manager_block);
	animation_manager->interpolator_controls[1].disable();
	object_wake(object_index);
	return;
}

void __cdecl object_compute_node_matrices_with_children(datum object_index)
{
	INVOKE(0x136924, 0x1257F4, object_compute_node_matrices_with_children, object_index);
	return;
}

bool object_can_activate_in_cluster(datum object_index, s_game_cluster_bit_vectors* cluster_activation)
{
	bool result = false;
	object_header_datum* object_header = (object_header_datum*)datum_get(object_header_data_get(), object_index);
	const object_datum* object = (object_datum*)object_header->datum;

	if (object->flags.test(_object_always_active_bit))
	{
		return true;
	}
	if (TEST_FLAG(FLAG(object_header->type), _object_mask_machine) && object_type_compute_activation(object_index, cluster_activation, &result) || object_header->cluster_index == NONE)
	{
		return result;
	}
	return BIT_VECTOR_TEST_FLAG(cluster_activation->cluster_bitvector, object_header->cluster_index);
}


bool set_object_position_if_in_cluster(s_location* location, datum object_index)
{
	object_datum* object = object_get_fast_unsafe(object_index);
	
	ASSERT(object->parent_object_index == NONE);
	
	scenario_location_from_point(location, &object->object_origin_point);

	if (location->cluster_index == NONE)
	{
		collision_bsp_test_sphere_result test_result;
		collision_bsp_test_sphere(global_collision_bsp_get(), 0, NULL, &object->object_origin_point, object->shadow_sphere_radius, &test_result);

		if (test_result.stack_depth)
		{
			scenario_location_from_leaf(location, test_result.buffer3.leaf_index);
		}
		else
		{
			scenario_location_from_point(location, &object->position);
		}
	}

	return location->cluster_index != NONE;
}

// Gets important info about the object and populates the s_object_payload argument with the appropriate data
void get_object_payload(datum object_index, s_object_payload* payload)
{
	const object_datum* object = object_get_fast_unsafe(object_index);
	uint16 object_collision_cull_flags = 0;
	if (object->flags.test(_object_uses_collidable_list_bit))
	{
		object_collision_cull_flags = collision_compute_object_cull_flags(object_index);
	}

	ASSERT(payload);

	payload->object_type = (int16)object->object_identifier.get_type();
	payload->object_collision_cull_flags = object_collision_cull_flags;
	payload->origin_point = object->object_origin_point;
	payload->bounding_sphere_radius = object->shadow_sphere_radius;
	return;
}

// Reconnects an object to the current bsp that's loaded
void object_reconnect_to_map(s_location* location, datum object_index)
{
	object_header_datum* object_header = (object_header_datum *)datum_get(object_header_data_get(), object_index);
	object_datum* object = (object_datum *)object_header->datum;
	bool cluster_index_is_null = object_header->cluster_index == NONE;

	ASSERT(DATUM_INDEX_TO_IDENTIFIER(object_index));
	ASSERT(object->parent_object_index == NONE);
	ASSERT(!object_header->flags.test(_object_header_connected_to_map_bit));
	ASSERT(!object_header->flags.test(_object_header_child_bit));
	ASSERT(!object->flags.test(_object_connected_to_map_bit));
	ASSERT(!object->flags.test(_object_in_limbo_bit));
	ASSERT(objects_can_connect_to_map());

	s_location* p_location = location;
	s_location scnr_location;

	if (!p_location)
	{
		scenario_location_from_point(&scnr_location, &object->object_origin_point);
		p_location = &scnr_location;
		if (scnr_location.cluster_index == NONE)
		{
			scenario_location_from_point(&scnr_location, &object->position);
		}
	}

	if (p_location->cluster_index == NONE)
	{
		object->flags.set(_object_outside_of_map_bit, true);
	}
	else
	{
		object->location.leaf_index = p_location->leaf_index;
		object->location.cluster_index = p_location->cluster_index;
		object->location.bsp_index = p_location->bsp_index;
		object_header->cluster_index = p_location->cluster_index;
		object->flags.set(_object_outside_of_map_bit, false);
	}
	s_game_cluster_bit_vectors cluster_bitvector;
	s_game_cluster_bit_vectors* p_cluster_bitvector = NULL;
	bool cluster_overflow = false;
	if (object->flags.test(_object_cinematic_visibility_bit))
	{
		csmemset(cluster_bitvector.cluster_bitvector, NONE, BIT_VECTOR_SIZE_IN_BYTES(get_global_structure_bsp()->clusters.count));
		p_cluster_bitvector = &cluster_bitvector;
	}

	s_object_payload payload;
	get_object_payload(object_index, &payload);

	cluster_partition* partition = collideable_object_cluster_partition_get();
	if (!object->flags.test(_object_uses_collidable_list_bit))
	{
		partition = noncollideable_object_cluster_partition_get();
	}

	cluster_partition_reconnect(partition,
		object_index,
		&object->first_cluster_reference,
		&object->object_origin_point,
		object->shadow_sphere_radius,
		&object->location,
		p_cluster_bitvector,
		sizeof(payload),
		&payload,
		&cluster_overflow);

	object->flags.set(_object_connected_to_map_bit, true);
	object_header->flags.set(_object_header_connected_to_map_bit, true);

	if (cluster_overflow)
	{
		const char* name = tag_get_name(object->tag_definition_index);
		error(8, 2, "### WARNING object %s touched too many clusters", name);
	}

	object_connect_lights_recursive(object_index, false, true, false, false);

	if (object_can_activate_in_cluster(object_index, game_get_cluster_activation()))
	{
		object_activate(object_index);
	}
	else if (!object->flags.test(_object_deleted_when_deactivated_bit) || simulation_query_object_is_predicted(object_index))
	{
		object_deactivate(object_index);
	}
	else
	{
		object_delete(object_index);
	}

	if (cluster_index_is_null && object_header->cluster_index != NONE)
	{
		object_cleanup_havok(object_index);
	}

	return;
}

void object_postprocess_node_matrices(datum object_index)
{
	const object_datum* object = object_get_fast_unsafe(object_index);
	const object_definition* object_tag = (object_definition*)tag_get_fast(object->tag_definition_index);

	const datum model_index = object_tag->object.model.index;
	if (model_index != NONE)
	{
		const s_model_definition* model_tag = (s_model_definition*)tag_get_fast(model_index);
		if (model_tag->render_model.index != NONE && model_tag->animation_graph.index != NONE)
		{
			int32 node_count;
			real_matrix4x3* node_matricies = object_get_node_matrices(object_index, &node_count);
			object_type_postprocess_node_matrices(object_index, node_count, node_matricies);
		}
	}
	return;
}

void __cdecl object_reconnect_to_physics(datum object_index)
{
	INVOKE(0x1323B3, 0x121283, object_reconnect_to_physics, object_index);
	return;
}

void __cdecl attachments_new(datum object_index)
{
	INVOKE(0x1348CA, 0x12379A, attachments_new, object_index);
	return;
}

void object_initialize_effects(datum object_index)
{
	const object_header_datum* object_header = (object_header_datum*)datum_get(object_header_data_get(), object_index);
	object_datum* object = (object_datum*)object_header->datum;

	if (object_header->type == _object_type_projectile)
	{
		object->object_projectile_datum = NONE;
	}
	else
	{
		widgets_new(object_index);
	}
	attachments_new(object_index);
	return;
}

// Not used on dedicated servers
void object_occlusion_data_initialize(datum object_index)
{
	INVOKE(0x26CFDD, 0x0, object_occlusion_data_initialize, object_index);
	return;
}

void free_object_memory(datum object_index)
{
	object_header_datum* object_header = (object_header_datum*)datum_get(object_header_data_get(), object_index);
	object_header->flags = (e_object_header_flags)0;
	if (object_header->datum != NULL)
	{
		memory_pool_block_free(get_object_table(), &object_header->datum);
	}
	datum_delete(object_header_data_get(), object_index);
	return;
}


void object_initialize_for_interpolation(datum object_index)
{
	uint16 abs_object_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(object_index);
	object_datum* object = object_get_fast_unsafe(object_index);
	object_definition* object_def = (object_definition*)tag_get_fast(object->tag_definition_index);

	if (object_def->object.model.index == NONE)
	{
		if ((int16)object_def->object.attachments.count <= 0)
		{
			return;
		}

		int16 tag_block_index = 0;
		while (1)
		{
			object_attachment_definition* attachment = object_def->object.attachments[tag_block_index];
			if (attachment->type.index != NONE)
			{
				tag_group type = attachment->type.group;
				if (type.group == 'lens'
					|| type.group == 'ligh'
					|| type.group == 'MGS2'
					|| type.group == 'tdtl'
					|| type.group == 'cont'
					|| type.group == 'effe')
				{
					break;
				}
			}
			if (++tag_block_index >= (int16)object_def->object.attachments.count)
			{
				return;
			}
		}
	}

	int32 nodes_count;
	real_point3d center_of_mass;
	real_matrix4x3* matrices = (real_matrix4x3*)object_header_block_get_with_count(object_index, &object->nodes_block, sizeof(real_matrix4x3), &nodes_count);
	object_get_center_of_mass(object_index, &center_of_mass);
	halo_interpolator_object_populate_interpolation_data(object_index, matrices, nodes_count, &object->position, &object->forward, &object->up, &center_of_mass);
	return;
}

datum object_allocate_header(datum tag_definition_index)
{
	datum object_index = NONE;
	if (tag_definition_index != NONE)
	{
		const object_definition* object_def = (object_definition*)tag_get_fast(tag_definition_index);
		const object_type_definition* object_type_definition = object_type_definition_get((e_object_type)object_def->object.object_type);
		const s_model_definition* model_definition = NULL;
		object_index = object_header_new(object_type_definition->datum_size);

		if (object_index != NONE)
		{
			object_header_datum* object_header = (object_header_datum*)datum_get(object_header_data_get(), object_index);
			object_datum* object = object_get_fast_unsafe(object_index);
		}
	}

	return object_index;
}

datum object_new_internal(datum object_index, object_placement_data* data)
{
	bool process_is_game_client = !Memory::IsDedicatedServer();

	const object_definition* object_def = (object_definition*)tag_get_fast(data->tag_index);
	const object_type_definition* object_type_definition = object_type_definition_get((e_object_type)object_def->object.object_type);
	const s_model_definition* model_definition = NULL;

	const datum object_model_index = object_def->object.model.index;
	if (object_model_index != NONE)
	{
		model_definition = (s_model_definition*)tag_get_fast(object_model_index);
	}

	halo_interpolator_setup_new_object(object_index);
	object_header_datum* object_header = (object_header_datum*)datum_get(object_header_data_get(), object_index);
	object_datum* object = object_get_fast_unsafe(object_index);

	object_header->flags.set(_object_header_post_update_bit, true);
	object_header->type = (int8)object_def->object.object_type;
	object->tag_definition_index = data->tag_index;

	if (data->object_identifier.get_source() == NONE)
	{
		object->object_identifier.create_dynamic((e_object_type)object_def->object.object_type);
		object->placement_index = NONE;
		object->structure_bsp_index = (uint8)get_global_structure_bsp_index();
	}
	else
	{
		ASSERT(data->scenario_datum_index != NONE);
		ASSERT(data->object_identifier.get_type() == object_def->object.object_type);
		object->object_identifier = data->object_identifier;
		object->placement_index = (int16)data->scenario_datum_index;
		object->structure_bsp_index = (uint8)data->object_identifier.get_origin_bsp();
	}

	object->position = data->position;
	object->forward = data->forward;
	object->up = data->up;
	object->translational_velocity = data->translational_velocity;
	object->angular_velocity = data->angular_velocity;
	object->scale = data->scale;

	bool enable = data->flags.test(_scenario_object_placement_bit_0);
	object->flags.set(_object_mirrored_bit, enable);
	enable = model_definition && model_definition->collision_model.index != NONE;
	object->flags.set(_object_uses_collidable_list_bit, enable);

	datum mode_index;
	datum coll_index;
	enable = object_is_prt_and_lightmapped(object_index, &mode_index, &coll_index);
	object->flags.set(_object_has_prt_or_lighting_info_bit, enable);

	object_header->cluster_index = NONE;
	location_invalidate(&object->location);
	object->first_cluster_reference = NONE;
	object->parent_object_index = NONE;
	object->next_index = NONE;
	object->current_weapon_datum = NONE;
	object->name_list_index = NONE;
	object->netgame_equipment_index = NONE;
	object->byte_108 = NONE;
	object->byte_109 = NONE;
	object->placement_policy = data->placement_policy;
	if (TEST_FLAG(object_def->object.flags, _object_definition_does_not_cast_shadow))
	{
		object->flags.set(_object_shadowless_bit, true);
	}

	if (object->flags.test(_object_hidden_bit))
	{
		object->flags.set(_object_hidden_bit, false);
		if (object_is_connected_to_map(object_index))
		{
			object_connect_lights_recursive(object_index, false, true, false, false);
		}
		object_update_collision_culling(object_index);
	}

	object->damage_owner_target_model_abs_index = data->damage_owner.target_model_abs_index;
	object->damage_owner_owner_index = data->damage_owner.owner_index;
	object->damage_owner_object_index = data->damage_owner.entity_index;
	object->model_variant_id = NONE;
	object->cached_render_state_index = NONE;
	object->field_D0 = NONE;
	object->physics_flags.set_unsafe(0);
	object->physics_flags.set(_object_physics_bit_8, data->flags.test(_scenario_object_placement_bit_3));
	object->havok_datum = NONE;
	object->simulation_entity_index = NONE;
	object->attached_to_simulation = 0;
	object->destroyed_constraints_flag = data->destroyed_constraints_flag;
	object->loosened_constraints_flag = data->loosened_constraints_flag;

	uint32 node_count = 1;
	uint32 collision_regions_count = 1;
	uint32 damage_info_damage_sections_size = 0;

	bool allow_interpolation = false;
	bool valid_animation_manager = false;

	if (model_definition)
	{
		if (model_definition->nodes.count >= 1)
		{
			node_count = model_definition->nodes.count;
		}
		else
		{
			const char* model_name = tag_get_name(object_model_index);
			const char* object_name = tag_get_name(data->tag_index);
			error(8, 2, "object '%s' model '%s' has invalid node count %d!", object_name, model_name, model_definition->nodes.count);
		}


		if (model_definition->collision_regions.count >= 1)
		{
			collision_regions_count = model_definition->collision_regions.count;
		}
		else
		{
			const char* model_name = tag_get_name(object_model_index);
			const char* object_name = tag_get_name(data->tag_index);
			error(8, 2, "object '%s' model '%s' has invalid region count %d!", object_name, model_name, model_definition->collision_regions.count);
		}

		if (model_definition->new_damage_info.count > 0 && model_definition->new_damage_info.data != NONE)
		{
			damage_info_damage_sections_size = model_definition->new_damage_info[0]->damage_sections.count;
		}

		if (model_definition->animation_graph.index != NONE)
		{
			c_animation_manager animation_manager;
			if (animation_manager.reset_graph(model_definition->animation_graph.index, object_model_index, true))
			{
				valid_animation_manager = true;
				allow_interpolation = !TEST_FLAG(FLAG(object_def->object.object_type), _object_mask_cannot_interpolate);

				// allow interpolation if object is device and device flags include interpolation
				if (TEST_FLAG(FLAG(object_def->object.object_type), _object_mask_device))
				{
					device_definition* device_def = (device_definition*)tag_get_fast(data->tag_index);
					if (TEST_FLAG(device_def->device.flags, _device_definition_allow_interpolation))
					{
						allow_interpolation = true;
					}
				}
			}
			else
			{
				const char* model_path = tag_get_name(object_model_index);
				const char* model_name = tag_name_strip_path(model_path);
				const char* graph_path = tag_get_name(model_definition->animation_graph.index);
				const char* graph_name = tag_name_strip_path(graph_path);
				error(3, 2, "graph '%s' is not compatible with model '%s'", graph_name, model_name);
			}
		}
	}


	const int16 orientation_size = (int16)(!allow_interpolation ? 0 : 32 * node_count);

	// TODO: header asserts here

	// Allocate object header blocks
	bool can_create_object =
		object_header_block_allocate(object_index, offsetof(object_datum, object_attachments_block), (uint16)8 * (uint16)object_def->object.attachments.count, 0)
		&& object_header_block_allocate(object_index, offsetof(object_datum, damage_sections_block), (int16)(8 * damage_info_damage_sections_size), 0)
		&& object_header_block_allocate(object_index, offsetof(object_datum, change_color_block), (uint16)24 * (uint16)object_def->object.change_colors.count, 0)
		&& object_header_block_allocate(object_index, offsetof(object_datum, nodes_block), (int16)(sizeof(real_matrix4x3) * node_count), 0)
		&& object_header_block_allocate(object_index, offsetof(object_datum, collision_regions_block), (int16)(10 * collision_regions_count), 0)
		&& object_header_block_allocate(object_index, offsetof(object_datum, original_orientation_block), orientation_size, 4)
		&& object_header_block_allocate(object_index, offsetof(object_datum, node_orientation_block), orientation_size, 4)
		&& object_header_block_allocate(object_index, offsetof(object_datum, animation_manager_block), (valid_animation_manager ? 144 : 0), 0)
		&& havok_can_allocate_space_for_instance_of_object_definition(data->tag_index);

	// If one of the object headers cannot be allocated then something has gone horribly wrong and we can't create our object
	bool out_of_objects = !can_create_object;
	if (can_create_object)
	{
		if (valid_animation_manager)
		{
			ASSERT(model_definition);
			ASSERT(model_definition->animation_graph.index != NONE);
			ASSERT(object);

			c_animation_manager* animation_manager = (c_animation_manager*)object_header_block_get(object_index, &object->animation_manager_block);
			animation_manager->initialize();
			
			bool graph_reset = animation_manager->reset_graph(model_definition->animation_graph.index, object_model_index, true);
			ASSERT(graph_reset);

			object->flags.set(_object_dynamic_lighting_recompute_bit, graph_reset);
		}

		// Null attachment block
		if (object_def->object.attachments.count > 0)
		{
			ASSERT(object);
			int32 attachments_count;
			object_attachment* object_attachments_block = (object_attachment*)object_header_block_get_with_count(object_index,
				&object->object_attachments_block,
				sizeof(object_attachment),
				&attachments_count);

			csmemset(object_attachments_block, NONE, sizeof(object_attachment) * attachments_count);
		}

		if (object_type_new(object_index, data, &out_of_objects))
		{
			bool object_flag_check = object->flags.test(_object_deleted_when_deactivated_bit);
			if (data->flags.test(_scenario_object_placement_bit_1) || data->flags.test(_scenario_object_placement_bit_2))
			{
				object->flags.set(_object_deleted_when_deactivated_bit, false);
			}

			update_object_variant_index(object_index, data->variant_name);
			update_object_region_information(object_index, data->region_index);
			object_set_initial_change_colors(object_index, data->active_change_colors_mask, data->change_colors);
			object_initialize_vitality(object_index, NULL, NULL);
			object_compute_change_colors(object_index);
			object->emblem_info = data->emblem_info;

			if (object->animation_manager_block.offset != NONE)
			{
				object_reset_interpolation(object_index);
			}

			object_compute_node_matrices_with_children(object_index);

			// If the object (can) connect to the map we make sure it gets connected
			if (objects_can_connect_to_map())
			{
				data->object_is_inside_cluster = set_object_position_if_in_cluster(&data->location, object_index);

				// If the object is inside a cluster set the location to the one passed in the placement data
				// If not then pass null
				s_location* p_location = (data->object_is_inside_cluster ? &data->location : NULL);
				object_reconnect_to_map(p_location, object_index);
			}

			object_postprocess_node_matrices(object_index);
			object_initialize_for_interpolation(object_index);

			object_wake(object_index);

			object->physics_flags.set(_object_physics_bit_2, data->flags.test(_scenario_object_placement_bit_5));

			object_reconnect_to_physics(object_index);
			object_initialize_effects(object_index);
			object_type_create_children(object_index);

			const datum object_creation_effect_index = object_def->object.creation_effect.index;
			if (object_creation_effect_index != NONE)
			{
				effect_new_from_object(object_creation_effect_index, &data->damage_owner, object_index, 0.f, 0.f, NULL, NULL);
			}

			// Not 100% sure what this function does but it has to do with occlusion
			// This function is nulled out on the dedi
			if (process_is_game_client)
			{
				object_occlusion_data_initialize(object_index);
			}

			object->flags.set(_object_deleted_when_deactivated_bit, object_flag_check);
			object_early_mover_new(object_index);

			if (object->flags.test(_object_deleted_when_deactivated_bit) && !object_header->flags.test(_object_header_active_bit))
			{
				if (objects_can_connect_to_map())
				{
					if (!data->flags.test(_scenario_object_placement_bit_1) && (!data->flags.test(_scenario_object_placement_bit_2) || object->location.cluster_index != NONE))
					{
						object_delete(object_index);
					}
				}
			}
		}
		else
		{
			object_type_delete(object_index);
			free_object_memory(object_index);
			object_index = NONE;
		}
	}
	else
	{
		free_object_memory(object_index);
		object_index = NONE;
	}

#ifdef _DEBUG
	if (object_index == NONE && data->tag_index != NONE)
	{
		const char* message = out_of_objects ? "OUT OF OBJECTS" : "OBJECT CREATION FAILED";
		const char* object_path = tag_get_name(data->tag_index);
		const char* object_name = tag_name_strip_path(object_path);
		error(8, 3, "%s: cannot create %s", message, object_name);

		// TODO: error globals here 
	}
#endif

	return object_index;
}

typedef datum (__cdecl* t_object_new)(object_placement_data* placement_data);
t_object_new p_object_new;

// Creates a new object
datum __cdecl object_new(object_placement_data* placement_data)
{
	datum object_index = NONE;

	if (!placement_data->flags.test(_scenario_object_placement_bit_4) 
		&& placement_data->tag_index != NONE)
	{
		object_type_adjust_placement(placement_data);
	}

	if (placement_data->tag_index != NONE)
	{
	 	const object_definition* object_def = (object_definition*)tag_get_fast(placement_data->tag_index);

		if (
			TEST_FLAG(FLAG(object_def->object.object_type),
				(FLAG(_object_type_creature) | FLAG(_object_type_crate) | FLAG(_object_type_machine) | FLAG(_object_type_vehicle) | FLAG(_object_type_biped)))
			)
		{
			havok_memory_garbage_collect();
		}

		object_index = object_allocate_header(placement_data->tag_index);
		if (object_index != NONE)
		{
			object_index = object_new_internal(object_index, placement_data);
		}
	}
	
	return object_index;
}

void __cdecl object_delete(datum object_index)
{
	INVOKE(0x136005, 0x124ED5, object_delete, object_index);
	return;
}

real_point3d* __cdecl object_get_center_of_mass(datum object_index, real_point3d* point)
{
	return INVOKE(0x132A23, 0x1218F3, object_get_center_of_mass, object_index, point);
}

void __cdecl object_get_origin(datum object_index, real_point3d* point_out, bool interpolated)
{
	real_point3d point;
	real_point3d interpolated_object_position;
	object_datum* object = object_get_fast_unsafe(object_index);
	if (interpolated && halo_interpolator_interpolate_object_position(object_index, &interpolated_object_position))
	{
		point = interpolated_object_position;
	}
	else
	{
		point = object->position;
		interpolated_object_position = object->position;
	}

	if (object->parent_object_index == NONE)
	{
		*point_out = point;
	}
	else
	{
		real_matrix4x3 interpolated_matrix;
		real_matrix4x3* transform_matrix = &interpolated_matrix;
		if (!interpolated || !halo_interpolator_interpolate_object_node_matrix(object->parent_object_index, object->matrix_index, &interpolated_matrix))
		{
			transform_matrix = object_get_node_matrix(object->parent_object_index, object->matrix_index);
		}
		matrix4x3_transform_point(transform_matrix, &interpolated_object_position, point_out);
	}
}

void __cdecl object_get_origin_interpolated(datum object_index, real_point3d* point_out)
{
	object_get_origin(object_index, point_out, true);
}

real_matrix4x3* object_get_node_matrix(datum object_index, int16 node_index)
{
	real_matrix4x3* nodes = (real_matrix4x3*)object_header_block_get(object_index, &object_get_fast_unsafe(object_index)->nodes_block);
	return &nodes[node_index];
}

real_matrix4x3* object_try_get_node_matrix_interpolated(datum object_index, int16 node_index, real_matrix4x3* out_mat)
{
	if (!halo_interpolator_interpolate_object_node_matrix(object_index, node_index, out_mat))
	{
		*out_mat = *object_get_node_matrix(object_index, node_index);
	}
	return out_mat;
}

real_matrix4x3* object_get_node_matrices(datum object_datum, int32* out_node_count)
{
	return (real_matrix4x3*)object_header_block_get_with_count(object_datum, &object_get_fast_unsafe(object_datum)->nodes_block, sizeof(real_matrix4x3), out_node_count);
}

int32 __cdecl object_get_skinning_matrices(datum object_index, int32 skinning_matrix_count, real_matrix4x3* object_skinning_matrices, real_matrix4x3* out_object_skinning_matrices)
{
	return INVOKE(0x1963C7, 0x0, object_get_skinning_matrices, object_index, skinning_matrix_count, object_skinning_matrices, out_object_skinning_matrices);
}

datum object_get_damage_owner(datum object_index)
{
	object_datum* object = (object_datum*)object_try_and_get_and_verify_type(object_index, NONE);
	return object->damage_owner_object_index;
}

void __cdecl object_apply_function_overlay_node_orientations(datum object_index, 
	render_model_definition* render_model, 
	c_animation_manager* animation_manager, 
	int32 a4, 
	int32 orientation_count, 
	real_orientation* orientations)
{
	INVOKE(0x134214, 0x1230E4, object_apply_function_overlay_node_orientations, object_index, render_model, animation_manager, a4, orientation_count, orientations);
	return;
}

real_point3d* __cdecl object_get_center_of_mass_interpolated(datum object_index, real_point3d* center_of_mass)
{
	if (!halo_interpolator_interpolate_center_of_mass(object_index, center_of_mass))
	{
		center_of_mass = object_get_center_of_mass(object_index, center_of_mass);
	}
	return center_of_mass;
}

datum __cdecl object_get_parent_recursive(datum parent_index)
{
	return INVOKE(0x132574, 0x121444, object_get_parent_recursive, parent_index);
}

typedef void(__cdecl* t_object_move_t)(datum);
t_object_move_t p_object_move;
void __cdecl object_move(datum object_index)
{
	INVOKE(0x137E6D, 0x126D3D, object_move, object_index);
	object_initialize_for_interpolation(object_index);
	return;
}

bool __cdecl object_update(datum object_index)
{
	return INVOKE(0x1352a9, 0x124179, object_update, object_index);	
}

void __cdecl object_pre_delete_recursive(datum object_index)
{
	INVOKE(0x1386E1, 0x1275B1, object_pre_delete_recursive, object_index);
	return;
}

void __cdecl object_delete_recursive(datum object_index, bool remove_from_map_before_deletion)
{
	INVOKE(0x13683D, 0x12570D, object_delete_recursive, object_index, remove_from_map_before_deletion);
	return;
}

void __cdecl objects_garbage_collection(void)
{
	INVOKE(0x1316A4, 0x120574, objects_garbage_collection);
	return;
}

void __cdecl objects_purge_deleted_objects(void)
{
	s_data_iterator<object_header_datum> object_header_it(object_header_data_get());
	while (object_header_it.get_next_datum())
	{
		object_header_datum* object_header = object_header_it.get_current_datum();
		if (object_header->flags.test(_object_header_being_deleted_bit))
		{
			object_pre_delete_recursive(object_header_it.get_current_datum_index());
			object_delete_recursive(object_header_it.get_current_datum_index(), true);
		}
	}
}

void object_get_velocities(datum object_index, real_vector3d* translational_velocity, real_vector3d* angular_velocity)
{
	INVOKE(0x132BDE, 0, object_get_velocities, object_index, translational_velocity, angular_velocity);
}

uint32 __cdecl object_search_for_objects_in_radius(int32 search_flags, uint32 object_type_flags, s_location *location,
	real_point3d* center, real32 radius, datum* out_object_indices, uint32 object_count)
{
	return INVOKE(0x1331BA, 0x12208A, object_search_for_objects_in_radius, search_flags, object_type_flags, location, center, radius, out_object_indices, object_count);
}

void __cdecl objects_post_update()
{
	object_globals_get()->objects_updating = true;

	s_data_iterator<object_header_datum> object_header_it(object_header_data_get());
	while (object_header_it.get_next_datum())
	{
		object_header_datum* object_header = object_header_it.get_current_datum();
		object_datum* object = object_get_fast_unsafe(object_header_it.get_current_datum_index());

		object_header->flags.set(_object_header_do_not_update_bit, false);

		if (object_header->flags.test(_object_header_post_update_bit) 
			&& object_header->flags.test(_object_header_active_bit) 
			&& object_header->flags.test(_object_header_awake_bit) 
			&& !object_header->flags.test(_object_header_being_deleted_bit)
			)
		{
			object_header->flags.set(_object_header_post_update_bit, false);
			
			object_update(object_header_it.get_current_datum_index());

			if (object_header->flags.test(_object_header_requires_motion_bit))
				object_move(object_header_it.get_current_datum_index());
		}

		if (object_header->flags.test(_object_header_active_bit)
			&& object_header->flags.test(_object_header_awake_bit)
			&& !object_header->flags.test(_object_header_being_deleted_bit))
		{
			if (object->flags.test(_object_hidden_bit) && !Memory::IsDedicatedServer())
			{
				// reset the interpolator for this object, if hidden
				// ### FIXME maybe hook object_hide and reset it there?
				object_initialize_for_interpolation(object_header_it.get_current_datum_index());
			}
		}
	}

	weapons_fire_barrels();
	objects_purge_deleted_objects();
	object_globals_get()->objects_updating = false;
	objects_garbage_collection();
}

int16 __cdecl internal_object_get_markers_by_string_id(datum object_index, string_id marker, object_marker* marker_object, int16 count, bool is_unit)
{
	int32 marker_index = 0;

	datum index = object_index;
	if (!is_unit)
	{
		index = object_get_parent_recursive(index);
	}
	if (index != NONE)
	{
		object_datum* object = object_get_fast_unsafe(index);
		object_definition* object_def = (object_definition*)tag_get_fast(object->tag_definition_index);

		const datum object_model_index = object_def->object.model.index;
		if (object_model_index != NONE)
		{
			s_model_definition* model_def = (s_model_definition*)tag_get_fast(object_model_index);

			int32 node_count;
			real_matrix4x3* node_matrices;

			if (!halo_interpolator_interpolate_object_node_matrices(object_index, &node_matrices, &node_count))
			{
				node_matrices = object_get_node_matrices(object_index, &node_count);
			}

			void* collision_regions = (real_matrix4x3*)object_header_block_get(object_index, &object->collision_regions_block);

			int32 marker_index = render_model_get_markers_by_name(model_def->render_model.index, 
				marker, 
				collision_regions, 
				NONE, 
				NULL, 
				node_count, 
				node_matrices, 
				object->flags.test(_object_mirrored_bit), 
				marker_object,
				count);

			if (marker_index)
			{
				return (int16)marker_index;
			}
		}
	}

	object_datum* object = object_get_fast_unsafe(object_index);
	marker_object->node_index = 0;
	matrix4x3_identity(&marker_object->node_matrix);
		
	if (!halo_interpolator_interpolate_object_node_matrix(object_index, 0, &marker_object->matrix))
	{
		marker_object->matrix = *object_get_node_matrix(object_index, 0);
	}
		
	marker_object->field_6C = 0;
	if (object->flags.test(_object_mirrored_bit))
	{
		scale_vector3d(&marker_object->matrix.vectors.left, -1.0f, &marker_object->matrix.vectors.left);
	}

	return (marker != 0 ? (int16)marker_index : 1);
}

int16 __cdecl object_get_markers_by_string_id(datum object_index, string_id marker, object_marker* marker_object, int16 count)
{
	return internal_object_get_markers_by_string_id(object_index, marker, marker_object, count, false);
}

// Replace calls to functions getting the marker by string id
void object_get_markers_by_string_id_replace_calls(void)
{
	// PatchCall(Memory::GetAddress(0x132792, 0x121662), internal_object_get_markers_by_string_id);	(Disabled as this is not interpolated)	TODO: Add parameter to internal_object_get_markers_by_string_id instead of not patchcalling
	// PatchCall(Memory::GetAddress(0x1327B1, 0x121681), internal_object_get_markers_by_string_id);	(Disabled as this is not interpolated)	TODO: Add parameter to internal_object_get_markers_by_string_id instead of not patchcalling
	// PatchCall(Memory::GetAddress(0x13823D, 0x12710D), internal_object_get_markers_by_string_id);	(Disabled as this is not interpolated)	TODO: Add parameter to internal_object_get_markers_by_string_id instead of not patchcalling
	// PatchCall(Memory::GetAddress(0x138257, 0x1214CB), internal_object_get_markers_by_string_id);	(Disabled as this is not interpolated)	TODO: Add parameter to internal_object_get_markers_by_string_id instead of not patchcalling
	// PatchCall(Memory::GetAddress(0x134C26, 0x123AF6), internal_object_get_markers_by_string_id);	// object_animation_callback

	PatchCall(Memory::GetAddress(0xAAFA4, 0x0), object_get_markers_by_string_id);			// Function for creating effects at markers
	PatchCall(Memory::GetAddress(0xFDF7D, 0x0), object_get_markers_by_string_id);			// effect_on_new_object_marker	
	return;
}

// Replace calls to object_new with our own
void object_new_replace_calls(void)
{
	DETOUR_ATTACH(p_object_new, Memory::GetAddress<t_object_new>(0x136CA7, 0x125B77), object_new);
	return;
}

void object_move_replace_calls(void)
{
	PatchCall(Memory::GetAddress(0x137f8f), object_move);
	PatchCall(Memory::GetAddress(0x1381b1), object_move);
	PatchCall(Memory::GetAddress(0x13814b), object_move);
	return;
}

void objects_apply_interpolation_patches()
{
	object_new_replace_calls();

	if (!Memory::IsDedicatedServer())
	{
		object_move_replace_calls();
		object_get_markers_by_string_id_replace_calls();

		PatchCall(Memory::GetAddress(0x4A53C, 0x437BA), objects_post_update);
		PatchCall(Memory::GetAddress(0xCD744, 0xB8ABD), object_get_origin_interpolated);
		PatchCall(Memory::GetAddress(0x1549AE, 0x0), object_get_origin_interpolated);
		PatchCall(Memory::GetAddress(0x13D406, 0x12C255), object_get_center_of_mass_interpolated);

		// Prevents the game from passing the runtime_node_flags to the animation manager when updating object_node_matricies
		// When they are passed to the animation manager it causes the game to reset? node positions causing a flipping state between frames.
		WriteValue<uint8>(Memory::GetAddress(0x135657, 0x124527), JMP_OP_CODE);
	}
}

void objects_apply_patches(void)
{
	objects_apply_interpolation_patches();
	return;
}

/* private code */

s_memory_pool* get_object_table(void)
{
	return *Memory::GetAddress<s_memory_pool**>(0x4E4610, 0x50C8E0);
};
