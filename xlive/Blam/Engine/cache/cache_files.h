#pragma once
#include "game/game_preferences.h"

#include "Blam/Cache/DataTypes/BlamTag.h"

#define FIRST_SHARED_TAG_INSTANCE_INDEX 10000
#define MAXIMUM_SIMULTANEOUS_TAG_INSTANCES_TOTAL 20000

struct cache_file_tag_instance
{
	blam_tag tag_group;
	int32 tag_index;
	uint32 data_offset;
	uint32 size;
};
CHECK_STRUCT_SIZE(cache_file_tag_instance, 16);

// Stores information about currently loaded tags
struct s_tags_header
{
	void* parent_info;
	int32 tag_parent_info_count;
	cache_file_tag_instance* tag_instances;
	datum scenario_index;
	datum globals_index;
	int field_14;
	int tag_count;
	int type;
};
CHECK_STRUCT_SIZE(s_tags_header, 32);

struct s_cache_header
{
	int32 header_signature;
	int32 version;
	int32 file_size;
	int32 field_C;
	int32 tag_offset;
	int32 data_offset;
	int32 data_size;
	int32 tag_size;
	int32 tag_offset_mask;
	int32 shared_tag_dependency_offset;
	int32 shared_tag_dependency_count;
	int8 some_path[256];
	char version_string[32];
	int16 type;	// e_scenario_type
	int16 pad;
	int32 crc;
	bool crc_valid;
	int8 pad_1[3];
	bool field_158;
	bool tracked__maybe;
	bool field_15A;
	bool field_15B;
	int32 field_15C;
	int32 field_160;
	int32 field_164;
	int32 field_168;
	int32 string_block_offset;
	int32 string_table_count;
	int32 string_table_size;
	int32 string_idx_offset;
	int32 string_table_offset;
	int8 uses_shared_map[3];
	int8 pad_2;
	FILETIME time;
	FILETIME main_menu_time;
	FILETIME shared_time;
	FILETIME campaign_time;
	char name[32];
	e_language language;
	char scenario_path[256];
	int32 minor_version;
	int32 tag_name_count;
	int32 tag_name_buffer_offset;
	int32 tag_name_buffer_size;
	int32 tag_name_offsets_offset;
	int32 language_pack_offset;
	int32 language_pack_size;
	datum secondary_ugh_tag_index;
	int32 geometry_data_offset;
	int32 geometry_data_size;
	int32 checksum;
	int32 mopp_code_checksum;
	int8 field_2F8[1284];
	int32 footer_signature;
};
CHECK_STRUCT_SIZE(s_cache_header, 0x800);

struct tag_iterator
{
	int32 field_0;
	int32 field_4;
	datum current_tag_index;
	datum next_tag_index;
	blam_tag tag_type;
};
CHECK_STRUCT_SIZE(tag_iterator, 20);

s_cache_header* cache_files_get_header(void);

s_tags_header* cache_files_get_tags_header(void);

cache_file_tag_instance* global_tag_instances_get(void);

tag_iterator* tag_iterator_new(tag_iterator* itr, blam_tag::tag_group_type type);

datum __cdecl tag_iterator_next(tag_iterator* itr);

void cache_file_map_clear_all_failures(void);

// Get tag data from tag index
void* __cdecl tag_get_fast(datum tag_index);