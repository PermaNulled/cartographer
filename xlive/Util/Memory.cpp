#include "stdafx.h"
#include "Memory.h"

enum H2Type : int
{
	Invalid = -1,
	UnsupportedVersion,
	H2Game,
	H2Server,
};

struct ProcessInfo
{
	HMODULE base;
	H2Type process_type;
};

DWORD Memory::baseAddress;
bool Memory::dedicatedServer;

ProcessInfo game_info;

H2Type DetectProcessType()
{
	H2Type h2TypeResult = H2Type::Invalid;

	// update 5/3/2022:
	// this now verifies game version as well, therefore the code below had to be commented out

	// try and detect type based on module name.
	/*if (GetModuleHandleW(L"halo2.exe"))
		return H2Type::H2Game;
	else if (GetModuleHandleW(L"h2server.exe"))
		return H2Type::H2Server;*/

	// fallback to checking file information in case the file was renamed.
	wchar_t exe_file_path[_MAX_PATH + 1];
	int result = GetModuleFileNameW(NULL, exe_file_path, ARRAYSIZE(exe_file_path));
	if (result <= _MAX_PATH && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		DWORD version_info_size = GetFileVersionInfoSizeW(exe_file_path, NULL);
		if (version_info_size != 0) {
			BYTE* version_info = new BYTE[version_info_size];
			if (GetFileVersionInfoW(exe_file_path, NULL, version_info_size, version_info)) {
				const wchar_t* orginal_filename;
				const wchar_t* original_product_version;
				size_t filename_len, product_version_len;
				// shouldn't be hardcoded but who cares
				VerQueryValueW(version_info, L"\\StringFileInfo\\040904b0\\OriginalFilename", (LPVOID*)&orginal_filename, &filename_len);
				VerQueryValueW(version_info, L"\\StringFileInfo\\040904b0\\ProductVersion", (LPVOID*)&original_product_version, &product_version_len);

				std::wstring exe_orginal_filename = orginal_filename;
				std::wstring exe_product_version = original_product_version;
				delete[] version_info;

				if (exe_orginal_filename == L"Halo2.exe")
					h2TypeResult = H2Type::H2Game;
				else if (exe_orginal_filename == L"h2server.exe")
					h2TypeResult = H2Type::H2Server;

				if (exe_product_version != L"1.00.00.11122")
					h2TypeResult = H2Type::UnsupportedVersion;
			}
		}
	}
	return h2TypeResult;
}

// TODO maybe this is not the best place to keep this
void Memory::Initialize()
{
	game_info.base = GetModuleHandle(NULL);
	game_info.process_type = DetectProcessType();

	static const char* unsupported_game_version_error =
		"Project Cartographer loaded into unsupported game version, will now exit!";

	static const char* unsupported_game_version_error_help =
		"Download latest official patch at \"halo2pc.com/downloads\", Latest Official Update.zip";

	switch (game_info.process_type)
	{
	case H2Type::Invalid:
		MessageBoxA(NULL, "Project Cartographer loaded into unsupported process, will now exit!", "ERROR!", MB_ICONERROR);
		TerminateProcess(GetCurrentProcess(), 1);
		break;
	case H2Type::UnsupportedVersion:
		MessageBoxA(NULL, unsupported_game_version_error, "ERROR!", MB_ICONERROR);
		MessageBoxA(NULL, unsupported_game_version_error_help, "", MB_ICONINFORMATION);
		TerminateProcess(GetCurrentProcess(), 1);
		break;
	case H2Type::H2Game:
		Memory::SetBaseAddress((DWORD)game_info.base, false);
		break;
	case H2Type::H2Server:
		Memory::SetBaseAddress((DWORD)game_info.base, true);
		break;
	default:
		assert(0);
		break;
	}
}