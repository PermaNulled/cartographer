#include "stdafx.h"
#include "xml_definition_loader.h"

#include "xml_definition_block.h"

#include "cache/cache_files.h"
#include "filesys/pc_file_system.h"
#include "tag_files/data_reference.h"
#include "tag_files/string_id.h"
#include "tag_files/tag_block.h"
#include "tag_files/tag_reference.h"
#include "tag_files/tag_loader/tag_injection.h"
#include "tag_files/tag_loader/tag_injection_define.h"

#define lazy_malloc_buffer(TYPE, COUNT)\
	(TYPE*)malloc(sizeof(TYPE) * (COUNT))

void c_xml_definition_loader::init(c_xml_definition_block* definition, FILE* file_handle, 
	s_cache_header* cache_header, cache_file_tags_header* tags_header, uint32 scenario_instance_offset, datum cache_index)
{
	this->m_tag_reference_offsets = nullptr;
	this->m_classless_tag_reference_offsets = nullptr;
	this->m_data_reference_offsets = nullptr;
	this->m_string_id_offsets = nullptr;
	this->m_tag_block_offsets = nullptr;
	this->m_data = nullptr;

	this->m_used_data = 0;
	this->m_definition = definition;
	this->m_file_handle = file_handle;
	this->m_cache_header = cache_header;
	this->m_tags_header = tags_header;
	this->m_scenario_instance_offset = scenario_instance_offset;

	this->m_cache_index = cache_index;
	this->m_total_data_size = this->m_definition->get_size();

	this->load_cache_info();

	this->reset_counts();
	// set the initial size of the data_loader
	this->calculate_total_data_size(this->m_definition, this->m_file_offset, 1);

	// seen some tags that have an instance size greater than the computed size? failsafe?
	if (this->m_instance.size > this->m_total_data_size)
	{
#if TAG_INJECTION_DEBUG
		LOG_DEBUG_GAME("[c_xml_definition_loader::init] computed size: {} instance size: {}", this->m_total_data_size, this->m_instance.size);
#endif
		this->m_total_data_size = this->m_instance.size;
	}

	this->initialize_arrays();
	this->load_tag_data();

#if TAG_INJECTION_DEBUG
	LOG_INFO_GAME("[c_xml_definition_loader::init] {:x} - {:x}", this->m_instance.data_offset, this->m_instance.data_offset + this->m_instance.size);
#endif
}

void c_xml_definition_loader::load_cache_info()
{
	uint32 instance_table_offset = this->m_cache_header->tag_offset + sizeof(s_tag_group_link) * this->m_tags_header->tag_group_link_set_count + sizeof(cache_file_tags_header);
	uint32 tag_data_start_offset = this->m_cache_header->tag_offset + this->m_cache_header->data_offset;

	uint32 tag_instance_offset = instance_table_offset + sizeof(cache_file_tag_instance) * DATUM_INDEX_TO_ABSOLUTE_INDEX(this->m_cache_index);

	cache_file_tag_instance instance;
	// read requested tag instance from file
	file_seek_and_read(this->m_file_handle, tag_instance_offset, sizeof(cache_file_tag_instance), 1, &instance);

	this->m_instance = instance;
	this->m_file_offset = tag_data_start_offset + this->m_instance.data_offset - this->m_scenario_instance_offset;
}

uint32 c_xml_definition_loader::resolve_cache_tag_data_offset(uint32 offset) const
{
	return (this->m_cache_header->tag_offset + this->m_cache_header->data_offset) + (offset - this->m_scenario_instance_offset);
}

void c_xml_definition_loader::reset_counts(void)
{
	this->m_tag_reference_offset_count = 0;
	this->m_classless_tag_reference_offset_count = 0;
	this->m_data_reference_offset_count = 0;
	this->m_string_id_offset_count = 0;
	this->m_tag_block_offset_count = 0;
	this->m_tag_reference_count = 0;
	return;
}

void c_xml_definition_loader::clear(void)
{
	if (this->m_tag_reference_offset_count)
	{
		free(this->m_tag_reference_offsets);
	}
	if (this->m_classless_tag_reference_offset_count)
	{
		free(this->m_classless_tag_reference_offsets);
	}
	if (this->m_data_reference_offset_count)
	{
		free(this->m_data_reference_offsets);
	}
	if (this->m_string_id_offset_count)
	{
		free(this->m_string_id_offsets);
	}
	if (this->m_tag_block_offset_count)
	{
		free(this->m_tag_block_offsets);
	}
	if (this->m_tag_reference_count)
	{
		free(this->m_tag_references);
	}
	if (this->m_data)
	{
		free(this->m_data);
	}
	this->reset_counts();
	return;
}

tag_reference t_tag_reference;
datum t_classless_tag_reference;
data_reference t_data_reference;
string_id t_string_id;
tag_block<> t_tag_block;

void c_xml_definition_loader::initialize_arrays_internal(c_xml_definition_loader* loader, const c_xml_definition_block* definition, uint32 file_offset, uint32 block_count)
{
	for (uint32 block_index = 0; block_index < block_count; block_index++)
	{
		uint32 base_offset = file_offset + definition->get_size() * block_index;
		for (uint32 i = 0; i < definition->get_tag_references_count(); i++)
		{
			file_seek_and_read(loader->m_file_handle, base_offset + definition->get_tag_reference_offset(i), sizeof(tag_reference), 1, &t_tag_reference);
			if (t_tag_reference.group.group != _tag_group_none && t_tag_reference.index != NONE)
				loader->m_tag_reference_offset_count++;
		}

		for (uint32 i = 0; i < definition->get_classless_tag_references_count(); i++)
		{
			file_seek_and_read(loader->m_file_handle, base_offset + definition->get_classless_tag_reference_offset(i), sizeof(datum), 1, &t_classless_tag_reference);
			if (t_classless_tag_reference != NONE)
				loader->m_classless_tag_reference_offset_count++;
		}

		for (uint32 i = 0; i < definition->get_data_references_count(); i++)
		{
			file_seek_and_read(loader->m_file_handle, base_offset + definition->get_data_reference_offset(i), sizeof(data_reference), 1, &t_data_reference);
			if (t_data_reference.size != 0)
				loader->m_data_reference_offset_count++;
		}

		for (uint32 i = 0; i < definition->get_string_id_count(); i++)
		{
			file_seek_and_read(loader->m_file_handle, base_offset + definition->get_string_id_offset(i), sizeof(string_id), 1, &t_string_id);
			if (t_string_id != 0)
			{
				loader->m_string_id_offset_count++;
			}
		}

		for (uint32 i = 0; i < definition->get_tag_block_count(); i++)
		{
			file_seek_and_read(loader->m_file_handle, base_offset + definition->get_tag_block(i)->get_offset(), sizeof(tag_block<>), 1, &t_tag_block);
			if (t_tag_block.count > 0)
			{
				loader->m_tag_block_offset_count++;
				initialize_arrays_internal(loader, definition->get_tag_block(i), loader->resolve_cache_tag_data_offset(t_tag_block.data), t_tag_block.count);
			}
		}
	}
}

void c_xml_definition_loader::initialize_arrays(void)
{
	this->initialize_arrays_internal(this, this->m_definition, this->m_file_offset, 1);

	if (this->m_tag_reference_offset_count)
		this->m_tag_reference_offsets = lazy_malloc_buffer(s_offset_link, this->m_tag_reference_offset_count);

	if (this->m_classless_tag_reference_offset_count)
		this->m_classless_tag_reference_offsets = lazy_malloc_buffer(s_offset_link, this->m_classless_tag_reference_offset_count);

	if (this->m_data_reference_offset_count)
		this->m_data_reference_offsets = lazy_malloc_buffer(s_memory_link, this->m_data_reference_offset_count);

	if (this->m_string_id_offset_count)
		this->m_string_id_offsets = lazy_malloc_buffer(s_offset_link, this->m_string_id_offset_count);

	if (this->m_tag_block_offset_count)
		this->m_tag_block_offsets = lazy_malloc_buffer(s_memory_link, this->m_tag_block_offset_count);

	if (this->m_tag_reference_offset_count + this->m_classless_tag_reference_offset_count)
		this->m_tag_references = lazy_malloc_buffer(datum, (this->m_tag_reference_offset_count + this->m_classless_tag_reference_offset_count));

	this->m_data = (int8*)calloc(this->m_total_data_size, sizeof(int8));
	return;
}

int8* c_xml_definition_loader::reserve_data(uint32 size)
{
	if (this->m_used_data + size > this->m_total_data_size)
	{
		DISPLAY_ASSERT("[c_xml_definition_loader::reserve_data]: RAN OUT OF SPACE");
	}
	ASSERT(this->m_data);

	const uint32 old_size = this->m_used_data;
	this->m_used_data += size;

#if TAG_INJECTION_DEBUG
	LOG_DEBUG_GAME("[c_xml_definition_loader::reserve_data] data: {:x} requested: {:x} used: {:x} total: {:x}", (uint32)this->m_data, size, this->m_used_data, this->m_total_data_size);
#endif

	return &this->m_data[old_size];
}

void c_xml_definition_loader::load_tag_data_internal(c_xml_definition_loader* loader, const c_xml_definition_block* definition, uint32 file_offset, int8* buffer, uint32 block_count)
{
	ASSERT(loader);
	ASSERT(definition);
	ASSERT(buffer);

	int8* cache_data = (int8*)calloc(block_count, definition->get_size());

	ASSERT(cache_data);

	fseek(loader->m_file_handle, file_offset, SEEK_SET);
	fread(cache_data, definition->get_size() * block_count, 1, loader->m_file_handle);
	csmemcpy(buffer, cache_data, definition->get_size() * block_count);
	free(cache_data);

	for (uint32 block_index = 0; block_index < block_count; block_index++)
	{
		for (uint32 i = 0; i < definition->get_tag_references_count(); i++)
		{
			uint32 calc_offset = definition->get_tag_reference_offset(i) + definition->get_size() * block_index;

			file_seek_and_read(loader->m_file_handle, file_offset + calc_offset, sizeof(tag_reference), 1, &t_tag_reference);
			if (t_tag_reference.group.group != _tag_group_none && t_tag_reference.index != NONE)
			{
				s_offset_link* link = &loader->m_tag_reference_offsets[loader->m_tag_reference_offset_count];
				link->cache_offset = file_offset + calc_offset;
				link->memory_offset = (uint32)buffer + calc_offset;

#if TAG_INJECTION_DEBUG
				csstrnzcpy(link->name, definition->get_tag_reference_name(i)->get_string(), sizeof(s_offset_link::name));
#endif

				loader->m_tag_references[loader->m_tag_reference_count] = t_tag_reference.index;
				loader->m_tag_reference_count++;
				loader->m_tag_reference_offset_count++;
			}
		}

		for (uint32 i = 0; i < definition->get_classless_tag_references_count(); i++)
		{
			uint32 calc_offset = definition->get_classless_tag_reference_offset(i) + definition->get_size() * block_index;
			
			file_seek_and_read(loader->m_file_handle, file_offset + calc_offset, sizeof(datum), 1, &t_classless_tag_reference);
			if (t_classless_tag_reference != NONE)
			{
				s_offset_link* link = &loader->m_classless_tag_reference_offsets[loader->m_classless_tag_reference_offset_count];

				link->cache_offset = file_offset + calc_offset;
				link->memory_offset = (uint32)buffer + calc_offset;

#if TAG_INJECTION_DEBUG
				csstrnzcpy(link->name, definition->get_classless_tag_reference_name(i)->get_string(), sizeof(s_offset_link::name));
#endif

				loader->m_tag_references[loader->m_tag_reference_count] = t_classless_tag_reference;
				loader->m_tag_reference_count++;
				loader->m_classless_tag_reference_offset_count++;
			}
		}

		for (uint32 i = 0; i < definition->get_data_references_count(); i++)
		{
			uint32 calc_offset = definition->get_data_reference_offset(i) + definition->get_size() * block_index;

			file_seek_and_read(loader->m_file_handle, file_offset + calc_offset, sizeof(data_reference), 1, &t_data_reference);
			if (t_data_reference.size != 0)
			{
				int8* data_cache = (int8*)malloc(t_data_reference.size);

				ASSERT(data_cache);

				int8* data_buffer = loader->reserve_data(t_data_reference.size);
				fseek(loader->m_file_handle, loader->resolve_cache_tag_data_offset(t_data_reference.data), SEEK_SET);
				fread(data_cache, t_data_reference.size, 1, loader->m_file_handle);
				csmemcpy(data_buffer, data_cache, t_data_reference.size);
				free(data_cache);

				s_memory_link* link = &loader->m_data_reference_offsets[loader->m_data_reference_offset_count];

				link->cache_offset = file_offset + calc_offset;
				link->memory_offset = (uint32)buffer + calc_offset;
				link->data = data_buffer;
				link->size = t_data_reference.size;

#if TAG_INJECTION_DEBUG
				LOG_DEBUG_GAME("[c_xml_definition_lodaer::load_tag_internal] data reference: {}-{:x} size: {:x} data: {:x}", i, file_offset + definition->get_data_reference_offset(i), t_data_reference.size, t_data_reference.data);
				csstrnzcpy(link->name, definition->get_data_reference_name(i)->get_string(), sizeof(s_offset_link::name));
#endif

				loader->m_data_reference_offset_count++;
			}
		}

		for (uint32 i = 0; i < definition->get_string_id_count(); i++)
		{
			uint32 calc_offset = definition->get_string_id_offset(i) + definition->get_size() * block_index;

			file_seek_and_read(loader->m_file_handle, file_offset + calc_offset, sizeof(string_id), 1, &t_string_id);
			if (t_string_id != 0)
			{
				loader->m_string_id_offsets[loader->m_string_id_offset_count].cache_offset = file_offset + calc_offset;
				loader->m_string_id_offsets[loader->m_string_id_offset_count].memory_offset = (uint32)buffer + calc_offset;
				loader->m_string_id_offset_count++;
			}
		}

		for (uint32 i = 0; i < definition->get_tag_block_count(); i++)
		{
			const c_xml_definition_block* definition_block = definition->get_tag_block(i);
			uint32 calc_offset = definition_block->get_offset() + definition->get_size() * block_index;

			file_seek_and_read(loader->m_file_handle, file_offset + calc_offset, sizeof(tag_block<>), 1, &t_tag_block);
			if (t_tag_block.count > 0)
			{
				int8* block_buffer = loader->reserve_data(definition_block->get_size() * t_tag_block.count);

				s_memory_link* link = &loader->m_tag_block_offsets[loader->m_tag_block_offset_count];

				link->cache_offset = file_offset + calc_offset;
				link->memory_offset = (uint32)buffer + calc_offset;
				link->size = definition_block->get_size() * t_tag_block.count;
				link->data = block_buffer;

#if TAG_INJECTION_DEBUG
				LOG_DEBUG_GAME("[c_xml_definition_loader::load_tag_internal] tag_block: {} name: {} count: {}, size: {:x}", i, definition->get_tag_block_name(i)->get_string(), t_tag_block.count, definition_block->get_size());
				csstrnzcpy(link->name, definition->get_tag_block_name(i)->get_string(), sizeof(s_offset_link::name));
#endif

				loader->m_tag_block_offset_count++;

				load_tag_data_internal(loader, definition_block, loader->resolve_cache_tag_data_offset(t_tag_block.data), block_buffer, t_tag_block.count);
			}
		}
	}
}

void c_xml_definition_loader::load_tag_data()
{
	this->reset_counts();

	int8* buffer = this->reserve_data(this->m_definition->get_size());

	this->load_tag_data_internal(this, this->m_definition, this->m_file_offset, buffer, 1);

#if TAG_INJECTION_DEBUG
	this->validate_data();
#endif
}

void c_xml_definition_loader::calculate_total_data_size(const c_xml_definition_block* definition, uint32 base_offset, uint32 block_count)
{

	for (uint32 block_index = 0; block_index < block_count; block_index++)
	{
		uint32 calc_offset = base_offset + block_index * definition->get_size();
		for (uint32 i = 0; i < definition->get_data_references_count(); i++)
		{
			data_reference reference;

			file_seek_and_read(this->m_file_handle, calc_offset + definition->get_data_reference_offset(i), sizeof(data_reference), 1, &reference);
			if (reference.size != 0)
			{
				this->m_total_data_size += reference.size;
#if TAG_INJECTION_DEBUG
				bool in_bounds = (this->m_instance.data_offset < reference.data && this->m_instance.data_offset + this->m_instance.size > reference.data);
				LOG_DEBUG_GAME("[c_xml_definition_loader::calc_data_size] {:x} data_reference  {} {} {:x} {}", base_offset, definition->get_data_reference_offset(i), reference.size, reference.data, in_bounds);
#endif
			}
		}


		for (uint32 i = 0; i < definition->get_tag_block_count(); i++)
		{
			tag_block<> block;

			const c_xml_definition_block* definition_block = definition->get_tag_block(i);

			file_seek_and_read(this->m_file_handle, calc_offset + definition_block->get_offset(), sizeof(tag_block<>), 1, &block);
			if (block.count != 0 && block.data != 0 && block.count != -1 && block.data != -1)
			{
				this->m_total_data_size += definition_block->get_size() * block.count;

#if TAG_INJECTION_DEBUG
				bool in_bounds = (this->m_instance.data_offset < block.data && this->m_instance.data_offset + this->m_instance.size > block.data);
				LOG_DEBUG_GAME(
					"[c_xml_definition_loader::calc_data_size] {:x} {} {} {} {:x} {}",
					base_offset,
					definition_block->get_name(),
					definition_block->get_offset(),
					block.count,
					block.data,
					in_bounds);
#endif

				this->calculate_total_data_size(definition->get_tag_block(i), resolve_cache_tag_data_offset(block.data), block.count);
			}
		}
	}
}


uint32 c_xml_definition_loader::get_total_size() const
{
	return this->m_total_data_size;
}

int8* c_xml_definition_loader::get_data() const
{
	return this->m_data;
}

uint32 c_xml_definition_loader::get_tag_reference_count(void) const
{
	return this->m_tag_reference_count;
}

datum c_xml_definition_loader::get_tag_reference(uint32 index) const
{
	return this->m_tag_references[index];
}

void c_xml_definition_loader::copy_tag_data(int8* out_buffer, uint32 base_offset)
{
	// copy the data into the out buffer
	csmemcpy(out_buffer, this->m_data, this->m_total_data_size);

	// resolve and update tag references
	for (uint32 i = 0; i < this->m_tag_reference_offset_count; i++)
	{
		s_offset_link* link = &this->m_tag_reference_offsets[i];
		tag_reference* reference = (tag_reference*)(out_buffer + link->memory_offset - this->m_data);
		datum resolved_index = tag_injection_resolve_cache_datum(reference->index);

#if TAG_INJECTION_DEBUG
		LOG_DEBUG_GAME("[c_xml_definition_loader::copy_tag_data]: updating tag reference: {} from: {:X} to {:X}", link->name, reference->index, resolved_index);
#endif

		reference->index = resolved_index;
	}

	// resolve and update classless tag references
	for (uint32 i = 0; i < this->m_classless_tag_reference_offset_count; i++)
	{
		s_offset_link* link = &this->m_classless_tag_reference_offsets[i];
		datum* reference = (datum*)(out_buffer + link->memory_offset - this->m_data);
		datum resolved_index = tag_injection_resolve_cache_datum(*reference);

#if TAG_INJECTION_DEBUG
		LOG_DEBUG_GAME("[c_xml_definition_loader::copy_tag_data]: updating classless tag reference: {} from: {:x} to {:x} addr: {:x}",
			link->name,
			*reference,
			resolved_index,
			link->memory_offset);
#endif

		*reference = resolved_index;
	}

	// update data references
	for (uint32 i = 0; i < this->m_data_reference_offset_count; i++)
	{
		s_memory_link* link = &this->m_data_reference_offsets[i];
		data_reference* reference = (data_reference*)(out_buffer + link->memory_offset - this->m_data);
		uint32 resolved_offset = ((uint32)link->data - (uint32)this->m_data);
		reference->data = (uint32)base_offset + resolved_offset;

#if TAG_INJECTION_DEBUG
		bool in_bounds = (resolved_offset + link->size <= this->m_total_data_size);
		LOG_DEBUG_GAME("[c_xml_definition_loader::copy_tag_data]: rebase tag_block: {} base_offset: {:x} block_offset {:x} block_size: {:x} total size: {:x} in bounds: {}",
			link->name,
			base_offset,
			resolved_offset,
			link->size,
			this->m_total_data_size,
			in_bounds);
#endif
	}

	// update tag blocks
	for (uint32 i = 0; i < this->m_tag_block_offset_count; i++)
	{
		s_memory_link* link = &this->m_tag_block_offsets[i];
		tag_block<>* reference = (tag_block<>*)(out_buffer + link->memory_offset - this->m_data);
		uint32 resolved_offset = ((uint32)link->data - (uint32)this->m_data);
		reference->data = (uint32)base_offset + resolved_offset;

#if TAG_INJECTION_DEBUG
		bool in_bounds = (resolved_offset + link->size <= this->m_total_data_size);
		LOG_DEBUG_GAME("[c_xml_definition_loader::copy_tag_data]: rebase tag_block: {} base_offset: {:x} block_offset {:x} block_size: {:x} total size: {:x} in bounds: {}",
			link->name,
			base_offset,
			resolved_offset,
			link->size,
			this->m_total_data_size,
			in_bounds);
#endif
	}
}

#if TAG_INJECTION_DEBUG
void c_xml_definition_loader::validate_data() const
{
	int8* cache_data = (int8*)malloc(this->m_definition->get_size());

	file_seek_and_read(this->m_file_handle, this->m_file_offset, this->m_definition->get_size(), 1, cache_data);

	int res = memcmp(cache_data, this->m_data, this->m_definition->get_size());

	if (res != 0)
	{
		LOG_ERROR_GAME("[c_xml_definition_loader::validate_data] Base definition block is invalid");
	}
	for (uint32 i = 0; i < this->m_data_reference_offset_count; i++)
	{
		int8* data_data = (int8*)malloc(this->m_data_reference_offsets[i].size);

		file_seek_and_read(this->m_file_handle, this->m_data_reference_offsets[i].cache_offset, this->m_data_reference_offsets[i].size, 1, data_data);
		int res = memcmp(data_data, (uint8*)this->m_data_reference_offsets[i].memory_offset, 1);
		if (res != 0)
		{
			LOG_ERROR_GAME("[c_xml_definition_loader::validate_data] data_reference is invalid");
		}
		free(data_data);
	}
	for (uint32 i = 0; i < this->m_tag_block_offset_count; i++)
	{
		int8* block_data = (int8*)malloc(this->m_tag_block_offsets[i].size);

		file_seek_and_read(this->m_file_handle, this->m_tag_block_offsets[i].cache_offset, this->m_tag_block_offsets[i].size, 1, block_data);
		int res = memcmp(block_data, (uint8*)this->m_tag_block_offsets[i].memory_offset, 1);
		if (res != 0)
		{
			LOG_ERROR_GAME("[c_xml_definition_loader::validate_data] tag_block is invalid");
		}
		free(block_data);
	}
	free(cache_data);
}
#endif

#undef lazy_malloc_buffer