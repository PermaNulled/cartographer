#include "stdafx.h"
#include "NetworkMessageTypeCollection.h"

#include "cartographer/twizzler/twizzler.h"
#include "game/game_allegiance.h"
#include "interface/user_interface_controller.h"
#include "memory/bitstream.h"
#include "networking/transport/network_channel.h"

#include "H2MOD/Modules/CustomVariantSettings/CustomVariantSettings.h"
#include "H2MOD/Modules/EventHandler/EventHandler.hpp"
#include "H2MOD/Modules/MapManager/MapManager.h"

uint8 g_network_message_type_collection[e_network_message_type_collection::k_network_message_type_collection_count * 32];

void register_network_message(void* network_message_collection, int type, const char* name, int a4, int size1, int size2, void* write_packet_method, void* read_packet_method, void* unk_callback)
{
	typedef void(__thiscall* register_packet_t)(void*, int, const char*, int, int, int, void*, void*, void*);
	auto register_packet = Memory::GetAddress<register_packet_t>(0x1E81D6, 0x1CA199);
	return register_packet(network_message_collection, type, name, a4, size1, size2, write_packet_method, read_packet_method, unk_callback);
}

const char* GetNetworkMessageName(int type)
{
	return network_message_type_collection_name[type];
}

bool MessageIsCustom(e_network_message_type_collection type)
{
	return type > e_network_message_type_collection::_test;
}

void __cdecl encode_map_file_name_message(c_bitstream* stream, int a2, const s_custom_map_filename* data)
{
	stream->write_string_wchar("map-file-name", &data->file_name, ARRAYSIZE(data->file_name));
	stream->write_integer("map-download-id", data->map_download_id, CHAR_BIT * sizeof(data->map_download_id));
}
bool __cdecl decode_map_file_name_message(c_bitstream* stream, int a2, s_custom_map_filename* data)
{
	stream->read_string_wchar("map-file-name", &data->file_name, ARRAYSIZE(data->file_name));
	data->map_download_id = stream->read_integer("map-download-id", CHAR_BIT * sizeof(data->map_download_id));
	return stream->error_occured() == false;
}

void __cdecl encode_request_map_filename_message(c_bitstream* stream, int a2, s_request_map_filename* data)
{
	stream->write_raw_data("user-identifier", &data->player_id, player_identifier_size_bits);
	stream->write_integer("map-download-id", data->map_download_id, CHAR_BIT * sizeof(data->map_download_id));
}
bool __cdecl decode_request_map_filename_message(c_bitstream* stream, int a2, s_request_map_filename* data)
{
	stream->read_raw_data("user-identifier", &data->player_id, player_identifier_size_bits);
	data->map_download_id = stream->read_integer("map-download-id", CHAR_BIT * sizeof(data->map_download_id));
	return stream->error_occured() == false;
}

void __cdecl encode_team_change_message(c_bitstream* stream, int a2, const s_team_change* data)
{
	stream->write_integer("team-index", data->team_index, SIZEOF_BITS(e_game_team));
}
bool __cdecl decode_team_change_message(c_bitstream* stream, int a2, s_team_change* data)
{
	data->team_index = (e_game_team)stream->read_integer("team-index", SIZEOF_BITS(e_game_team));
	return stream->error_occured() == false;
}

void __cdecl encode_rank_change_message(c_bitstream* stream, int a2, const s_rank_change* data)
{
	stream->write_integer("rank", data->rank, CHAR_BITS);
}
bool __cdecl decode_rank_change_message(c_bitstream* stream, int a2, s_rank_change* data)
{
	data->rank = (int8)stream->read_integer("rank", CHAR_BITS);
	return stream->error_occured() == false;
}

void __cdecl encode_anti_cheat_message(c_bitstream* stream, int a2, const s_anti_cheat* data)
{
	stream->write_bool("", data->enabled);
}
bool __cdecl decode_anti_cheat_message(c_bitstream* stream, int a2, s_anti_cheat* data)
{
	data->enabled = stream->read_bool("");
	return stream->error_occured() == false;
}

void register_custom_network_message(void* network_messages)
{
	typedef void(__cdecl* register_test_packet_t)(void* network_messages);
	auto p_register_test_message = Memory::GetAddress<register_test_packet_t>(0x1ECE05, 0x1CD7BE);

	p_register_test_message(network_messages);

	register_network_message(network_messages, _request_map_filename, "request-map-filename", 0, sizeof(s_request_map_filename), sizeof(s_request_map_filename),
		(void*)encode_request_map_filename_message, (void*)decode_request_map_filename_message, NULL);

	register_network_message(network_messages, _custom_map_filename, "map-file-name", 0, sizeof(s_custom_map_filename), sizeof(s_custom_map_filename),
		(void*)encode_map_file_name_message, (void*)decode_map_file_name_message, NULL);

	register_network_message(network_messages, _rank_change, "rank-change", 0, sizeof(s_rank_change), sizeof(s_rank_change),
		(void*)encode_rank_change_message, (void*)decode_rank_change_message, NULL);

	register_network_message(network_messages, _anti_cheat, "anti-cheat", 0, sizeof(s_anti_cheat), sizeof(s_anti_cheat),
		(void*)encode_anti_cheat_message, (void*)decode_anti_cheat_message, NULL);

	register_network_message(network_messages, _custom_variant_settings, "variant-settings", 0, CustomVariantSettingsPacketSize, CustomVariantSettingsPacketSize,
		(void*)CustomVariantSettings::EncodeVariantSettings, (void*)CustomVariantSettings::DecodeVariantSettings, NULL);
}

typedef void(__stdcall* handle_out_of_band_message_t)(void* thisx, network_address* address, e_network_message_type_collection message_type, int a4, void* packet);
handle_out_of_band_message_t p_handle_out_of_band_message;

void __stdcall handle_out_of_band_message_hook(void* thisx, network_address* address, e_network_message_type_collection message_type, int a4, void* packet)
{
	/* surprisingly the game doesn't use this too much, pretty much for request-join and time-sync packets */
	LOG_TRACE_NETWORK("{} - Received message: {} from peer index: {}",
		__FUNCTION__, GetNetworkMessageName(message_type), NetworkSession::GetPeerIndexFromNetworkAddress(address));

	if (!MessageIsCustom(message_type))
		p_handle_out_of_band_message(thisx, address, message_type, a4, packet);
}

typedef void(__stdcall* handle_channel_message_t)(void* thisx, int network_channel_index, e_network_message_type_collection message_type, int dynamic_data_size, void* packet);
handle_channel_message_t p_handle_channel_message;

void __stdcall handle_channel_message_hook(void* thisx, int network_channel_index, e_network_message_type_collection message_type, int dynamic_data_size, void* packet)
{
	/*
		This handles received in-band data
	*/

	network_address addr;
	ZeroMemory(&addr, sizeof(network_address));
	s_network_channel* peer_network_channel = s_network_channel::get(network_channel_index);

	switch (message_type)
	{
	case _request_map_filename:
	{
		s_request_map_filename* received_data = (s_request_map_filename*)packet;
		LOG_TRACE_NETWORK("[H2MOD-CustomMessage] received on handle_channel_message_hook request-map-filename from XUID: {}",
			received_data->player_id);
		if (peer_network_channel->is_channel_state_5()
			&& peer_network_channel->get_network_address(&addr))
		{
			LOG_TRACE_NETWORK("  - network address: {:x}", ntohl(addr.address.ipv4));

			int32 peer_index = NetworkSession::GetPeerIndexFromNetworkAddress(&addr);
			c_network_session* session = NULL;
			network_life_cycle_in_squad_session(&session);
			if (peer_index != NONE && !session->is_peer_local(peer_index))
			{
				s_custom_map_filename data;
				ZeroMemory(&data, sizeof(s_custom_map_filename));

				std::wstring map_filename;
				mapManager->GetMapFilename(map_filename);
				if (!map_filename.empty())
				{
					wcsncpy_s(data.file_name, map_filename.c_str(), map_filename.length());
					data.map_download_id = received_data->map_download_id;

					LOG_TRACE_NETWORK(L"[H2MOD-CustomMessage] sending map file name packet to player id: {}, peer index: {}, map name: {}, download id {}",
						received_data->player_id,
						peer_index, map_filename.c_str(), received_data->map_download_id);

					c_network_observer* observer = session->m_network_observer;
					s_session_peer* peer = session->get_session_peer(peer_index);

					if (peer->is_remote_peer)
						observer->send_message(session->m_session_index, peer->observer_channel_index, false, _custom_map_filename, sizeof(s_custom_map_filename), &data);
				}
				else
				{
					LOG_TRACE_NETWORK(L"[H2MOD-CustomMessage] no map file name found, abort sending packet! player id: {}, peer idx: {} map filename: {}",
						received_data->player_id, peer_index, map_filename.c_str());
				}
			}
		}
		break;
	}

	case _custom_map_filename:
	{
		if (peer_network_channel->is_channel_state_5())
		{
			s_custom_map_filename* received_data = (s_custom_map_filename*)packet;
			if (received_data->map_download_id != NONE)
			{
				auto map_download_query = mapManager->GetDownloadQueryById(received_data->map_download_id);
				if (map_download_query != nullptr)
				{
					map_download_query->SetMapNameToDownload(received_data->file_name);
					LOG_TRACE_NETWORK(L"[H2MOD-CustomMessage] received on handle_channel_message_hook custom_map_filename: {}",
						received_data->file_name);
				}
				else
				{
					// unlikely
					LOG_TRACE_NETWORK("[H2MOD-CustomMessage] - query with id {:X} hasn't been found!",
						received_data->map_download_id);
					break;
				}
			}

			LOG_TRACE_NETWORK(L"[H2MOD-CustomMessage] - received map name: {}, no download ID",
				received_data->file_name);
		}
		break;
	}

	case _rank_change:
	{
		if (peer_network_channel->is_channel_state_5())
		{
			s_rank_change* recieved_data = (s_rank_change*)packet;
			LOG_TRACE_NETWORK(L"H2MOD-CustomMessage] recieved on handle_channel_message_hook rank_change: {}",
				recieved_data->rank);
			network_session_interface_set_local_user_rank(0, recieved_data->rank);
		}
		break;
	}

	case _anti_cheat:
	{
		if (peer_network_channel->is_channel_state_5())
		{
			s_anti_cheat* recieved_data = (s_anti_cheat*)packet;
			twizzler_set_status(recieved_data->enabled);
		}
		break;
	}

	case _custom_variant_settings:
	{
		if (peer_network_channel->is_channel_state_5())
		{
			auto recieved_data = (CustomVariantSettings::s_variant_settings*)packet;
			CustomVariantSettings::UpdateCustomVariantSettings(recieved_data);
		}
		break;
	}

	// default packet
	case _leave_session:
	{
		if (peer_network_channel->is_channel_state_5()
			&& peer_network_channel->get_network_address(&addr))
		{
			auto peer_index = NetworkSession::GetPeerIndexFromNetworkAddress(&addr);
			EventHandler::NetworkPlayerEventExecute(EventExecutionType::execute_before, peer_index, EventHandler::NetworkPlayerEventType::remove);
		}
		break; // don't return, leave the game to update state
	}

	default:
		break;
	} // switch (message_type)

	if (peer_network_channel->get_network_address(&addr))
	{
		LOG_TRACE_NETWORK("{} - Received message: {} from peer index: {}, address: {:x}",
			__FUNCTION__, GetNetworkMessageName(message_type), NetworkSession::GetPeerIndexFromNetworkAddress(&addr), ntohl(addr.address.ipv4));
	}
	else
	{
		LOG_ERROR_NETWORK("{} - Received message: {} from network channel: {} possibly invalid payload or type",
			__FUNCTION__, GetNetworkMessageName(message_type), network_channel_index);
	}

	if (!MessageIsCustom(message_type))
		p_handle_channel_message(thisx, network_channel_index, message_type, dynamic_data_size, packet);

	switch (message_type)
	{
	case _membership_update:
		network_session_membership_update_local_players_teams();
		break;
	case _player_add:
		if (peer_network_channel->is_channel_state_5()
			&& peer_network_channel->get_network_address(&addr))
		{
			int32 peer_index = NetworkSession::GetPeerIndexFromNetworkAddress(&addr);
			EventHandler::NetworkPlayerEventExecute(EventExecutionType::execute_after, peer_index, EventHandler::NetworkPlayerEventType::add);
			NetworkMessage::SendAntiCheat(peer_index);
		}
		break;
	default:
		break;
	}
}

void NetworkMessage::SendRequestMapFilename(int mapDownloadId)
{
	c_network_session* session = NULL;
	network_life_cycle_in_squad_session(&session);

	if (session->established()
		&& !session->is_host())
	{
		s_request_map_filename data;
		XUserGetXUID(0, &data.player_id);
		data.map_download_id = mapDownloadId;

		c_network_observer* observer = session->m_network_observer;
		s_session_peer* peer = session->get_session_peer(session->m_session_host_peer_index);

		if (peer->is_remote_peer) {
			observer->send_message(session->m_session_index, peer->observer_channel_index, false, _request_map_filename, sizeof(s_request_map_filename), &data);

			LOG_TRACE_NETWORK("{} session host peer index: {}, observer index {}, observer is remote peer: {}, session index: {}",
				__FUNCTION__,
				session->m_session_host_peer_index,
				peer->observer_channel_index,
				peer->is_remote_peer,
				session->m_session_index);
		}
	}
}

void NetworkMessage::SendRankChange(int peer_index, BYTE rank)
{
	c_network_session* session = NULL;
	network_life_cycle_in_squad_session(&session);
	if (session->is_host())
	{
		s_rank_change data;
		data.rank = rank;

		c_network_observer* observer = session->m_network_observer;
		s_session_peer* peer = session->get_session_peer(peer_index);

		if (peer_index != NONE && !session->is_peer_local(peer_index))
		{
			if (peer->is_remote_peer) {
				observer->send_message(session->m_session_index, peer->observer_channel_index, false, _rank_change, sizeof(s_rank_change), &data);
			}
		}
	}
}
void NetworkMessage::SendAntiCheat(int peer_index)
{
	c_network_session* session = NULL;
	network_life_cycle_in_squad_session(&session);

	if (session->is_host())
	{
		c_network_observer* observer = session->m_network_observer;
		s_session_peer* peer = session->get_session_peer(peer_index);

		s_anti_cheat data;
		data.enabled = g_twizzler_status;
		if (peer_index != NONE && !session->is_peer_local(peer_index)) {
			if (peer->is_remote_peer) {
				observer->send_message(session->m_session_index, peer->observer_channel_index, false, _anti_cheat, sizeof(s_anti_cheat), &data);
			}
		}
	}
}

void NetworkMessage::ApplyGamePatches()
{
	WritePointer(Memory::GetAddress(0x1AC733, 0x1AC901), g_network_message_type_collection);
	WritePointer(Memory::GetAddress(0x1AC8F8, 0x1ACAC6), g_network_message_type_collection);
	WriteValue<BYTE>(Memory::GetAddress(0x1E825E, 0x1CA221), e_network_message_type_collection::k_network_message_type_collection_count);
	WriteValue<int>(Memory::GetAddress(0x1E81C6, 0x1CA189), e_network_message_type_collection::k_network_message_type_collection_count * 32);

	PatchCall(Memory::GetAddress(0x1B5196, 0x1A8EF4), register_custom_network_message);

	p_handle_channel_message = (handle_channel_message_t)DetourClassFunc(Memory::GetAddress<BYTE*>(0x1E929C, 0x1CB25C), (BYTE*)handle_channel_message_hook, 8);
	p_handle_out_of_band_message = (handle_out_of_band_message_t)DetourClassFunc(Memory::GetAddress<BYTE*>(0x1E907B, 0x1CB03B), (BYTE*)handle_out_of_band_message_hook, 8);
}