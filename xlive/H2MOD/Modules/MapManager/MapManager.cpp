#include "stdafx.h"

#include "MapManager.h"

#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Modules/OnScreenDebug/OnscreenDebug.h"

#include "networking/NetworkMessageTypeCollection.h"
#include "networking/logic/life_cycle_manager.h"

#include "main/map_repository.h"
#include "main/game_preferences.h"

std::unique_ptr<MapManager> mapManager(std::make_unique<MapManager>());

const wchar_t* k_map_download_source_text[k_language_count] =
{
	L"repository",
};

#pragma region allow host to start game, without all players loading the map

typedef int32(__cdecl* network_life_cycle_session_get_global_map_precache_status_t)(int32*, e_network_session_map_status*);
network_life_cycle_session_get_global_map_precache_status_t p_network_life_cycle_session_get_global_map_precache_status;

// allow hosts to start the game while other peers didn't load the map
// TODO: simplify and cleanup
int32 __cdecl network_life_cycle_session_get_global_map_precache_status_hook(int32* out_lowest_load_percentage, e_network_session_map_status* out_host_map_status) {
	// this just gets the current network_session, but has some extra misc checks

	c_network_session* session = nullptr;
	e_network_session_map_status result_map_status = _network_session_map_status_none;
	uint32 result_precache_percentage = 0;
	bool someone_downloading_map = false;
	e_network_session_map_status local_peer_map_status, host_peer_map_status;
	local_peer_map_status = host_peer_map_status = _network_session_map_status_none;
	int32 peer_count_with_map_status_precached = 0;
	int32 peer_count_with_map_status_downloading = 0;
	int32 peer_count_with_map_status_unable_to_precache = 0;

	if (c_game_life_cycle_manager::get()->get_active_session(&session)
		&& session->established())
	{
		s_session_membership* membership = &session->m_session_membership;

		if (out_host_map_status)
		{
			*out_host_map_status = membership->membership_peers[session->m_session_host_peer_index].map_status;
		}

		result_map_status = _network_session_map_status_loaded;
		result_precache_percentage = 100; // i don't think this is used anymore, it has been replaced by the loading screen in H2v from Xbox

		for (int32 i = 0; i < membership->peer_count; i++)
		{
			// NOTE UPDATE 7/29/2021: now this checks if there's any peer that can load the map, instead of if there's any peer that cannot load the map
			// *************

			// now we only check our peer and session host peer, instead of all the membership_peers
			// but make sure the game won't start if we have just 1 player that doesn't have the map
			if (session->m_local_peer_index == i)
				local_peer_map_status = membership->membership_peers[i].map_status;

			if (session->m_session_host_peer_index == i)
				host_peer_map_status = membership->membership_peers[i].map_status;

			switch (membership->membership_peers[i].map_status)
			{
			case _network_session_map_status_unable_to_precache:
				result_precache_percentage = 0;
				peer_count_with_map_status_unable_to_precache++;
				break;

			case _network_session_map_status_precaching:
				result_precache_percentage = MIN(membership->membership_peers[i].map_progress_percentage, result_precache_percentage); // get the least map precaching percentage
				break;

			case _network_session_map_status_loaded:
			case _network_session_map_status_precached:
				peer_count_with_map_status_precached++;
				break;

			case _network_session_map_status_downloading:
				someone_downloading_map = true;
				peer_count_with_map_status_downloading++;
				break;

			default:
				break;
			}
		}

		// checks if local/host map status is fine
		// if not just let the game know map cannot be loaded
		auto test_map_status = [&](e_network_session_map_status status) -> bool
			{
				switch (status)
				{
				case _network_session_map_status_loaded:
				case _network_session_map_status_precached:
					if (peer_count_with_map_status_precached > 0) {
						result_map_status = _network_session_map_status_precached;
						return true;
					}
					else if (peer_count_with_map_status_downloading > peer_count_with_map_status_unable_to_precache) {
						result_map_status = _network_session_map_status_downloading;
						return false;
					}
					else {
						result_map_status = _network_session_map_status_unable_to_precache;
						return false;
					}
				case _network_session_map_status_precaching:
					result_map_status = _network_session_map_status_precaching;
					return false;
				case _network_session_map_status_unable_to_precache:
					result_map_status = _network_session_map_status_unable_to_precache;
					result_precache_percentage = 0;
					return false;
				case _network_session_map_status_downloading:
					result_map_status = _network_session_map_status_downloading;
					result_precache_percentage = 0;
					return false;
				default:
					return false;
				}
			};

		// first test the host map load
		if (test_map_status(host_peer_map_status))
		{
			// then the local peer
			test_map_status(local_peer_map_status);
		}
	}

	if (out_lowest_load_percentage)
		*out_lowest_load_percentage = result_precache_percentage;

	return result_map_status;
}

bool __stdcall game_life_cycle_get_map_load_status(void* thisx, c_network_session* session, uint32* out_peers_unable_to_load_map_mask)
{
	s_session_membership* membership = &session->m_session_membership;

	uint32 peers_map_not_loaded_mask = 0;
	if (membership->peer_count > 0)
		peers_map_not_loaded_mask = FLAG(membership->peer_count) - 1; // set all flags

	int32 peers_that_can_load_map = 0;
	bool local_or_host_peer_cannot_load_map = false;

	for (int32 i = 0; i < membership->peer_count; i++) {
		// NOTE UPDATE 7/29/2021: now this checks if there's any peer that can load the map, instead of if there's any peer that cannot load the map
		// *************

		// check only session host and local peer indexes
		//if ((i == session->local_peer_index || i == session->m_session_host_peer_index) 
			//|| !more_than_one_peer_other_than_dedicated_server(session)) 
		{
			switch (membership->membership_peers[i].map_status)
			{
			case _network_session_map_status_none:
			case _network_session_map_status_unable_to_precache:
			case _network_session_map_status_precaching:
			case _network_session_map_status_downloading:
				if (session->is_peer_local(i) || session->is_peer_session_host(i))
					local_or_host_peer_cannot_load_map = true;
				break;
			case _network_session_map_status_precached:
			case _network_session_map_status_loaded:
				peers_map_not_loaded_mask &= ~FLAG(i);
				peers_that_can_load_map++;
				break;
			default:
				break;
			}
		}
	}

	if (out_peers_unable_to_load_map_mask)
		*out_peers_unable_to_load_map_mask = peers_map_not_loaded_mask;

	// subtract 1 from peers_that_can_load_map to remove the host/local peer
	return !local_or_host_peer_cannot_load_map && peers_that_can_load_map - (session->m_session_parameters.dedicated_server ? 1 : 0) > 0;
}

#pragma endregion

/**
* Download map callback
*/
char __cdecl handle_map_download_callback() {
	// TODO add force download stop? for now no
	// mapManager->forceStopDownloadQueries()

	// add download query
	mapManager->AddDownloadQuery(L"");
	auto query = mapManager->GetLastDownloadQueryAdded();
	// request the filename
	NetworkMessage::SendRequestMapFilename(query->id);
	// then start download thread
	query->StartMapDownload();

	return 1;
}

typedef bool(__cdecl* map_download_leave_session_cb_t)();
map_download_leave_session_cb_t p_map_download_leave_session_cb;

bool map_download_dialog_callback()
{
	return p_map_download_leave_session_cb();
}

/**
* Menu constructor hook
*/
void __cdecl ui_open_map_download_confirm_dialog(int32 a1, int32 a2, int32 a3, int16 a4, void* map_download_callback, void* leave_game_callback, int32 a7, int32 a8, int32 a9, int32 a10)
{
	p_map_download_leave_session_cb = (map_download_leave_session_cb_t)leave_game_callback;
	INVOKE(0x20E2E0, 0x0, ui_open_map_download_confirm_dialog, a1, a2, a3, a4, handle_map_download_callback, map_download_dialog_callback, a7, a8, a9, a10);
}

int32 __cdecl map_download_get_progress_percentage()
{
	if (!mapManager->m_mapDownloadQueryList.empty())
		return mapManager->m_mapDownloadQueryList.front()->GetDownloadPercentage();
	else
		return 0;
}

void map_download_get_download_source_text(int32 a1, wchar_t* out_text)
{
	if (out_text != NULL)
		wcsncpy_s(out_text, 512, k_map_download_source_text[0], _TRUNCATE);
}

/*
	Seems to be a function responsible for loading data about maps when displaying them.
	This is hooked to fix/re-add removed custom map images.
*/
void* user_interface_data = nullptr;
void __declspec(naked) load_map_data_for_display_nak() {
	__asm {
		mov eax, [esp + 4 + 0xC] // grab map_data pointer from stack (callee frame)
		mov ecx, [eax + 0x964] // mov bitmap pointer into ecx
		mov[ebx], ecx // mov bitmap pointer into map_data on stack
		mov eax, user_interface_data
		ret // return to original load_map_data_for display function
	}
}
/**
* Makes changes to game functionality
*/
void MapManager::ApplyPatches() {

	if (!Memory::IsDedicatedServer()) {

		BYTE jmp[] = { 0xEB };

		WriteBytes(Memory::GetAddress(0x215A9E), jmp, sizeof(jmp)); /* Allow map download in network */
		WriteBytes(Memory::GetAddress(0x215AC9), jmp, sizeof(jmp)); /* Disable "Match has begun" bullshit */
		PatchCall(Memory::GetAddress(0x244A4A), ui_open_map_download_confirm_dialog); /* Redirect the menu constructor to our code to replace the game's map downloading code callback */

		/* Redirects map downloading percentage to our custom downloader */
		PatchCall(Memory::GetAddress(0x244B77), map_download_get_progress_percentage);
		PatchCall(Memory::GetAddress(0x22EE41), map_download_get_progress_percentage);
		PatchCall(Memory::GetAddress(0x244B8F), map_download_get_download_source_text);

		//Hooked to fix custom map images.
		PatchCall(Memory::GetAddress(0x593F0), load_map_data_for_display_nak);
		user_interface_data = Memory::GetAddress<void*>(0x9712C8);
	}

	// allow host to start the game, even if there are peers that didn't load the map
	p_network_life_cycle_session_get_global_map_precache_status = (network_life_cycle_session_get_global_map_precache_status_t)DetourFunc(Memory::GetAddress<BYTE*>(0x1B1929, 0x197879), (BYTE*)network_life_cycle_session_get_global_map_precache_status_hook, 8);

	DetourClassFunc(Memory::GetAddress<BYTE*>(0x1D76C5, 0x1BCD32), (BYTE*)game_life_cycle_get_map_load_status, 8);

	// disable the game's downloading implementation
	NopFill(Memory::GetAddress(0x1B5421, 0x1A917F), 5);

	// custom map cache patches/hooks
	c_custom_map_manager::ApplyCustomMapExtensionLimitPatches();
}

/**
* Actually calls the real map reload function in halo2.exe
*/
void MapManager::ReloadAllMaps() {
	get_custom_map_manager()->load_custom_map_data_cache();
	get_custom_map_manager()->start_custom_map_sync();
}

bool MapManager::GetMapFilename(std::wstring& buffer) {
	wchar_t map_file_location[256];
	c_network_session* session = NULL;

	// we want this to work in-game too
	if (/*p_get_lobby_state() == game_lobby_states::in_lobby && */ network_life_cycle_in_squad_session(&session)) {
		ZeroMemory(map_file_location, sizeof(map_file_location));
		NetworkSession::GetMapFileLocation(map_file_location, ARRAYSIZE(map_file_location));

		std::wstring unicodeMapFileLocation(map_file_location);
		std::size_t mapNameOffset = unicodeMapFileLocation.find_last_of(L'\\');
		std::size_t mapNameOffsetEnd = unicodeMapFileLocation.find_last_not_of(L'.');
		std::wstring filename = unicodeMapFileLocation.substr(mapNameOffset + 1, mapNameOffsetEnd);
		if (!filename.empty()) {
			//if the filename exists and the current map english name is equal to the iterated map name
			buffer = filename;
			return true;
		}
	}

	return false;
}

void MapManager::MapDownloadUpdateTick()
{
	if (m_mapDownloadQueryList.empty())
		return;

	for (auto it = m_mapDownloadQueryList.begin(); it != m_mapDownloadQueryList.end(); )
	{
		MapDownloadQuery& query = **it;
		if (query.m_downloadFinished)
		{
			it = m_mapDownloadQueryList.erase(it);
		}
		else
		{
			++it;
		}
	}
}

bool MapDownloadQuery::ShouldStopDownload() {
	return m_forceStopDownload == true || !NetworkSession::LocalPeerIsEstablished();
}

void MapDownloadQuery::StopDownload() {
	m_forceStopDownload = true;
};

MapDownloadQuery::MapDownloadQuery(const std::wstring& _mapToDownload, unsigned int _downloadId) {
	id = _downloadId;
	SetMapNameToDownload(_mapToDownload);
}

size_t map_download_curl_write_data_cb(void* ptr, size_t size, size_t nmemb, FILE* stream) {
	return fwrite(ptr, size, nmemb, stream);
}

static int32 map_download_curl_xferinfo_cb(void* p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
	MapDownloadQuery* mapDownloadQuery = (MapDownloadQuery*)p;
	mapDownloadQuery->SetDownloadPercentage((int)(((float)dlnow / (float)dltotal) * 100.f));
	return mapDownloadQuery->ShouldStopDownload();
}

bool MapDownloadQuery::DownloadFromRepo() {
	std::string url(cartographerMapRepoURL + "/");

	FILE* fp = nullptr;
	CURL* curl = nullptr;
	CURLcode res;

	std::wstring mapFilePathWide(get_custom_map_folder_path());
	std::string nonUnicodeMapFilePath(mapFilePathWide.begin(), mapFilePathWide.end());
	nonUnicodeMapFilePath += m_clientMapFilename;

	curl = curl_interface_init_no_verify();
	if (curl) {
		fp = fopen(nonUnicodeMapFilePath.c_str(), "wb");
		if (fp == nullptr) {
			LOG_TRACE_GAME("{} - unable to open map file at: {}", __FUNCTION__, nonUnicodeMapFilePath.c_str());
			curl_easy_cleanup(curl);
			return false;
		}

		char* url_encoded_map_filename = curl_easy_escape(curl, m_clientMapFilename.c_str(), m_clientMapFilename.length());
		url += url_encoded_map_filename;
		curl_free(url_encoded_map_filename);

		//fail if 404 or any other type of http error
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, map_download_curl_write_data_cb);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, map_download_curl_xferinfo_cb);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, (void*)this);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		res = curl_easy_perform(curl);
		long http_code = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		/* always cleanup */
		curl_easy_cleanup(curl);
		fclose(fp);

		if (res == CURLE_OK)
		{
			if (http_code != 404 && get_custom_map_manager()->add_custom_map_entry_by_map_file_path(mapFilePathWide + m_clientMapFilenameWide)) {
				//if we succesfully downloaded the map, return true
				return true;
			}
			return false;
		}
		else
		{
			LOG_ERROR_GAME("{} - {} failed with error: {}", __FUNCTION__, STRINGIFY(curl_easy_perform()), (uint32)res);
			if (res == CURLE_ABORTED_BY_CALLBACK)
				addDebugText("Map downloading aborted because of user input!");

			remove(nonUnicodeMapFilePath.c_str());

			return false;
		}
	}

	return false;
}

void MapDownloadQuery::StartMapDownload()
{
	STD_CHRONO_DEFINE_TIME_AND_CLOCK(_time, _clock);

	auto mapDownloadThread = [this]() {
		int32* mapDownloadStatus = Memory::GetAddress<int32*>(0x422570);

		auto timeAtStart = _clock::now();

		while (!m_readyToDownload && _clock::now() - timeAtStart < 5s)
			Sleep(50);

		// set the game to downloading map state
		*mapDownloadStatus = NONE;

		if (m_readyToDownload) {
			LOG_TRACE_GAME("[h2mod-mapmanager] map file to download: {}", m_clientMapFilename);

			//TODO: set map filesize
			//TODO: if downloading from repo files, try p2p
			if (!DownloadFromRepo()
				&& !NetworkSession::LocalPeerIsSessionHost())
			{
				LOG_TRACE_GAME("[h2mod-mapmanager] {}() - {}() failed, leaving session!",
					STRINGIFY(handle_map_download_callback),
					STRINGIFY(DownloadFromRepo));

				addDebugText("Failed to download custom map %s .", m_clientMapFilename.c_str());
				NetworkSession::LeaveSession(); // download has failed
			}
		}
		else {
			// no map filename (probably packet hasn't been received)
			addDebugText("Failed to download custom map, no filename to download.");
			LOG_TRACE_GAME("[h2mod-mapmanager] no map filename received from host!");
			NetworkSession::LeaveSession();
		}

		// set the game to map is loaded state
		*mapDownloadStatus = 0;
		m_downloadFinished = true;
	};

	if (NetworkSession::LocalPeerIsEstablished())
		std::thread(mapDownloadThread).detach();
	else
		m_downloadFinished = true;
}


void MapDownloadQuery::SetMapNameToDownload(const std::wstring& _mapNameToDownloadWide)
{
	if (!_mapNameToDownloadWide.empty())
	{
		m_clientMapFilenameWide = _mapNameToDownloadWide;
		m_clientMapFilename = std::string(_mapNameToDownloadWide.begin(), _mapNameToDownloadWide.end());
		m_readyToDownload = true;
	}
}

void MapDownloadQuery::SetMapNameToDownload(const wchar_t* _mapNameToDownload)
{
	SetMapNameToDownload(std::wstring(_mapNameToDownload));
}

int MapDownloadQuery::GetDownloadPercentage(void) const
{
	return m_downloadPercentage;
}

void MapDownloadQuery::SetDownloadPercentage(int _downloadPercentage)
{
	m_downloadPercentage = _downloadPercentage;
}
