#pragma once
#include "player_profile.h"

/* constants */

#define k_maximum_enumerated_saved_game_files_any_type_per_memory_unit 4096
#define k_maximum_enumerated_default_save_game_files 101
#define k_maximum_enumerated_cached_save_game_files 4

#define ENUMERATED_INDEX_IS_DEFAULT_SAVE(enumerated_file_index) \
	(((enumerated_file_index) & 0x200000) != 0)

/* enums */

enum e_saved_game_file_type : int32
{
	_saved_game_file_type_profile = 0,
	_saved_game_file_type_game_variant_slayer,
	_saved_game_file_type_game_variant_koth,
	_saved_game_file_type_game_variant_race,
	_saved_game_file_type_game_variant_oddball,
	_saved_game_file_type_game_variant_juggernaut,
	_saved_game_file_type_game_variant_headhunter,
	_saved_game_file_type_game_variant_ctf,
	_saved_game_file_type_game_variant_assault,
	_saved_game_file_type_game_variant_territories,
	k_number_of_saved_game_file_types,

	_saved_game_file_type_invalid = NONE,
};

/* structures */

struct s_saved_game_file_loading_information
{
	int8 data[591];
	wchar_t file_path[MAX_PATH];
	wchar_t meta_information[128];
};
ASSERT_STRUCT_SIZE(s_saved_game_file_loading_information, 1368);

struct s_saved_game_main_menu_globals_default_save_file
{
	e_saved_game_file_type type;

	// buffer is the size of the largest possible save file type
	int8 buffer[sizeof(s_saved_game_player_profile)];
};
ASSERT_STRUCT_SIZE(s_saved_game_main_menu_globals_default_save_file, 0x120C);

struct s_saved_game_main_menu_globals_save_file_info
{
	wchar_t file_path[256];
	uint32 unk_1;
	int32 unk_2;
	wchar_t display_name[16];
	uint32 unk_3;
	e_saved_game_file_type type;
	uint32 unk_5;
};
ASSERT_STRUCT_SIZE(s_saved_game_main_menu_globals_save_file_info, 0x234);


struct s_saved_game_files_globals_cached_game_save
{
	uint32 enumerated_index;
	s_saved_game_main_menu_globals_save_file_info file_info;
};

struct s_saved_game_main_menu_globals
{
	c_static_array_tracked<s_saved_game_main_menu_globals_default_save_file, k_maximum_enumerated_default_save_game_files> default_save_files;
	c_static_array_tracked<s_saved_game_main_menu_globals_save_file_info, k_maximum_enumerated_saved_game_files_any_type_per_memory_unit> save_files;
	datum saved_game_file_index_salt;
	int8 gap2[131072];
};
ASSERT_STRUCT_SIZE(s_saved_game_main_menu_globals, 2907848);

struct s_saved_game_files_globals
{
	bool async_io_complete;
	bool unk_1;
	bool memory_initialized_for_game;
	bool cache_files_exist;
	int32 unk_3;
	int32 unk_4;
	int32 unk_5;
	int32 unk_6;
	c_static_array_tracked<s_saved_game_files_globals_cached_game_save, k_maximum_enumerated_cached_save_game_files> cached_save_files;
	bool meta_data_lock_done;
	int32 unk_7;
	int32 locked_thread_id;
	int32 unk_8;
};
ASSERT_STRUCT_SIZE(s_saved_game_files_globals, 2312);

/* prototypes */

s_saved_game_main_menu_globals* saved_game_main_menu_globals_get();

s_saved_game_files_globals* saved_game_files_globals_get();

bool saved_games_get_file_info(s_saved_game_main_menu_globals_save_file_info* out_info, uint32 enumerated_index);

const wchar_t* saved_games_get_file_type_as_string(e_saved_game_file_type file_type);

bool saved_games_append_file_type_to_path(wchar_t* in_path, e_saved_game_file_type file_type, wchar_t* out_path);

void saved_games_get_display_name(uint32 enumerated_index, wchar_t* display_name);

void saved_game_files_apply_hooks();