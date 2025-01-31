#include "stdafx.h"
#include "Achievements.h"

#include "cseries/cseries_strings.h"
#include "H2MOD/Modules/Accounts/Accounts.h"
#include "H2MOD/Modules/Shell/Config.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"

const char k_cartographer_unlock_url[] = k_cartographer_url"/achievement-api/unlock.php";
const char k_cartographer_achivement_list_url[] = k_cartographer_url"/achievement-api/achievement_list.php?xuid=";

using namespace rapidjson;
std::map<DWORD, bool> achievementList;
std::unordered_map<std::string, bool> AchievementMap;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

void AchievementUnlock(unsigned long long xuid, int achievement_id, XOVERLAPPED* pOverlapped)
{
	LOG_TRACE_GAME("[H2Mod-Achievement] - Unlocking achievement ID: {:d}", achievement_id);

	CURL *curl;
	CURLcode res;
	std::string readBuffer;


	curl = curl_interface_init_no_verify();
	if (curl) {
		rapidjson::Document document;
		document.SetObject();


		Value token(kStringType);
		token.SetString(H2CurrentAccountLoginToken, document.GetAllocator());
		document.AddMember("token", token, document.GetAllocator());
		document.AddMember("id", achievement_id, document.GetAllocator());
		document.AddMember("xuid", Value().SetUint64(xuid), document.GetAllocator());


		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		document.Accept(writer);

		curl_easy_setopt(curl, CURLOPT_URL, k_cartographer_unlock_url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer.GetString());
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

	}
}

void GetAchievements(unsigned long long xuid)
{
	CURL *curl;
	CURLcode res;
	std::string readBuffer;

	curl = curl_interface_init_no_verify();
	if (curl) {

		c_static_string<512> server_url(k_cartographer_achivement_list_url);
		server_url.append(std::to_string(xuid).c_str());
		curl_easy_setopt(curl, CURLOPT_URL, server_url.get_string());
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		rapidjson::Document document;

		document.Parse(readBuffer.c_str());

		achievementList.clear();
		for (auto& achievement : document["achievements"].GetArray())
		{
			int id = (int)std::stoll(achievement.GetString());
			achievementList[id] = 1;
		}
	}
}