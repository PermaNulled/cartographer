#include "stdafx.h"
#include "map_repository.h"

#include "cache/cache_files.h"
#include "interface/signal_slot.h"
#include "interface/user_interface_memory.h"
#include "memory/data.h"
#include "scenario/scenario_definitions.h"
#include "shell/shell.h"
#include "tag_files/files_windows.h"
#include "text/unicode.h"



#pragma region 50 map limit removal

// TODO: consider replacing this implementation with a linked list

const wchar_t* custom_map_cache_filename_client = L".h2mdat";

// TODO: determine whether or not the pre-release version of vista's map files are supported (11028.07.03.23.1927.main)
const static char* offically_supported_builds[] =
{
	"11081.07.04.30.0934.main",
	"11122.07.08.24.1808.main",
};

bool __cdecl scenario_is_supported_build(const char* build)
{
	bool result = false;

	for (int32 i = 0; i < NUMBEROF(offically_supported_builds); i++)
	{
		if (strcmp(build, offically_supported_builds[i]) == 0)
		{
			result = true;
			break;
		}
	}

	if (!result)
	{
		LOG_TRACE_FUNC("Build '{}' is not offically supported, consider repacking and updating map with supported tools!", build);
	}

	// allow map to be loaded regardless of the cache version
	result = true;

	return result;
}

static s_custom_map_entry* get_custom_map_entry_list_from_header(s_custom_map_file_cache_header* header)
{
	return (s_custom_map_entry*)((uint8*)header + sizeof(s_custom_map_file_cache_header));
}

c_custom_map_manager* get_custom_map_manager()
{
	c_custom_map_manager* custom_map_manager = Memory::GetAddress<c_custom_map_manager*>(0x482D70, 0x4A70D8);
	return custom_map_manager;
}

const wchar_t* get_custom_map_folder_path()
{
	return get_custom_map_manager()->m_custom_maps_folder_path;
}

int get_directory_path_from_id(e_directory_id id, LPCWSTR pMore, LPWSTR pszPath, bool is_folder)
{
	typedef int(__cdecl* t_get_directory_path_from_id)(e_directory_id id, LPCWSTR pMore, LPWSTR pszPath, bool is_folder);
	auto p_get_directory_path_by_id = Memory::GetAddressRelative<t_get_directory_path_from_id>(0x48EF9E, 0x474A15);
	return p_get_directory_path_by_id(id, pMore, pszPath, is_folder);
}

void c_custom_map_manager::create_custom_map_data_directory()
{
	auto p__create_custom_map_data_directory = Memory::GetAddressRelative<void(__cdecl*)()>(0x4C1FD4, 0x48F715);
	p__create_custom_map_data_directory();
}

void __thiscall c_custom_map_manager::mark_all_cached_maps_for_deletion()
{
	EnterCriticalSection(m_lock);

	if (m_new_custom_map_entry_list_buffer == nullptr)
	{
		LeaveCriticalSection(m_lock);
		return;
	}

	for (uint16 i = 0; i < m_map_count; i++)
		m_new_custom_map_entry_list_buffer[i].entry_marked_for_deletion = true;

	LeaveCriticalSection(m_lock);
}

bool __thiscall c_custom_map_manager::remove_marked_for_deletion()
{
	EnterCriticalSection(m_lock);

	bool maps_removed = false;
	uint16 map_count_before_remove = m_map_count;

	for (uint16 i = 0; i < m_map_count; )
	{
		if (m_new_custom_map_entry_list_buffer[i].entry_marked_for_deletion)
		{
			remove_entry_by_index(i);
		}
		else
		{
			i++;
		}
	}

	maps_removed = map_count_before_remove > m_map_count;

	LeaveCriticalSection(m_lock);

	return maps_removed;
}

bool c_custom_map_manager::read_custom_map_data_cache_from_file(const utf8* path, 
	s_custom_map_file_cache_header* custom_map_data_header, 
	uint32 custom_map_data_cache_buffer_size
)
{
	s_file_reference cache_file;

	bool success = false;
	bool invalid_file = false;
	e_file_open_error open_file_error_code;

	file_reference_create_from_path(&cache_file, path, false);
	e_file_open_flags flags = (e_file_open_flags)(_file_open_sequential_scan_bit | _permission_read_bit);

	// ### TODO FIXME clean this up
	uint32 entry_count = 0;
	s_custom_map_entry* entry_list = get_custom_map_entry_list_from_header(custom_map_data_header);

	do 
	{
		if (!file_open(&cache_file, flags, &open_file_error_code))
		{
			LOG_TRACE_GAME("{} - failed to open custom map data cache file!",
				__FUNCTION__);

			break;
		}

		// first we read the header
		if (!file_read(&cache_file, sizeof(s_custom_map_file_cache_header), true, custom_map_data_header))
		{
			invalid_file = true;
			LOG_TRACE_GAME("{} - failed to read custom map data cache file header!",
				__FUNCTION__);

			break;
		}

		if (custom_map_data_header->signature != custom_map_cache_signature)
		{
			LOG_TRACE_GAME("{} - invalid custom map data cache file signature! deleting file if possible, name is reserved!",
				__FUNCTION__);

			invalid_file = true;
			break;
		}

		if (custom_map_data_header->version != k_custom_map_data_cache_header_version)
		{
			LOG_TRACE_GAME("{} - custom map data cache file header version mismatch! deleting file if possible",
				__FUNCTION__);

			invalid_file = true;
			break;
		}

		entry_count = custom_map_data_header->entry_count;
		if (entry_count > k_maximum_number_of_custom_multiplayer_maps_new)
		{
			LOG_TRACE_GAME("{} - custom map data cache file exceeds new map limit size, {} > max: {}",
				__FUNCTION__,
				entry_count,
				k_maximum_number_of_custom_multiplayer_maps_new);

			break;
		}

		// if signature matches, read the cache contents
		uint32 file_custom_map_entries_size = entry_count * sizeof(s_custom_map_entry);

		if (!file_read(&cache_file, file_custom_map_entries_size, true, entry_list))
		{
			LOG_TRACE_GAME("{} - failed reading custom map data cache file!",
				__FUNCTION__);

			break;
		}

		// clear unused custom map entries
		size_t remaining_size = custom_map_data_cache_buffer_size - sizeof(s_custom_map_file_cache_header) - file_custom_map_entries_size;
		csmemset((uint8*)entry_list + file_custom_map_entries_size, 0, remaining_size);
		success = true;

	} while (0);

	if (open_file_error_code == _file_open_error_success)
	{
		file_close(&cache_file);
		if (invalid_file)
			file_delete(&cache_file);
	}

	if (success)
	{
		LOG_TRACE_GAME("loaded custom map paths: ");
		for (uint32 i = 0; i < entry_count; i++)
		{
			LOG_TRACE_GAME(L"	custom map path: {}", entry_list[i].file_path);
		}
	}

	return success;
}

bool c_custom_map_manager::write_custom_map_data_cache_to_file(const utf8* path, s_custom_map_file_cache_header* custom_map_data_cache)
{
	s_file_reference cache_file;

	bool success = false;
	e_file_open_error open_file_error_code = _file_open_error_unknown;

	file_reference_create_from_path(&cache_file, path, false);
	bool create_file_success = file_create(&cache_file);

	do 
	{
		if (!file_open(&cache_file, _permission_write_bit, &open_file_error_code))
		{
			LOG_ERROR_GAME("{} - failed to open custom map cache file, error code: {}", __FUNCTION__, (uint32)open_file_error_code);

			break;
		}

		file_set_hidden(&cache_file, true);
		
		uint32 size_to_write = sizeof(s_custom_map_file_cache_header) + sizeof(s_custom_map_entry) * custom_map_data_cache->entry_count;

		if (!file_write(&cache_file, size_to_write, custom_map_data_cache))
		{
			LOG_ERROR_GAME("{} - failed to write custom map cache to file!",
				__FUNCTION__);

			break;
		}

		LOG_TRACE_GAME("{} - saved custom map data cache, map count: {}, write size: {}",
			__FUNCTION__, custom_map_data_cache->entry_count, size_to_write);

		if (!file_set_eof(&cache_file))
		{
			LOG_ERROR_GAME("{} - failed to set custom map cache file size!",
				__FUNCTION__);

			break;
		}

		success = true;

	} while (0);


	if (open_file_error_code == _file_open_error_success)
	{
		file_close(&cache_file);
	}
	else if (!create_file_success && open_file_error_code == _file_open_error_not_found)
	{
		LOG_ERROR_GAME("{} - failed to create custom map data cache while file not present!", __FUNCTION__);
	}

	return success;
}

void __thiscall c_custom_map_manager::save_custom_map_data()
{
	WCHAR path_wide[MAX_PATH];
	CHAR path_multibyte[MAX_PATH];
	bool custom_map_data_path_available = false;

	EnterCriticalSection(m_lock);

	s_custom_map_file_cache_header* custom_map_data_to_save = m_custom_map_file_data_cache;

	custom_map_data_to_save->signature = custom_map_cache_signature;
	custom_map_data_to_save->entry_count = m_map_count;
	custom_map_data_to_save->index_of_last_previewed_bitmap = m_index_of_last_previewed_bitmap;

	if (get_directory_path_from_id(e_directory_id::_custom_map_data_dir, L"\0", path_wide, true))
	{
		custom_map_data_path_available = true;
		wcscat(path_wide, custom_map_cache_filename_client);
	}

	if (!custom_map_data_path_available
		|| !WideCharToMultiByte(CP_UTF8, 0, path_wide, -1, path_multibyte, MAX_PATH, 0, 0)
		|| !write_custom_map_data_cache_to_file(path_multibyte, custom_map_data_to_save))
	{
		LOG_ERROR_GAME("{} - failed to save custom map data cache!", __FUNCTION__);
	}

	LeaveCriticalSection(m_lock);
}

void c_custom_map_manager::load_map_data_cache_from_file_cache(s_custom_map_file_cache_header* custom_map_file_cache)
{

}

void __thiscall c_custom_map_manager::load_custom_map_data_cache()
{
	//typedef void(__thiscall* cache_custom_map_file_image_preview)(c_custom_map_manager* thisx);
	//auto p_cache_custom_map_file_image_previews = Memory::GetAddress<cache_custom_map_file_image_preview>(0x4CC30, 0x41501);
	//return p_cache_custom_map_file_image_previews(this);

	EnterCriticalSection(m_lock);

	WCHAR path_wide[MAX_PATH];
	CHAR path_multibyte[MAX_PATH];
	bool custom_map_data_path_available = false;

	uint32 map_file_cache_buffer_size = sizeof(s_custom_map_file_cache_header) + sizeof(s_custom_map_entry) * k_maximum_number_of_custom_multiplayer_maps_new;
	s_custom_map_file_cache_header* custom_map_cache_file_header = (s_custom_map_file_cache_header*)new char[map_file_cache_buffer_size];

	// first create the custom map data directory (if not created already)
	create_custom_map_data_directory();

	if (get_directory_path_from_id(e_directory_id::_custom_map_data_dir, L"\0", path_wide, true))
	{
		custom_map_data_path_available = true;
		wcscat(path_wide, custom_map_cache_filename_client);
	}

	if (!custom_map_data_path_available
		|| !WideCharToMultiByte(CP_UTF8, 0, path_wide, -1, path_multibyte, MAX_PATH, 0, 0)
		|| !read_custom_map_data_cache_from_file(path_multibyte, custom_map_cache_file_header, map_file_cache_buffer_size))
	{
		// if we fail clear the buffer
		csmemset(custom_map_cache_file_header, 0, map_file_cache_buffer_size);
		custom_map_cache_file_header->signature = custom_map_cache_signature;
		custom_map_cache_file_header->version = k_custom_map_data_cache_header_version;

		LOG_ERROR_GAME("{} - failed to read custom map data cache!", 
			__FUNCTION__);
	}

	//load_map_data_cache_from_file_cache(custom_map_cache_file_header);

	if (m_custom_map_file_data_cache != nullptr)
		delete[] m_custom_map_file_data_cache;

	this->m_custom_map_file_data_cache = custom_map_cache_file_header;
	this->m_map_count = (uint16)custom_map_cache_file_header->entry_count;
	this->m_new_custom_map_entry_list_buffer	
		= get_custom_map_entry_list_from_header(custom_map_cache_file_header);

	this->m_index_of_last_previewed_bitmap = custom_map_cache_file_header->index_of_last_previewed_bitmap;

	LeaveCriticalSection(m_lock);
}

void __thiscall c_custom_map_manager::start_custom_map_sync()
{
	// this code doesn't need any modifications, just some functions replaced inside it
	typedef void(__thiscall* start_custom_map_sync_t)(c_custom_map_manager* thisx);
	auto p_start_custom_map_sync = Memory::GetAddress<start_custom_map_sync_t>(0x4D021, 0x419B5);
	p_start_custom_map_sync(this);
}

uint32 __thiscall c_custom_map_manager::get_custom_map_list_ids(s_custom_map_id* out_ids, uint32 out_ids_count)
{
	EnterCriticalSection(m_lock);
	for (uint16 i = 0; i < m_map_count; i++)
	{
		m_new_custom_map_entry_list_buffer[i].entry_marked_for_deletion = false;
		if (out_ids != nullptr
			&& out_ids_count != 0)
		{
			if (i < out_ids_count)
			{
				csmemcpy(out_ids[i].map_sha256_hash, m_new_custom_map_entry_list_buffer[i].map_sha256_hash, k_sha256_hash_size);
				wcsncpy(out_ids[i].map_name, m_new_custom_map_entry_list_buffer[i].map_name, k_max_map_name_size);
			}
		}
	}

	uint32 map_count = this->m_map_count;
	LeaveCriticalSection(m_lock);

	return map_count;
}

uint32 __thiscall c_custom_map_manager::get_custom_map_list_ids_by_map_name(const wchar_t* map_name, s_custom_map_id* out_ids, uint32 out_ids_count)
{
	EnterCriticalSection(m_lock);

	uint32 matching_count_found = 0;

	// used by dedicated server
	for (uint16 i = 0; i < m_map_count; i++)
	{
		if (!_wcsnicmp(m_new_custom_map_entry_list_buffer[i].map_name, map_name, k_max_map_name_size))
		{
			m_new_custom_map_entry_list_buffer[i].entry_marked_for_deletion = false;
			if (out_ids != nullptr
				&& out_ids_count != 0)
			{
				if (matching_count_found < out_ids_count)
				{
					csmemcpy(out_ids[matching_count_found].map_sha256_hash, m_new_custom_map_entry_list_buffer[i].map_sha256_hash, k_sha256_hash_size);
					wcsncpy(out_ids[matching_count_found].map_name, m_new_custom_map_entry_list_buffer[i].map_name, k_max_map_name_size);
				}
			}

			matching_count_found++;
		}
	}

	LeaveCriticalSection(m_lock);

	return matching_count_found;
}

// TODO de-duplicate custom map entry matching code
// because just for 1 line of code we re-write same code over and over

uint32 __thiscall c_custom_map_manager::find_matching_entries_by_file_path(const wchar_t* file_path, s_custom_map_entry** out_custom_map_entries, uint32 out_custom_map_entries_count)
{
	EnterCriticalSection(m_lock);

	if (m_new_custom_map_entry_list_buffer == nullptr
		|| file_path == nullptr)
	{
		LeaveCriticalSection(m_lock);
		return 0;
	}

	uint32 matching_count_found = 0;

	for (uint16 i = 0; i < m_map_count; i++)
	{
		if (!_wcsnicmp(m_new_custom_map_entry_list_buffer[i].file_path, file_path, k_map_file_path_size))
		{
			m_new_custom_map_entry_list_buffer[i].entry_marked_for_deletion = false;
			if (out_custom_map_entries)
			{
				if (matching_count_found < out_custom_map_entries_count)
					out_custom_map_entries[matching_count_found] = &m_new_custom_map_entry_list_buffer[i];
			}

			matching_count_found++;
		}
	}

	LeaveCriticalSection(m_lock);
	return matching_count_found;
}

uint32 __thiscall c_custom_map_manager::find_matching_entries_by_sha256_hash(const BYTE* hash, s_custom_map_entry** out_custom_map_entries, uint32 out_custom_map_entries_count)
{
	EnterCriticalSection(m_lock);

	if (m_new_custom_map_entry_list_buffer == nullptr
		|| hash == nullptr)
	{
		LeaveCriticalSection(m_lock);
		return 0;
	}

	uint32 matching_count_found = 0;

	for (uint16 i = 0; i < m_map_count; i++)
	{
		if (!memcmp(m_new_custom_map_entry_list_buffer[i].map_sha256_hash, hash, k_sha256_hash_size))
		{
			m_new_custom_map_entry_list_buffer[i].entry_marked_for_deletion = false;
			if (out_custom_map_entries)
			{
				if (matching_count_found < out_custom_map_entries_count)
					out_custom_map_entries[matching_count_found] = &m_new_custom_map_entry_list_buffer[i];
			}

			matching_count_found++;
		}
	}

	LeaveCriticalSection(m_lock);
	return matching_count_found;
}

uint32 __thiscall c_custom_map_manager::find_matching_entries_by_map_name_and_hash(const wchar_t* map_name, const BYTE* sha256_hash, s_custom_map_entry** out_custom_map_entries, uint32 out_custom_map_entries_count)
{
	EnterCriticalSection(m_lock);

	if (m_new_custom_map_entry_list_buffer == nullptr
		|| map_name == nullptr)
	{
		LeaveCriticalSection(m_lock);
		return 0;
	}

	uint32 matching_count_found = 0;

	for (uint16 i = 0; i < m_map_count; i++)
	{
		if (!memcmp(m_new_custom_map_entry_list_buffer[i].map_sha256_hash, sha256_hash, k_sha256_hash_size) && !_wcsnicmp(m_new_custom_map_entry_list_buffer[i].map_name, map_name, k_max_map_name_size))
		{
			m_new_custom_map_entry_list_buffer[i].entry_marked_for_deletion = false;
			if (out_custom_map_entries)
			{
				if (matching_count_found < out_custom_map_entries_count)
					out_custom_map_entries[matching_count_found] = &m_new_custom_map_entry_list_buffer[i];
			}

			matching_count_found++;
		}
	}

	LeaveCriticalSection(m_lock);
	return matching_count_found;
}

uint32 __thiscall c_custom_map_manager::find_matching_entries_by_map_name(const wchar_t* map_name, s_custom_map_entry** out_custom_map_entries, uint32 out_custom_map_entries_count)
{
	EnterCriticalSection(m_lock);

	if (m_new_custom_map_entry_list_buffer == nullptr
		|| map_name == nullptr)
	{
		LeaveCriticalSection(m_lock);
		return 0;
	}

	uint32 matching_count_found = 0;

	for (uint16 i = 0; i < m_map_count; i++)
	{
		if (!_wcsnicmp(m_new_custom_map_entry_list_buffer[i].map_name, map_name, k_max_map_name_size))
		{
			m_new_custom_map_entry_list_buffer[i].entry_marked_for_deletion = false;
			if (out_custom_map_entries)
			{
				if (matching_count_found < out_custom_map_entries_count)
					out_custom_map_entries[matching_count_found] = &m_new_custom_map_entry_list_buffer[i];
			}

			matching_count_found++;
		}
	}

	LeaveCriticalSection(m_lock);
	return matching_count_found;
}

bool __thiscall c_custom_map_manager::get_entry_by_id(const s_custom_map_id* custom_map_id, s_custom_map_entry** out_entry)
{
	// custom_scenario_test_map_name_instead_of_hash
	if (shell_command_line_flag_is_set(_shell_command_line_flag_custom_map_entry_test_map_name_instead_of_hash))
		return find_matching_entries_by_map_name(custom_map_id->map_name, out_entry, 1) != 0;
	else
		return find_matching_entries_by_sha256_hash(custom_map_id->map_sha256_hash, out_entry, 1) != 0;
}

bool __thiscall c_custom_map_manager::entry_is_duplicate(const s_custom_map_entry* entry)
{
	EnterCriticalSection(m_lock);

	bool duplicate = false;

	if (find_matching_entries_by_file_path(entry->file_path, 0, 0) > 0u
		|| find_matching_entries_by_map_name_and_hash(entry->map_name, entry->map_sha256_hash, 0, 0) > 0u)
	{
		duplicate = true;
	}

	LeaveCriticalSection(m_lock);

	return duplicate;
}

bool __thiscall c_custom_map_manager::validate_entry_data(const s_custom_map_entry* entries, uint32 count)
{
	typedef bool(__thiscall* validate_entry_data_t)(c_custom_map_manager*, const s_custom_map_entry* entry, uint32 count);
	auto p_validate_entry_data = Memory::GetAddressRelative<validate_entry_data_t>(0x4C1E01, 0x48F542);
	return p_validate_entry_data(this, entries, count * sizeof(s_custom_map_entry));
}

void __thiscall c_custom_map_manager::remove_entry_by_index(uint16 idx)
{
	// check if the element to be deleted is the last one
	// if so just memset to 0
	if (idx == m_map_count - 1)
	{
		csmemset(&m_new_custom_map_entry_list_buffer[idx], 0, sizeof(s_custom_map_entry));
	}
	else
	{
		// otherwise just move the memory from the next element to last in the now empty entry
		// TODO: consider replacing this implementation with a linked list
		// this might get very expensive
		// but it shouldn't execute too many times
		csmemmove(&m_new_custom_map_entry_list_buffer[idx], &m_new_custom_map_entry_list_buffer[idx + 1], (m_map_count - 1 - idx) * sizeof(s_custom_map_entry));
	}

	m_map_count--;
}

bool __thiscall c_custom_map_manager::remove_entries_matching_file_path(const s_custom_map_entry* entry)
{
	EnterCriticalSection(m_lock);

	bool removed = false;
	for (uint16 i = 0; i < m_map_count; )
	{
		if (!_wcsnicmp(m_new_custom_map_entry_list_buffer[i].file_path, entry->file_path, k_map_file_path_size))
		{
			removed = true;
			remove_entry_by_index(i);
		}
		else
		{
			i++;
		}
	}

	LeaveCriticalSection(m_lock);
	return removed;
}

bool __thiscall c_custom_map_manager::remove_duplicates_by_map_name_and_hash(const s_custom_map_entry* entry)
{
	EnterCriticalSection(m_lock);

	bool removed = false;
	for (uint16 i = 0; i < m_map_count; i++)
	{
		if (!memcmp(m_new_custom_map_entry_list_buffer[i].map_sha256_hash, entry->map_sha256_hash, k_sha256_hash_size) 
			&& !_wcsnicmp(m_new_custom_map_entry_list_buffer[i].map_name, entry->map_name, k_max_map_name_size))
		{
			//LOG_TRACE_GAME(L"{} - removed: {} from cache matching cached entry: map name: {} map path: {}",
				//__FUNCTIONW__, entry->file_path, new_custom_map_entries_buffer[i].map_name, new_custom_map_entries_buffer[i].file_path);
			remove_entry_by_index(i);
			removed = true;
		}
	}

	LeaveCriticalSection(m_lock);
	return removed;
}

bool __thiscall c_custom_map_manager::delete_single_entry(const s_custom_map_entry* entry)
{
	EnterCriticalSection(m_lock);

	bool removed = false;

	for (uint16 i = 0; i < m_map_count; i++)
	{
		if (!memcmp(&m_new_custom_map_entry_list_buffer[i], entry, sizeof(s_custom_map_entry)))
		{
			remove_entry_by_index(i);
			removed = true;
			break;
		}
	}

	LeaveCriticalSection(m_lock);
	return removed;
}

bool __thiscall c_custom_map_manager::add_entry(const s_custom_map_entry* entry)
{
	EnterCriticalSection(m_lock);

	bool success = true;

	if (m_map_count < k_maximum_number_of_custom_multiplayer_maps_new && validate_entry_data(entry, 1) && !entry_is_duplicate(entry))
	{
		csmemcpy(&m_new_custom_map_entry_list_buffer[m_map_count++], entry, sizeof(s_custom_map_entry));
	}
	else
	{
		success = false;
	}

	LeaveCriticalSection(m_lock);
	return success;
}

bool __thiscall c_custom_map_manager::remove_duplicates_and_add_entry(const s_custom_map_entry* entry)
{
	remove_entries_matching_file_path(entry);
	remove_duplicates_by_map_name_and_hash(entry);
	return add_entry(entry);
}

// compared to the other function
// this reads the file and populates the map entry datas
bool __thiscall c_custom_map_manager::remove_duplicates_write_entry_data_and_add(s_custom_map_entry* entry)
{
	remove_entries_matching_file_path(entry);
	remove_duplicates_by_map_name_and_hash(entry);

	typedef bool(__thiscall* read_map_data_and_add_entry_t)(c_custom_map_manager*, s_custom_map_entry* entry);
	auto p_read_map_data_and_add_entry = Memory::GetAddressRelative<read_map_data_and_add_entry_t>(0x44CC35);

	return p_read_map_data_and_add_entry(this, entry);
}

bool open_cache_header(const wchar_t* file_path, void* cache_header_ptr, HANDLE* map_handle)
{
	typedef char(__cdecl open_cache_header_t)(const wchar_t* file_path, void* lpBuffer, HANDLE* map_handle, DWORD NumberOfBytesRead);
	auto p_open_cache_header = Memory::GetAddress<open_cache_header_t*>(0x642D0, 0x4C327);
	return p_open_cache_header(file_path, cache_header_ptr, map_handle, 0);
}

void close_cache_header(HANDLE* map_handle)
{
	typedef void (__cdecl* close_cache_header_t)(HANDLE*);
	auto p_close_cache_header = Memory::GetAddress<close_cache_header_t>(0x64C03, 0x4CC5A);
	p_close_cache_header(map_handle);
}

bool __cdecl validate_and_read_custom_map_data(s_custom_map_entry* custom_map_entry)
{
	s_cache_header header;
	HANDLE map_cache_handle;
	wchar_t* file_name = custom_map_entry->file_path;
	if (!open_cache_header(file_name, &header, &map_cache_handle))
		return false;
	if (header.header_signature != 'head' || header.footer_signature != 'foot' || header.file_size <= 0 || header.version != 8)
	{
		LOG_TRACE_FUNCW(L"\"{}\" has invalid header", file_name);
		return false;
	}
	if (header.type > 5 || header.type < 0)
	{
		LOG_TRACE_FUNCW(L"\"{}\" has bad scenario type", file_name);
		return false;
	}
	if (strnlen_s(header.name, k_max_map_name_size) >= 32 || strnlen_s(header.version_string, 32) >= 32)
	{
		LOG_TRACE_FUNCW(L"\"{}\" has invalid version or name string", file_name);
		return false;
	}
	if (header.type != scenario_type_multiplayer && header.type != scenario_type_singleplayer)
	{
		LOG_TRACE_FUNCW(L"\"{}\" is not playable", file_name);
		return false;
	}

	close_cache_header(&map_cache_handle);
	// needed because the game loads the human readable map name and description from scenario after checks
	// without this the map is just called by it's file name

	// todo move the code for loading the descriptions to our code and get rid of this
	typedef int (__cdecl* validate_and_add_custom_map_internal_t)(s_custom_map_entry*);
	auto validate_and_add_custom_map_internal = Memory::GetAddress<validate_and_add_custom_map_internal_t>(0x4F690, 0x56890);
	if (!validate_and_add_custom_map_internal(custom_map_entry))
	{
		LOG_TRACE_FUNCW(L"warning \"{}\" has bad checksums or is blacklisted, map may not work correctly", file_name);
		std::wstring fallback_name;
		if (strnlen_s(header.name, sizeof(header.name)) > 0) {
			wchar_t fallback_name_c[32];
			utf8_string_to_wchar_string(header.name, fallback_name_c, NUMBEROF(fallback_name_c));
			fallback_name.append(fallback_name_c);
		}
		else {
			std::wstring full_file_name = file_name;
			auto start = full_file_name.find_last_of('\\');
			fallback_name = full_file_name.substr(start != std::wstring::npos ? start : 0, full_file_name.find_last_not_of('.'));
		}
		wcsncpy_s(custom_map_entry->map_name, fallback_name.c_str(), fallback_name.length());
	}
	// load the map even if some of the checks failed, will still mostly work
	return true;
}

bool __thiscall c_custom_map_manager::add_custom_map_entry_by_map_file_path(const std::wstring& file_path)
{
	return add_custom_map_entry_by_map_file_path(file_path.c_str());
}

bool __thiscall c_custom_map_manager::add_custom_map_entry_by_map_file_path(const wchar_t* file_path)
{
	s_custom_map_entry custom_map_new_entry;
	ZeroMemory(&custom_map_new_entry, sizeof(s_custom_map_entry));

	wcscpy_s(custom_map_new_entry.file_path, k_map_file_path_size, file_path);

	bool map_loaded = false;
	if (validate_and_read_custom_map_data(&custom_map_new_entry)) {

		if (remove_duplicates_write_entry_data_and_add(&custom_map_new_entry)) {
			map_loaded = true;
		}
	}

	return map_loaded;
}

void __thiscall c_custom_map_manager::initialize()
{
	m_index_of_last_previewed_bitmap = 0;
	m_map_count = 0;
	m_lock = new CRITICAL_SECTION;
	InitializeCriticalSection(m_lock);
}

// atexit
void __thiscall c_custom_map_manager::cleanup()
{
	m_new_custom_map_entry_list_buffer = nullptr;
	if (m_custom_map_file_data_cache)
		delete[] m_custom_map_file_data_cache;

	m_custom_map_file_data_cache = nullptr;

	DeleteCriticalSection(m_lock);
	delete m_lock;
}

static __declspec(naked) void jmp_get_entry_by_id() { __asm jmp c_custom_map_manager::get_entry_by_id }
static __declspec(naked) void jmp_load_custom_map_data_cache() { __asm jmp c_custom_map_manager::load_custom_map_data_cache }
static __declspec(naked) void jmp_start_custom_map_sync() { __asm jmp c_custom_map_manager::start_custom_map_sync }
static __declspec(naked) void jmp_mark_all_cached_maps_for_deletion() { __asm jmp c_custom_map_manager::mark_all_cached_maps_for_deletion }
static __declspec(naked) void jmp_find_matching_entries_by_file_path() { __asm jmp c_custom_map_manager::find_matching_entries_by_file_path }
static __declspec(naked) void jmp_find_matching_entries_by_map_name_and_hash() { __asm jmp c_custom_map_manager::find_matching_entries_by_map_name_and_hash }
static __declspec(naked) void jmp_delete_single_entry() { __asm jmp c_custom_map_manager::delete_single_entry }
static __declspec(naked) void jmp_remove_duplicates_and_add_entry() { __asm jmp c_custom_map_manager::remove_duplicates_and_add_entry }
static __declspec(naked) void jmp_save_custom_map_data() { __asm jmp c_custom_map_manager::save_custom_map_data }
static __declspec(naked) void jmp_remove_marked_for_deletion() { __asm jmp c_custom_map_manager::remove_marked_for_deletion }
static __declspec(naked) void jmp_add_entry() { __asm jmp c_custom_map_manager::add_entry }
static __declspec(naked) void jmp_remove_duplicates_write_entry_data_and_add() { __asm jmp c_custom_map_manager::remove_duplicates_write_entry_data_and_add }
static __declspec(naked) void jmp_cleanup() { __asm jmp c_custom_map_manager::cleanup }
static __declspec(naked) void jmp_initialize() { __asm jmp c_custom_map_manager::initialize }
static __declspec(naked) void jmp_get_custom_map_list_ids_by_map_name() { __asm jmp c_custom_map_manager::get_custom_map_list_ids_by_map_name }

// custom map selection list code
class c_custom_game_custom_map_list // : public c_list_widget
{
public:
	//c_list_item_widget item_widget[14];

	c_custom_game_custom_map_list* __thiscall constructor_hook(int a2)
	{
		typedef c_custom_game_custom_map_list*(__thiscall* original_constructor_t)(c_custom_game_custom_map_list*, int);
		auto p_original_constructor = Memory::GetAddress<original_constructor_t>(0x25AE3B);

		// execute first part of the function
		p_original_constructor(this, a2);
		// then load the map list

		// here we replace the custom map list allocator
		DWORD thisptr = (DWORD)this;

		s_data_array** custom_map_menu_list = (s_data_array**)(thisptr + 112);

		// first we get the map count
		*custom_map_menu_list = nullptr;
		s_custom_map_id* map_ids_buffer = nullptr;
		uint32 custom_map_available_count = get_custom_map_manager()->get_custom_map_list_ids(nullptr, 0);

		if (custom_map_available_count > 0)
		{
			*custom_map_menu_list = ui_list_data_new("custom game custom maps", custom_map_available_count, sizeof(s_custom_map_id));
			data_make_valid(*custom_map_menu_list);
			map_ids_buffer = new s_custom_map_id[custom_map_available_count];
			get_custom_map_manager()->get_custom_map_list_ids(map_ids_buffer, custom_map_available_count);

			// ### FIXME ugly...
			for (uint32 i = custom_map_available_count; i-- > 0; )
			{
				s_custom_map_id* list_entry
					= &((s_custom_map_id*)(*custom_map_menu_list)->data)[DATUM_INDEX_TO_ABSOLUTE_INDEX(datum_new(*custom_map_menu_list))];
				csmemcpy(list_entry, &map_ids_buffer[i], sizeof(s_custom_map_id));
			}
		}

		if (map_ids_buffer != nullptr)
			delete[] map_ids_buffer;

		_slot_linker* linker = (_slot_linker*)(thisptr + 172);
		_slot* slot = (_slot*)(thisptr + 2032);
		linker->link(slot);

		return this;
	}
};
//ASSERT_STRUCT_SIZE(c_custom_game_custom_map_list, 3292);

static __declspec(naked) void jmp_c_custom_game_custom_map_list_constructor_hook() { __asm jmp c_custom_game_custom_map_list::constructor_hook }

void c_custom_map_manager::ApplyCustomMapExtensionLimitPatches()
{
	WriteJmpTo(Memory::GetAddressRelative(0x44CF41, 0x441819), jmp_get_entry_by_id);
	WriteJmpTo(Memory::GetAddressRelative(0x44CC30, 0x490356), jmp_load_custom_map_data_cache);
	WriteJmpTo(Memory::GetAddressRelative(0x4C19B4, 0x48F0F5), jmp_mark_all_cached_maps_for_deletion);
	WriteJmpTo(Memory::GetAddressRelative(0x4C1D65, 0x48F4A6), jmp_find_matching_entries_by_file_path);
	WriteJmpTo(Memory::GetAddressRelative(0x4C1C1F, 0x48F360), jmp_find_matching_entries_by_map_name_and_hash);
	WriteJmpTo(Memory::GetAddressRelative(0x4C2669, 0x48FDAA), jmp_delete_single_entry);
	WriteJmpTo(Memory::GetAddressRelative(0x44CDA6, 0x441677), jmp_remove_duplicates_and_add_entry);
	WriteJmpTo(Memory::GetAddressRelative(0x4C2D4D, 0x49048E), jmp_save_custom_map_data);
	WriteJmpTo(Memory::GetAddressRelative(0x4C27F3, 0x48FF34), jmp_remove_marked_for_deletion);
	WriteJmpTo(Memory::GetAddressRelative(0x4C259B, 0x48FCDC), jmp_add_entry);
	WriteJmpTo(Memory::GetAddressRelative(0x44CD1E, 0x4415EF), jmp_remove_duplicates_write_entry_data_and_add);

	PatchCall(Memory::GetAddressRelative(0x439E56, 0x40BAB8), jmp_start_custom_map_sync);

	// custom map data menu list hook/patches
	if (!Memory::IsDedicatedServer())
	{
		PatchCall(Memory::GetAddressRelative(0x64F708), jmp_c_custom_game_custom_map_list_constructor_hook);

		// jump to function end just before updating custom map list
		// jmp near 0xB7
		BYTE jmp_to_epilog_sub_65AE3B[] = { 0xE9, 0xB7, 0x00, 0x00, 0x00 };
		WriteBytes(Memory::GetAddressRelative(0x65AEFD), jmp_to_epilog_sub_65AE3B, sizeof(jmp_to_epilog_sub_65AE3B));
	}
	else
	{
		WriteJmpTo(Memory::GetAddressRelative(0x0, 0x4418F9), jmp_get_custom_map_list_ids_by_map_name);
	}

	WriteJmpTo(Memory::GetAddressRelative(0x4C18BD, 0x48EFFE), jmp_initialize);

	// replace game's atexit cleanup impl
	WriteJmpTo(Memory::GetAddressRelative(0x79A0DC, 0x73E9E5), jmp_cleanup);

	PatchCall(Memory::GetAddress(0x1E49A2, 0x1EDF0), validate_and_read_custom_map_data);
	PatchCall(Memory::GetAddress(0x4D3BA, 0x417FE), validate_and_read_custom_map_data);
	PatchCall(Memory::GetAddress(0x4CF26, 0x41D4E), validate_and_read_custom_map_data);
	PatchCall(Memory::GetAddress(0x8928, 0x1B6482), validate_and_read_custom_map_data);

	WriteJmpTo(Memory::GetAddress(0x1467, 0x12E2), scenario_is_supported_build);
}

// '50 map limit removal' region end
#pragma endregion