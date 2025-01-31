#pragma once

extern bool g_force_cartographer_update;

void InitH2Config();
void DeinitH2Config();
void SaveH2Config();
void ReadH2Config();

#ifndef _CARTOGRAPHER_DLL_CONF
// temporary config files 
// for testing purposes
#define USE_DEV_PREVIEW_CONFIG_FILE_PATHS 1
#endif

struct _H2Config_language {
	int code_main;
	int code_variant;
};

enum e_override_texture_resolution : int
{
	tex_low,
	tex_default,
	tex_high,
	tex_ultra
};

enum H2Config_Experimental_Rendering_Mode : byte
{
	_rendering_mode_none,
	_rendering_mode_original_game_frame_limit
};

#define k_cartographer_url "https://cartographer.online"
extern const char* k_cartographer_map_repo_url;

extern unsigned long H2Config_master_ip;
extern unsigned short H2Config_master_port_login;
extern unsigned short H2Config_master_port_relay;

extern bool H2Portable;
extern bool H2Config_isConfigFileAppDataLocal;
extern unsigned short H2Config_base_port;
extern unsigned long H2Config_ip_lan;
extern unsigned long H2Config_ip_broadcast_override;

extern _H2Config_language H2Config_language;
extern bool H2Config_custom_labels_capture_missing;
extern bool H2Config_skip_intro;
extern bool H2Config_discord_enable;
extern bool H2Config_controller_aim_assist;
extern int H2Config_fps_limit;
extern int8 H2Config_static_lod_state;
extern bool H2Config_shader_lod_max;
extern bool H2Config_light_suppressor;
extern bool H2Config_disable_ingame_keyboard;
extern bool H2Config_hide_ingame_chat;
extern bool H2Config_xDelay;
extern bool H2Config_voice_chat;
extern char H2Config_dedi_server_name[XUSER_NAME_SIZE];
extern char H2Config_dedi_server_playlist[256];
extern int H2Config_additional_pcr_time;
extern bool H2Config_debug_log;
extern int H2Config_debug_log_level;
extern bool H2Config_debug_log_console;
extern char H2Config_login_identifier[255];
extern char H2Config_login_password[255];
extern short H2Config_team_bit_flags;
extern bool H2Config_team_flag_array[8];
extern char H2Config_stats_authkey[32 + 1];
extern bool H2Config_vip_lock;
extern bool H2Config_even_shuffle_teams;
extern bool H2Config_koth_random;
extern H2Config_Experimental_Rendering_Mode H2Config_experimental_fps;

extern int H2Config_hotkeyIdHelp;
extern int H2Config_hotkeyIdAlignWindow;
extern int H2Config_hotkeyIdWindowMode;
extern int H2Config_hotkeyIdToggleHideIngameChat;
extern int H2Config_hotkeyIdGuide;
extern int H2Config_hotkeyIdConsole;
extern int H2Config_minimum_player_start;

extern e_override_texture_resolution H2Config_Override_Shadows;
extern e_override_texture_resolution H2Config_Override_Water;

extern bool H2Config_upnp_enable;
extern bool H2Config_no_events;
extern bool H2Config_spooky_boy;

extern bool H2Config_force_off_d3d9ex;
extern bool H2Config_force_off_sm3;

#ifndef NDEBUG
extern int H2Config_forced_event;
#endif

