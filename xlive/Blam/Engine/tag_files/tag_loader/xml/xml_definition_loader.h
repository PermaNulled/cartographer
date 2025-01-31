#pragma once
#include "xml_definition_block.h"

#include "cache/cache_files.h"
#include "tag_files/tag_loader/tag_injection_define.h"

struct s_offset_link
{
	uint32 cache_offset;
	uint32 memory_offset;
#if TAG_INJECTION_DEBUG
	char name[64];
#endif
};

struct s_memory_link : s_offset_link
{
	int8* data;
	uint32 size;
};

class c_xml_definition_loader
{
public:
	c_xml_definition_loader(void) = default;
	~c_xml_definition_loader(void) = default;
	void init(
		c_xml_definition_block* definition,
		FILE* file_handle,
		s_cache_header* cache_header,
		cache_file_tags_header* tags_header,
		uint32 scenario_instance_offset,
		datum cache_index);
	
	void clear(void);

	uint32 get_total_size(void) const;
	int8* get_data(void) const;

	uint32 get_tag_reference_count(void) const;
	datum get_tag_reference(uint32 index) const;
	void copy_tag_data(int8* out_buffer, uint32 base_offset);

#if TAG_INJECTION_DEBUG
	void validate_data() const;
#endif
private:
	int8* m_data;
	uint32 m_used_data;
	uint32 m_total_data_size;
	c_xml_definition_block* m_definition;

	FILE* m_file_handle;

	s_cache_header* m_cache_header;
	cache_file_tags_header* m_tags_header;
	uint32 m_scenario_instance_offset;

	datum m_cache_index;

	uint32 m_file_offset;	// offset in file to tag data from 0
	cache_file_tag_instance m_instance;		// tag instance loaded from cache


	s_offset_link* m_tag_reference_offsets;
	s_offset_link* m_classless_tag_reference_offsets;
	s_memory_link* m_data_reference_offsets;
	s_offset_link* m_string_id_offsets;
	s_memory_link* m_tag_block_offsets;
	datum* m_tag_references;

	uint32 m_tag_reference_offset_count;
	uint32 m_classless_tag_reference_offset_count;
	uint32 m_data_reference_offset_count;
	uint32 m_string_id_offset_count;
	uint32 m_tag_block_offset_count;
	uint32 m_tag_reference_count;

	void load_cache_info();
	uint32 resolve_cache_tag_data_offset(uint32 offset) const;
	void reset_counts(void);

	static void initialize_arrays_internal(c_xml_definition_loader* loader, const c_xml_definition_block* definition, uint32 file_offset, uint32 block_count);
	void initialize_arrays(void);
	int8* reserve_data(uint32 size);
	static void load_tag_data_internal(c_xml_definition_loader* loader, const c_xml_definition_block* definition, uint32 file_offset, int8* buffer, uint32 block_count);
	void load_tag_data();
	void calculate_total_data_size(const c_xml_definition_block* definition, uint32 base_offset, uint32 block_size);
};

