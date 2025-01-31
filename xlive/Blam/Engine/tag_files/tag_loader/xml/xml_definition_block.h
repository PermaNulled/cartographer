#pragma once
#include "tinyxml/tinyxml2.h"

#include "cseries/cseries_strings.h"
#include "tag_files/tag_loader/tag_injection_define.h"


class c_xml_definition_block
{
	tinyxml2::XMLElement* m_element;
	uint32 m_offset;
	uint32 m_size;
	c_static_string<64> m_name;


	uint32 m_tag_reference_count;
	uint32* m_tag_references;

	uint32 m_classless_tag_reference_count;
	uint32* m_classless_tag_references;

	uint32 m_data_reference_count;
	uint32* m_data_references;

	uint32 m_string_id_count;
	uint32* m_string_ids;

	uint32 m_tag_block_count;
	c_xml_definition_block* m_tag_blocks;

#if TAG_INJECTION_DEBUG
	c_static_string<64>* m_tag_reference_names;
	c_static_string<64>* m_classless_tag_reference_names;
	c_static_string<64>* m_data_reference_names;
	c_static_string<64>* m_tag_block_names;
#endif


public:
	c_xml_definition_block(void) = default;
	c_xml_definition_block(tinyxml2::XMLElement* base_element, uint32 offset, uint32 size);
	~c_xml_definition_block(void) = default;

	void reset_counts();
	void get_element_counts();
	void allocate_buffers();
	void populate_buffers();
	void clear();

	uint32 get_size() const;
	uint32 get_offset() const;
	const char* get_name() const;

	uint32 get_tag_references_count() const;
	uint32 get_tag_reference_offset(uint32 index) const;

	uint32 get_classless_tag_references_count() const;
	uint32 get_classless_tag_reference_offset(uint32 index) const;

	uint32 get_data_references_count() const;
	uint32 get_data_reference_offset(uint32 index) const;

	uint32 get_string_id_count() const;
	uint32 get_string_id_offset(uint32 index) const;

	uint32 get_tag_block_count() const;
	c_xml_definition_block* get_tag_block(uint32 index) const;

#if TAG_INJECTION_DEBUG
	c_static_string<64>* get_tag_reference_name(uint32 index) const;
	c_static_string<64>* get_classless_tag_reference_name(uint32 index) const;
	c_static_string<64>* get_data_reference_name(uint32 index) const;
	c_static_string<64>* get_tag_block_name(uint32 index) const;
#endif

};
