#include "stdafx.h"
#include "Infection.h"

#include "game/game.h"
#include "game/game_engine_util.h"
#include "game/game_time.h"
#include "interface/user_interface_controller.h"
#include "items/item_collection_definition.h"
#include "scenario/scenario.h"
#include "networking/logic/life_cycle_manager.h"
#include "units/units.h"

#include "H2MOD.h"
#include "H2MOD/Modules/SpecialEvents/SpecialEvents.h"
#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Modules/EventHandler/EventHandler.hpp"
#include "interface/new_hud_draw.h"

std::vector<uint64> Infection::zombieIdentifiers;

#define k_humans_team _game_team_red
#define k_zombie_team _game_team_green

bool initialSpawn;
bool infectedPlayed[k_number_of_users]{};
int32 zombiePlayerIndex = NONE;
int32 last_time_at_game_should_not_end = 0;
const wchar_t *const infectionSoundTable[k_language_count][e_infection_sounds::_infection_end]
{
	{SND_INFECTION_EN, SND_INFECTED_EN, SND_NEW_ZOMBIE_EN },
	{SND_INFECTION_JP, SND_INFECTED_JP, SND_NEW_ZOMBIE_JP },
	{SND_INFECTION_GE, SND_INFECTED_GE, SND_NEW_ZOMBIE_GE },
	{SND_INFECTION_FR, SND_INFECTED_FR, SND_NEW_ZOMBIE_FR },
	{SND_INFECTION_ES, SND_INFECTED_ES, SND_NEW_ZOMBIE_ES },
	{SND_INFECTION_IT, SND_INFECTED_IT, SND_NEW_ZOMBIE_IT },
	{SND_INFECTION_KO, SND_INFECTED_KO, SND_NEW_ZOMBIE_KO },
	{SND_INFECTION_CH, SND_INFECTED_CH, SND_NEW_ZOMBIE_CH },
};


int Infection::calculateZombiePlayerIndex()
{
	c_network_session* session = NULL;
	network_life_cycle_in_squad_session(&session);

	if (session->get_player_count() > 0)
	{
		std::mt19937 mt_rand(rd());
		std::vector<int32> activePlayersIndices = NetworkSession::GetActivePlayerIndicesList();
		std::uniform_int_distribution<int> dist(0, session->get_player_count() - 1);
	
		if (activePlayersIndices.empty())
			return NONE;

		int32 infectedPlayerIndex = activePlayersIndices[dist(mt_rand)];
		LOG_TRACE_GAME(L"[h2mod-infection] random infection player index: {}, with name: {}", infectedPlayerIndex, session->get_player_name(infectedPlayerIndex));

		return infectedPlayerIndex;
	}

	return NONE;
}

void Infection::sendTeamChange()
{
	c_network_session* session = NULL;
	if (network_life_cycle_in_squad_session(&session))
	{
		if (session->is_host())
		{
			int32 player_count = session->get_player_count();

			if (player_count > 0)
			{
				int32 player_array_index = 0;
				datum player_indexes[k_maximum_players] = {};
				e_game_team player_teams[k_maximum_players] = {};

				for (int32 i = 0; i < k_maximum_players; i++)
				{
					if (session->is_session_player_active(i))
					{
						e_game_team team = zombiePlayerIndex == i ? k_zombie_team : k_humans_team;
						bool is_current_player_zombie = zombiePlayerIndex == i;

						player_indexes[player_array_index] = i;
						player_teams[player_array_index++] = team;

						LOG_TRACE_GAME(L"[h2mod-infection] sent team change packet to player index: {}, with name: {}, infected?: {}",
							i,
							session->get_player_name(i),
							is_current_player_zombie
						);
					}
				}

				session->switch_players_to_teams(player_indexes, player_array_index, player_teams);
			}
		}
	}
}

void Infection::triggerSound(e_infection_sounds sound, int sleep)
{
	const int language_id = *Memory::GetAddress<int*>(0x412818);

	if (infectionSoundTable[language_id][sound] != nullptr)
	{
		LOG_TRACE_GAME(L"[h2mod-infection] Triggering sound {}", infectionSoundTable[language_id][sound]);
		H2MOD::custom_sound_play(infectionSoundTable[language_id][sound], sleep);
	}
}

void Infection::InitClient()
{
	LOG_TRACE_GAME("[h2mod-infection] Disabling slayer sounds");
	H2MOD::disable_score_announcer_sounds(FLAG(_sound_type_slayer) | ALL_SOUNDS_NO_SLAYER);

	for (int16 i = 0; i < k_number_of_users; i++)
		infectedPlayed[i] = false;

	initialSpawn = true;

	//Change Local Player's Team to Human if Not in Green
	//(In case player wants to start as Alpha Zombie leave him green)

	for(int32 i = 0; i < k_number_of_users; i++)
	{
		e_controller_index user_controller = players_get_controller_index_from_user_index(i);
		if(user_controller != k_no_controller)
		{
			e_game_team team = (e_game_team)s_session_interface_globals::get()->users[i].properties.team_index;
			if (team != k_zombie_team) {
				user_interface_controller_set_desired_team_index(user_controller, k_humans_team);
				user_interface_controller_update_network_properties(user_controller);
			}
		}
	}
}

void Infection::resetZombiePlayerStatus() {
	zombieIdentifiers.clear();
}

void Infection::setZombiePlayerStatus(uint64 identifier)
{
	zombieIdentifiers.push_back(identifier);
}

void Infection::InitHost() {
	LOG_TRACE_GAME("[h2mod-infection] Host init setting unit speed patch");
	//Applying SpeedCheck fix
	H2MOD::set_unit_speed_patch(true);

	LOG_TRACE_GAME("[h2mod-infection] Host init resetting zombie player data status");
	Infection::resetZombiePlayerStatus();
}

bool Infection::shouldEndGame()
{
	int humanCount, zombieCount, playerCount;
	humanCount = zombieCount = playerCount = 0;
	player_iterator player_it;
	while (player_it.get_next_active_player())
	{
		int currentPlayerIndex = player_it.get_current_player_index();
		uint64 playerIdentifier = player_it.get_current_player_id();
		bool isZombie = std::find(Infection::zombieIdentifiers.begin(), Infection::zombieIdentifiers.end(), playerIdentifier) != Infection::zombieIdentifiers.end();

		if (isZombie)
			zombieCount++;
		else
			humanCount++;

		playerCount++;
	}

	return playerCount > 1 && (zombieCount == 0 || humanCount == 0);
}

void Infection::resetWeaponInteractionAndEmblems() {
	LOG_TRACE_GAME("[h2mod-infection] Resetting weapons interactions and emblem visibility");
	player_user_weapon_interaction_reset();
	hud_player_indicators_draw_reset();
}

void Infection::preSpawnServerSetup() {

	/*
		Game state players should be initialized when we are about to spawn a player
	*/

	c_network_session* session = NULL;
	network_life_cycle_in_squad_session(&session);
	
	player_iterator player_it;
	while (player_it.get_next_active_player())
	{
		int32 currentPlayerIndex = player_it.get_current_player_index();
		uint64 playerIdentifier = player_it.get_current_player_id();
		bool isZombie = std::find(Infection::zombieIdentifiers.begin(), Infection::zombieIdentifiers.end(), playerIdentifier) != Infection::zombieIdentifiers.end();

		bool zombie_team_status_human = isZombie == false && s_player::get_team(currentPlayerIndex) == k_zombie_team;
		if (zombie_team_status_human) {
			// if the player just joined the and he doesn't have zombie status, and his team is green, add him in the array
			setZombiePlayerStatus(playerIdentifier);
			isZombie = true;
		}

		LOG_TRACE_GAME(L"[h2mod-infection] Zombie pre spawn index={}, isZombie={}, playerIdentifier={}, playerName:{}", currentPlayerIndex, isZombie, playerIdentifier, s_player::get_name(currentPlayerIndex));
		if (isZombie) 
		{
			s_player::set_unit_character_type(currentPlayerIndex, _character_type_flood);
			if (s_player::get_team(currentPlayerIndex) != k_zombie_team) 
			{
				// prevent the fucks from switching to humans in the pre-game lobby after joining
				session->switch_player_team(player_it.get_current_player_datum_index(), k_zombie_team);
			}
		}
		else 
		{
			if (get_current_special_event() == _special_event_halloween && H2Config_spooky_boy)
				s_player::set_unit_character_type(currentPlayerIndex, _character_type_skeleton);
			else
				s_player::set_unit_character_type(currentPlayerIndex, _character_type_spartan);
		}
	}
}

void Infection::setPlayerAsHuman(int playerIndex) {
	if (get_current_special_event() == _special_event_halloween && H2Config_spooky_boy)
		s_player::set_unit_character_type(playerIndex, _character_type_skeleton);
	else
		s_player::set_unit_character_type(playerIndex, _character_type_spartan);
	s_player::set_unit_speed(playerIndex, 1.0f);
}

void Infection::setPlayerAsZombie(int playerIndex) {
	s_player::set_unit_character_type(playerIndex, _character_type_flood);
	s_player::set_unit_speed(playerIndex, 1.1f);

	call_give_player_weapon(playerIndex, e_weapons_datum_index::energy_blade, 1);
}

void Infection::onGameTick()
{
	if(get_game_life_cycle() == _life_cycle_in_game && NetworkSession::LocalPeerIsSessionHost())
	{
		if (get_game_time_ticks() > 0)
		{
			bool should_end_game = shouldEndGame();

			// check if the current game should be ended
			if (!should_end_game)
			{
				// and update the time if it shouldn't be
				last_time_at_game_should_not_end = get_game_time_ticks();
			}

			// check the difference between game time now
			// if the time wasn't updated for more than 5 seconds, end the game
			if (game_ticks_to_seconds((real32)(get_game_time_ticks() - last_time_at_game_should_not_end)) > 5.f)
			{
				NetworkSession::EndGame();
			}
		}
	}
}

void Infection::removeUnwantedItems()
{
	const datum shotgun_ammo_equip_datum = tag_loaded(_tag_group_equipment, "objects\\powerups\\shotgun_ammo\\shotgun_ammo");

	tag_iterator iterator;
	tag_iterator_new(&iterator, _tag_group_item_collection);

	while (tag_iterator_next(&iterator) != NONE)
	{
		const char* tag_name = tag_get_name(iterator.current_tag_index);
		if (strstr(tag_name, "multiplayer\\powerups") ||
			strncmp(tag_name, "multiplayer\\single_weapons\\frag_grenades", 256) == 0 ||
			strncmp(tag_name, "multiplayer\\single_weapons\\plasma_grenades", 256) == 0)
		{
			item_collection_definition* itmc = (item_collection_definition*)tag_get_fast(iterator.current_tag_index);

			for (int i = 0; i < itmc->item_permutations.count; i++)
			{
				itmc->item_permutations[i]->item.group.group = _tag_group_equipment;
				itmc->item_permutations[i]->item.index = shotgun_ammo_equip_datum;
			}
		}
	}

	//Replace vehicles with shotgun ammo
	scenario* scenario_definition = get_global_scenario();
	for (int32 i = 0; i < scenario_definition->netgame_equipment.count; i++)
	{
		scenario_netgame_equipment* netgame_equipment = scenario_definition->netgame_equipment[i];
		if (netgame_equipment->item_vehicle_collection.group.group == _tag_group_vehicle_collection)
		{
			netgame_equipment->classification = netgame_item_classification_powerup;
			netgame_equipment->item_vehicle_collection.group.group = _tag_group_item_collection;
			netgame_equipment->item_vehicle_collection.index = NONE;
		}
	}
}

void Infection::Initialize()
{
	LOG_TRACE_GAME("{} - infection initialization!");
	removeUnwantedItems();

	if (!Memory::IsDedicatedServer())
		Infection::InitClient();

	if (NetworkSession::LocalPeerIsSessionHost()) {
		Infection::InitHost();

		last_time_at_game_should_not_end = 0;
		zombiePlayerIndex = Infection::calculateZombiePlayerIndex();
		EventHandler::register_callback(onGameTick, EventType::game_loop, EventExecutionType::execute_after);

		LOG_TRACE_GAME("[h2mod-infection] Peer host calculated zombie index {}", zombiePlayerIndex);
		if (zombiePlayerIndex == NONE) {
			LOG_TRACE_GAME("[h2mod-infection] Failed selecting a zombie!");
		}
		else {
			LOG_TRACE_GAME("[h2mod-infection] Peer host setting player as human");
			//send out the team change packets to peers
			Infection::sendTeamChange();
			Infection::setZombiePlayerStatus(NetworkSession::GetPlayerId(zombiePlayerIndex));
		}
	}
}

void Infection::Dispose()
{
	LOG_TRACE_GAME("{} - infection dispose!");

	if(NetworkSession::LocalPeerIsSessionHost())
	{
		EventHandler::remove_callback(onGameTick, EventType::game_loop, EventExecutionType::execute_after);
	}

	Infection::resetWeaponInteractionAndEmblems();
	if (!game_is_predicted()) {
		H2MOD::set_unit_speed_patch(false);
	}

	return;
}

CustomVariantId Infection::GetVariantId()
{
	return CustomVariantId::_id_infection;
}

void Infection::OnMapLoad(ExecTime execTime, s_game_options* options)
{
	switch (execTime)
	{
	case ExecTime::_preEventExec:
		break;

	case ExecTime::_postEventExec:
		switch (options->game_mode)
		{
			// cleanup when loading main menu
		case _game_mode_multiplayer:
			this->Initialize();
			break;
			/*case _main_menu:
				this->Dispose();
				break;*/
		default:
			break;
		}

		break;

	case ExecTime::_ExecTimeUnknown:
	default:
		LOG_TRACE_GAME("{} - unknown execTime", __FUNCTION__);
		break;
	}
}

void Infection::OnPlayerDeath(ExecTime execTime, datum player_index)
{
	int32 absPlayerIdx = DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index);
	datum playerUnitDatum = s_player::get_unit_index(player_index);

	switch (execTime)
	{
	case ExecTime::_preEventExec:
		// to note after the original function executes, the controlled unit by this player is set to NONE
		if (game_engine_in_round())
		{
			if (!Memory::IsDedicatedServer())
			{
				if (s_player::get_team(player_index) != k_zombie_team)
				{
					s_player* player = s_player::get(player_index);

					if (player->user_index != NONE)
					{
						LOG_TRACE_GAME(L"[h2mod-infection] Infected local player, Name={}, identifier={}", s_player::get_name(player_index_from_user_index(player->user_index)), player->identifier);
						user_interface_controller_set_desired_team_index(player->controller_index, k_zombie_team);
						user_interface_controller_update_network_properties(player->controller_index);
						s_player::set_unit_character_type(player_index, _character_type_flood);
					}
					else
					{
						//if not, then this is a new zombie
						LOG_TRACE_GAME(L"[h2mod-infection] Player died, name={}, identifer={}", s_player::get_name(player_index), player->identifier);
						Infection::triggerSound(_snd_new_zombie, 1000);
					}
				}
			}

			// host code
			if (!game_is_predicted())
			{
				void* unit_object = object_try_and_get_and_verify_type(playerUnitDatum, _object_mask_biped);
				if (unit_object) {
					if (unit_get_team_index(playerUnitDatum) != k_zombie_team) {
						Infection::setZombiePlayerStatus(s_player::get_id(player_index));
					}
					else {
						// take away zombie's weapons
						unit_delete_all_weapons(playerUnitDatum);
					}
				}
			}
		}
		break;

	case ExecTime::_postEventExec:
		break;

	case ExecTime::_ExecTimeUnknown:
	default:
		LOG_TRACE_GAME("{} - unknown execTime", __FUNCTION__);
		break;
	}
}

void Infection::OnPlayerSpawn(ExecTime execTime, datum playerIdx)
{
	int absPlayerIdx = DATUM_INDEX_TO_ABSOLUTE_INDEX(playerIdx);
	datum playerUnitDatum = s_player::get_unit_index(playerIdx);

	switch (execTime)
	{
		// prespawn handler
	case ExecTime::_preEventExec:
		// to note after the original function executes, the controlled unit by this player is set to NONE

		if (!Memory::IsDedicatedServer())
		{
			s_player* player = s_player::get(playerIdx);
			LOG_TRACE_GAME(L"[h2mod-infection] Client pre spawn, playerIndex={}, playerIdentifier={}", absPlayerIdx, player->identifier);

			if(player->user_index != NONE)
			{
				e_game_team team = (e_game_team)s_session_interface_globals::get()->users[player->user_index].properties.team_index;

				// change biped if user is a zombie

				if(team == k_zombie_team)
				{
					LOG_TRACE_GAME("[h2mod-infection] Client is infected! switching bipeds: {}", absPlayerIdx);
					s_player::set_unit_character_type(playerIdx, _character_type_flood);
				}
			}
		}

		if (!game_is_predicted())
			Infection::preSpawnServerSetup();
		break;

		// postspawn handler
	case ExecTime::_postEventExec:

		// client only
		if (!Memory::IsDedicatedServer())
		{
			s_player* player = s_player::get(playerIdx);

			if(player->user_index != NONE)
			{
				if(initialSpawn)
				{
					triggerSound(e_infection_sounds::_snd_infection, 1000);
					initialSpawn = false;
				}

				e_game_team team = (e_game_team)s_session_interface_globals::get()->users[player->user_index].properties.team_index;

				if(team == k_zombie_team && !infectedPlayed[player->user_index])
				{
					triggerSound(_snd_infected, 500);
					infectedPlayed[player->user_index] = true;
				}
				if(team == k_humans_team)
				{
					player_user_weapon_interaction_set(player->user_index, true);
					hud_player_indicators_draw_enabled_set(player->user_index, false);
				}
				else if (team == k_zombie_team)
				{
					s_player::set_unit_character_type(playerIdx, _character_type_flood);
					player_user_weapon_interaction_set(player->user_index, false);
					hud_player_indicators_draw_enabled_set(player->user_index, true);
				}
			}
		}

		// host only (both client/dedicated server)
		if (!game_is_predicted())
		{
			LOG_TRACE_GAME("[h2mod-infection] Spawn player server index={}", absPlayerIdx);
			void* unit_object = object_try_and_get_and_verify_type(playerUnitDatum, _object_mask_biped);
			if (unit_object) {
				//if the unit_object data pointer is not nullptr, the spawned object is "alive"
				e_game_team team = unit_get_team_index(playerUnitDatum);
				LOG_TRACE_GAME("[h2mod-infection] Spawn player server index={}, unit team index={}", absPlayerIdx, (int16)team);
				if (team == k_humans_team) {
					Infection::setPlayerAsHuman(absPlayerIdx);
				}

				if (team == k_zombie_team) {
					Infection::setPlayerAsZombie(absPlayerIdx);
				}
			}
		}

		break;

	case ExecTime::_ExecTimeUnknown:
	default:
		LOG_TRACE_GAME("{} - unknown execTime", __FUNCTION__);
		break;
	}
}
