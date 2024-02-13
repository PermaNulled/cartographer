#include "stdafx.h"
#include "simulation_queue_global_events.h"

#include "simulation.h"
#include "simulation_encoding.h"
#include "game_interface/simulation_game_action.h"

#include "game/game.h"
#include "game/game_engine.h"
#include "memory/bitstream.h"


void simulation_queue_player_event_insert(e_simulation_queue_player_event_type event_type, datum player_index, const s_simulation_queue_player_event_data* event_data)
{
    if (!game_is_playback())
    {
        uint8 data[128];
        c_bitstream stream(data, sizeof(data));

        uint16 abs_player_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);

        stream.begin_writing(1);
        stream.write_integer("player-index", abs_player_index, k_player_index_bit_count);
        stream.write_bool("active", event_data->active);

        int32 space_used = stream.get_space_used_in_bytes();
        if (!stream.error_occured())
        {
            s_simulation_queue_element* element = NULL;
            c_simulation_world* world = simulation_get_world();
            world->simulation_queue_allocate(_simulation_queue_element_type_player_event, space_used, &element);
            if (element)
            {
                csmemcpy((void*)element->data, data, space_used);
                world->simulation_queue_enqueue(element);
            }
        }
        stream.finish_writing(NULL);
    }
    return;
}

void simulation_queue_player_event_apply(const s_simulation_queue_element* element)
{
    c_bitstream stream(element->data, element->data_size);
	stream.begin_reading();

	uint16 abs_player_index = stream.read_integer("player-index", k_player_index_bit_count);
	bool active = stream.read_bool("active");

	if (!stream.error_occured())
	{
		datum player_index = player_index_from_absolute_player_index(abs_player_index);
		s_player* player = (s_player*)datum_get(s_player::get_data(), player_index);
		if (TEST_BIT(player->flags, 0) != active)
		{
			SET_FLAG(player->flags, 0, active);
			if (active)
			{
				game_engine_player_activated(player_index);
			}
		}
		simulation_action_game_engine_player_update(player_index, 0x200);
	}

    stream.finish_reading();
	return;
}

void simulation_queue_player_update_insert(const simulation_player_update* player_update)
{
    if (!game_is_playback())
    {
        uint8 data[k_simulation_payload_size_max];
        c_bitstream stream(data, sizeof(data));
        stream.begin_writing(1);
        simulation_player_update_encode(&stream, player_update);
        
        int32 space_used = stream.get_space_used_in_bytes();
        if (!stream.error_occured())
        {
            s_simulation_queue_element* element = NULL;
            c_simulation_world* world = simulation_get_world();
            world->simulation_queue_allocate(_simulation_queue_element_type_player_update_event, space_used, &element);
            if (element)
            {
                csmemcpy(element->data, data, space_used);
                world->simulation_queue_enqueue(element);
            }
        }
        stream.finish_writing(NULL);
    }
    return;
}

void simulation_queue_player_update_apply(const s_simulation_queue_element* element)
{
    simulation_player_update update;
    memset(&update, 0, sizeof(simulation_player_update));

    c_bitstream stream(element->data, element->data_size);
    stream.begin_reading();
    simulation_player_update_decode(&stream, &update);

    if (stream.error_occured())
    {
        // ASSERT HERE
    }
    else if (!simulation_players_apply_update(&update))
    {
       // ASSERT HERE
    }

    stream.finish_reading();
    return;
}