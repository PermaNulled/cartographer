#include "stdafx.h"
#include "data.h"

#include "Blam/Cache/TagGroups/object_definition.hpp"

bool next_index_range_checks(const s_data_array* data, datum* new_next_index)
{
	if (data->next_index >= data->max_elements) { return true; }

	*new_next_index = data->next_index;
	
	if (data->next_index == -1) { return true; }

	return false;
}

// This function sucks
datum __cdecl datum_new(s_data_array* data)
{
	bool b_found = false;
	datum next_idx;
	datum last_free_index = data->last_free_index;

	if (last_free_index >= data->next_index)
	{
		b_found = true;
	}

	if (!b_found)
	{
		while (((1 << (last_free_index & 0x1F)) & data->active_bit_mask.m_flags[last_free_index >> 5]) != 0)
		{
			if (++last_free_index >= data->next_index)
			{
				b_found = true;
				break;
			}
		}

		if (!b_found)
		{
			next_idx = last_free_index;
		}
	}
	
	if (b_found)
	{
		if (next_index_range_checks(data, &next_idx)) { return DATUM_INDEX_NONE; }
	}
	else
	{
		if (last_free_index == -1)
		{
			if (next_index_range_checks(data, &next_idx)) { return DATUM_INDEX_NONE; }
		}
	}
	
	
	s_object_header* object_header = (s_object_header*)&data->data[next_idx * data->single_element_size];
	data->active_bit_mask.m_flags[next_idx >> 5] |= 1 << (next_idx & 0x1F);
	++data->total_elements_used;
	data->last_free_index = next_idx + 1;
	if (data->next_index <= next_idx) 
	{ 
		data->next_index = next_idx + 1; 
	}
		
	memset(object_header, 0, data->single_element_size);
	object_header->datum_salt = data->next_datum++;
	
	if (data->next_datum == DATUM_INDEX_NONE) 
	{ 
		data->next_datum = 0x8000; 
	}

	return next_idx | (object_header->datum_salt << 16);
}

void __cdecl datum_delete(s_data_array* data_array, datum index)
{
	s_object_header* object;

	object = (s_object_header*)&data_array->data[DATUM_INDEX_TO_ABSOLUTE_INDEX(index) * data_array->single_element_size];
	if ((data_array->flags & 8) != 0) { memset(object, 186, data_array->single_element_size); }

	data_array->active_bit_mask.m_flags[DATUM_INDEX_TO_ABSOLUTE_INDEX(index) >> 5] &= ~(1 << (index & 0x1F));
	object->datum_salt = 0;

	if (DATUM_INDEX_TO_ABSOLUTE_INDEX(index) < data_array->last_free_index)
		data_array->last_free_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(index);
	if (DATUM_INDEX_TO_ABSOLUTE_INDEX(index) + 1 == data_array->next_index)
	{
		do
		{
			--data_array->next_index;
			object = (s_object_header*)((char*)object - data_array->single_element_size);
		} while (data_array->next_index > 0 && !object->datum_salt);
		--data_array->total_elements_used;
	}
	else
	{
		--data_array->total_elements_used;
	}
}