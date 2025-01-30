#include "stdafx.h"
#include "Config.h"

#include "cartographer/twizzler/twizzler.h"
#include "render/render_lod_new.h"
#include "rasterizer/dx9/rasterizer_dx9.h"
#include "rasterizer/dx9/rasterizer_dx9_9on12.h"

#include "H2MOD/GUI/imgui_integration/Console/ComVar.h"
#include "H2MOD/GUI/imgui_integration/Console/ImGui_ConsoleImpl.h"

#include "H2MOD/Modules/OnScreenDebug/OnscreenDebug.h"
#include "H2MOD/Modules/Shell/H2MODShell.h"
#include "H2MOD/Modules/Shell/Startup/Startup.h"
#include "H2MOD/Utils/Utils.h"
#include "Util/SimpleIni.h"

#pragma region Config IO

#define k_h2config_version_number "1"
#define k_h2config_version_section "H2ConfigurationVersion:" k_h2config_version_number
#define k_h2config_version_debug_section "Debug:" k_h2config_version_number

bool g_force_cartographer_update = false;

const wchar_t* H2ConfigFilenames[] = { L"%wshalo2config%d.ini", L"%wsh2serverconfig%d.ini" };

unsigned long H2Config_master_ip = inet_addr("149.56.81.89");
unsigned short H2Config_master_port_login = 27020;
unsigned short H2Config_master_port_relay = 1001;

//config variables
bool H2Portable = false;//TODO

std::string cartographerURL = "https://cartographer.online";
std::string cartographerMapRepoURL = "http://www.h2maps.net/Cartographer/CustomMaps";

unsigned short H2Config_base_port = 2000;
unsigned long H2Config_ip_lan = htonl(INADDR_NONE);
unsigned long H2Config_ip_broadcast_override = htonl(INADDR_BROADCAST);
_H2Config_language H2Config_language = { -1, 0 };
bool H2Config_custom_labels_capture_missing = false;
bool H2Config_skip_intro = false;
bool H2Config_discord_enable = true;
//bool H2Config_controller_aim_assist = true;
int H2Config_fps_limit = 60;
int8 H2Config_static_lod_state = _render_lod_disabled;
bool H2Config_disable_ingame_keyboard = false;
bool H2Config_hide_ingame_chat = false;
bool H2Config_xDelay = true;
bool H2Config_shader_lod_max = false;
bool H2Config_light_suppressor = false;
bool H2Config_voice_chat = false;
char H2Config_dedi_server_name[XUSER_NAME_SIZE] = { "" };
char H2Config_dedi_server_playlist[256] = { "" };
int H2Config_additional_pcr_time = 25;
bool H2Config_debug_log = false;
int H2Config_debug_log_level = 2;
bool H2Config_debug_log_console = false;
char H2Config_login_identifier[255] = { "" };
char H2Config_login_password[255] = { "" };
int H2Config_minimum_player_start = 0;
char H2Config_team_bit_flags_str[] = "1-1-1-1-1-1-1-1";
bool H2Config_team_flag_array[8];
short H2Config_team_bit_flags = 0xFF;
char H2Config_stats_authkey[32 + 1] = { "" };
bool H2Config_vip_lock = false;
bool H2Config_even_shuffle_teams = false;
bool H2Config_koth_random = true;
bool H2Config_intel_sky_hack = false;
H2Config_Experimental_Rendering_Mode H2Config_experimental_fps = _rendering_mode_none;

// ### TODO FIXME remove CSimpleIniA garbage
// for now improve the code by simplifying it
template<typename T>
static std::enable_if_t<!std::is_same_v<T, bool> && std::is_integral_v<T>, bool>
	get_config_entry(CSimpleIniA* simple_ini, const char* section_key, const char* config_name, const char* default_setting, T* out_value)
{
	bool result = false;
	std::string exception;
	std::string str_value = simple_ini->GetValue(section_key, config_name, default_setting);
	if (ComVar(out_value).SetFromStr(str_value, 0, exception))
	{
		//CartographerConsole::LogToTab(_console_tab_logs, "config: success setting \"%s\"", config_name);
		result = true;
	}
	else
	{
		CartographerConsole::LogToTab(_console_tab_logs, "get config: %s, default: %s - threw exception: [%s]", config_name, default_setting, exception.c_str());
	}

	return result;
}

template<typename T>
static std::enable_if_t<std::is_same_v<T, bool>, bool>
	get_config_entry(CSimpleIniA* simple_ini, const char* section_key, const char* config_name, const char* default_setting, T* out_value)
{
	bool result = false;
	std::string exception;
	std::string str_value = simple_ini->GetValue(section_key, config_name, default_setting);
	if (ComVar(out_value).SetFromStr(str_value, exception))
	{
		//CartographerConsole::LogToTab(_console_tab_logs, "config: success setting \"%s\"", config_name);
		result = true;
	}
	else
	{
		CartographerConsole::LogToTab(_console_tab_logs, "get config: %s, default: %s - threw exception: [%s]", config_name, default_setting, exception.c_str());
	}

	return result;
}

template<typename T>
static std::enable_if_t<std::is_same_v<std::remove_all_extents_t<T>, char>, bool>
get_config_entry(CSimpleIniA* simple_ini, const char* section_key, const char* config_name, const char* default_setting, const T** out_value)
{
	bool result = false;
	*out_value = simple_ini->GetValue(section_key, config_name, default_setting);
	result = true;
	return result;
}

#define CONFIG_GET(_simple_ini, _config_name, _default_setting, _out_value) \
	get_config_entry(_simple_ini, k_h2config_version_section, _config_name, _default_setting, _out_value)

#define CONFIG_GET_DEBUG_KEY(_simple_ini, _config_name, _default_setting, _out_value) \
	get_config_entry(_simple_ini, k_h2config_version_debug_section, _config_name, _default_setting, _out_value)

template<typename T>
static std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, char>, bool>
set_config_entry(CSimpleIniA* simple_ini, const char* section_key, const char* config_name, T* value, const char* comment = NULL)
{
	bool result = true;
	std::string value_as_string;

	try
	{
		value_as_string = ComVar(value).AsString();
	}
	catch (...)
	{
		result = false;
		CartographerConsole::LogToTab(_console_tab_logs, "set config: %s, threw exception!", config_name);
	}

	if (result)
	{
		simple_ini->SetValue(section_key, config_name, value_as_string.c_str(), comment);
	}

	return result;
}

template<typename T>
static std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, char>, bool>
set_config_entry(CSimpleIniA* simple_ini, const char* section_key, const char* config_name, T value, const char* comment = NULL)
{
	return set_config_entry(simple_ini , section_key, config_name, &value, comment);
}

template<typename T>
static std::enable_if_t<std::is_same_v<std::remove_all_extents_t<T>, char>, bool>
set_config_entry(CSimpleIniA* simple_ini, const char* section_key, const char* config_name, const T* value, const char* comment = NULL)
{
	bool result = true;
	simple_ini->SetValue(section_key, config_name, value, comment);
	return result;
}

// sets a config with comment
#define CONFIG_SET_C(_simple_ini, _config_name, _value, _comment) \
	set_config_entry(_simple_ini, k_h2config_version_section, _config_name, _value, _comment)

// and without
#define CONFIG_SET(_simple_ini, _config_name, _value) \
	set_config_entry(_simple_ini, k_h2config_version_section, _config_name, _value)

#define CONFIG_SET_DEBUG_KEY(_simple_ini, _config_name, _value) \
	set_config_entry(_simple_ini, k_h2config_version_debug_section, _config_name, _value)

e_override_texture_resolution H2Config_Override_Shadows;
e_override_texture_resolution H2Config_Override_Water;

bool H2Config_upnp_enable = true;
bool H2Config_no_events = false;
bool H2Config_spooky_boy = true;
#ifndef NDEBUG
int H2Config_forced_event = 0;
#endif

bool H2Config_force_off_d3d9ex = false;
bool H2Config_force_off_sm3 = false;

int H2Config_hotkeyIdHelp = VK_F2;
int H2Config_hotkeyIdAlignWindow = VK_F7;
int H2Config_hotkeyIdWindowMode = VK_F8;
int H2Config_hotkeyIdToggleHideIngameChat = VK_F9;
int H2Config_hotkeyIdGuide = VK_HOME;
int H2Config_hotkeyIdConsole = VK_F10;

bool ownsConfigFile = false;
bool H2Config_isConfigFileAppDataLocal = false;

void SaveH2Config() {
	addDebugText("Saving H2Configuration File...");

	if (!Memory::IsDedicatedServer()) {
		extern int current_language_main;
		extern int current_language_sub;
		H2Config_language.code_main = current_language_main;
		H2Config_language.code_variant = current_language_sub;
	}

	wchar_t fileConfigPath[1024];
	if (FlagFilePathConfig) {
		swprintf(fileConfigPath, ARRAYSIZE(fileConfigPath), FlagFilePathConfig);
	}
	else if (H2Portable || !H2Config_isConfigFileAppDataLocal) {
		swprintf(fileConfigPath, ARRAYSIZE(fileConfigPath), H2ConfigFilenames[Memory::IsDedicatedServer()], H2ProcessFilePath, _Shell::GetInstanceId());
	}
	else {
		swprintf(fileConfigPath, ARRAYSIZE(fileConfigPath), H2ConfigFilenames[Memory::IsDedicatedServer()], H2AppDataLocal, _Shell::GetInstanceId());
	}

	addDebugText(L"Saving config: \"%ws\"", fileConfigPath);
	FILE* fileConfig = nullptr;
	errno_t err = _wfopen_s(&fileConfig, fileConfigPath, L"wb");

	if (err != 0) {
		_Shell::FileErrorDialog(err);
		addDebugText("ERROR: Unable to write H2Configuration file!");
	}
	else {
#pragma region Put Data To File
		CSimpleIniA ini;
		ini.SetUnicode();

		fprintf(fileConfig,
			"#--- Halo 2 Project Cartographer Configuration File ---"
			"\n\n"
			"# h2portable Options:"
			"\n# 0 - Config files are read from executable's launch directory then AppDataLocal if missing. Will write to AppDataLocal if not read from the execution directory."
			"\n# 1 - All config files are read and written to the executable's launch directory (however will still scan and read from AppDataLocal if missing)."
			"\n\n"
			"# base_port Options:"
			"\n# <1 - 65526> - The port the game binds to including any of the nine (9) afterward: UDP and/or TCP (Upper limit: 65535 - 9 = 65526)."
			"\n\n"

			"# wan_ip Options:"
			"\n# lan_ip Options:"
			"\n# This option is used for when you cannot join games hosted on the same local network due to NAT issues."
			"\n# Configuring these settings for an internal network address avoids the requirement for that host user to port forward."
			"\n# <IPv4> - External IP Address of the local / internal network user you are trying to connect to. If blank, the External IP returned from the Master Login is used."
			"\n# <IPv4> - Internal IP Address of the local / internal network user you are trying to connect to."
			"\n\n"
		);

		if (!Memory::IsDedicatedServer()) {
			fprintf(fileConfig,
				"# language_code Options (Client):"
				"\n# <main>x<variant> - Sets the main/custom language for the game."
				"\n# --- <main> ---"
				"\n# -1 - System Default"
				"\n# 0  - English"
				"\n# 1  - Japanese"
				"\n# 2  - German"
				"\n# 3  - French"
				"\n# 4  - Spanish"
				"\n# 5  - Italian"
				"\n# 6  - Korean"
				"\n# 7  - Chinese"
				"\n# --- <variant> ---"
				"\n# 0  - Default"
				"\n\n"

				"# language_label_capture Options (Client):"
				"\n# Capture new labels not in the custom language file under the language currently in use."
				"\n# 0 - Ignore listening for unrecorded labels."
				"\n# 1 - Listen for and record any labels/strings not seen before."
				"\n\n"

				"# skip_intro Options (Client):"
				"\n# 0 - Normal Intro."
				"\n# 1 - No Intro."
				"\n\n"

				"# discord_enable Options (Client):"
				"\n# 0 - Disables Discord Rich Presence."
				"\n# 1 - Enables Discord Rich Presence."
				"\n\n"

				"# fps_limit Options (Client):"
				"\n# <uint> - 0 disables the built in frame limiter. >0 is the fps limit of the game."
				"\n\n"

				"# static_lod_state Options (Client):"
				"\n# 0 - Disables the Level of Detail level enforcement for models."
				"\n# The following describes each Level of Detail setting:"
				"\n# 1 - L1 - Very Low"
				"\n# 2 - L2 - Low"
				"\n# 3 - L3 - Medium"
				"\n# 4 - L4 - High"
				"\n# 5 - L5 - Very High"
				"\n# 6 - L6 - Cinematic"
				"\n\n"

				"# Force Max Shader LOD Options (Client):"
				"\n# 0 - Disable shader_lod_max patch. game uses default shader lod settings"
				"\n# 1 - Enable shader_lod_max patch. game uses highest quality shaders at all times"
				"\n\n"

				"# Disable Light Suppresion Options (Client):"
				"\n# 0 - Disable light_suppressor patch. game suppresses lights when multiple of them are onscreen"
				"\n# 1 - Enable light_suppressor patch. game dosen't suppress lights when multiple of them are onscreen"
				"\n\n"

				"# disable_ingame_keyboard Options (Client):"
				"\n# 0 - Normal Game Controls."
				"\n# 1 - Disables ONLY Keyboard when in-game & allows controllers when game is not in focus."
				"\n\n"

				"# hide_ingame_chat Options (Client):"
				"\n# 0 - In-game chat is displayed normally."
				"\n# 1 - In-game chat is hidden."
				"\n\n"
			);
		}

		fprintf(fileConfig,
			"# enable_xdelay Options:"
			"\n# 0 - Non-host players cannot delay the game start countdown timer."
			"\n# 1 - Non-host players can delay the game start countdown timer (native default)."
			"\n\n"
		);

		fprintf(fileConfig,
			"# debug_log Options:"
			"\n# 0 - Disables logging."
			"\n# 1 - Enables logging."
			"\n\n"

			"# debug_log_level Options:"
			"\n# 0 - Trace, tell me *everything*."
			"\n# 1 - Debug, give me the dirty details."
			"\n# 2 - Info, occasionally helpful information."
			"\n# 3 - Warning, what probably shouldn't be happening."
			"\n# 4 - Error, bad news only, please."
			"\n# 5 - Critical, I only want to see death and destruction."
			"\n\n"

			"# debug_log_console Options:"
			"\n# 0 - Disables console window logging."
			"\n# 1 - Enables console window logging, will display all output from all loggers."
			"\n\n"
		);

		if (Memory::IsDedicatedServer()) {
			fprintf(fileConfig,
				"# server_name Options (Server):"
				"\n# Sets the name of the server up to 15 characters long."
				"\n# Leave blank/empty for no effect."
				"\n\n"

				"# server_playlist Options (Server):"
				"\n# Sets the playlist of the server up to 255 characters long."
				"\n# Leave blank/empty for no effect."
				"\n\n"

				"# login_identifier Options (Server):"
				"\n# The email or username used to login to an account."
				"\n# Note: Server accounts *should not* be signed into multiple times concurrently *unless* it is all on the same computer (i.e. only exempt when running multiple server instances)."
				"\n\n"

				"# login_password Options (Server):"
				"\n# The password used to login to the defined account."
				"\n\n"

				"# additional_pcr_time Options (Server):"
				"\n# By default, 25 seconds are added to post game carnage time from the playlist setting."
				"\n# Now you have the possibility to change it to your preference."
				"\n\n"
        
		        "# minimum_player_start options (Server):"
		        "\n# Changes the starting behaviour of the countdown, setting this to any value (1-16) will cause the"
		        "\n# Server to not start until the player count is equal to or above the given value. A value of 0 will disable this setting."
		        "\n\n"

				"# vip_lock (Server):"
				"\n# This flag tells the server to lock the game to VIP mode when the game starts"
				"\n# Players who are in the lobby when the game starts are added to VIP and can rejoin if there are connection issues"
				"\n# The VIP list will be cleared when the lobby reaches Post game"
				"\n\n"

				"# shuffle_even_teams (Server):"
				"\n# This flag tells the server to force even and shuffle teams before starting"
				"\n# The server will automatically organize teams before starting if the game is uneven"
				"\n# This setting is dependent on the settings for team_enable_bit_flags and the servers current max players."
				"\n# The server will attempt to fill each team with an equal amount of players for every enabled team."
				"\n# As an example if the server allows 8 players to join and has 2 teams enabled it will place 4 players in each team before the game can start"
				"\n# If you configure these settings to not line up to an equal amount of players on each team it will not start."
				"\n# As an example if you set the player count to 4 and have 3 teams enabled you will never get an even amount of players on each team."
				"\n\n"

				"# koth_random (Server):"
				"\n# This flag tells which behaviour the koth will use for getting the next hill"
				"\n# True (default) will have the server select the hill randomly"
				"\n# false will have the server select the hill in order"
				"\n\n"

				"# enable_anti_cheat (Server):"
				"\n# This flag will enable anti-cheat on your server."
				"\n\n"
			);
      
		}

		if (!Memory::IsDedicatedServer()) {
			fprintf(fileConfig,
				"# hotkey_... Options (Client):"
				"\n# The number used is the keyboard Virtual-Key (VK) Code in base-10 integer form."
				"\n# The codes in hexadecimal (base-16) form can be found here:"
				"\n# https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx"
				"\n\n"
			);
		}

		CONFIG_SET(&ini, "h2portable", &H2Portable);
		CONFIG_SET(&ini, "base_port", &H2Config_base_port);

		ComVarAddrIpv4 address_lan(&H2Config_ip_lan);
		bool lanaddr_override_valid = H2Config_ip_lan != htonl(INADDR_NONE) && H2Config_ip_lan != htonl(INADDR_ANY);
		CONFIG_SET(&ini, "lan_ip", lanaddr_override_valid ? address_lan.AsString().c_str() : "");

		ComVarAddrIpv4 address_broadcast(&H2Config_ip_broadcast_override);
		bool broadcast_override_valid = H2Config_ip_broadcast_override != htonl(INADDR_ANY);
		CONFIG_SET(&ini, "broadcast_ip", broadcast_override_valid ? address_broadcast.AsString().c_str() : "255.255.255.255");

		CONFIG_SET(&ini, "upnp", &H2Config_upnp_enable);

		if (!Memory::IsDedicatedServer()) {
			std::string lang_str(std::to_string(H2Config_language.code_main) + "x" + std::to_string(H2Config_language.code_variant));
			CONFIG_SET(&ini, "language_code", lang_str.c_str());
		}

		if (!Memory::IsDedicatedServer()) {
			CONFIG_SET(&ini, "language_label_capture", &H2Config_custom_labels_capture_missing);

			CONFIG_SET(&ini, "skip_intro", &H2Config_skip_intro);

			CONFIG_SET(&ini, "discord_enable", &H2Config_discord_enable);

			CONFIG_SET(&ini, "fps_limit", &H2Config_fps_limit);

			CONFIG_SET(&ini, "static_lod_state", &H2Config_static_lod_state);

			CONFIG_SET(&ini, "shader_lod_max", &H2Config_shader_lod_max);

			CONFIG_SET(&ini, "light_suppressor", &H2Config_light_suppressor);

			CONFIG_SET(&ini, "disable_ingame_keyboard", &H2Config_disable_ingame_keyboard);

			CONFIG_SET(&ini, "hide_ingame_chat", &H2Config_hide_ingame_chat);

			CONFIG_SET(&ini, "override_shadows", std::to_string(H2Config_Override_Shadows).c_str());
			CONFIG_SET(&ini, "override_water", std::to_string(H2Config_Override_Water).c_str());

			CONFIG_SET(&ini, "no_events", &H2Config_no_events);

			CONFIG_SET(&ini, "skeleton_biped", &H2Config_spooky_boy);
#ifndef NDEBUG
			CONFIG_SET(&ini, "forced_event", &H2Config_forced_event);
#endif
			CONFIG_SET(&ini, "force_off_d3d9ex", &H2Config_force_off_d3d9ex);
			CONFIG_SET(&ini, "force_off_sm3", &H2Config_force_off_sm3);
			CONFIG_SET(&ini, "use_d3d9on12", &g_rasterizer_dx9on12_enabled);
			CONFIG_SET(&ini, "disable_amd_or_ati_patches", &g_rasterizer_dx9_driver_globals.disable_amd_or_ati_patches);
			CONFIG_SET(&ini, "intel_sky_hack", &H2Config_intel_sky_hack);
		}

		CONFIG_SET(&ini, "enable_xdelay", &H2Config_xDelay);

		if (Memory::IsDedicatedServer()) {
			CONFIG_SET(&ini, "server_name", H2Config_dedi_server_name);

			CONFIG_SET(&ini, "server_playlist", H2Config_dedi_server_playlist);

			CONFIG_SET(&ini, "minimum_player_start", &H2Config_minimum_player_start);
			CONFIG_SET(&ini, "additional_pcr_time", &H2Config_additional_pcr_time);

			CONFIG_SET(&ini, "vip_lock", &H2Config_vip_lock);
			CONFIG_SET(&ini, "shuffle_even_teams", &H2Config_even_shuffle_teams);
			CONFIG_SET(&ini, "koth_random", &H2Config_koth_random);
			CONFIG_SET(&ini, "enable_anti_cheat", &g_twizzler_status);

			CONFIG_SET(&ini, "login_identifier", H2Config_login_identifier);
			CONFIG_SET(&ini, "login_password", H2Config_login_password);

			CONFIG_SET_C(&ini, "stats_auth_key", H2Config_stats_authkey,
				"# DO NOT CHANGE THIS OR YOUR SERVER WILL NO LONGER TRACK STATS");

			CONFIG_SET_C(&ini, "teams_enabled_bit_flags", H2Config_team_bit_flags_str,
				"# teams_enabled_bit_flags (Server)"
				"\n# By default, the game reads team bitflags from the current map."
				"\n# With this option, you can enable which teams are enabled."
				"\n# Each bit corresponds to a team. Example below where we disable Blue, Green and Pink teams:"
				"\n# Teams:   Red  Blue  Yellow  Green  Purple  Orange  Brown  Pink"
				"\n#           |     |     |       |       |       |      |      |  "
				"\n#          \\/    \\/    \\/      \\/      \\/      \\/     \\/     \\/  "
				"\n#           1  -  0  -  1   -   0   -   1   -   1  -   1  -  0"
				"\n");
		}

		if (!Memory::IsDedicatedServer()) {

			CONFIG_SET_C(&ini, "hotkey_help", H2Config_hotkeyIdHelp, std::string("# " + GetVKeyCodeString(H2Config_hotkeyIdHelp)).c_str());
			CONFIG_SET_C(&ini, "hotkey_align_window", H2Config_hotkeyIdAlignWindow, std::string("# " + GetVKeyCodeString(H2Config_hotkeyIdAlignWindow)).c_str());
			CONFIG_SET_C(&ini, "hotkey_window_mode", H2Config_hotkeyIdWindowMode, std::string("# " + GetVKeyCodeString(H2Config_hotkeyIdWindowMode)).c_str());
			CONFIG_SET_C(&ini, "hotkey_hide_ingame_chat", H2Config_hotkeyIdToggleHideIngameChat, std::string("# " + GetVKeyCodeString(H2Config_hotkeyIdToggleHideIngameChat)).c_str());
			CONFIG_SET_C(&ini, "hotkey_guide", H2Config_hotkeyIdGuide, std::string("# " + GetVKeyCodeString(H2Config_hotkeyIdGuide)).c_str());
			CONFIG_SET_C(&ini, "hotkey_console", H2Config_hotkeyIdConsole, std::string("# " + GetVKeyCodeString(H2Config_hotkeyIdConsole)).c_str());
		}

		CONFIG_SET_DEBUG_KEY(&ini, "debug_log", &H2Config_debug_log);
		CONFIG_SET_DEBUG_KEY(&ini, "debug_log_level", &H2Config_debug_log_level);
		CONFIG_SET_DEBUG_KEY(&ini, "debug_log_console", &H2Config_debug_log_console);

		ini.SaveFile(fileConfig);

		fputs("\n", fileConfig);
#pragma endregion
		fclose(fileConfig);
	}

	addDebugText("End saving H2Configuration file.");
}


void ReadH2Config() {
	addDebugText("Reading H2Configuration file...");

	int readInstanceIdFile = _Shell::GetInstanceId();
	wchar_t local[1024];
	wcscpy_s(local, ARRAYSIZE(local), H2AppDataLocal);
	H2Config_isConfigFileAppDataLocal = false;

	errno_t err = 0;
	FILE* fileConfig = nullptr;
	wchar_t fileConfigPath[1024];

	if (FlagFilePathConfig) {
		wcscpy_s(fileConfigPath, ARRAYSIZE(fileConfigPath), FlagFilePathConfig);
		addDebugText(L"Reading flag config: \"%ws\"", fileConfigPath);
		err = _wfopen_s(&fileConfig, fileConfigPath, L"rb");
	}
	else {
		do {
			wchar_t* checkFilePath = H2ProcessFilePath;
			if (H2Config_isConfigFileAppDataLocal) {
				checkFilePath = local;
			}
			swprintf(fileConfigPath, ARRAYSIZE(fileConfigPath), H2ConfigFilenames[Memory::IsDedicatedServer()], checkFilePath, readInstanceIdFile);
			addDebugText(L"Reading config: \"%ws\"", fileConfigPath);
			err = _wfopen_s(&fileConfig, fileConfigPath, L"rb");

			if (err) {
				addDebugText("H2Configuration file does not exist, error code: 0x%x", err);
			}
			H2Config_isConfigFileAppDataLocal = !H2Config_isConfigFileAppDataLocal;
			if (err && !H2Config_isConfigFileAppDataLocal) {
				--readInstanceIdFile;
			}
		} while (err && readInstanceIdFile > 0);
		H2Config_isConfigFileAppDataLocal = !H2Config_isConfigFileAppDataLocal;
	}

	if (err) {
		addDebugText("ERROR: No H2Configuration files could be found!");
		g_force_cartographer_update = true;
		H2Config_isConfigFileAppDataLocal = true;
	}
	else {
		ownsConfigFile = (readInstanceIdFile == _Shell::GetInstanceId());

		if (!Memory::IsDedicatedServer()) {
			extern int current_language_main;
			extern int current_language_sub;
			H2Config_language.code_main = current_language_main;
			H2Config_language.code_variant = current_language_sub;
		}

		CSimpleIniA ini;
		ini.SetUnicode();

		SI_Error rc = ini.LoadFile(fileConfig);
		if (rc < 0)
		{
			addDebugText("ini.LoadFile() failed with error: %d while trying to read configuration file!", (int)rc);
		}
		else
		{
			CONFIG_GET(&ini, "h2portable", "false", &H2Portable);
			CONFIG_GET(&ini, "base_port", "2000", &H2Config_base_port);
			CONFIG_GET(&ini, "upnp", "true", &H2Config_upnp_enable);
			CONFIG_GET(&ini, "enable_xdelay", "true", &H2Config_xDelay);

			CONFIG_GET_DEBUG_KEY(&ini, "debug_log", "false", &H2Config_debug_log);
			CONFIG_GET_DEBUG_KEY(&ini, "debug_log_level", "2", &H2Config_debug_log_level);
			CONFIG_GET_DEBUG_KEY(&ini, "debug_log_console", "false", &H2Config_debug_log_console);

			const char* ip_lan = nullptr;
			CONFIG_GET(&ini, "lan_ip", "", &ip_lan);
			H2Config_ip_lan = htonl(INADDR_NONE);
			if (ip_lan)
			{
				bool lan_addr_override_valid = 
					strnlen_s(ip_lan, 15) >= 7 
					&& (inet_addr(ip_lan) != htonl(INADDR_NONE) || inet_addr(ip_lan) != htonl(INADDR_ANY));

				if (lan_addr_override_valid)
				{
					H2Config_ip_lan = inet_addr(ip_lan);
				}
			}

			const char* ip_broadcast = nullptr;
			CONFIG_GET(&ini, "broadcast_ip", "255.255.255.255", &ip_broadcast);
			H2Config_ip_broadcast_override = htonl(INADDR_BROADCAST);
			if (ip_broadcast)
			{
				bool broadcast_addr_override_valid =
					strnlen_s(ip_broadcast, 15) >= 7
					&& inet_addr(ip_broadcast) != htonl(INADDR_ANY);

				if (broadcast_addr_override_valid)
				{
					H2Config_ip_broadcast_override = inet_addr(ip_broadcast);
				}
			}

			// client only
			if (!Memory::IsDedicatedServer())
			{
				const char* language_code;
				CONFIG_GET(&ini, "language_code", "-1x0", &language_code);

				std::string language_code_str(language_code);
				if (!language_code_str.empty())
				{
					size_t delimiter_offset = language_code_str.find('x');
					if (delimiter_offset != std::string::npos)
					{
						std::string code_main_substr = language_code_str.substr(0, delimiter_offset);
						std::string code_variant_substr = language_code_str.substr(delimiter_offset + 1, language_code_str.size());

						H2Config_language.code_main = stol(code_main_substr);
						H2Config_language.code_variant = stol(code_variant_substr);
					}
				}

				CONFIG_GET(&ini, "language_label_capture", "false", &H2Config_custom_labels_capture_missing);
				CONFIG_GET(&ini, "skip_intro", "false", &H2Config_skip_intro);
				CONFIG_GET(&ini, "discord_enable", "true", &H2Config_discord_enable);
				CONFIG_GET(&ini, "fps_limit", "120", &H2Config_fps_limit);

				CONFIG_GET(&ini, "static_lod_state", "0", &H2Config_static_lod_state);

				switch(1)
				{
					case 0:
						H2Config_experimental_fps = _rendering_mode_none;
						break;
					case 1:
						H2Config_experimental_fps = _rendering_mode_original_game_frame_limit;
						break;
					default:
						//Incase any of the old rendering modes were used for a higher fps, set it back to 60.
						H2Config_fps_limit = 60;
						break;
				}
				
				CONFIG_GET(&ini, "shader_lod_max", "false", &H2Config_shader_lod_max);
				CONFIG_GET(&ini, "light_suppressor", "false", &H2Config_light_suppressor);
				CONFIG_GET(&ini, "disable_ingame_keyboard", "false", &H2Config_disable_ingame_keyboard);
				CONFIG_GET(&ini, "hide_ingame_chat", "false", &H2Config_hide_ingame_chat);

				CONFIG_GET(&ini, "hotkey_help", "0x72", &H2Config_hotkeyIdHelp); // VK_F2
				CONFIG_GET(&ini, "hotkey_align_window", "0x76", &H2Config_hotkeyIdAlignWindow); // VK_F7
				CONFIG_GET(&ini, "hotkey_window_mode", "0x77", &H2Config_hotkeyIdWindowMode); // VK_F8
				CONFIG_GET(&ini, "hotkey_hide_ingame_chat", "0x78", &H2Config_hotkeyIdToggleHideIngameChat); // VK_F9
				CONFIG_GET(&ini, "hotkey_guide", "0x24", &H2Config_hotkeyIdGuide); // VK_HOME
				CONFIG_GET(&ini, "hotkey_console", "0x79", &H2Config_hotkeyIdConsole); // VK_F10

				int shadows_override;
				CONFIG_GET(&ini, "override_shadows", "1", &shadows_override);

				switch(shadows_override)
				{
					case 0:
						H2Config_Override_Shadows = e_override_texture_resolution::tex_low;
					break;
					case 1:
					default:
						H2Config_Override_Shadows = e_override_texture_resolution::tex_default;
					break;
					case 2:
						H2Config_Override_Shadows = e_override_texture_resolution::tex_high;
					break;
					case 3:
						H2Config_Override_Shadows = e_override_texture_resolution::tex_ultra;
					break;
				}

				int water_override;
				CONFIG_GET(&ini, "override_water", "1", &water_override);

				switch(water_override)
				{
					case 0:
						H2Config_Override_Water = e_override_texture_resolution::tex_low;
						break;
					default:
					case 1:
						H2Config_Override_Water = e_override_texture_resolution::tex_default;
						break;
					case 2:
						H2Config_Override_Water = e_override_texture_resolution::tex_high;
						break;
					case 3:
						H2Config_Override_Water = e_override_texture_resolution::tex_ultra;
						break;
				}
				CONFIG_GET(&ini, "no_events", "false", &H2Config_no_events);
				CONFIG_GET(&ini, "skeleton_biped", "true", &H2Config_spooky_boy);

#ifndef NDEBUG
				H2Config_forced_event = ini.GetLongValue(k_h2config_version_section, "forced_event", H2Config_forced_event);
#endif

				CONFIG_GET(&ini, "force_off_d3d9ex", "false", &H2Config_force_off_d3d9ex);
				CONFIG_GET(&ini, "force_off_sm3", "false", &H2Config_force_off_sm3);
				CONFIG_GET(&ini, "use_d3d9on12", "false", &g_rasterizer_dx9on12_enabled);
				CONFIG_GET(
					&ini,
					"disable_amd_or_ati_patches",
					"false",
					&g_rasterizer_dx9_driver_globals.disable_amd_or_ati_patches);


				CONFIG_GET(&ini, "intel_sky_hack", "false", &H2Config_intel_sky_hack);
				*Memory::GetAddress<bool*>(0x41F6A9) = !H2Config_intel_sky_hack;
			}

			// dedicated server only
			if (Memory::IsDedicatedServer())
			{
				const char* server_name = NULL; 
				CONFIG_GET(&ini, "server_name", "", &server_name);
				if (server_name) {
					strncpy(H2Config_dedi_server_name, server_name, XUSER_MAX_NAME_LENGTH);
				}

				const char* server_playlist = NULL;
				CONFIG_GET(&ini, "server_playlist", "", &server_playlist);
				if (server_playlist) {
					strncpy(H2Config_dedi_server_playlist, server_playlist, sizeof(H2Config_dedi_server_playlist));
				}

				CONFIG_GET(&ini, "additional_pcr_time", "25", &H2Config_additional_pcr_time);
        
				CONFIG_GET(&ini, "minimum_player_start", "0", &H2Config_minimum_player_start);
				CONFIG_GET(&ini, "vip_lock", "false", &H2Config_vip_lock);
				CONFIG_GET(&ini, "shuffle_even_teams", "false", &H2Config_even_shuffle_teams);
				CONFIG_GET(&ini, "koth_random", "true", &H2Config_koth_random);

				CONFIG_GET(&ini, "enable_anti_cheat", "true", &g_twizzler_status);
				twizzler_set_status(g_twizzler_status);

				const char* login_identifier = NULL;
				CONFIG_GET(&ini, "login_identifier", "", &login_identifier);
				if (login_identifier) {
					strncpy(H2Config_login_identifier, login_identifier, sizeof(H2Config_login_identifier));
				}

				const char* login_password = NULL;
				CONFIG_GET(&ini, "login_password", "", &login_password);
				if (login_password) {
					strncpy(H2Config_login_password, login_password, sizeof(H2Config_login_password));
				}

				const char* stats_authkey = NULL;
				CONFIG_GET(&ini, "stats_auth_key", "", &stats_authkey);
				if (stats_authkey) {
					strncpy(H2Config_stats_authkey, stats_authkey, sizeof(H2Config_stats_authkey) - 1); // - 1 for the null character
				}

				std::string team_bit_mask(ini.GetValue(k_h2config_version_section, "teams_enabled_bit_flags", H2Config_team_bit_flags_str));
				if (!team_bit_mask.empty())
				{
					strncpy_s(H2Config_team_bit_flags_str, sizeof(H2Config_team_bit_flags_str), team_bit_mask.c_str(), 15);
					H2Config_team_bit_flags = 0;
					csmemset(H2Config_team_flag_array, 0, sizeof(H2Config_team_flag_array));

					size_t true_bit_value_count = std::count(team_bit_mask.begin(), team_bit_mask.end(), '1');
					size_t false_bit_value_count = std::count(team_bit_mask.begin(), team_bit_mask.end(), '0');

					const char team_bit_to_find[] = "01";
					size_t occurance_offset;
					occurance_offset = team_bit_mask.find_first_of(team_bit_to_find, 0);

					// TODO move to function
					// validate first
					if (true_bit_value_count + false_bit_value_count == 8
						&& occurance_offset != std::string::npos)
					{
						// then loop
						for (int i = 0; i < 8; i++)
						{
							if (team_bit_mask.at(occurance_offset) == '1') // check if the team is enabled
							{
								H2Config_team_bit_flags |= FLAG(i); // if so, enable the flag
								H2Config_team_flag_array[i] = true;
							}

							occurance_offset = team_bit_mask.find_first_of(team_bit_to_find, occurance_offset + 1);
							if (occurance_offset == std::string::npos)
								break;
						}
					}
				}
			}
		}

		fclose(fileConfig);

		if (!ownsConfigFile) {
			if (H2Config_base_port < 64000 + 1)
				H2Config_base_port += 1000;
			else if (H2Config_base_port < 65535 - 10 + 1)
				H2Config_base_port += 10;
		}
	}

	addDebugText("End reading H2Configuration file.");
}
#pragma endregion

#pragma region Config Init/Deinit
void InitH2Config() {
	H2Config_disable_ingame_keyboard = _Shell::GetInstanceId() > 1 ? true : false;
	ReadH2Config();
}
void DeinitH2Config() {
	SaveH2Config();
}
#pragma endregion
