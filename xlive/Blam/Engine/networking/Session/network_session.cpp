#include "stdafx.h"
#include "network_session.h"

#include "game/game.h"
#include "networking/logic/life_cycle_manager.h"

#include "interface/user_interface_controller.h"

/* public code */

/* PRIVATE NAMESPACE, acts like static keyword */
// 
namespace NetworkSession
{
	c_network_session* GetNetworkSessions();
	c_network_session* GetActiveNetworkSession();
}

bool NetworkSession::PlayerIsActive(datum player_index)
{
	return GetActiveNetworkSession()->is_session_player_active(player_index);
}

std::vector<uint64> NetworkSession::GetActivePlayerIdList()
{
	std::vector<uint64> activePlayerIdList;
	if (GetPlayerCount() > 0)
	{
		for (int32 playerIdx = 0; playerIdx < k_maximum_players; playerIdx++)
		{
			if (PlayerIsActive(playerIdx))
			{
				uint64 playerId = GetPlayerId(playerIdx);
				activePlayerIdList.emplace_back(playerId);
			}
		}
	}

	return activePlayerIdList;
}

std::vector<int32> NetworkSession::GetActivePlayerIndicesList()
{
	std::vector<int32> activePlayersIndices;
	if (GetPlayerCount() > 0)
	{
		for (int32 playerIndex = 0; playerIndex < k_maximum_players; playerIndex++)
		{
			if (PlayerIsActive(playerIndex))
				activePlayersIndices.emplace_back(playerIndex);
		}
	}

	return activePlayersIndices;
}

c_network_session* NetworkSession::GetNetworkSessions()
{
	return *Memory::GetAddress<c_network_session**>(0x51C474, 0x520B94);
}

c_network_session* NetworkSession::GetActiveNetworkSession()
{
	// ### FIXME replace this function wherever this is still used
	// for now, return the pointer to the active squad session regardless to avoid crashing the game
	/*c_network_session* session = NULL;
	network_life_cycle_in_squad_session(&session);*/
	return c_game_life_cycle_manager::get()->m_active_squad_session;
}

bool NetworkSession::LocalPeerIsSessionHost()
{
	return GetActiveNetworkSession()->is_host();
}

bool NetworkSession::LocalPeerIsSessionLeader()
{
	return GetActiveNetworkSession()->is_local_peer_session_leader();
}

bool NetworkSession::LocalPeerIsEstablished()
{
	return GetActiveNetworkSession()->established();
}

// returns NONE (-1) if fails
int32 NetworkSession::GetPeerIndexFromNetworkAddress(const network_address* address)
{
	return GetActiveNetworkSession()->get_peer_index_from_address(address);
}

bool NetworkSession::GetMapFileLocation(wchar_t* buffer, size_t size)
{
	// host-only
	typedef bool(__thiscall* get_map_file_location_t)(c_network_session* session, wchar_t* buffer, size_t size);
	auto p_get_map_file_location = Memory::GetAddress<get_map_file_location_t>(0x1C5678, 0x19CD4A);
	return p_get_map_file_location(GetActiveNetworkSession(), buffer, size);
}

int32 NetworkSession::GetPeerCount()
{
	return GetActiveNetworkSession()->m_session_membership.peer_count;
}

int32 NetworkSession::GetLocalPeerIndex()
{
	return GetActiveNetworkSession()->get_local_peer_index();
}

IN_ADDR NetworkSession::GetLocalNetworkAddress()
{
	return GetActiveNetworkSession()->m_session_membership.membership_peers[GetLocalPeerIndex()].secure_address.inaOnline;
}

int32 NetworkSession::GetPeerIndex(datum player_index)
{
	return GetPlayerInformation(player_index)->peer_index;
}

int32 NetworkSession::GetPlayerCount()
{
	return GetActiveNetworkSession()->m_session_membership.player_count;
}

s_membership_player* NetworkSession::GetPlayerInformation(datum player_index)
{
	return &GetActiveNetworkSession()->m_session_membership.players[DATUM_INDEX_TO_ABSOLUTE_INDEX(player_index)];
}

const wchar_t* NetworkSession::GetPlayerName(datum player_index)
{
	return GetPlayerInformation(player_index)->properties[0].player_name;
}

uint64 NetworkSession::GetPlayerId(datum player_index)
{
	return GetPlayerInformation(player_index)->identifier;
}

int8 NetworkSession::GetPlayerTeam(datum player_index)
{
	return GetPlayerInformation(player_index)->properties[0].team_index;
}

int32 NetworkSession::GetPeerIndexFromId(uint64 xuid)
{
	if (GetPlayerCount() > 0)
	{
		for (int32 player_index = 0; player_index < k_maximum_players; player_index++)
		{
			if (PlayerIsActive(player_index) && GetPlayerId(player_index) == xuid)
				return GetPeerIndex(player_index);
		}
	}
	return NONE;
}

void NetworkSession::KickPeer(int32 peer_index)
{
	typedef void(__thiscall* game_session_boot_t)(c_network_session*, int, bool);
	auto p_game_session_boot = Memory::GetAddress<game_session_boot_t>(0x1CCE9B);

	if (peer_index < GetPeerCount())
	{
		LOG_TRACE_GAME("{} - about to kick peer index = {}", __FUNCTION__, peer_index);
		p_game_session_boot(NetworkSession::GetActiveNetworkSession(), peer_index, true);
	}
}

void NetworkSession::EndGame()
{
	INVOKE(0x215470, 0x197F32, NetworkSession::EndGame);
}

wchar_t* NetworkSession::GetGameVariantName()
{
	return GetActiveNetworkSession()->m_session_parameters.game_variant.variant_name;
}

bool NetworkSession::IsVariantTeamPlay()
{
	return TEST_BIT(GetActiveNetworkSession()->m_session_parameters.game_variant.flags, _game_engine_teams_bit);
}

void NetworkSession::LeaveSession()
{
	if (Memory::IsDedicatedServer())
		return;

	if (game_is_ui_shell())
	{
		// request_squad_browser
		*Memory::GetAddress<bool*>(0x978BAC) = true;

		typedef void(__cdecl* load_main_menu_with_context_t)(int context);
		auto p_load_main_menu_with_context = Memory::GetAddress<load_main_menu_with_context_t>(0x08EAF);
		p_load_main_menu_with_context(0);
	}

	typedef int(__cdecl* leave_game_type_t)(int a1);
	auto p_leave_session = Memory::GetAddress<leave_game_type_t>(0x216388);
	p_leave_session(0);
}

s_session_interface_globals* s_session_interface_globals::get()
{
	return Memory::GetAddress<s_session_interface_globals*>(0x51A590, 0x520408);
}

s_session_interface_user* session_interface_get_local_user_properties(int32 user_index)
{
	return &s_session_interface_globals::get()->users[user_index];
}

e_network_session_class network_squad_session_get_session_class()
{
	//return INVOKE(0x1B1643, 0x0, network_squad_session_get_session_class);

	e_network_session_class out_class = _network_session_class_unknown;
	c_network_session* session = NULL;
	if (network_life_cycle_in_squad_session(&session))
	{
		if (session->established())
		{
			out_class = session->m_session_class;
		}
	}
	return out_class;
}


bool network_session_interface_set_local_user_character_type(int32 user_index, e_character_type character_type)
{
	s_session_interface_user* user_properties = session_interface_get_local_user_properties(user_index);
	
	// Don't change the character type if the user doesn't exist
	if (user_properties->user_exists)
	{
		user_properties->properties.profile_traits.profile.player_character_type = character_type;
		return true;
	}

	return false;
}

bool network_session_interface_get_local_user_identifier(int32 user_index, s_player_identifier* out_identifier)
{
	s_session_interface_user* user_properties = session_interface_get_local_user_properties(user_index);
	if (user_properties->user_exists)
	{
		*out_identifier = user_properties->network_user_identifier;
		return true;
	}
	return false;
}

void network_session_interface_set_local_user_rank(int32 user_index, int8 rank)
{
	s_session_interface_user* user_properties = session_interface_get_local_user_properties(user_index);
	user_properties->properties.player_displayed_skill = rank;
	user_properties->properties.player_overall_skill = rank;
	return;
}

bool __cdecl network_session_interface_get_local_user_properties(int32 user_index, int32* out_controller_index, s_player_properties* out_properties, int32* out_player_voice, int32* out_player_text_chat)
{
	return INVOKE(0x1B10E0, 0x1970A8, network_session_interface_get_local_user_properties, user_index, out_controller_index, out_properties, out_player_voice, out_player_text_chat);
}

void __cdecl network_globals_switch_environment(int32 a1, bool a2)
{
	INVOKE(0x1B54CF, 0x1A922D, network_globals_switch_environment, a1, a2);
}

void network_session_apply_patches()
{
}

void network_session_membership_update_local_players_teams()
{
	c_network_session* session = NULL;
	if (network_life_cycle_in_squad_session(&session))
	{
		if (session->established() || session->local_state_joining_session())
		{
			int32 local_peer_index = session->get_local_peer_index();

			for (int32 i = 0; i < k_number_of_users; i++)
			{
				datum player_index = session->get_peer_membership(local_peer_index)->local_players_indexes[i];
				if (player_index != NONE)
				{
					const s_membership_player* membership_player = session->get_player_membership(player_index);
					user_interface_controller_set_desired_team_index((e_controller_index)i, (e_game_team)membership_player->properties[0].team_index);
					user_interface_controller_update_network_properties((e_controller_index)i);
				}
			}
		}
	}
}

void network_session_set_player_team(datum player_index, e_game_team team)
{
	c_network_session* session = NULL;
	if (network_life_cycle_in_squad_session(&session))
	{
		if (session->established() 
			&& session->is_host())
		{
			s_membership_player* membership_player = session->get_player_membership(player_index);
			membership_player->properties[0].team_index = team;

			if (session->peer_index_local_peer(membership_player->peer_index))
			{
				user_interface_controller_set_desired_team_index((e_controller_index)membership_player->controller_index, (e_game_team)membership_player->properties[0].team_index);
				user_interface_controller_update_network_properties((e_controller_index)membership_player->controller_index);
			}
		}
	}
}

void c_network_session::switch_players_to_teams(datum* player_indexes, int32 player_count, e_game_team* team_indexes)
{
	if (is_host())
	{
		for (int32 i = 0; i < player_count; i++)
		{
			get_player_membership(player_indexes[i])->properties[0].team_index = team_indexes[i];
		}
		request_membership_update();
		network_session_membership_update_local_players_teams();
	}
}