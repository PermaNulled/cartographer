#include "stdafx.h"

#include "Updater.h"
#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Modules/OnScreenDebug/OnscreenDebug.h"
#include "H2MOD/Modules/Shell/Startup/Startup.h"
#include "H2MOD/Utils/Utils.h"

#include "main/main.h"

const char k_cartographer_update_ini_url[] = k_cartographer_url"/update1.ini";

bool fork_cmd_elevate(const wchar_t* cmd, wchar_t* flags = 0) {
	SHELLEXECUTEINFO shExInfo = { 0 };
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	shExInfo.lpVerb = L"runas";                // Operation to perform
	shExInfo.lpFile = cmd;       // Application to start    
	shExInfo.lpParameters = flags ? flags : L"";                  // Additional parameters
	shExInfo.lpDirectory = 0;
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;

	if (ShellExecuteEx(&shExInfo))
	{
		//WaitForSingleObject(shExInfo.hProcess, INFINITE);
		CloseHandle(shExInfo.hProcess);
		return true;
	}
	return false;
}

bool fork_cmd(LPSTR cmd) {

	PROCESS_INFORMATION piProcInfo;
	STARTUPINFOA siStartInfo;
	bool bSuccess;

	// Set up members of the PROCESS_INFORMATION structure. 
	SecureZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDERR and STDOUT handles for redirection.
	SecureZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
	siStartInfo.cb = sizeof(STARTUPINFO);
	//siStartInfo.dwFlags |= STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	//siStartInfo.wShowWindow = SW_HIDE; // execute hide 

	// Create the child process. 
	bSuccess = CreateProcessA(//NULL,
		NULL,     // command line 
		cmd,
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		0,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION

	return bSuccess;
}

long long sizeOfDownload = 0;
long long sizeOfDownloaded = 0;

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	size_t written = fwrite(ptr, size, nmemb, stream);
	sizeOfDownloaded += nmemb;
	return written;
}


static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
	/* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
	/* 'userdata' is set with CURLOPT_HEADERDATA */

	sscanf(buffer, "Content-Length:%lld", &sizeOfDownload);
	return nitems * size;
}

int DownloadFile(const char* url, wchar_t* local_full_path) {
	sizeOfDownload = sizeOfDownloaded = 0;
	CURL *curl;
	FILE *fp;
	CURLcode res;
	curl = curl_interface_init_no_verify();
	if (curl) {
		CreateDirTree(local_full_path);
		fp = _wfopen(local_full_path, L"wb");
		if (!fp) {
			addDebugText("Failed to obtain FILE* for DL from: %s to: %s", url, local_full_path);
			return 2;
		}
		//FIXME: <Insert Pinned Public Key Here>
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
		res = curl_easy_perform(curl);
		/* always cleanup */
		curl_easy_cleanup(curl);
		fclose(fp);
		sizeOfDownload = sizeOfDownloaded = 0;
		return 0;
	}
	return 1;
}

typedef struct {
	char need_to_update;
	bool core_file;
	char location_id;
	char* local_name;
	char* server_uri;
	DWORD crc32;
} UpdateFileEntry;

std::vector<UpdateFileEntry*> UpdateFileEntries;

static void freeUpdateFileEntry(UpdateFileEntry* update_file_entry) {
	free(update_file_entry->local_name);
	free(update_file_entry->server_uri);
	free(update_file_entry);
}

static const char* H2UpdateVersionStr = "[Version:%[^]]]";
static const char* H2UpdateLocationsStr[] = { "[Temp]", "[Game]", "[AppData]" };
static char H2UpdateVersion[30] = "0";
static char current_location_id = 0;

bool updater_has_files_to_download = false;
bool updater_has_files_to_install = false;


static int interpretUpdateEntry(char* fileLine, char* version, int lineNumber) {
	bool read_it = false;

	if (!version) {
		read_it = true;
	}
	else {
		char rtn_result = CmpVersions(H2UpdateVersion, version);
		if (rtn_result == 0) {
			read_it = true;
		}
		else if (rtn_result & 0b10000) {

		}
		else {
			strcpy_s(H2UpdateVersion, version);
			int entry_count = UpdateFileEntries.size();
			for (int i = entry_count-1; i >= 0; i--) {
				if (!UpdateFileEntries[i]->core_file) {
					freeUpdateFileEntry(UpdateFileEntries[i]);
					UpdateFileEntries.erase(UpdateFileEntries.begin() + i);
				}
			}
			read_it = true;
		}
	}

	if (read_it) {//and weep
		bool unrecognised = false;
		bool duplicated = false;
		bool incorrect = false;
		bool dontSave = false;
		int fileLineLen = strlen(fileLine);
		int tempint1 = -1;
		DWORD tempDWORD = 0;
		unsigned short tempushort1 = -1;
		int tempint2 = -1;
		float tempfloat1 = NULL;
		char tempstr1[260] = "";
		char tempstr2[260] = "";
		if (fileLine[0] == '#' || fileLine[0] == ';' || fileLineLen < 5) {
			unrecognised = true;
		}
		else if (strstr(fileLine, H2UpdateLocationsStr[0])) {
			current_location_id = 0;
		}
		else if (strstr(fileLine, H2UpdateLocationsStr[1])) {
			current_location_id = 1;
		}
		else if (strstr(fileLine, H2UpdateLocationsStr[2])) {
			current_location_id = 2;
		}
		else if (sscanf_s(fileLine, "%[^=]=%zx=%s", &tempstr1, 260, &tempDWORD, &tempstr2, 260) == 3) {
			
			UpdateFileEntry* update_file_entry = (UpdateFileEntry*)malloc(sizeof(UpdateFileEntry));

			update_file_entry->need_to_update = 2;

			update_file_entry->core_file = version == 0;

			update_file_entry->location_id = current_location_id;

			update_file_entry->crc32 = tempDWORD;

			int namebufflen = strlen(tempstr1) + 1;
			update_file_entry->local_name = (char*)malloc(namebufflen);
			csmemcpy(update_file_entry->local_name, tempstr1, namebufflen);

			namebufflen = strlen(tempstr2) + 1;
			update_file_entry->server_uri = (char*)malloc(namebufflen);
			csmemcpy(update_file_entry->server_uri, tempstr2, namebufflen);

			bool save = true;

			int entry_count = UpdateFileEntries.size();
			for (int i = entry_count - 1; i >= 0; i--) {
				if (UpdateFileEntries[i]->core_file == update_file_entry->core_file && UpdateFileEntries[i]->location_id == update_file_entry->location_id && strcmp(UpdateFileEntries[i]->local_name, update_file_entry->local_name) == 0) {
					freeUpdateFileEntry(update_file_entry);
					save = false;
					break;
				}
			}
			
			if (save)
				UpdateFileEntries.push_back(update_file_entry);
			else
				duplicated = true;

			//incorrect = true;
		}
		else {
			unrecognised = true;
		}

		if (unrecognised || duplicated || incorrect) {
			if (duplicated) {
				addDebugText("ERROR: Duplicated update file on Line: %d", lineNumber);
			}
			else if (incorrect) {
				addDebugText("ERROR: Incorrect update file on Line: %d", lineNumber);
			}
		}
	}
	return 0;
}


static void FetchUpdateDetails() {
	wchar_t* dir_temp = _wgetenv(L"TEMP");

	wchar_t dir_temp_h2[1024];
	swprintf(dir_temp_h2, ARRAYSIZE(dir_temp_h2), L"%ws\\Halo2\\", dir_temp);
	CreateDirectory(dir_temp_h2, NULL);

	wchar_t dir_update[1024];
	swprintf(dir_update, ARRAYSIZE(dir_update), L"%ws\\Halo2\\Update\\", dir_temp);
	CreateDirectory(dir_update, NULL);

	wchar_t dir_update_old[1024];
	swprintf(dir_update_old, ARRAYSIZE(dir_update_old), L"%ws\\Halo2\\UpdateOld\\", dir_temp);
	CreateDirectory(dir_update_old, NULL);

	int entry_count = UpdateFileEntries.size();
	for (int i = entry_count - 1; i >= 0; i--) {
		freeUpdateFileEntry(UpdateFileEntries[i]);
	}
	UpdateFileEntries.clear();
	current_location_id = 0;
	H2UpdateVersion[0] = '0';
	H2UpdateVersion[1] = 0;

	updater_has_files_to_download = false;
	updater_has_files_to_install = false;

	addDebugText("Fetching Update Details.");
	char* rtn_result = 0;
	int rtn_code = MasterHttpResponse(k_cartographer_update_ini_url, "", &rtn_result);
	if (rtn_code == 0) {
		addDebugText("Got Update Details.");

		ReadIniFile(rtn_result, false, H2UpdateVersionStr, "0", interpretUpdateEntry);

		free(rtn_result);

		addDebugText("Interpreted the details.");

		int entry_count = UpdateFileEntries.size();
		for (int i = entry_count - 1; i >= 0; i--) {
			if (!UpdateFileEntries[i]->need_to_update)
				continue;
			for (int j = i - 1; j >= 0; j--) {
				if (UpdateFileEntries[j]->need_to_update && UpdateFileEntries[i]->location_id == UpdateFileEntries[j]->location_id && strcmp(UpdateFileEntries[i]->local_name, UpdateFileEntries[j]->local_name) == 0) {
					UpdateFileEntries[j]->need_to_update = 0;
				}
			}
			if (UpdateFileEntries[i]->need_to_update) {

				wchar_t existingfilepath[1024 + 260] = L"";

				wchar_t* existingfpdir = dir_temp_h2;
				if (UpdateFileEntries[i]->location_id == 1) {
					existingfpdir = H2ProcessFilePath;
				}
				else if (UpdateFileEntries[i]->location_id == 2) {
					existingfpdir = H2AppDataLocal;
				}

				swprintf(existingfilepath, ARRAYSIZE(existingfilepath), L"%ws%hs", existingfpdir, UpdateFileEntries[i]->local_name);

				DWORD crc32_file = 0;
				if (ComputeFileCrc32Hash(existingfilepath, crc32_file)) {
					if (UpdateFileEntries[i]->crc32 == crc32_file) {
						UpdateFileEntries[i]->need_to_update = 0;
						continue;
					}
				}

				if (UpdateFileEntries[i]->location_id) {
					swprintf(existingfilepath, ARRAYSIZE(existingfilepath), L"%ws%hs\\%hs", dir_update, H2UpdateLocationsStr[UpdateFileEntries[i]->location_id], UpdateFileEntries[i]->local_name);
				}
				else {
					swprintf(existingfilepath, ARRAYSIZE(existingfilepath), L"%ws%hs", dir_update, UpdateFileEntries[i]->local_name);
				}

				if (ComputeFileCrc32Hash(existingfilepath, crc32_file)) {
					if (UpdateFileEntries[i]->crc32 == crc32_file) {
						UpdateFileEntries[i]->need_to_update = 1;
						continue;
					}
				}

			}
		}
		addDebugText("Filtered & locally checked the Download List.");
	}


	//prompt user with changes

	entry_count = UpdateFileEntries.size();
	for (int i = 0; i < entry_count; i++) {
		if (UpdateFileEntries[i]->need_to_update == 2) {
			updater_has_files_to_download = true;
		}
		if (UpdateFileEntries[i]->need_to_update == 1 && UpdateFileEntries[i]->location_id > 0) {
			updater_has_files_to_install = true;
		}
		if (updater_has_files_to_download && updater_has_files_to_install)
			break;
	}

	std::string im_lazy = "";

	for (int i = 0; i < entry_count; i++) {
		if (UpdateFileEntries[i]->need_to_update == 2) {
			std::string im_lazy2 = UpdateFileEntries[i]->server_uri;
			im_lazy += im_lazy2 + "\n";
		}
	}

	//Download the following:
	if (im_lazy.length() > 0)
		im_lazy = "Download the following:\n" + im_lazy;

	std::string im_lazy1 = "";

	for (int i = 0; i < entry_count; i++) {
		if (UpdateFileEntries[i]->need_to_update == 1 && UpdateFileEntries[i]->location_id > 0) {
			std::string im_lazy2 = UpdateFileEntries[i]->local_name;
			std::string im_lazy3 = H2UpdateLocationsStr[UpdateFileEntries[i]->location_id];
			im_lazy1 += im_lazy3 + "\\" + im_lazy2 + "\n";
		}
	}

	//Install the following:
	if (im_lazy1.length() > 0)
		im_lazy += "Install the following:\n" + im_lazy1;

	//Up to date.
	if (im_lazy.length() <= 0)
		im_lazy = "Up to date!";

	extern char* autoUpdateText;
	char* autoUpdateTextAlt = autoUpdateText;
	autoUpdateText = 0;
	if (autoUpdateTextAlt) {
		free(autoUpdateTextAlt);
	}
	autoUpdateTextAlt = (char*)malloc(im_lazy.size() + 1);
	strcpy_s(autoUpdateTextAlt, im_lazy.size() + 1, im_lazy.c_str());
	autoUpdateText = autoUpdateTextAlt;
}

bool DownloadUpdatedFiles() {
	
	//download list
	bool files_downloaded = false;

	wchar_t* dir_temp = _wgetenv(L"TEMP");

	wchar_t dir_update[1024];
	swprintf(dir_update, ARRAYSIZE(dir_update), L"%ws\\Halo2\\Update\\", dir_temp);

	int entry_count = UpdateFileEntries.size();
	for (int i = 0; i < entry_count; i++) {
		if (UpdateFileEntries[i]->need_to_update == 2) {
			files_downloaded = true;
			wchar_t existingfilepath[1024 + 260] = L"";
			if (UpdateFileEntries[i]->location_id) {
				swprintf(existingfilepath, ARRAYSIZE(existingfilepath), L"%s%hs\\%hs", dir_update, H2UpdateLocationsStr[UpdateFileEntries[i]->location_id], UpdateFileEntries[i]->local_name);
			}
			else {
				swprintf(existingfilepath, ARRAYSIZE(existingfilepath), L"%s%hs", dir_update, UpdateFileEntries[i]->local_name);
			}
			_wremove(existingfilepath);
			
			c_static_string<1024> download_url(k_cartographer_url);
			download_url.append(UpdateFileEntries[i]->server_uri);
			DownloadFile(download_url.get_string(), existingfilepath);
		}
	}

	return files_downloaded;
}

static HANDLE hThreadDownloader = 0;

static DWORD WINAPI DownloadThread(LPVOID lParam)
{
	int operation_id = (int)lParam;

	if (operation_id == 0) {
		FetchUpdateDetails();
	}
	else if (operation_id == 1) 
	{
		if (DownloadUpdatedFiles())
		{
			FetchUpdateDetails();
		}
	}
	
	hThreadDownloader = 0;
	return 0;
}

void GSDownloadInit() {
}

void GSDownloadCheck() {
	if (!hThreadDownloader)
		hThreadDownloader = CreateThread(NULL, 0, DownloadThread, (LPVOID)0, 0, NULL);
}

void GSDownloadDL() {
	if (!hThreadDownloader)
		hThreadDownloader = CreateThread(NULL, 0, DownloadThread, (LPVOID)1, 0, NULL);
}

void GSDownloadInstall() {

	wchar_t* dir_temp = _wgetenv(L"TEMP");

	wchar_t dir_update[1024];
	swprintf(dir_update, ARRAYSIZE(dir_update), L"%ws\\Halo2\\Update\\", dir_temp);


	std::string updater_params = "";

	int entry_count = UpdateFileEntries.size();
	for (int i = 0; i < entry_count; i++) {
		if (UpdateFileEntries[i]->need_to_update == 1 && UpdateFileEntries[i]->location_id > 0) {
			updater_params += "\"";
			updater_params += H2UpdateLocationsStr[UpdateFileEntries[i]->location_id];
			updater_params += "\\";
			updater_params += UpdateFileEntries[i]->local_name;
			updater_params += "\" \"";

			char existingfilepath[1024 + 260] = "";

			wchar_t* existingfpdir = 0;
			if (UpdateFileEntries[i]->location_id == 1) {
				existingfpdir = H2ProcessFilePath;
			}
			else if (UpdateFileEntries[i]->location_id == 2) {
				existingfpdir = H2AppDataLocal;
			}

			snprintf(existingfilepath, 1024 + 260, "%ws%hs", existingfpdir, UpdateFileEntries[i]->local_name);

			updater_params += existingfilepath;

			updater_params += "\" ";
		}
	}

	//exec installer
	//quit game

	wchar_t existingfilepathupdater[1024 + 260] = L"";
	swprintf(existingfilepathupdater, ARRAYSIZE(existingfilepathupdater), L"%wsh2pc-update.exe", dir_update);

	if (updater_params.size() > 0) {
		int updater_params_buflen = 20 + updater_params.size();
		wchar_t* updater_params_flags = (wchar_t*)malloc(sizeof(wchar_t) * updater_params_buflen);
		swprintf(updater_params_flags, updater_params_buflen, L"-p %d -t 5000 %s", GetCurrentProcessId(), std::wstring(updater_params.begin(), updater_params.end()).c_str());
		if (fork_cmd_elevate(existingfilepathupdater, updater_params_flags)) {
			addDebugText("Shutting down to update!");
			main_quit();
		}
		else {
			addDebugText("Failed to fork updater!");
			addDebugText(existingfilepathupdater);
			// ### FIXME failed to fork updater
		}
		free(updater_params_flags);
	}
	else {
		addDebugText("No Files to Update!");
	}

}

void GSDownloadCancel() {
	//DWORD result = WaitForSingleObject(hThreadDownloader, 0);
	//if (result == WAIT_OBJECT_0) {
		// the thread handle is signaled - the thread has terminated
	//}
	//else {
		// the thread handle is not signaled - the thread is still alive
	//}


	if (hThreadDownloader) {
		TerminateThread(hThreadDownloader, 1);
		hThreadDownloader = 0;
		sizeOfDownload = sizeOfDownloaded = 0;
	}

	extern char* autoUpdateText;
	if (autoUpdateText) {
		free(autoUpdateText);
		autoUpdateText = 0;
	}

	updater_has_files_to_download = false;
	updater_has_files_to_install = false;
}


void InitGSDownload() {

}

void DeinitGSDownload() {

}