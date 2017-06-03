#include <windows.h>
#include "H2MOD_Infection.h"
#include "H2MOD.h"
#include "xliveless.h"
#include "h2mod.pb.h"
#include <stdlib.h>
#include <mutex>

extern bool isHost;
bool infected_played = false;
bool first_spawn = true;
int zombie = 0;
int PlayerCount = (h2mod->Server) ? 0x3000470C : 0x30004B60;
int L_index;




void Infection::FindZombie()
{
	TRACE_GAME("[H2MOD-Infection] - FindZombie()");
	
	int genrand = rand();
	int sizeofgroup = h2mod->NetworkPlayers.size();
	zombie = genrand % (sizeofgroup + 1); //Random Alpha Zombie Index

#pragma region LocalHostArea
	if (!h2mod->Server)
	{	

		InfectionPlayer *Local = new InfectionPlayer;
		wcscpy(&Local->PlayerName[0], h2mod->get_local_player_name());

		TRACE_GAME("[H2Mod-Infection] - Zombie: %i", zombie);

		if (zombie == 0)
		{
			TRACE_GAME("[H2Mod-Infection] - Local host is zombie!");


			Local->infected = true;
			infected_players[Local] = true;

			h2mod->set_local_team_index(3);

		}
		else
		{
			TRACE_GAME("[H2Mod-Infection] - Local host is a human!");
			Local->infected = false;
			infected_players[Local] = false;

			h2mod->set_local_team_index(0);
		}

	}
#pragma endregion
#pragma region NetworkPlayers	
	if (h2mod->NetworkPlayers.size() > 0)
	{
		zombie--;
		//zombie = rand() % (h2mod->NetworkPlayers.size());
		int i = 0;
		for (auto it = h2mod->NetworkPlayers.begin(); it != h2mod->NetworkPlayers.end(); ++it)
		{
			InfectionPlayer *nPlayer = new InfectionPlayer;
			wcscpy(&nPlayer->PlayerName[0], it->first->PlayerName);

			//If NetworkPlayer index(i)  matches The Alpha Zombie index.Start Infecting him.
			if (i == zombie)
			{
				TRACE_GAME("[H2Mod-Infection] Found Zombie: %ws", it->first->PlayerName);

				//Adding to the list of Infected Players.
				nPlayer->infected = true;
				infected_players[nPlayer] = true;   

				H2ModPacket teampak;

				teampak.set_type(H2ModPacket_Type_set_player_team);
				h2mod_set_team *set_team = teampak.mutable_h2_set_player_team();

				set_team->set_team(3);
				set_team->set_name((char*)it->first->PlayerName, 32);

				char* SendBuf = new char[teampak.ByteSize()];
				teampak.SerializeToArray(SendBuf, teampak.ByteSize());

				it->first->PacketData = SendBuf;
				it->first->PacketSize = teampak.ByteSize();
				it->first->PacketsAvailable = true;

			}
			else
			{
				TRACE_GAME("[H2Mod-Infection] %ws is a human!", it->first->PlayerName);

				nPlayer->infected = false;
				infected_players[nPlayer] = false;
			}

			i++;
		}
	}
#pragma endregion

}



void Infection::Initialize()
{
	
#pragma region H2v Stuffs
	if (!h2mod->Server)
	{
		h2mod->DisableSound(SoundType::Slayer);
		h2mod->DisableSound(SoundType::GainedTheLead);
		h2mod->DisableSound(SoundType::LostTheLead);
		h2mod->DisableSound(SoundType::TeamChange);

		infected_played = false;
		first_spawn = true;

		//Change Local Player's Team to Human if Not in Green
		//(In case player wants to start as Alpha Zombie leave him green)
		if (h2mod->get_local_team_index() != 3)
			h2mod->set_local_team_index(0);
	}
#pragma endregion
#pragma region Host/Dedi Work
	if (isHost||h2mod->Server)
	{
		TRACE_GAME("[H2Mod-Infection] - Initializing!");
		TRACE_GAME("[H2Mod-Infection] - this->infected_players.size(): %i", this->infected_players.size());

		h2mod->set_unit_speed_patch(true);//Applying SpeedCheck fix
		h2mod->PatchAutoPickups(true);
		
		


		//This Block is to Reset Infected Players List(For a New Match)
		if (this->infected_players.size() > 0)
		{
			for (auto it = this->infected_players.begin(); it != this->infected_players.end(); ++it)
			{
				delete[] it->first;
			}

			this->infected_players.clear();
		}
		//Find our Alpha Zombie and Turn rest to Humans
		this->FindZombie();
	}
#pragma endregion 
	h2mod->PatchNewRound(true);	  //OnNew_Round Patch
	
	

}

void Infection::Deinitialize()
{
	TRACE_GAME("[H2Mod-Infection] - Deinitializing!");
	
	h2mod->PatchNewRound(false);
	if (isHost||h2mod->Server)
	{	

		h2mod->set_unit_speed_patch(false);
		h2mod->PatchAutoPickups(false);
		
	}
}

void Infection::PreSpawn(int PlayerIndex)
{

	TRACE_GAME("[H2Mod-Infection] - Prespawn( %i ) ", PlayerIndex);

	wchar_t* playername = h2mod->get_player_name_from_index(PlayerIndex);

#pragma region NetworkPlayers(Client)
	if (!isHost&&!h2mod->Server)
	{
		//If Player being spawned is LocalUser/Player
		if (wcscmp(playername, h2mod->get_local_player_name()) == 0)
		{
			//Change Biped if LocalUser is in GreenTeam
			if (h2mod->get_local_team_index() == 3)
			{
				h2mod->set_unit_biped(BipedType::Elite, PlayerIndex);

			}
		}
	}

#pragma endregion

#pragma region LocalHost/Dedi Work

	if (isHost||h2mod->Server)
	{

		for (auto it = this->infected_players.begin(); it != this->infected_players.end(); ++it)
		{
			if (wcscmp(playername, &it->first->PlayerName[0]) == 0)
			{
				TRACE_GAME("[H2mod-Infection] - Prespawn(%i) player %ws infection value is : %i", PlayerIndex, it->first->PlayerName, it->first->infected);
				TRACE_GAME("[H2Mod-Infection] - Prespawn(%i) playername %ws : %ws", PlayerIndex, playername, it->first->PlayerName);

                //If Player Being Spawned is in the list of Infected Players
				if (it->first->infected == true)
				{
					h2mod->set_unit_biped(BipedType::Elite, PlayerIndex);
				}
				else
				{
					h2mod->set_unit_biped(BipedType::MasterChief, PlayerIndex);
				}
			}
		}

	}
#pragma endregion
}

void Infection::SpawnPlayer(int PlayerIndex)
{
	TRACE_GAME("[H2Mod-Infection] - SpawnPlayer(%i)", PlayerIndex);

#pragma region InfectionHandling
	if (isHost||h2mod->Server)
	{
		int unit_datum_index = h2mod->get_unit_datum_from_player_index(PlayerIndex);
		int unit_object = call_get_object(unit_datum_index, 3);

		if (unit_object)//if Spawned(alive)
		{
			
#pragma region HumansPowerUp
			if (h2mod->get_unit_team_index(unit_datum_index) == 0)
			{

					h2mod->set_unit_biped(BipedType::MasterChief, PlayerIndex);
					h2mod->set_unit_speed(1.0f, PlayerIndex);
					GivePlayerWeapon(PlayerIndex, Weapon::shotgun, 1);
					GivePlayerWeapon(PlayerIndex, Weapon::magnum, 0);
				
			}
#pragma endregion
#pragma region ZombiesPowerUp
			if (h2mod->get_unit_team_index(unit_datum_index) == 3)
			{
#pragma region AlphaZombie

				if (PlayerIndex == zombie)
				{
					h2mod->set_unit_biped(BipedType::Elite, PlayerIndex);
					h2mod->set_unit_speed(1.2f, PlayerIndex);
					GivePlayerWeapon(PlayerIndex, Weapon::energy_blade, 1);
				}
#pragma endregion 
				else
				{
					h2mod->set_unit_biped(BipedType::Elite, PlayerIndex);
					h2mod->set_unit_speed(1.1f, PlayerIndex);
					GivePlayerWeapon(PlayerIndex, Weapon::energy_blade, 1);
				}
				
			}
#pragma endregion
		}
	}

#pragma endregion
#pragma region SoundHandling
	if (!h2mod->Server)
	{
		wchar_t* playername = h2mod->get_player_name_from_index(PlayerIndex);
		if (wcscmp(playername, h2mod->get_local_player_name()) == 0)
		{
			if (first_spawn == true)
			{
				h2mod->SoundMap[L"sounds/infection.wav"] = 1000;


				first_spawn = false;
			}

			if (h2mod->get_local_team_index() == 3 && infected_played == false)
			{
				h2mod->SoundMap[L"sounds/infected.wav"] = 500;

				infected_played = true;
			}
			if (h2mod->get_local_team_index() == 3)
			{
				h2mod->PatchWeaponsInteraction(false);
				h2mod->PatchVehicleInteraction(false);
			}
			

		}
	}
#pragma endregion 
}

void Infection::PlayerInfected(int unit_datum_index)
{
	TRACE_GAME("[H2Mod-Infection] - PlayerInfected()", unit_datum_index);

#pragma region Host/Dedi Stuff
	
	int unit_object = call_get_object(unit_datum_index, 3);
	if (unit_object)
	{		
			int pIndex = h2mod->get_player_index_from_unit_datum(unit_datum_index);
			if (isHost || h2mod->Server)
			{
		
				//Add the Dead Player to The List of Infected	

				wchar_t* playername = h2mod->get_player_name_from_index(pIndex);
				for (auto it = this->infected_players.begin(); it != this->infected_players.end(); ++it)
				{
					if (wcscmp(playername, &it->first->PlayerName[0]) == 0)
					{
						TRACE_GAME("[H2Mod-Infection] - PlayerInfected() PlayerInfected %ws compared to %ws", playername, &it->first->PlayerName[0]);
						TRACE_GAME("[H2Mod-Infection] PlayerInfected() Made %ws infected!", &it->first->PlayerName[0]);
						it->first->infected = true;
					}
				}

				//call_unit_reset_equipment(unit_datum_index);//Take away Weapons.


			}

#pragma endregion
#pragma region H2v Stuffs
			if (!h2mod->Server)
			{ 

				if (unit_object)
				{
					if (h2mod->get_unit_team_index(unit_datum_index) != 3)
					{
						if (isHost)
							h2mod->set_unit_biped(BipedType::Elite, pIndex);

						TRACE_GAME("[H2Mod-Infection] PlayerInfected() player died: %ws", h2mod->get_player_name_from_index(pIndex));
						TRACE_GAME("[H2Mod-Infection] PlayerInfected() Local Player: %ws", h2mod->get_local_player_name());
						TRACE_GAME("[H2Mod-Infection] PlayerInfected() pIndex: %08X", unit_datum_index, pIndex);


						//If LocalUser/Player has Died.Change Teams to Green(Zombie)
						if (wcscmp(h2mod->get_player_name_from_index(pIndex), h2mod->get_local_player_name()) == 0)
						{
							h2mod->set_local_team_index(3);
						}
						//If Any other NetworkPlayer has Died.Play Sound.
						else if (*(BYTE*)(unit_object+0xAA)==0)
						{
							h2mod->SoundMap[L"sounds/new_zombie.wav"] = 1000;
							if (this->GetLasManStandingIndex() != -1)
								h2mod->SoundMap[L"sounds/last_man_standing.mp3"] = 1000;

						}
					}

				}
			}
#pragma endregion
			if (h2mod->Server || isHost)
			{

				if (*(BYTE*)PlayerCount > 1)
				{
					if (this->ZombiesCount() == *(BYTE*)PlayerCount)
						h2mod->CallRoundManage(0);
				}

				L_index = this->GetLasManStandingIndex();

				if (L_index != -1)
				{
					int DA = h2mod->get_dynamic_player_base(L_index, 0);
					*(float*)(DA + 0xF0) = 2.0f;
				}
			}
	}

	

}

void Infection::NextRound()
{
	
	if (!h2mod->Server)
	{
		h2mod->set_local_team_index(0);
		h2mod->PatchWeaponsInteraction(true);
		h2mod->PatchVehicleInteraction(true);
	}

	if (isHost || h2mod->Server)
	{
		TRACE_GAME("[H2Mod-Infection] - Starting New Round!");	
		
		
	//Reset Infected Players List(For a New Round)
		if (this->infected_players.size() > 0)
		{
			for (auto it = this->infected_players.begin(); it != this->infected_players.end(); ++it)
			{
				delete[] it->first;
			}

			this->infected_players.clear();
		}
		//Find our Alpha Zombie and Turn rest to Humans
		this->FindZombie();
	}

}

int Infection::ZombiesCount()
{
	int z = 0;
	
	for (int i = 0;i < *(BYTE*)PlayerCount;i++)
	{
		if (h2mod->get_Player_team_index(i) == 3)
			z++;
	}
	return z;
}

int Infection::GetLasManStandingIndex()
{
	
	int r = -1;
	if (*(BYTE*)PlayerCount - this->ZombiesCount() == 1)
	{
		for (int i = 0;i < *(BYTE*)PlayerCount;i++)
		{
			if (h2mod->get_Player_team_index(i) == 0)
			{
				r = i;
				break;
			}
		}
	}

	return r;
}
