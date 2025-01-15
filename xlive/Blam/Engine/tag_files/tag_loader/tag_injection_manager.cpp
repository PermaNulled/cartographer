#include "stdafx.h"
#include "tag_injection_manager.h"

#include "bitmaps/bitmap_group.h"
#include "cache/pc_geometry_cache.h"
#include "cache/pc_texture_cache.h"
#include "creatures/creature_definitions.h"
#include "filesys/pc_file_system.h"
#include "geometry/geometry_block.h"
#include "models/render_model_definitions.h"
#include "physics/collision_model_definitions.h"
#include "physics/physics_model_definitions.h"
#include "render/weather_definitions.h"
#include "units/biped_definitions.h"
#include "units/vehicle_definitions.h"

extern bool g_force_cartographer_update;

/* constants */

#define k_relative_maps_directory L".\\maps\\"
#define k_relative_mods_directory L".\\mods\\"
#define k_relative_mods_maps_directory k_relative_mods_directory L"maps\\"
#define k_relative_mods_plugin_directory k_relative_mods_directory L"plugins\\"


c_tag_injecting_manager::c_tag_injecting_manager():
	m_agents{}, m_base_tag_data_size(0), m_injectable_used_size(0), m_instances(nullptr),
	m_active_map_verified(false), m_active_map_cache_header(),
	m_active_map_tags_header(), m_active_map_scenario_instance_offset(),
	m_active_map_instance_table_offset(0), m_active_map_file_handle(nullptr)
{
	this->m_agents_initialized.clear();
}

void c_tag_injecting_manager::init_directories()
{
	this->m_base_map_directory.set(k_relative_maps_directory);
	this->m_mods_map_directory.set(k_relative_mods_maps_directory);
	this->m_plugins_directory.set(k_relative_mods_plugin_directory);
}

void c_tag_injecting_manager::set_base_map_tag_data_size(const uint32 size)
{
	this->m_base_tag_data_size = size;
}

uint32 c_tag_injecting_manager::get_base_map_tag_data_size() const
{
	return this->m_base_tag_data_size;
}

void c_tag_injecting_manager::set_instance_table(cache_file_tag_instance* table)
{
	this->m_instances = table;
}

uint16 c_tag_injecting_manager::get_entry_count() const
{
	return this->m_table.get_entry_count();
}

c_tag_injection_table* c_tag_injecting_manager::get_table()
{
	return &this->m_table;
}

bool c_tag_injecting_manager::find_map(const wchar_t* map_name, c_static_wchar_string<MAX_PATH>* out_string) const
{
	c_static_wchar_string<MAX_PATH> test_path;
	test_path.set(this->m_base_map_directory.get_string());
	test_path.append(map_name);
	test_path.append(L".map");
	if (PathFileExists(test_path.get_string()))
	{
		if (out_string)
		{
			out_string->set(test_path.get_string());
		}
		return true;
	}
	else
	{
		// Test if map exists in mods folder
		test_path.set(this->m_mods_map_directory.get_string());
		test_path.append(map_name);
		test_path.append(L".map");
		if (PathFileExists(test_path.get_string()))
		{
			if (out_string)
			{
				out_string->set(test_path.get_string());
			}
			return true;
		}
		// Exit and create a popup if a map is missing
		else
		{
			const wchar_t format[] = L"[c_tag_injecting_manager::find_map] could not locate %s.map in any valid content location";
			wchar_t output_wide[NUMBEROF(format) + MAX_PATH];
			
			usnzprintf(output_wide, NUMBEROF(output_wide), format, map_name);
			LOG_ERROR_GAME(output_wide);
			g_force_cartographer_update = true;
			return false;
		}
	}
}

void c_tag_injecting_manager::set_active_map(const wchar_t* map_name)
{
	c_static_wchar_string<MAX_PATH> t_path;
	if (!this->find_map(map_name, &t_path))
		return;

	// if the requested map is already the active map of the loader no need to do any more work.
	if (t_path.is_equal(this->m_active_map.get_string()))
		return;

	this->m_active_map.set(t_path.get_string());

	this->m_active_map_verified = false;

	if (this->m_active_map_file_handle)
		fclose(this->m_active_map_file_handle);

	this->m_active_map_verified = true;
	
	// TODO: write out error
	this->m_active_map_file_handle = _wfsopen(this->m_active_map.get_string(), L"rb", SH_DENYNO);

	// Read cache header from map file
	file_seek_and_read(this->m_active_map_file_handle, 0, sizeof(s_cache_header), 1, &this->m_active_map_cache_header);

	// Read tags header from map file
	file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_offset, sizeof(cache_file_tags_header), 1, &this->m_active_map_tags_header);


	this->m_active_map_instance_table_offset = this->m_active_map_cache_header.tag_offset + sizeof(s_tag_group_link) * this->m_active_map_tags_header.tag_group_link_set_count + sizeof(cache_file_tags_header);

	// Read the scenario instance from map file
	cache_file_tag_instance temp_instance;
	file_seek_and_read(this->m_active_map_file_handle, m_active_map_instance_table_offset, sizeof(cache_file_tag_instance), 1, &temp_instance);

	this->m_active_map_scenario_instance_offset = temp_instance.data_offset;

}

bool c_tag_injecting_manager::get_active_map_verified() const
{
	return this->m_active_map_verified;
}

void c_tag_injecting_manager::reset()
{
	// Set map verified too false, to prevent things from being loaded after a commit
	this->m_active_map_verified = false;

	// close the handle to the active map
	if (this->m_active_map_file_handle)
		fclose(this->m_active_map_file_handle);

	// just for safety clear the stored data for active map
	this->m_active_map.clear();
	csmemset(&this->m_active_map_cache_header, 0, sizeof(s_cache_header));
	csmemset(&this->m_active_map_tags_header, 0, sizeof(cache_file_tags_header));
	this->m_active_map_scenario_instance_offset = 0;

	this->m_table.clear();

	this->m_injectable_used_size = 0;
}



cache_file_tag_instance c_tag_injecting_manager::get_tag_instance_from_cache(datum cache_datum) const
{
	cache_file_tag_instance temp_instance;
	uint32 instance_offset = this->m_active_map_instance_table_offset + sizeof(cache_file_tag_instance) * DATUM_INDEX_TO_ABSOLUTE_INDEX(cache_datum);
	file_seek_and_read(this->m_active_map_file_handle, instance_offset, sizeof(cache_file_tag_instance), 1, &temp_instance);
	return temp_instance;
}

tag_group c_tag_injecting_manager::get_tag_group_by_datum(datum cache_datum) const
{
	return this->get_tag_instance_from_cache(cache_datum).group_tag;
}

void c_tag_injecting_manager::load_raw_data_from_cache(datum injected_index) const
{
	// resource caches are disabled on H2Server
	if (Memory::IsDedicatedServer())
		return;

	// There is probably a struct here but can't identify anything
	HANDLE* g_cache_handle = Memory::GetAddress<HANDLE*>(0x4AE8A8, 0x4CF128);
	uint32* g_cache_handle_geometry_block_offset = Memory::GetAddress<DWORD*>(0x4AE8B0, 0x4CF130);
	uint32* g_cache_handle_geometry_block_size = Memory::GetAddress<DWORD*>(0x4AE8B4, 0x4CF134);

	// a little precaution to circumvent unexpected behaviour
	const uint32 previous_geometry_block_offset = *g_cache_handle_geometry_block_offset;
	const uint32 previous_geometry_block_size = *g_cache_handle_geometry_block_size;
	const HANDLE previous_cache_handle = *g_cache_handle;

	// Reset geometry block values to allow functions to reset them when called.
	*g_cache_handle_geometry_block_offset = 0x0;
	*g_cache_handle_geometry_block_size = 0x0;

	cache_file_tag_instance* tag_info = &this->m_instances[DATUM_INDEX_TO_ABSOLUTE_INDEX(injected_index)];
	uint8* tag_data = (uint8*)cache_get_tag_data() + this->m_instances[DATUM_INDEX_TO_ABSOLUTE_INDEX(injected_index)].data_offset;

	//fail safe
	if (DATUM_INDEX_TO_ABSOLUTE_INDEX(tag_info->tag_index) != DATUM_INDEX_TO_ABSOLUTE_INDEX(injected_index))
	{
		LOG_ERROR_GAME("[tag_loader] failed to resolve datum to correct instance, game will crash");
	}

#if TAG_INJECTION_DEBUG
	c_static_string260 str;
	this->get_name_by_tag_datum(tag_info->group_tag.group, this->m_table.get_entry_by_injected_index(injected_index)->cache_index, str.get_buffer());
	LOG_DEBUG_GAME("[c_tag_injecting_mananger::load_raw] loading {} index {:x}", str.get_string(), injected_index);
#endif

	//supposing full length
	HANDLE new_file_handle = CreateFile(this->m_active_map.get_string(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

	*g_cache_handle = new_file_handle;

	switch (tag_info->group_tag.group)
	{
		case 'mode':
		{
			render_model_definition* model_definition = (render_model_definition*)tag_data;
			if (model_definition->sections.count > 0)
			{
				int current_section_index = 0;
				do
				{
					render_model_section* model_section = model_definition->sections[current_section_index];

					geometry_block_resource* t_res = model_section->geometry_block_info.resources[0];
					pc_geometry_cache_preload_geometry(&model_section->geometry_block_info, (e_pc_geometry_cache_preload_flags)(_pc_geometry_cache_preload_flag_2 | _pc_geometry_cache_preload_blocking));

					++current_section_index;
				} while (current_section_index < model_definition->sections.count);
			}
			break;
		}

		case 'bitm':
		{
			int old_list_field = *Memory::GetAddress<DWORD*>(0xA49270 + 0x1FC);
			bitmap_group* bitmap_definition = (bitmap_group*)tag_data;

			for (int i = 0; i < bitmap_definition->bitmaps.count; i++)
			{
				bitmap_data* bitmap_item = bitmap_definition->bitmaps[i];

				*Memory::GetAddress<bitmap_data**>(0xA49270 + 0x1FC) = bitmap_item;

				pc_texture_cache_preload_bitmap(bitmap_item, 2, 0, nullptr);
				pc_texture_cache_preload_bitmap(bitmap_item, 1, 0, nullptr);
				pc_texture_cache_preload_bitmap(bitmap_item, 0, 0, nullptr);
			}
			*Memory::GetAddress<DWORD*>(0xA49270 + 0x1FC) = old_list_field;
			break;
		}
		case 'weat':
		{
			auto weather_tag = reinterpret_cast<c_weather_system*>(tag_data);
			for (auto i = 0; i < weather_tag->m_particle_system.count; i++)
			{
				pc_geometry_cache_preload_geometry(&weather_tag->m_particle_system[i]->m_geometry, (e_pc_geometry_cache_preload_flags)(_pc_geometry_cache_preload_flag_2 | _pc_geometry_cache_preload_blocking));
			}
			break;
	}
	default:
		break;
	}
	*g_cache_handle = previous_cache_handle;

	CloseHandle(new_file_handle);

	*g_cache_handle_geometry_block_offset = previous_geometry_block_offset;
	*g_cache_handle_geometry_block_size = previous_geometry_block_size;
}

void c_tag_injecting_manager::apply_definition_fixup(e_tag_group group, datum injected_index)
{
	switch(group)
	{
		case _tag_group_biped:
			{
				biped_definitions_fixup(injected_index);;
				break;
			}
		case _tag_group_creature:
			{
				creature_definitions_fixup(injected_index);
				break;
			}
		case _tag_group_vehicle:
			{
				vehicle_definitions_fixup(injected_index);
				break;
			}
		case _tag_group_collision_model:
			{
				collision_model_definitions_fixup(injected_index);
				break;
			}
		case _tag_group_physics_model:
			{
				physics_model_definitions_fixup(injected_index, false);
				break;
			}
		default:
			break;
	}
}

void c_tag_injecting_manager::initialize_shader_template(datum injected_datum)
{
	if (Memory::IsDedicatedServer())
		return;

	typedef bool(__cdecl t_init_shader_template)(int a1);
	auto p_init_shader_template = Memory::GetAddress<t_init_shader_template*>(0x2694E6);
	tag_iterator* stem_iterator = Memory::GetAddress<tag_iterator*>(0xA4AF10);

	stem_iterator->next_tag_index = injected_datum;
	p_init_shader_template(1);
}

datum c_tag_injecting_manager::get_tag_datum_by_name(e_tag_group group, const char* tag_name) const
{
	if(!this->m_active_map_verified)
	{
		LOG_ERROR_GAME("[c_tag_injecting_mananger::get_tag_datum_by_name] active map has not be set for tag: {}", tag_name);
		return NONE;
	}
	
	cache_file_tag_instance temp_instance;

	int32 current_offset = 0;
	int32 next_offset = 0;
	uint32 current_size = 0;
	char name_buffer[MAX_PATH] = {};

	for(int32 current_index = 0; current_index < this->m_active_map_cache_header.debug_tag_name_count; ++current_index)
	{
		if (current_index + 1 != this->m_active_map_cache_header.debug_tag_name_count)
		{
			// Get the offset of the current index
			file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_offsets_offset + sizeof(uint32) * current_index, sizeof(uint32), 1, &current_offset);

			// If the current offset is -1 it means we have reached the end of the index table
			if (current_offset == NONE)
				break;

			// Get the offset of the next index
			file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_offsets_offset + sizeof(uint32) * (current_index + 1), sizeof(uint32), 1, &next_offset);

			// Current size is calculated using the offsets of the two indexes
			// if next offset is none, the current offset is the end of the table and just read max path
			if (next_offset == NONE)
				current_size = MAX_PATH;
			else
				current_size = next_offset - current_offset;

			// Read the current debug name
			file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_buffer_offset + current_offset, current_size, 1, &name_buffer);

			if(csstricmp(tag_name, name_buffer) == 0)
			{
				file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_instance_table_offset + (current_index * sizeof(cache_file_tag_instance)), sizeof(cache_file_tag_instance), 1, &temp_instance);
				if (temp_instance.group_tag.group == group)
					return temp_instance.tag_index;
			}
		}
		else
		{
			// Get the offset of the current index
			file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_offsets_offset + sizeof(uint32) * current_index, sizeof(uint32), 1, &current_offset);

			// Current size is calculated using the total size of the buffer and the current offset;
			current_size = (this->m_active_map_cache_header.tag_name_offsets_offset + (this->m_active_map_cache_header.debug_tag_name_count * sizeof(uint32))) - current_offset;

			// Read the current debug name
			file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_buffer_offset + current_offset, current_size, 1, &name_buffer);

			if (csstricmp(tag_name, name_buffer) == 0)
			{
				file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_instance_table_offset + (current_index * sizeof(cache_file_tag_instance)), sizeof(cache_file_tag_instance), 1, &temp_instance);
				if (temp_instance.group_tag.group == group)
					return temp_instance.tag_index;
			}
		}
	}
	return NONE;
}

void c_tag_injecting_manager::get_name_by_tag_datum(e_tag_group group, datum cache_datum, char* out_name) const
{
	if (!this->m_active_map_verified)
	{
		LOG_ERROR_GAME("[c_tag_injecting_mananger::get_name_by_tag_datum] active map has not be set for tag: {:x}", cache_datum);
		out_name[0] = '\0';
		return;
	}

	uint16 absolute_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(cache_datum);

	cache_file_tag_instance temp_instance;
	file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_instance_table_offset + (absolute_index * sizeof(cache_file_tag_instance)), sizeof(cache_file_tag_instance), 1, &temp_instance);

	if(temp_instance.tag_index != cache_datum || temp_instance.group_tag.group != group)
	{
		out_name[0] = '\0';
		return;
	}

	int32 current_offset = 0;
	uint32 next_offset = 0;
	uint32 current_size = 0;

	if (absolute_index + 1 != this->m_active_map_cache_header.debug_tag_name_count)
	{
		// Get the offset of the cache index
		file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_offsets_offset + sizeof(uint32) * absolute_index, sizeof(uint32), 1, &current_offset);

		// If the current offset is -1 it means we have reached the end of the index table
		if (current_offset == NONE)
		{
			out_name[0] = '\0';
			return;
		}

		// Get the offset of the next index
		file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_offsets_offset + sizeof(uint32) * (absolute_index + 1), sizeof(uint32), 1, &next_offset);

		// Current size is calculated using the offsets of the two indexes
		// if next offset is none, the current offset is the end of the table and just read max path
		current_size = (next_offset == NONE ? MAX_PATH : next_offset - current_offset);

		// Read the current debug name
		file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_buffer_offset + current_offset, current_size, 1, out_name);
	}
	else
	{
		// Get the offset of the cache index
		file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_offsets_offset + sizeof(uint32) * absolute_index, sizeof(uint32), 1, &current_offset);

		// If the current offset is -1 it means we have reached the end of the index table
		if (current_offset == NONE)
		{
			out_name[0] = '\0';
			return;
		}

		// Current size is calculated using the total size of the buffer and the current offset;
		current_size = (this->m_active_map_cache_header.tag_name_offsets_offset + (this->m_active_map_cache_header.debug_tag_name_count * sizeof(uint32))) - current_offset;

		// Read the current debug name
		file_seek_and_read(this->m_active_map_file_handle, this->m_active_map_cache_header.tag_name_buffer_offset + current_offset, current_size, 1, out_name);
	}

	return;
}

bool c_tag_injecting_manager::initialize_agent(tag_group group)
{
	uint32 tag_group_index = tag_group_get_as_index(group);

	if(tag_group_index == NONE)
	{
		return false;
	}

	// check static flags if agent is already initialized
	if (this->m_agents_initialized.test(tag_group_index))
		return true;

	// non-terminated sting correction
	// flip string and terminate
	char tag_class[5];
	tag_class[0] = group.string[3];
	tag_class[1] = group.string[2];
	tag_class[2] = group.string[1];
	tag_class[3] = group.string[0];
	tag_class[4] = '\0';

	tag_class[3] = (tag_class[3] == ' ' ? '\0' : tag_class[3]);

	wchar_t wide_tag_class[5];
	utf8_string_to_wchar_string(tag_class, wide_tag_class, NUMBEROF(wide_tag_class));

	c_static_wchar_string<MAX_PATH> plugin_path;
	plugin_path.set(this->m_plugins_directory.get_string());
	plugin_path.append(wide_tag_class);
	plugin_path.append(L".xml");

	// Exit and create a popup if a plugin is missing
	if (!PathFileExists(plugin_path.get_string()))
	{
		const wchar_t format[] = L"[c_tag_injecting_manager::initialize_agent] Plugin file could not be located %s";
		wchar_t output_wide[NUMBEROF(format) + MAX_PATH];

		usnzprintf(output_wide, NUMBEROF(output_wide), format, plugin_path.get_string());
		LOG_ERROR_GAME(output_wide);
		g_force_cartographer_update = true;
		return false;
	}
	this->m_agents[tag_group_index].init(group, plugin_path.get_string());
	this->m_agents_initialized.set(tag_group_index, true);
	return true;
}

c_xml_definition_agent* c_tag_injecting_manager::get_agent(tag_group group)
{
	if(!this->initialize_agent(group))
	{
		// non-terminated sting correction
		// flip string and terminate
		char null_terminated_class[5];
		null_terminated_class[0] = group.string[3];
		null_terminated_class[1] = group.string[2];
		null_terminated_class[2] = group.string[1];
		null_terminated_class[3] = group.string[0];
		null_terminated_class[4] = '\0';

		if (null_terminated_class[3] == ' ')
			null_terminated_class[3] = '\0';

		LOG_ERROR_GAME("[c_tag_injecting_manager::get_agent] failed to initialize agent for {}", null_terminated_class);
		return nullptr;
	}
	return &this->m_agents[tag_group_get_as_index(group)];
}

datum c_tag_injecting_manager::load_tag(e_tag_group group, const char* tag_name, bool load_dependencies)
{
	datum cache_datum = this->get_tag_datum_by_name(group, tag_name);
	if (cache_datum != NONE)
	{
#if TAG_INJECTION_DEBUG
		LOG_DEBUG_GAME("[c_tag_injecting_mananger::load_tag] loading {} with depencies {} datum {}", tag_name, load_dependencies, cache_datum);
#endif
		return this->load_tag(group, cache_datum, load_dependencies);
	}
	return NONE;
}

datum c_tag_injecting_manager::load_tag(e_tag_group group, datum cache_datum, bool load_dependencies)
{
	if (this->m_table.has_entry_by_cache_index(cache_datum))
		return NONE;

	s_tag_injecting_table_entry* new_entry = this->m_table.init_entry(cache_datum, group);

	c_xml_definition_agent* agent = this->get_agent({group});
	
	datum result;
	if (agent)
	{
		new_entry->loaded_data->init(
			agent->get_definition(),
			this->m_active_map_file_handle,
			&this->m_active_map_cache_header,
			&this->m_active_map_tags_header,
			this->m_active_map_scenario_instance_offset,
			cache_datum);

		if (load_dependencies)
		{
			c_tag_injecting_manager::load_dependencies(this, new_entry);
		}
		result = new_entry->injected_index;
	}
	// Force update if agent cannot be retrieved
	else
	{
		g_force_cartographer_update = true;
		result = NONE;
	}

	return result;
}

void c_tag_injecting_manager::load_tag_internal(c_tag_injecting_manager* manager, tag_group group, datum cache_datum,
                                                bool load_dependencies)
{
	cache_file_tag_instance inst = manager->get_tag_instance_from_cache(cache_datum);
	if (inst.tag_index != cache_datum || inst.group_tag.group != group.group)
		return;

	if (inst.size == 0 || inst.data_offset == 0)
		return;

	if (manager->m_table.has_entry_by_cache_index(cache_datum))
		return;

#if TAG_INJECTION_DEBUG
	c_static_string260 name;
	manager->get_name_by_tag_datum(group.group, cache_datum, name.get_buffer());

	char tag_class[5];
	tag_class[0] = group.string[3];
	tag_class[1] = group.string[2];
	tag_class[2] = group.string[1];
	tag_class[3] = group.string[0];
	tag_class[4] = '\0';

	LOG_DEBUG_GAME("[c_tag_injection_manager::load_tag] loading dependency {} {}", name.get_string(), tag_class);
#endif

	s_tag_injecting_table_entry* new_entry = manager->m_table.init_entry(cache_datum, group.group);
	c_xml_definition_agent* agent = manager->get_agent(group);
	ASSERT(agent);
	new_entry->loaded_data->init(
		agent->get_definition(),
		manager->m_active_map_file_handle,
		&manager->m_active_map_cache_header,
		&manager->m_active_map_tags_header,
		manager->m_active_map_scenario_instance_offset,
		cache_datum);

	if (load_dependencies)
	{
		c_tag_injecting_manager::load_dependencies(manager, new_entry);
	}
}

void c_tag_injecting_manager::load_dependencies(c_tag_injecting_manager* manager, const s_tag_injecting_table_entry* new_entry)
{
	for (uint32 i = 0; i < new_entry->loaded_data->get_tag_reference_count(); i++)
	{
		datum tag_index = new_entry->loaded_data->get_tag_reference(i);
		tag_group t_group = manager->get_tag_group_by_datum(tag_index);
		if (t_group.group != _tag_group_sound)
		{
			load_tag_internal(manager, t_group, tag_index, true);
		}
	}
	return;
}

void c_tag_injecting_manager::inject_tags()
{
#if TAG_INJECTION_DEBUG
	for (uint16 i = 0; i < this->m_table.get_entry_count(); i++)
	{
		s_tag_injecting_table_entry* entry = this->m_table.get_entry(i);
		char tag_class[5];
		tag_class[0] = entry->type.string[3];
		tag_class[1] = entry->type.string[2];
		tag_class[2] = entry->type.string[1];
		tag_class[3] = entry->type.string[0];
		tag_class[4] = '\0';
		c_static_string260 tag_name;
		this->get_name_by_tag_datum(entry->type.group, entry->cache_index, tag_name.get_buffer());
		LOG_DEBUG_GAME("[table_dump]: cache_index: {:x} injected_index: {:x} type: {} tag_name: {}", entry->cache_index, entry->injected_index, tag_class, tag_name.get_string());
	}
#endif
	for(uint16 i = 0; i < this->m_table.get_entry_count(); i++)
	{
		s_tag_injecting_table_entry* entry = this->m_table.get_entry(i);

		if(entry->is_injected)
			continue;

		entry->is_injected = true;

		uint32 injection_offset = this->m_base_tag_data_size + this->m_injectable_used_size;

#if TAG_INJECTION_DEBUG
		const uint32 start = (uint32)cache_get_tag_data();
		const uint32 end = start + this->get_base_map_tag_data_size() + k_injectable_allocation_size;
		bool in_range = ((uint32)cache_get_tag_data() + injection_offset) >= start && ((uint32)cache_get_tag_data() + injection_offset) < end;

		LOG_DEBUG_GAME("[c_tag_injecting_manager::inject_tags] injection_offset: {:x} is valid: {} start: {:x} end: {:x}",
			(uint32)cache_get_tag_data() + injection_offset, in_range, start, end);
#endif


		cache_file_tag_instance* injection_instance = &this->m_instances[DATUM_INDEX_TO_ABSOLUTE_INDEX(entry->injected_index)];

		injection_instance->group_tag = entry->type;
		injection_instance->data_offset = injection_offset;
		injection_instance->size = entry->loaded_data->get_total_size();
		injection_instance->tag_index = entry->injected_index;

#if TAG_INJECTION_DEBUG
		char tag_class[5];
		tag_class[0] = entry->type.string[3];
		tag_class[1] = entry->type.string[2];
		tag_class[2] = entry->type.string[1];
		tag_class[3] = entry->type.string[0];
		tag_class[4] = '\0';

		c_static_string260 tag_name;
		this->get_name_by_tag_datum(entry->type.group, entry->cache_index, tag_name.get_buffer());

		LOG_DEBUG_GAME("[c_tag_injecting_manager::inject_tags] type: {} injection_offset: {:x} data_size: {:x} tag_name: {} datum: {:x}", tag_class, injection_offset, injection_instance->size, tag_name.get_string(), entry->injected_index);
#endif

		entry->loaded_data->copy_tag_data((int8*)(cache_get_tag_data() + injection_offset), injection_offset);

		if(entry->type.group == _tag_group_bitmap || entry->type.group == _tag_group_render_model || entry->type.group == _tag_group_weather_system)
			this->load_raw_data_from_cache(entry->injected_index);

		this->apply_definition_fixup(entry->type.group, entry->injected_index);

		if (entry->type.group == _tag_group_shader_template)
			this->initialize_shader_template(entry->injected_index);

		this->m_injectable_used_size += entry->loaded_data->get_total_size();
	}
#if TAG_INJECTION_DEBUG
	LOG_DEBUG_GAME("[c_tag_injecting_manager::inject_tags] Injection Complete");
#endif
}

void* c_tag_injecting_manager::extend_tag_block(void* block, uint32 entry_size, uint32 count)
{
	s_tag_block* basic_block = (s_tag_block*)block;

	uint32 base_block_total_size = entry_size * basic_block->count;
	uint32 new_block_total_size = base_block_total_size + (entry_size * count);

	int8* base_block_location = (int8*)(cache_get_tag_data() + basic_block->data);

	uint32 injection_offset = this->m_base_tag_data_size + this->m_injectable_used_size;
	int8* injection_location = (int8*)(cache_get_tag_data() + injection_offset);

	csmemcpy(injection_location, base_block_location, base_block_total_size);

	basic_block->data = injection_offset;
	basic_block->count += count;

	this->m_injectable_used_size += new_block_total_size;

	// return the location in memory where the first newly added block exists
	return (void*)(cache_get_tag_data() + injection_offset + base_block_total_size);
}

void* c_tag_injecting_manager::reserve_space_in_cache_memory(uint32 size, uint32* out_data_offset)
{
	const uint32 reserved_offset = this->m_base_tag_data_size + this->m_injectable_used_size;

	this->m_injectable_used_size += size;

	*out_data_offset = reserved_offset;

	return (void*)(cache_get_tag_data() + reserved_offset);
}
