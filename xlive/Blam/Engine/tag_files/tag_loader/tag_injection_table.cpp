#include "stdafx.h"
#include "tag_injection_table.h"

enum
{
	k_injection_table_entry_count_per_resize = 100
};

c_tag_injection_table::c_tag_injection_table(void)
{
	this->m_entry_count = 0;
	this->m_table_size = 0;
	this->m_table = nullptr;
	this->resize_table();
	return;
}

c_tag_injection_table::~c_tag_injection_table(void)
{
	for(uint16 i = 0; i < this->m_entry_count; ++i)
	{
		ASSERT(this->m_table[i].loaded_data);
		this->m_table[i].loaded_data->clear();
		delete this->m_table[i].loaded_data;
	}

	ASSERT(this->m_table);
	free(this->m_table);

	return;
}

void c_tag_injection_table::clear()
{
	for (uint16 i = 0; i < this->m_entry_count; ++i)
	{
		ASSERT(this->m_table[i].loaded_data);
		this->m_table[i].loaded_data->clear();
		delete this->m_table[i].loaded_data;
	}

	ASSERT(this->m_table);
	free(this->m_table);

	this->m_entry_count = 0;
	this->m_table_size = 0;
	this->m_table = nullptr;
	this->resize_table();
}

uint16 c_tag_injection_table::get_entry_count() const
{
	return this->m_entry_count;
}

s_tag_injecting_table_entry* c_tag_injection_table::init_entry(datum cache_index, e_tag_group type)
{
	if (this->m_entry_count >= this->m_table_size)
	{
		this->resize_table();
	}

	s_tag_injecting_table_entry* result = &this->m_table[this->m_entry_count];
	result->cache_index = cache_index;
	result->injected_index = k_first_injected_datum + this->m_entry_count;
	result->type = { type };
	result->is_initialized = true;
	result->is_injected = false;
	result->loaded_data = new c_xml_definition_loader();
	this->m_entry_count++;

	return result;
}

s_tag_injecting_table_entry* c_tag_injection_table::get_entry(uint16 index) const
{
	return &this->m_table[index];
}

s_tag_injecting_table_entry* c_tag_injection_table::get_entry_by_cache_index(datum datum_index) const
{
	for (uint16 i = 0; i < this->m_entry_count; i++)
	{
		if (this->m_table[i].cache_index == datum_index)
			return &this->m_table[i];
	}
	return nullptr;
}

s_tag_injecting_table_entry* c_tag_injection_table::get_entry_by_injected_index(datum datum_index) const
{
	return this->get_entry(DATUM_INDEX_TO_ABSOLUTE_INDEX(datum_index) - k_first_injected_datum);
}

bool c_tag_injection_table::has_entry_by_cache_index(datum datum_index) const
{
	for (uint16 i = 0; i < this->m_entry_count; i++)
	{
		if (this->m_table[i].cache_index == datum_index)
			return &this->m_table[i].is_initialized;
	}
	return false;
}

void c_tag_injection_table::resize_table()
{
	// Allocate new larger buffer
	s_tag_injecting_table_entry* new_buffer = (s_tag_injecting_table_entry*)calloc(
		this->m_table_size + k_injection_table_entry_count_per_resize,
		sizeof(s_tag_injecting_table_entry));

	if (this->m_table)
	{
		// Copy old buffer into the new one
		csmemcpy(new_buffer, this->m_table, sizeof(s_tag_injecting_table_entry) * this->m_table_size);

		// Free the old buffer
		free(this->m_table);
	}
	// Increase the size of the table
	this->m_table_size += k_injection_table_entry_count_per_resize;
	// Place the new buffer
	this->m_table = new_buffer;
	return;
}
