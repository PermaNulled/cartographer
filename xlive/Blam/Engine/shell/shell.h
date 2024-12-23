#pragma once

enum e_shell_tool_type
{
	_shell_tool_type_editing_tools = 0,
	_shell_tool_type_game = 2
};

enum e_shell_command_line_flags : int32
{
	_shell_command_line_flag_windowed,
	_shell_command_line_flag_disable_voice_chat,
	_shell_command_line_flag_nosound,
	_shell_command_line_flag_d3d9ex_enabled, // allows the d3d ex version to be used instead
	_shell_command_line_flag_disable_hardware_vertex_processing, // force hardware vertex processing off
	_shell_command_line_flag_novsync,
	_shell_command_line_flag_unlock_xbox_live_menus, // allows access to xbox live menus even in offline
	_shell_command_line_flag_nointro, // disables intro movie
	_shell_command_line_flag_unk8, // some tag thing?
	_shell_command_line_flag_unk9, // some tag thing?
	_shell_command_line_flag_unk10, // some tag thing?
	_shell_command_line_flag_unk11, // some tag thing?
	_shell_command_line_flag_unk12, // some tag thing?
	_shell_command_line_flag_unk13, // some tag thing?
	_shell_command_line_flag_unk14, // some tag thing?
	_shell_command_line_flag_unk15,
	_shell_command_line_flag_unk16, // fuzzer/automated testing? (sapien)
	_shell_command_line_flag_xbox_live_silver_account, // if true, disables 'gold-only' features, like quickmatch etc
	_shell_command_line_flag_custom_map_entry_test_map_name_instead_of_hash, // flag to test the map name instead of the hash of the custom map
	_shell_command_line_flag_ui_fast_test_no_start, // same as below but doesn't start a game
	_shell_command_line_flag_ui_fast_test, // auto navigates the UI selecting the default option
	_shell_command_line_flag_unk21, // player controls related, is checked when using a vehicle
	_shell_command_line_flag_monitor_count,
	_shell_command_line_flag_unk23,
	_shell_command_line_flag_unk24,
	_shell_command_line_flag_unk25, // something to do with game time?
	_shell_command_line_flag_unk26,
	_shell_command_line_flag_unk27, // network? value seems unused?
	_shell_command_line_flag_high_quality, // forced sound reverb ignoring CPU score and disable forcing low graphical settings (sapien)

	k_number_of_shell_command_line_flags
};


void shell_apply_patches(void);

e_shell_tool_type shell_tool_type(void);

bool shell_command_line_flag_is_set(e_shell_command_line_flags flag);
int32 __cdecl shell_command_line_flag_get(e_shell_command_line_flags flag);
void shell_command_line_flag_set(e_shell_command_line_flags flag, int32 state);

bool shell_initialize(void);

void shell_dispose(void);

void __cdecl timing_initialize(int32 a1);