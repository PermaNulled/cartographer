#include "stdafx.h"
#include "CustomVariantSettings.h"

#include "camera/observer.h"
#include "memory/bitstream.h"
#include "game/game.h"
#include "game/game_time.h"
#include "networking/logic/life_cycle_manager.h"
#include "networking/NetworkMessageTypeCollection.h"
#include "physics/physics_constants.h"
#include "units/units.h"

#include "H2MOD/Modules/EventHandler/EventHandler.hpp"


CustomVariantSettings::s_variant_settings currentVariantSettings;
CustomVariantSettings::s_variant_settings defaultCustomVariantSettings;
std::map<std::wstring, CustomVariantSettings::s_variant_settings> customVariantSettingsMap;

namespace CustomVariantSettings
{
	void __cdecl EncodeVariantSettings(c_bitstream* stream, int a2, s_variant_settings* data)
	{
		stream->write_raw_data("gravity", &data->gravity, sizeof(data->gravity) * CHAR_BIT); //16.
		stream->write_raw_data("game speed", &data->gameSpeed, sizeof(data->gameSpeed) * CHAR_BIT);
		stream->write_bool("infinite ammo", data->infiniteAmmo);
		stream->write_bool("explosion physics", data->explosionPhysics);
		stream->write_integer("hill rotation", (byte)data->hillRotation, 8);
		stream->write_bool("infinite grenades", data->infiniteGrenades);
		stream->write_integer("forced field of view", data->forced_fov, sizeof(data->forced_fov) * CHAR_BIT);
	}
	bool __cdecl DecodeVariantSettings(c_bitstream* stream, int a2, s_variant_settings* data)
	{
		real32 gravity, gamespeed;
		stream->read_raw_data("gravity", &gravity, sizeof(gravity) * CHAR_BIT);
		data->gravity = gravity;
		stream->read_raw_data("game speed", &gamespeed, sizeof(gamespeed) * CHAR_BIT);
		data->gameSpeed = gamespeed;
		
		data->infiniteAmmo = stream->read_bool("infinite ammo");
		data->explosionPhysics = stream->read_bool("explosion physics");
		data->hillRotation = (e_hill_rotation)stream->read_integer("hill rotation", 8);
		data->infiniteGrenades = stream->read_bool("infinite grenades");
		data->forced_fov = (uint8)stream->read_integer("forced field of view", sizeof(data->forced_fov) * CHAR_BIT);
		return stream->error_occured() == false;
	}

	void UpdateCustomVariantSettings(s_variant_settings* data)
	{
		currentVariantSettings = *data;
	}

	void SendCustomVariantSettings(int32 peer_index)
	{
		c_network_session* session = NULL;
		network_life_cycle_in_squad_session(&session);
		if (session->is_host() && Memory::IsDedicatedServer())
		{
			//TODO: Find and map out struct with current variant information.
			auto VariantName = std::wstring(Memory::GetAddress<wchar_t*>(0, 0x534A18));

			auto customVariantSetting = customVariantSettingsMap.find(VariantName);
			if (customVariantSetting != customVariantSettingsMap.end())
			{
				currentVariantSettings = customVariantSetting->second;
				if (currentVariantSettings != defaultCustomVariantSettings) {
					c_network_observer* observer = session->m_network_observer;
					s_session_peer* peer = session->get_session_peer(peer_index);
					if (peer_index != NONE && !session->is_peer_local(peer_index))
					{
						if (peer->is_remote_peer)
							observer->send_message(
								session->m_session_index, 
								peer->observer_channel_index, 
								false, 
								_custom_variant_settings, 
								CustomVariantSettingsPacketSize, 
								&currentVariantSettings);
					}
				}
			}
		}
	}
	void ResetSettings()
	{
		currentVariantSettings = defaultCustomVariantSettings;
		ApplyCustomSettings(&currentVariantSettings);
	}

	void OnPlayerSpawn(datum playerIndex)
	{
		if (!game_is_predicted()) 
		{
			if (currentVariantSettings.infiniteGrenades)
			{
				s_player::set_player_unit_grenade_count(playerIndex, _unit_grenade_human_fragmentation, 4, false);
				s_player::set_player_unit_grenade_count(playerIndex, _unit_grenade_covenant_plasma, 4, false);
			}
		}
	}

	void ApplyCustomSettings(s_variant_settings* newVariantSettings)
	{
		//
		//Anything to be done on host and client goes here.
		//
		time_globals::get()->game_speed = newVariantSettings->gameSpeed;
		s_physics_constants::get()->gravity = newVariantSettings->gravity * s_physics_constants::get_default_gravity();
		//mov [ecx+6], ax
		static BYTE InfiniteAmmoMagazineASM[] = { 0x66, 0x89, 0x41, 0x06 };
		//movss [edi+00000184],xmm0
		static BYTE InfiniteAmmoBatteryASM[] = { 0xF3, 0x0F, 0x11, 0x87, 0x84, 0x01, 0x00, 0x00 };
		if (newVariantSettings->infiniteAmmo)
		{
			//Nop remove ammo from clips
			NopFill(Memory::GetAddress(0x15F3EA, 0x1436AA), sizeof(InfiniteAmmoMagazineASM));
			//Nop remove energy from battery.
			NopFill(Memory::GetAddress(0x15f7c6, 0x143A86), sizeof(InfiniteAmmoBatteryASM));
		}
		else
		{
			WriteBytes(Memory::GetAddress(0x15F3EA, 0x1436AA), InfiniteAmmoMagazineASM, sizeof(InfiniteAmmoMagazineASM));
			WriteBytes(Memory::GetAddress(0x15f7c6, 0x143A86), InfiniteAmmoBatteryASM, sizeof(InfiniteAmmoBatteryASM));
		}

		static BYTE InfiniteGrenadesASM[] = { 0x80, 0x84, 0x30, 0x52, 0x02, 0x00, 0x00, 0xFF };
		if (newVariantSettings->infiniteGrenades)
		{
			NopFill(Memory::GetAddress(0x1666A8, 0x15C168), sizeof(InfiniteGrenadesASM));
		}
		else
		{
			WriteBytes(Memory::GetAddress(0x1666A8, 0x15C168), InfiniteGrenadesASM, sizeof(InfiniteGrenadesASM));
		}

		//Client Only
		if (!Memory::IsDedicatedServer()) {
			if (newVariantSettings->explosionPhysics)
				WriteValue(Memory::GetAddress(0x17a44b), (BYTE)0x1E);
			else
				WriteValue(Memory::GetAddress(0x17a44b), (BYTE)0);
		}

		if (newVariantSettings->forced_fov != 0)
		{
			observer_set_suggested_field_of_view(newVariantSettings->forced_fov);
		}

		//Server Only
		if (Memory::IsDedicatedServer())
		{

		}

		// *((_DWORD *)v1 + 94) = seconds_to_ticks_imprecise(1);
		//Changing the argument passed to seconds_to_ticks_impercise.
		WriteValue(Memory::GetAddress(0x55d01, 0x5e1f9), (BYTE)newVariantSettings->spawnProtection);
	}
	void ApplyCurrentSettings()
	{
		// no idea why this is needed to be executed on blue screen but whatever
		ApplyCustomSettings(&currentVariantSettings);
	}
	void OnGameLifeCycleUpdate(e_game_life_cycle state)
	{
		switch (state)
		{
		case _life_cycle_in_game:
			ApplyCurrentSettings();
			break;
		case _life_cycle_none:
		case _life_cycle_pre_game:
		case _life_cycle_post_game:
			ResetSettings();
			break;
		default:
			break;
		}
	}
	void OnNetworkPlayerEvent(int peerIndex, EventHandler::NetworkPlayerEventType type)
	{
		if(type == EventHandler::NetworkPlayerEventType::add)
		{
			SendCustomVariantSettings(peerIndex);
		}
	}
	void OnMatchCountdown()
	{
		// Anything host related before the game starts goes here.
		c_network_session* session = NULL;
		if (network_life_cycle_in_squad_session(&session))
		{
			for (int32 i = 0; i < session->get_peer_count(); i++)
			{
				if (!session->is_peer_local(i))
					SendCustomVariantSettings(i);
			}
		}
	}

	typedef int(__cdecl get_next_hill_index_t)(int previousHill);
	get_next_hill_index_t* p_get_next_hill_index;
	int __cdecl get_next_hill_index(int previousHill)
	{
		static int currentPredefinedIndex = 0;
		int hillCount = *Memory::GetAddress<int*>(0x4dd0a8, 0x5008e8);

		//Return -1 to tell the engine there is no koth hills on the map.
		if (hillCount <= 0)
			return -1;
		switch (currentVariantSettings.hillRotation)
		{
		case _sequential:
			if (previousHill + 1 >= hillCount)
				return 0;
			return previousHill + 1;
		case _reverse:
			if (previousHill - 1 <= 0)
				return hillCount;
			return previousHill - 1;
		case _predefined:
			if (currentPredefinedIndex == 15)
				currentPredefinedIndex = 0;
			else if (currentVariantSettings.predefinedHillSet[currentPredefinedIndex + 1] == 0)
				currentPredefinedIndex = 0;
			else
				++currentPredefinedIndex;
			return currentVariantSettings.predefinedHillSet[currentPredefinedIndex] - 1;
		default:
		case _random:
			return p_get_next_hill_index(previousHill);
		}
		//Just in case.
		return -1;
	}

	void ApplyHooks()
	{
		PatchCall(Memory::GetAddress(0x10FE1F, 0xDC3CF), get_next_hill_index);
		PatchCall(Memory::GetAddress(0x10FE55, 0xDC405), get_next_hill_index);
		p_get_next_hill_index = Memory::GetAddress<get_next_hill_index_t*>(0x10DF1E, 0xDA4CE);
	}

	void Initialize()
	{
		ApplyHooks();
		
		EventHandler::register_callback(OnGameLifeCycleUpdate, EventType::gamelifecycle_change);
		EventHandler::register_callback(OnMatchCountdown, EventType::countdown_start, EventExecutionType::execute_after);
		EventHandler::register_callback(OnNetworkPlayerEvent, EventType::network_player, EventExecutionType::execute_after);
		EventHandler::register_callback(ApplyCurrentSettings, EventType::blue_screen, EventExecutionType::execute_after);
		EventHandler::register_callback(OnPlayerSpawn, EventType::player_spawn, EventExecutionType::execute_after);
	}
}