#include "stdafx.h"
#include "MapSlots.h"

#include "cache/cache_files.h"
#include "filesys/pc_file_system.h"
#include "game/game_globals.h"
#include "main/level_definitions.h"
#include "scenario/scenario_definitions.h"
#include "tag_files/tag_loader/tag_injection.h"
#include "Util/filesys.h"


// TODO Move to Blam/Engine/main/levels once the below are complete
// TODO Cleanup this code
// TODO Add support for singleplayer maps in the future

/* enums */

enum e_default_maps_to_add
{
	_default_map_to_add_tombstone,
	_default_map_to_add_desolation,
	k_default_map_to_add_count
};

/* constants */

#define k_multiplayer_first_unused_slot 23
#define k_max_map_slots 49
#define k_starting_map_index 3000

const wchar_t* k_default_maps_to_add[k_default_map_to_add_count] = 
{
	L"highplains",
	L"derelict"
};

/* globals */

s_multiplayer_ui_level_definition g_added_multiplayer_level_data[k_default_map_to_add_count] = {};

/* prototypes */

static void map_slots_get_multiplayer_level_data(FILE* map_handle, s_multiplayer_ui_level_definition* multiplayer_level_data);

/* public code */

namespace MapSlots
{
	void CacheMapData(void)
	{
		//lots copied over from Tag Loader, using this function to grab the Level data in the scenario tag
		//And using that to construct a new s_multiplayer_levels_block and grab the bitmap datum for tag loading
		c_static_wchar_string260 game_path;
		game_path.set(GetExeDirectoryWide().c_str());

		LOG_TRACE_GAME("[Map Slots]: Startup - Caching map data");

		for (uint32 i = 0; i < k_default_map_to_add_count; ++i)
		{
			c_static_wchar_string260 map_location;
			map_location.set(game_path.get_string());
			map_location.append(L"\\maps\\");
			map_location.append(k_default_maps_to_add[i]);
			map_location.append(L".map");

			FILE* map_handle = NULL;
			errno_t error = _wfopen_s(&map_handle, map_location.get_string(), L"rb");
			if (!error && map_handle != NULL)
			{
				map_slots_get_multiplayer_level_data(map_handle, &g_added_multiplayer_level_data[i]);
				fclose(map_handle);
			}
			else
			{
				LOG_TRACE_GAME(L"[Map Slots]: Startup - Map File Missing {}", map_location.get_string());
			}
		}

		return;
	}

	void add_new_multiplayer_map_slots_game(void)
	{
		s_ui_levels_definition* ui_levels = game_globals_get_ui_levels();
		
		if (ui_levels)
		{
			int32 added_maps = 0;
			for (const s_multiplayer_ui_level_definition& newSlot : g_added_multiplayer_level_data)
			{
				if (added_maps + k_multiplayer_first_unused_slot < k_max_map_slots)
				{
					LOG_TRACE_FUNCW(L"Adding {}", newSlot.level_descriptions.name[_language_english]);
					s_multiplayer_ui_level_definition* slot = ui_levels->multiplayer_levels[added_maps + k_multiplayer_first_unused_slot];

					//Write the data loaded from the maps into the unused slot
					memcpy(slot, &newSlot, sizeof(newSlot));
					//Resolve the loaded bitmap datum
					slot->bitmap.index = tag_injection_resolve_cache_datum(newSlot.bitmap.index);

					//Change the map id and sort ID so that the maps are 
					//placed in order at the end of the list
					int32 new_map_id = k_starting_map_index + added_maps;
					slot->map_id = new_map_id;
					slot->sort_order = new_map_id;
					added_maps++;
				}
				else
				{
					LOG_TRACE_FUNC("Max Multiplayer added slots reached");
					break;
				}
			}
		}

		return;
	}

	int32 add_new_multiplayer_map_slots_server(void)
	{
		int32 added_map_count = 0;
		s_multiplayer_ui_level_definition* multiplayer_levels = Memory::GetAddress<s_multiplayer_ui_level_definition*>(0, 0x419510);
		for (const s_multiplayer_ui_level_definition& newSlot : g_added_multiplayer_level_data)
		{
			if (k_multiplayer_first_unused_slot + added_map_count < k_max_map_slots)
			{
				s_multiplayer_ui_level_definition* slot = &multiplayer_levels[k_multiplayer_first_unused_slot + added_map_count];
				LOG_TRACE_FUNCW("Adding {}", newSlot.level_descriptions.name[_language_english]);
				DWORD dwBack[2];
				VirtualProtect(slot, sizeof(s_multiplayer_ui_level_definition), PAGE_EXECUTE_READWRITE, &dwBack[0]);

				//Write the data loaded from the maps into the unused slot
				memcpy(slot, &newSlot, sizeof(s_multiplayer_ui_level_definition));

				//Change the map id and sort ID so that the maps are 
				//placed in order at the end of the list
				int32 new_map_id = k_starting_map_index + added_map_count;
				slot->map_id = new_map_id;
				slot->sort_order = new_map_id;
				VirtualProtect(slot, sizeof(s_multiplayer_ui_level_definition), dwBack[0], &dwBack[1]);
				added_map_count++;
			}
			else
			{
				LOG_TRACE_FUNC("Max Multiplayer added slots reached");
				break;
			}
		}

		return added_map_count;
	}
	
	//H2Server reads the level data from mainmenu.map
	/*typedef bool(__cdecl p_store_multiplayer_level_data)();
	p_store_multiplayer_level_data* c_store_multiplayer_level_data;*/

	typedef int32(__cdecl sub_map_slot_t)(int32 a1);
	sub_map_slot_t* p_sub_map_slot;

	int32* MapSlotCount;
	int32 __cdecl store_multiplayer_level_data(int32 a1)
	{
		*MapSlotCount += add_new_multiplayer_map_slots_server();

		return p_sub_map_slot(a1);
	}

	void map_slots_apply_dedi_hooks(void)
	{
		MapSlotCount = Memory::GetAddress<int32*>(0, 0x41950C);
		//c_store_multiplayer_level_data = Memory::GetAddress<p_store_multiplayer_level_data*>(0, 0x6A22);
		p_sub_map_slot = Memory::GetAddress<sub_map_slot_t*>(0, 0x3C8C3);
		//PatchCall(Memory::GetAddress(0, 0xBBAE), store_multiplayer_level_data);
		PatchCall(Memory::GetAddress(0, 0x6ACC), store_multiplayer_level_data);
		return;
	}

	void OnMapLoad(void)
	{
		// Load all the added maps bitmaps
		LOG_TRACE_GAME("[Map Slots]: OnMapLoad - Tag Loading Bitmaps");
		for (uint32 i = 0; i < k_default_map_to_add_count; ++i)
		{
			tag_injection_set_active_map(k_default_maps_to_add[i]);
			if (tag_injection_active_map_verified())
			{
				tag_injection_load(_tag_group_bitmap, g_added_multiplayer_level_data[i].bitmap.index, false);
				tag_injection_inject();
			}
			else
			{
				g_added_multiplayer_level_data[i].bitmap.index = NONE;
			}
		}

		add_new_multiplayer_map_slots_game();
		return;
	}

	void Initialize(void)
	{
		CacheMapData();

		if (Memory::IsDedicatedServer())
		{
			map_slots_apply_dedi_hooks();
		}

		return;
	}
}

static void map_slots_get_multiplayer_level_data(FILE* map_handle, s_multiplayer_ui_level_definition* multiplayer_level_data)
{
	ASSERT(map_handle);
	ASSERT(multiplayer_level_data);

	// Read cache header from map file
	s_cache_header cache_header;
	file_seek_and_read(map_handle, 0, sizeof(s_cache_header), 1, &cache_header);

	// Read tags header from map file
	cache_file_tags_header cache_file_tags_header;
	file_seek_and_read(map_handle, cache_header.tag_offset, sizeof(cache_file_tags_header), 1, &cache_file_tags_header);

	const uint32 active_map_instance_table_offset = cache_header.tag_offset + sizeof(s_tag_group_link) * cache_file_tags_header.tag_group_link_set_count + sizeof(cache_file_tags_header);

	// Read the scenario instance from map file
	cache_file_tag_instance scenario_instance;
	file_seek_and_read(map_handle, active_map_instance_table_offset, sizeof(cache_file_tag_instance), 1, &scenario_instance);

	// Scenario is the first tag in the tag data, so we can just go to the starting offset of tag data
	const uint32 tag_data_offset = cache_header.tag_offset + cache_header.data_offset;
	
	// Get the offset of the level data block
	const uint32 level_data_block_offset = tag_data_offset + offsetof(scenario, level_data);

	// Read tag block of level data
	s_tag_block tag_block;
	file_seek_and_read(map_handle, level_data_block_offset, sizeof(tag_block), 1, &tag_block);

	const uint32 level_data_offset = tag_data_offset + (tag_block.data - scenario_instance.data_offset);
	const uint32 multiplayer_level_data_block_offset = level_data_offset + offsetof(s_scenario_level_data, multiplayer);

	// Read tag block of multiplayer level data
	file_seek_and_read(map_handle, multiplayer_level_data_block_offset, sizeof(tag_block), 1, &tag_block);

	const uint32 multiplayer_level_data_offset = tag_data_offset + (tag_block.data - scenario_instance.data_offset);

	// Read the multiplayer level data
	file_seek_and_read(map_handle, multiplayer_level_data_offset, sizeof(s_multiplayer_ui_level_definition), 1, multiplayer_level_data);

	//Fix incase the maps level data is incorrectly setup
	if (strlen(multiplayer_level_data->path) == 0)
	{
		csstrnzcpy(multiplayer_level_data->path, cache_header.scenario_path, NUMBEROF(multiplayer_level_data->path));
	}

	return;
}
