#include "stdafx.h"
#include "tag_injection.h"

#include "tag_injection_manager.h"

/* globals */

c_tag_injecting_manager* g_manager = NULL;

/*	todo: figure out how to redo this, so it doesn't blow
	Currently the entire base tag table is just copied into a new buffer and then the pointer in engine
	is changed to it This just wastes memory.

	possible solution.. Scan the tag table for available empty tag instances and use those, they will exist.
*/
cache_file_tag_instance* g_tag_table = NULL;

/* public code */

bool tag_injection_check_map_exists(const wchar_t* map_name)
{
	if (!Memory::IsDedicatedServer())
	{
		return g_manager->find_map(map_name, nullptr);
	}
	else
	{
		// just in case
		return false;
	}
}

void tag_injection_set_active_map(const wchar_t* map_name)
{
	if (!Memory::IsDedicatedServer())
	{
		g_manager->set_active_map(map_name);
	}
}

bool tag_injection_active_map_verified()
{
	if (!Memory::IsDedicatedServer())
	{
		return g_manager->get_active_map_verified();
	}
	else
	{
		// Just in case
		return false;
	}
}


datum tag_injection_load(e_tag_group group, const char* tag_name, bool load_dependencies)
{
	return g_manager->load_tag(group, tag_name, load_dependencies);
}

datum tag_injection_load(e_tag_group group, datum cache_datum, bool load_dependencies)
{
	return g_manager->load_tag(group, cache_datum, load_dependencies);
}

void tag_injection_inject(void)
{
	g_manager->inject_tags();
	return;
}

bool tag_injection_is_injected(datum injected_index)
{
	if (injected_index == NONE)
		return false;
	return g_manager->get_table()->get_entry_by_injected_index(injected_index)->is_injected;
}

datum tag_injection_resolve_cache_datum(datum cache_datum)
{
	s_tag_injecting_table_entry* entry = g_manager->get_table()->get_entry_by_cache_index(cache_datum);
	if (!entry)
	{
		cache_file_tag_instance* inst = &g_tag_table[DATUM_INDEX_TO_ABSOLUTE_INDEX(cache_datum)];
		if (inst->data_offset != 0)
			return inst->tag_index;

		return NONE;
	}

	return entry->injected_index;
}

void tag_injection_scenario_load_setup(uint32 allocation_size)
{
	LOG_DEBUG_GAME("[tag_injection_scenario_load_setup]: Setting up for injection - Alloc Size: {:x}", allocation_size);

	g_manager->set_base_map_tag_data_size(allocation_size + 32);

	//Clear the table
	for (uint16 i = k_first_injected_datum; i < g_manager->get_entry_count(); i++)
	{
		g_tag_table[i] = cache_file_tag_instance{ _tag_group_none, NONE, 0, 0 };
	}

	g_manager->reset();

	uint32* tag_table_start = Memory::GetAddress<uint32*>(0x47CD50, 0x4A29B8);
	csmemset((BYTE*)g_tag_table, 0, 0x3BA40);

	if (*tag_table_start != NULL)
	{
		csmemcpy(g_tag_table, (BYTE*)*tag_table_start, 0x3BA40);
		*tag_table_start = (uint32)g_tag_table;
		s_cache_file_memory_globals* cache_file_memory_globals = cache_file_memory_globals_get();
		cache_file_memory_globals->tags_header->tag_instances = (cache_file_tag_instance*)g_tag_table;
	}
}

void* tag_injection_extend_block(void* block, uint32 entry_size, uint32 count)
{
	return g_manager->extend_tag_block(block, entry_size, count);
}

void* tag_injection_reserve_cache_memory(uint32 size, uint32* out_data_offset)
{
	return g_manager->reserve_space_in_cache_memory(size, out_data_offset);
}

void tag_injection_apply_hooks(void)
{
	return;
}

void tag_injection_initialize(void)
{
	g_tag_table = (cache_file_tag_instance*)malloc(sizeof(cache_file_tag_instance) * k_max_tag_instance_count);
	g_manager = new c_tag_injecting_manager();
	g_manager->set_instance_table(g_tag_table);
	g_manager->init_directories();
	tag_injection_apply_hooks();
	return;
}

void tag_injection_deinitialize(void)
{
	free(g_tag_table);
	delete g_manager;
	return;
}
