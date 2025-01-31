#pragma once
#include "tag_files/tag_groups.h"
#include "xml/xml_definition_loader.h"

#define k_first_injected_datum 15268
#define k_max_tag_instance_count 65535

struct s_tag_injecting_table_entry
{
	bool is_initialized;
	bool is_injected;
	bool is_preloaded;
	datum cache_index;
	datum injected_index;
	tag_group type;
	c_xml_definition_loader* loaded_data;
	uint8* injected_data;
};

class c_tag_injection_table
{
private:
	s_tag_injecting_table_entry* m_table;
	uint16 m_entry_count;
	uint16 m_table_size;

	void resize_table();
public:
	c_tag_injection_table(void);
	~c_tag_injection_table(void);

	void clear();

	uint16 get_entry_count() const;

	s_tag_injecting_table_entry* init_entry(datum cache_index, e_tag_group type);

	s_tag_injecting_table_entry* get_entry(uint16 index) const;
	s_tag_injecting_table_entry* get_entry_by_cache_index(datum datum_index) const;
	s_tag_injecting_table_entry* get_entry_by_injected_index(datum datum_index) const;
	bool has_entry_by_cache_index(datum datum_index) const;
};