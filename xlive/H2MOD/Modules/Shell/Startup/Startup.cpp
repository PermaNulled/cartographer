#include "stdafx.h"
#include "Startup.h"

#include "../Config.h"
#include "../H2MODShell.h"

#include "cseries/cseries_strings.h"
#include "cseries/cseries_windows_debug_pc.h"
#include "shell/shell.h"
#include "shell/shell_windows.h"

#include "H2MOD.h"
#include "H2MOD/Modules/Accounts/AccountLogin.h"
#include "H2MOD/Modules/Accounts/Accounts.h"
#include "H2MOD/Modules/OnScreenDebug/OnscreenDebug.h"
#include "H2MOD/Utils/Utils.h"

#include "Util/filesys.h"
#include "Util/hash.h"

#include <io.h>

const wchar_t* k_client_process_name = L"Halo2Client";
const wchar_t* k_server_process_name = L"H2Server";

namespace filesystem = std::filesystem;

// xLiveLess specific logger
h2log *xlive_log = nullptr;

// Mod specific logger
h2log *h2mod_log = nullptr;

// General network logger
h2log *network_log = nullptr;

// Map loading logger
h2log *checksum_log = nullptr;

// OnScreenDebug logger
h2log *onscreendebug_log = nullptr;

// Console logger, receives output from all loggers
h2log *console_log = nullptr;

wchar_t* H2ProcessFilePath = 0;
wchar_t* H2AppDataLocal = 0;
wchar_t* FlagFilePathConfig = 0;

void PostH2Config() {

	wchar_t mutexName2[256];
	swprintf(mutexName2, ARRAYSIZE(mutexName2), L"Halo2BasePort#%d", H2Config_base_port);
	HANDLE mutex2 = CreateMutex(0, TRUE, mutexName2);
	DWORD lastErr2 = GetLastError();
	if (lastErr2 == ERROR_ALREADY_EXISTS) {
		addDebugText("Base port %d is already bound to!\nExpect MP to not work!", H2Config_base_port);
		_Shell::OpenMessageBox(NULL, MB_ICONWARNING, "BASE PORT BIND WARNING!", "Base port %d is already bound to!\nExpect MP to not work!", H2Config_base_port);
	}
	addDebugText("Base port: %d.", H2Config_base_port);
}

wchar_t xinput_path[_MAX_PATH];

bool configureXinput() {
	auto report_error = [](const std::string &message) {
		MessageBoxA(NULL, message.c_str(), "Xinput config error!", MB_OK);
	};

	if (!Memory::IsDedicatedServer()) {
		if (_Shell::GetInstanceId() > 1) {
			swprintf(xinput_path, ARRAYSIZE(xinput_path), L"xinput/p%02d/xinput9_1_0.dll", _Shell::GetInstanceId());
			LOG_TRACE_FUNCW(L"Changing xinput path to '{0}' : '{1}'", xinput_path, xinput_path);

			WritePointer(Memory::GetAddress() + 0x8AD28, xinput_path);

			char xinputName[_MAX_PATH];
			char xinputdir[_MAX_PATH];
			csprintf(xinputdir, NUMBEROF(xinputdir), "xinput/p%02d", _Shell::GetInstanceId());
			csprintf(xinputName, NUMBEROF(xinputdir), "%s/xinput9_1_0.dll", xinputdir);

			/* Creates a directory and displays error if it fails */
			auto create_dir = [=](const std::string &path) -> bool {
				if (LOG_CHECK(!CreateDirectoryA(path.c_str(), NULL) && GetLastError() != ERROR_ALREADY_EXISTS))
				{
					report_error("Failed to create '" + path + "' directory, check logs!");
					return false;
				}
				return true;
			};

			if (!create_dir("xinput") || !create_dir(xinputdir))
			{
				return false;
			}

			LOG_TRACE_FUNC("xinput path: {}", xinputName);
			if (_access_s(xinputName, 02))
			{
				if (errno == EACCES)
				{
					report_error("Can't access the xinput dll for writing");
					return false;
				}

				if (_access_s("xinput9_1_0.dll", 04) != 0) {
					char xinputError[] = "ERROR! An xinput9_1_0.dll does not exist in the local game directory or is inaccessible!\nFor \'Split-screen\' play, any supported .dll is required.";
					addDebugText(xinputError);
					_Shell::OpenMessageBox(NULL, MB_ICONERROR, "DLL Missing Error", xinputError);
					exit(EXIT_FAILURE);
				}
				int xinput_index = -1;
				const char *xinput_md5_durazno_0_6_0_0 = "6140ae76b26a633ce2255f6fc88fbe34";
				const long xinput_offset_durazno_0_6_0_0 = 0x196de;
				const bool xinput_unicode_durazno_0_6_0_0 = true;
				const char *xinput_md5_x360ce_3_3_1_444 = "8f24e36d5f0a71c8a412cec6323cd781";
				const long xinput_offset_x360ce_3_3_1_444 = 0x1ea54;
				const bool xinput_unicode_x360ce_3_3_1_444 = true;
				const char *xinput_md5_x360ce_3_4_1_1357 = "5236623449893c0e1e98fc95f067fcff";
				const long xinput_offset_x360ce_3_4_1_1357 = 0x16110;
				const bool xinput_unicode_x360ce_3_4_1_1357 = false;
				const int xinput_array_length = 3;
				static const char* xinput_md5[xinput_array_length] = { xinput_md5_durazno_0_6_0_0, xinput_md5_x360ce_3_3_1_444, xinput_md5_x360ce_3_4_1_1357 };
				static const long xinput_offset[xinput_array_length] = { xinput_offset_durazno_0_6_0_0, xinput_offset_x360ce_3_3_1_444, xinput_offset_x360ce_3_4_1_1357 };
				static const bool xinput_unicode[xinput_array_length] = { xinput_unicode_durazno_0_6_0_0, xinput_unicode_x360ce_3_3_1_444, xinput_unicode_x360ce_3_4_1_1357 };
				std::string available_xinput_md5;
				if (!hashes::calc_file_md5(L"xinput9_1_0.dll", available_xinput_md5))
				{
					report_error("Failed to hash original xinput9_1_0.dll, file might be missing?");
					return false;
				}
				for (int i = 0; i < xinput_array_length; i++) {
					if (strcmp(xinput_md5[i], available_xinput_md5.c_str()) == 0) {
						xinput_index = i;
						break;
					}
				}
				if (xinput_index < 0) {
					char xinputError[] = "ERROR! For \'Split-screen\' play, a supported xinput9_1_0.dll is required to be installed in the local game directory!\nOr you may install a custom one manually in the xinput/p??/ folders.";
					addDebugText(xinputError);
					_Shell::OpenMessageBox(NULL, MB_ICONERROR, "Incorrect DLL Error", xinputError);
					exit(EXIT_FAILURE);
				}

				if (!LOG_CHECK(CopyFileA("xinput9_1_0.dll", xinputName, FALSE)))
				{
					report_error("Failed to copy Xinput DLL for patching!");
					return false;
				}

				FILE* xinput_patched = fopen(xinputName, "r+b");
				if (!xinput_patched) {
					_Shell::FileErrorDialog(errno);
					report_error("Can't open xinput file for patching.");
					exit(EXIT_FAILURE);
				}

				int len_to_write = 2;
				uint8 assmXinputDuraznoNameEdit[] = 
				{ 
					(uint8)(0x30 + (_Shell::GetInstanceId() / 10)),
					(uint8)(0x30 + (_Shell::GetInstanceId() % 10)),
					(uint8)(0x30 + (_Shell::GetInstanceId() % 10))
				};
				if (xinput_unicode[xinput_index]) {
					assmXinputDuraznoNameEdit[1] = 0x00;
					len_to_write = 3;
				}
				if (fseek(xinput_patched, xinput_offset[xinput_index], SEEK_SET) == 0 && fwrite(assmXinputDuraznoNameEdit, sizeof(BYTE), len_to_write, xinput_patched) == len_to_write) {
					addDebugText("Successfully copied and patched original xinput9_1_0.dll");
				}
				else {
					fclose(xinput_patched);
					remove(xinputName);
					addDebugText("ERROR! Failed to write hex edit to file: %s", xinputName);
					_Shell::OpenMessageBox(NULL, MB_OK, "DLL Edit Error", "ERROR! Failed to write hex edit to file:\n %s", xinputName);
					exit(EXIT_FAILURE);
				}
				fclose(xinput_patched);
			}
		}
	}
	addDebugText("Finished Processing Instance Number.");
	return true;
}

void InitLocalAppData() {
	addDebugText("Find AppData Local.");

	if (H2AppDataLocal != 0) {
		free(H2AppDataLocal);
		H2AppDataLocal = 0;
	}

	wchar_t* userprofile = _wgetenv(L"USERPROFILE");

	wchar_t local2[1024];

	swprintf(local2, ARRAYSIZE(local2), L"%ws\\AppData\\Local\\", userprofile);
	struct _stat64i32 sb;
	if (_wstat(local2, &sb) == 0 && sb.st_mode & S_IFDIR) {
		swprintf(local2, ARRAYSIZE(local2), L"%ws\\AppData\\Local\\Microsoft\\", userprofile);
		CreateDirectoryW(local2, NULL);
		int fperrno1 = GetLastError();
		if (fperrno1 == ERROR_ALREADY_EXISTS || fperrno1 == ERROR_SUCCESS) {
#if USE_DEV_PREVIEW_CONFIG_FILE_PATHS
			swprintf(local2, ARRAYSIZE(local2), L"%ws\\AppData\\Local\\Microsoft\\Halo 2\\DevPreview\\", userprofile);
#else
			swprintf(local2, ARRAYSIZE(local2), L"%ws\\AppData\\Local\\Microsoft\\Halo 2\\", userprofile);
#endif
			CreateDirectoryW(local2, NULL);
			int fperrno1 = GetLastError();
			if (fperrno1 == ERROR_ALREADY_EXISTS || fperrno1 == ERROR_SUCCESS) {
				int appdatabuflen = wcslen(local2) + 1;
				H2AppDataLocal = (wchar_t*)calloc(appdatabuflen, sizeof(wchar_t));
				wcscpy_s(H2AppDataLocal, appdatabuflen, local2);
			}
		}
	}
	else if (swprintf(local2, ARRAYSIZE(local2), L"%ws\\Local Settings\\Application Data\\", userprofile), _wstat(local2, &sb) == 0 && sb.st_mode & S_IFDIR)
	{
		swprintf(local2, ARRAYSIZE(local2), L"%ws\\Local Settings\\Application Data\\Microsoft\\", userprofile);
		CreateDirectoryW(local2, NULL);
		int fperrno1 = GetLastError();
		if (fperrno1 == ERROR_ALREADY_EXISTS || fperrno1 == ERROR_SUCCESS) {
#if USE_DEV_PREVIEW_CONFIG_FILE_PATHS
			swprintf(local2, ARRAYSIZE(local2), L"%ws\\Local Settings\\Application Data\\Microsoft\\Halo 2\\DevPreview\\", userprofile);
#else
			swprintf(local2, ARRAYSIZE(local2), L"%ws\\Local Settings\\Application Data\\Microsoft\\Halo 2\\", userprofile);
#endif
			CreateDirectoryW(local2, NULL);
			int fperrno1 = GetLastError();
			if (fperrno1 == ERROR_ALREADY_EXISTS || fperrno1 == ERROR_SUCCESS) {
				int appdatabuflen = wcslen(local2) + 1;
				H2AppDataLocal = (wchar_t*)calloc(appdatabuflen, sizeof(wchar_t));
				wcscpy_s(H2AppDataLocal, appdatabuflen, local2);
			}
		}
	}

	if (H2AppDataLocal == nullptr) {
		int appdatabuflen = wcslen(H2ProcessFilePath) + 1;
		H2AppDataLocal = (wchar_t*)calloc(appdatabuflen, sizeof(wchar_t));
		wcscpy_s(H2AppDataLocal, appdatabuflen, H2ProcessFilePath);
		addDebugText("ERROR: Could not find AppData Local. Using Process File Path:");
		addDebugText(H2AppDataLocal);
	}
	else {
		addDebugText("Found AppData Local: %s", H2AppDataLocal);
	}
}

CRITICAL_SECTION log_section;

// use only after initLocalAppData has been called
// by default useAppDataLocalPath is set to true, if not specified
void prepareLogFileName(const wchar_t* logFileName, c_static_wchar_string<MAX_PATH>* path, bool useAppDataLocalPath) {
	const wchar_t* process_name = Memory::IsDedicatedServer() ? k_server_process_name : k_client_process_name;
	
	path->set(useAppDataLocalPath ? H2AppDataLocal : L"");

	wchar_t instance_string[3] = {};
	swprintf(instance_string, NUMBEROF(instance_string), L"%d", _Shell::GetInstanceId());
	
	c_static_wchar_string<MAX_PATH> folders;
	folders.append(L"logs\\");
	folders.append(process_name);
	folders.append(L"\\instance");
	folders.append(instance_string);

	path->append(folders.get_string());

	// try making logs directory
	if (!filesystem::create_directories(path->get_string()) && !filesystem::is_directory(filesystem::status(path->get_string())))
	{
		// try locally if we didn't already
		if (useAppDataLocalPath
			&& filesystem::create_directories(folders.get_string()) || filesystem::is_directory(filesystem::status(folders.get_string())))
			path->set(folders.get_string());
		else
			path->set(L""); // fine then
	}

	path->append(L"\\");
	path->append(logFileName);
	path->append(L".log");
	return;
}

///Before the game window appears
void InitH2Startup() {
	InitializeCriticalSection(&log_section);

	DETOUR_BEGIN();
	cseries_debug_initialize();
	Memory::Initialize();

	shell_windows_initialize();
	shell_apply_patches();
	shell_windows_apply_patches();
	// ### TODO remove entirely
	_Shell::Initialize();
	DETOUR_COMMIT();

	int ArgCnt;
	LPWSTR* ArgList = CommandLineToArgvW(GetCommandLineW(), &ArgCnt);
	H2ProcessFilePath = (wchar_t*)calloc(wcslen(ArgList[0]) + 1, sizeof(wchar_t));
	int rtncodepath = GetWidePathFromFullWideFilename(ArgList[0], H2ProcessFilePath);
	if (rtncodepath == -1) {
		std::wstring path = GetExeDirectoryWide();
		path.append(L"\\");
		H2ProcessFilePath = (wchar_t*)calloc(path.length() + 1, sizeof(wchar_t));
		_swprintf(H2ProcessFilePath, path.c_str());
	}

	// fix the game not finding the files it needs if the current directory is not the install directory
	SetCurrentDirectoryW(GetExeDirectoryWide().c_str());
	//If H2ProcessFilePath is empty (Server Console Mode?) set to working directory
	
	InitLocalAppData();

	// initialize curl
	curl_global_init(CURL_GLOBAL_ALL);
	atexit([] { curl_global_cleanup(); });

	// after localAppData filepath initialized, we can initialize OnScreenDebugLog
	InitOnScreenDebugText();

	if (Memory::IsDedicatedServer()) {
		addDebugText("Process is Dedi-Server");
	}
	else {
		addDebugText("Process is Client");
	}

	if (ArgList != NULL)
	{
		for (int i = 0; i < ArgCnt; i++)
		{
			if (wcsstr(ArgList[i], L"-h2config=") != NULL)
			{
				if (wcslen(ArgList[i]) < 255)
				{
					int pfcbuflen = wcslen(ArgList[i] + 10) + 1;
					FlagFilePathConfig = (wchar_t*)malloc(sizeof(wchar_t) * pfcbuflen);
					swprintf(FlagFilePathConfig, pfcbuflen, ArgList[i] + 10);
				}
			}
		}
	}

	InitH2Config();
	PostH2Config();

	EnterCriticalSection(&log_section);

	// prepare default log files if enabled, after we read the H2Config
	bool should_enable_console_log = H2Config_debug_log && H2Config_debug_log_console;
	console_log = h2log::create_console("CONSOLE MAIN", should_enable_console_log, H2Config_debug_log_level);

	c_static_wchar_string<MAX_PATH> path;
	prepareLogFileName(L"h2xlive", &path);
	xlive_log = h2log::create("XLive", path.get_string(), H2Config_debug_log, H2Config_debug_log_level);
	LOG_DEBUG_XLIVE(DLL_VERSION_STR);

	prepareLogFileName(L"h2mod", &path);
	h2mod_log = h2log::create("H2MOD", path.get_string(), H2Config_debug_log, H2Config_debug_log_level);
	LOG_DEBUG_GAME(DLL_VERSION_STR);
	
	prepareLogFileName(L"h2network", &path);
	network_log = h2log::create("Network", path.get_string(), H2Config_debug_log, H2Config_debug_log_level);
	LOG_DEBUG_NETWORK(DLL_VERSION_STR);

	//checksum_log = h2log::create("Checksum", prepareLogFileName(L"checksum"), true, 0);
	LeaveCriticalSection(&log_section);
	InitH2Accounts();

	if (!configureXinput())
		exit(EXIT_FAILURE);

	H2MOD::Initialize();

	addDebugText("ProcessStartup finished.");
}

///After the game window appears
void H2DedicatedServerStartup() {
	// initialize default data to run under LAN
	// if the server runs in LIVE mode, check XLiveSignIn/XLiveSignOut in AccountLogin.cpp
	if (Memory::IsDedicatedServer())
	{
		addDebugText("Signing in dedicated server locally.");

		AccountEdit_remember = false;

		BYTE abEnet[6];
		BYTE abOnline[20];
		XNetRandom(abEnet, sizeof(abEnet));
		XNetRandom(abOnline, sizeof(abOnline));
		ConfigureUserDetails("[Username]", "12345678901234567890123456789012", rand(), 0, H2Config_ip_lan, ByteToHexStr(abEnet, sizeof(abEnet)).c_str(), ByteToHexStr(abOnline, sizeof(abOnline)).c_str(), false);
	}
}

void DeinitH2Startup() {
	extern void DeinitRunLoop();
	DeinitRunLoop();
	extern void DeinitCustomLanguage();
	DeinitCustomLanguage();
	DeinitH2Accounts();
	DeinitH2Config();

	free(H2AppDataLocal);
	free(H2ProcessFilePath);
	return;
}
