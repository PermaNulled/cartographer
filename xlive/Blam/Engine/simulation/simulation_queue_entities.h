#pragma once
#include "simulation_queue.h"
#include "simulation_type_collection.h"

struct s_simulation_queue_entity_data
{
	int32 entity_index;
	e_simulation_entity_type entity_type;
	int32 creation_data_size;
	uint8* creation_data;
	int32 state_data_size;
	uint8* state_data;
};

struct s_simulation_queue_decoded_creation_data
{
	int32 entity_index;
	e_simulation_entity_type entity_type;
	datum gamestate_index;
	uint32 initial_update_mask;
	uint32 creation_data_size;
	uint8 creation_data[k_simulation_entity_maximum_creation_data_size];
	uint32 state_data_size;
	uint8 state_data[k_simulation_entity_maximum_state_data_size];
};

struct s_simulation_queue_decoded_update_data
{
	int32 entity_index;
	e_simulation_entity_type entity_type;
	datum gamestate_index;
	uint32 update_mask;
	uint32 state_data_size;
	uint8 state_data[k_simulation_entity_maximum_state_data_size];
};

c_simulation_entity_definition* simulation_queue_entities_get_definition(e_simulation_entity_type type);

// creation
void simulation_queue_entity_creation_insert(s_simulation_queue_element* element);
bool simulation_queue_entity_creation_allocate(s_simulation_queue_entity_data* simulation_queue_entity_data, uint32 update_mask, s_simulation_queue_element** element, int32* gamestate_index);
void simulation_queue_entity_creation_apply(const s_simulation_queue_element* element);

// update
void simulation_queue_entity_update_insert(s_simulation_queue_element* simulation_queue_element);
bool simulation_queue_entity_update_allocate(s_simulation_queue_entity_data* sim_queue_entity_data, int32 gamestate_index, uint32 update_mask, s_simulation_queue_element** element);
void simulation_queue_entity_update_apply(const s_simulation_queue_element* element);

// deletion
void simulation_queue_entity_deletion_insert(s_simulation_game_entity* entity);
void simulation_queue_entity_deletion_apply(const s_simulation_queue_element* element);

// promotion
void simulation_queue_entity_promotion_insert(s_simulation_game_entity* entity);
void simulation_queue_entity_promotion_apply(const s_simulation_queue_element* element);
