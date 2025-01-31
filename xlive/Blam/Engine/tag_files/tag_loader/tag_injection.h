#pragma once
#include "tag_files/tag_groups.h"

/* constants */

#define k_carto_shared_map L"carto_shared"
#define k_events_map k_carto_shared_map

/* prototypes */

bool tag_injection_check_map_exists(const wchar_t* map_name);
/**
 * \brief Sets the map that will be accessed for loading tag data
 * \param map_name the filename of the map file without .map
 */
void tag_injection_set_active_map(const wchar_t* map_name);
bool tag_injection_active_map_verified(void);

datum tag_injection_load(e_tag_group group, const char* tag_name, bool load_dependencies);
datum tag_injection_load(e_tag_group group, datum cache_datum, bool load_dependencies);

void tag_injection_inject(void);

bool tag_injection_is_injected(datum injected_index);

datum tag_injection_resolve_cache_datum(datum cache_datum);

void tag_injection_scenario_load_setup(uint32 allocation_size);

void* tag_injection_extend_block(void* block, uint32 entry_size, uint32 count);

void* tag_injection_reserve_cache_memory(uint32 size, uint32* out_data_offset);

void tag_injection_apply_hooks(void);

void tag_injection_initialize(void);

void tag_injection_deinitialize(void);
