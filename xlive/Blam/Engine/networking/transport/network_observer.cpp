#include "stdafx.h"

#include "network_observer.h"
#include "network_channel.h"

s_network_observer_configuration* g_network_configuration;

#if defined(LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS) 
#	if LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS == true

// variables, TODO configurable
real32 _online_netcode_client_rate_real = k_online_netcode_client_rate_real;
real32 _online_netcode_server_rate_real = k_online_netcode_server_rate_real;

int32 _online_netcode_client_max_bandwidth_per_channel = k_online_netcode_client_max_bandwidth_per_channel;
int32 _online_netcode_server_max_bandwidth_per_channel = k_online_netcode_server_max_bandwidth_per_channel;

// ### TODO FIXME:
int32	_online_netcode_window_size = -1;

#	endif // LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS == true
#endif // defined(LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS)

// LIVE netcode research
void __cdecl initialize_network_observer_configuration()
{
	/*
		Configuration constants used by the game to define how LIVE netcode will behave
	*/

	g_network_configuration->field_0 = 1;
	g_network_configuration->field_2C = 500;
	g_network_configuration->field_30 = 500;
	g_network_configuration->field_4C = 500;
	g_network_configuration->field_50 = 500;
	g_network_configuration->field_78 = 500;
	g_network_configuration->field_94 = 1.0f;
	g_network_configuration->field_98 = 30.0f;
	g_network_configuration->field_80 = 4000;
	g_network_configuration->field_84 = 4000;
	g_network_configuration->field_100 = 4000;
	g_network_configuration->unk_floats_A0[0] = 1.0f;
	g_network_configuration->unk_floats_A0[1] = 0.66666669f;
	g_network_configuration->unk_floats_A0[2] = 0.5f;
	g_network_configuration->unk_floats_A0[3] = 0.33333334f;
	g_network_configuration->unk_floats_A0[4] = 0.25f;
	g_network_configuration->unk_floats_A0[5] = 0.2f;
	g_network_configuration->unk_floats_A0[6] = 0.1f;
	g_network_configuration->unk_total_flt_array_elements = 7;
	g_network_configuration->field_4 = 0;
	g_network_configuration->field_24 = 3;
	g_network_configuration->field_28 = 0;
	g_network_configuration->field_48 = 4;
	g_network_configuration->field_54 = 1000;
	g_network_configuration->field_58 = 2000;
	g_network_configuration->field_6C = 15000;
	g_network_configuration->field_70 = 1000;
	g_network_configuration->field_74 = 2000;
	g_network_configuration->field_7C = 2000;
	g_network_configuration->field_88 = 1000;
	g_network_configuration->field_8C = 5000;
	g_network_configuration->field_90 = 0.5f;
	g_network_configuration->field_E0 = 200;
	g_network_configuration->field_E4 = 0.25f;
	g_network_configuration->field_E8 = 0.5f;
	g_network_configuration->field_EC = 0.75f;
	g_network_configuration->field_F0 = 12.0f;
	g_network_configuration->field_F4 = 2000;
	g_network_configuration->field_F8 = 2000;
	g_network_configuration->field_FC = 30;
	g_network_configuration->field_104 = 0.5;
	g_network_configuration->field_108 = 64;
	g_network_configuration->field_10C = 96;
	g_network_configuration->field_110 = 0.33333334f;
	g_network_configuration->field_114 = 10240 * 4; // H2v - 10240, 60 tick = H2v * 4
	g_network_configuration->field_118 = 1000;
	g_network_configuration->field_11C = 0.5f;
	g_network_configuration->field_120 = 3;
	g_network_configuration->field_124 = 40;
	g_network_configuration->field_128 = 5000;
	g_network_configuration->field_12C = 15;
	g_network_configuration->field_130 = 100;
	g_network_configuration->field_134 = 50 * 2; // h2v - 50, 60 tick - 100
	g_network_configuration->field_138 = 50 * 4; // h2v - 50, 60 tick - 200
	g_network_configuration->field_13C = 32;     // h2v - 15, 60 tick - 32
	g_network_configuration->field_140 = 32;     // h2v - 8, 60 tick - 32
	g_network_configuration->field_144 = 1;
	g_network_configuration->field_148 = 2000;
	g_network_configuration->field_14C = 4096 * 4; // h2v - 4096, 60 tick = 4096 * 4
	g_network_configuration->field_150 = 71680 * 4; // H2v - 71680, 60 tick = H2v * 4
	g_network_configuration->field_154 = 1000;
	g_network_configuration->max_bits_per_second_single_player = 30720 * 4; // H2v = 30720, 60 tick = H2v * 4
	g_network_configuration->max_bits_per_second_full_lobby = 262144; // H2v - 122880, 60 tick = 262144
	g_network_configuration->max_bits_per_second_splitscreen_players = 512000;
	g_network_configuration->field_1C4 = 0.5f;
	g_network_configuration->field_1A0 = 1500;
	g_network_configuration->field_1A4 = 1500;
	g_network_configuration->field_178 = 0.1f;
	g_network_configuration->field_190 = 0.2f;
	g_network_configuration->field_1C8 = 20.0f;
	g_network_configuration->field_168 = 40;
	g_network_configuration->field_1D8 = 40;
	g_network_configuration->field_164 = 65536; // H2v 8192, 60 tick = 65536
	g_network_configuration->field_16C = 320;
	g_network_configuration->field_170 = 3;
	g_network_configuration->field_174 = 32;
	g_network_configuration->field_17C = 4;
	g_network_configuration->field_180 = 0.80000001f;
	g_network_configuration->field_184 = 10;
	g_network_configuration->field_188 = 21;
	g_network_configuration->field_18C = 3072 * 4; // H2v - 3072, 60 tick = H2V * 4
	g_network_configuration->field_194 = 5120 * 4; // H2v - 5120, 60 tick = H2V * 4
	g_network_configuration->field_198 = 0.30000001f;
	g_network_configuration->field_19C = 5000;
	g_network_configuration->field_1B4 = 3;
	g_network_configuration->field_1B8 = 6144 * 4; // H2v - 6144, 60 tick = H2v * 4
	g_network_configuration->field_1BC = 20000;
	g_network_configuration->field_1A8 = 30720 * 4; // H2v - 30720, 60 tick = H2v * 4;
	g_network_configuration->field_1AC = 0.75f;
	g_network_configuration->field_1B0 = 60000;
	g_network_configuration->field_1C0 = 6;
	g_network_configuration->field_1CC = 0;
	g_network_configuration->field_1D0 = 0.00039062501f;
	g_network_configuration->field_1D4 = 160;
	g_network_configuration->field_1DC = 120;
	g_network_configuration->field_1E0 = 3;
	g_network_configuration->field_1E4 = 10;
	g_network_configuration->field_1E8 = 10;
	g_network_configuration->field_1EC = 3;
	g_network_configuration->field_1F0 = 8000;
	g_network_configuration->field_1F4 = 6;
	g_network_configuration->field_1F8 = 8192 * 4; // H2v - 8192, 60 tick  = H2v * 4 = 32768
	g_network_configuration->field_1FC = 0.33333334f;
	g_network_configuration->field_200 = 4096 * 4; // H2v - 4096, 60 tick  = H2v * 4 = 16384
}

void c_network_observer::send_message(int32 session_index, int32 observer_index, bool send_out_of_band, int32 type, int32 size, void* data)
{
	typedef void(__thiscall* observer_channel_send_message_t)(c_network_observer*, int32, int32, bool, int32, int32, void*);
	INVOKE_TYPE(0x1BED40, 0x1B8C1A, observer_channel_send_message_t, this, session_index, observer_index, send_out_of_band, type, size, data);
}

bool __cdecl is_network_observer_mode_managed()
{
	// or in other terms this checks if the network protocol is LIVE
	// it is used for migrating the host before starting the game (which causes the short delay when the game starts in a p2p session)
	// in LAN mode it's disabled
	return false;
}

void c_network_observer::reset_network_observer_bandwidth_preferences()
{
	// reset the network bandwidth preferences
	SecureZeroMemory(Memory::GetAddress<void*>(0x47E9D8 + 0x1DC), k_network_preferences_size);
}

bool __thiscall c_network_observer::get_bandwidth_results(int32 *out_throughput, real32 *out_satiation, int32 *a4)
{
	// let the game know we don't have any bandwidth measurements available to save
	return false;
}

void __declspec(naked) jmp_get_bandwidth_results() { __asm jmp c_network_observer::get_bandwidth_results }

// raw WinSock has a 28 bytes packet overhead for the packet header, unlike Xbox LIVE, which has 44 bytes (28 bytes + whatever LIVE packet header adds)
int32 __cdecl transport_get_packet_overhead_hook(int32 protocol_type)
{
	enum e_protocol_type : int32
	{
		_protocol_udp_loopback = 2,
		_protocol_udp,
		_protocol_tcp, // not entirely sure if this is TCP
		k_protocol_count
	};

	switch ((e_protocol_type)protocol_type)
	{

	// replace XNet UDP header overhead with WinSock overhead
	case _protocol_udp_loopback:
		// return 44;
		return 28;

	case _protocol_udp:
		//return 48;
		return 28 + 4;
		
	case _protocol_tcp:
		return 56;
	
	default:
		break;
	}

	return 0;
}

bool __thiscall c_network_observer::channel_should_send_packet_hook(
	int32 network_channel_index,
	bool a3,
	bool a4,
	int32 a5,
	int32* out_send_sequenced_packet,
	int32* out_force_fill_packet,
	int32* out_packet_size,
	int32* out_voice_size,
	int32 out_voice_chat_data_buffer_size,
	uint8* out_voice_chat_data_buffer)
{
	typedef bool(__thiscall* channel_should_send_packet_t)(c_network_observer*, int32, bool, bool, int32, int32*, int32*, int32*, int32*, int32, uint8*);
	auto p_channel_should_send_packet = Memory::GetAddressRelative<channel_should_send_packet_t>(0x5BEE8D, 0x5B8D67);

	int32 observer_index = NONE;
	for (int32 i = 0; i < 16; i++)
	{
		if (this->m_observer_channels[i].state != _observer_channel_state_none
			&& this->m_observer_channels[i].channel_index == network_channel_index)
		{
			observer_index = i;
			break;
		}
	}

	if (observer_index == NONE)
		return false;

	s_network_channel* network_channel = s_network_channel::get(network_channel_index);
	s_observer_channel* observer_channel = &this->m_observer_channels[observer_index];

	// we modify the network channel paramters to force the network tickrate
	const real32 _temp_network_rate					= observer_channel->net_rate_managed_stream;
	const int32	 _temp_network_bandwidth_per_stream	= observer_channel->net_managed_stream_bandwidth;
	const int32  _temp_network_window_size			= observer_channel->net_managed_stream_window_size;

#if defined(LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS) 
#	if LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS == true

	// first we check if we are dealing with a managed network stream
	if (observer_channel->managed_stream)
	{
		// check if we're host
		if (network_channel->is_simulation_authority())
		{
			observer_channel->net_rate_managed_stream = _online_netcode_server_rate_real;
			observer_channel->net_managed_stream_bandwidth = _online_netcode_server_max_bandwidth_per_channel;
		}
		else
		{
			observer_channel->net_rate_managed_stream = _online_netcode_client_rate_real;
			observer_channel->net_managed_stream_bandwidth = _online_netcode_client_max_bandwidth_per_channel;
		}

		observer_channel->net_managed_stream_window_size = _online_netcode_window_size;
	}

#	endif // LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS == true
#endif // defined(LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS)

	bool ret = p_channel_should_send_packet(this, network_channel_index, a3, a4, a5, out_send_sequenced_packet, out_force_fill_packet, out_packet_size, out_voice_size, out_voice_chat_data_buffer_size, out_voice_chat_data_buffer);

	// then we reset the values back to normal
	observer_channel->net_rate_managed_stream		= _temp_network_rate;
	observer_channel->net_managed_stream_bandwidth		= _temp_network_bandwidth_per_stream;
	observer_channel->net_managed_stream_window_size	= _temp_network_window_size;

	return ret;
}

static void __declspec(naked) jmp_network_observer_channel_should_send_packet_hook() { __asm jmp c_network_observer::channel_should_send_packet_hook }

void c_network_observer::force_constant_network_rate()
{
	// patches to force static network rate and bandwidth
	// because the bandwidth management is totaly fucked/busted in Halo 2 Vista
	// but there are some benefits in using Online's netcode
	// like limited network packet rate, otherwise people with uncapped FPS will overflow host's packet buffer

	// replace vtable pointer of network_observer::channel_should_send_packet
	WritePointer(Memory::GetAddressRelative(0x7C615C, 0x781C48), jmp_network_observer_channel_should_send_packet_hook);

	// don't force packet filling when game simulation is attached
	// otherwise we send packets filled with nothing...
	// no idea if this was done on purpose or not...
	// but it makes no sense to send network packets filled with barely any game data (around 30 bytes on avg) and the rest of the packet size
	// filled with nothing
	// unless original XNet transport layer had packet compression but even then it's rather dumb
	// or maybe the UDP protocol has something like that, no idea
	NopFill(Memory::GetAddressRelative(0x5BF000, 0x5B8EDA), 14);
}

void c_network_observer::apply_patches()
{
#if USE_LIVE_NETWORK_PROTOCOL
#	if INCREASE_NETWORK_TICKRATE_OBSOLETE == true
	// increase the network tickrate of hosts to 60
	static real32 netcode_tickrate = k_online_netcode_tickrate_real;

	WritePointer(Memory::GetAddress(0x1BDE27, 0x1B7D01) + 4, &netcode_tickrate);
	WritePointer(Memory::GetAddress(0x1BE2FA, 0x1B81D4) + 4, &netcode_tickrate);
	WritePointer(Memory::GetAddress(0x1BFB3C, 0x1B9A1C) + 4, &netcode_tickrate);
	WritePointer(Memory::GetAddress(0x1C11FA, 0x1BB0DA) + 4, &netcode_tickrate);
	WritePointer(Memory::GetAddress(0x1C12BF, 0x1BB19F) + 4, &netcode_tickrate);

	g_network_configuration = Memory::GetAddress<s_network_observer_configuration*>(0x4F960C, 0x523B5C);
	PatchCall(Memory::GetAddress(0x1ABE23, 0x1AC328), initialize_network_observer_configuration);

	// other config patches
	WriteValue<int32>(Memory::GetAddress(0x1AB4A1, 0x1AB9A6) + 6, 20480 * 4); // 60 tick = H2v * 4
	WriteValue<int32>(Memory::GetAddress(0x1AB4AB, 0x1AB9B0) + 6, 51200 * 4); // 60 tick = H2v * 4
	WriteValue<int32>(Memory::GetAddress(0x1AB4C9, 0x1AB9CE) + 6, 65536 * 4); // 60 tick = H2v * 4
	WriteValue<int32>(Memory::GetAddress(0x1AB4D3, 0x1AB9D8) + 6, 32768 * 4); // 60 tick = H2v * 4

	WriteValue<real32>(Memory::GetAddress(0x3A03CC, 0x360E54), 8192.f * 2.f);  // 60 tick = H2v * 2, H2v = 8192
	WriteValue<real32>(Memory::GetAddress(0x3C60F0, 0x381BDC), 40960.f * 4.f); // 60 tick = H2v * 4, H2v = 40960
	WriteValue<real32>(Memory::GetAddress(0x3C60F4, 0x381BE0), 30720.f * 4.f); // 60 tick = H2v * 4, H2v = 30720
	WriteValue<real32>(Memory::GetAddress(0x3C60F8, 0x381BE4), 53248.f);		 // 60 tick = 53248, H2v = 9216

	WriteValue<int32>(Memory::GetAddress(0x1AB4FF, 0x1ABA04) + 1, 8192 * 2);  // h2v = 8192, 60 tick = h2v * 4
	WriteValue<int32>(Memory::GetAddress(0x1AB504, 0x1ABA09) + 1, 40960 * 4); // h2v = 40960, 60 tick = h2v * 4

	WriteValue<int32>(Memory::GetAddress(0x1AB558, 0x1ABA5D) + 1, 15360 * 4); // h2v = 15360, 60 tick = h2v * 4
	WriteValue<int32>(Memory::GetAddress(0x1AB55D, 0x1ABA62) + 2, 61440 * 4); // h2v = 61440, 60 tick = h2v * 4

	WriteValue<int32>(Memory::GetAddress(0x1AB582, 0x1ABA87) + 1, 131072 * 4); // 60 tick - 524288
	WriteValue<int32>(Memory::GetAddress(0x1AB587, 0x1ABA8C) + 1, 262144 * 4); // 60 tick - 1048576

	WriteValue<int32>(Memory::GetAddress(0x1AB5B6, 0x1ABABB) + 6, 10240 * 4); // 60 tick - 40960

	// prevent the game from setting the client's tickrate to half of host network tickrate
	NopFill(Memory::GetAddress(0x1BFBE7, 0x1B9AC7), 19);
	NopFill(Memory::GetAddress(0x1BE33A, 0x1B8214), 15);
	NopFill(Memory::GetAddress(0x1BDF1D, 0x1B7DF7), 18);
#	endif // if INCREASE_NETWORK_TICKRATE_OBSOLETE == true
#else
	// disables LIVE netcode
	WriteValue<uint8>(Memory::GetAddress(0x1B555B, 0x1A92B9) + 1, 0);
	// disable ping bars
	NopFill(Memory::GetAddress(0x1D4E33, 0x1C1B7D), 2);
	WriteValue<uint8>(Memory::GetAddress(0x1D4E35, 0x1C1B7F), 0xEB); // jmp
#endif

	// increase the network heap size
	WriteValue<int32>(Memory::GetAddress(0x1ACCC8, 0x1ACE96) + 6, k_network_heap_size);

	PatchCall(Memory::GetAddress(0x1E0FEE, 0x1B5EDE), jmp_get_bandwidth_results);

	WriteJmpTo(Memory::GetAddressRelative(0x5AC1BD, 0x5A6B76), transport_get_packet_overhead_hook);

#if defined(LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS) 
#if LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS == true
	force_constant_network_rate();
#endif // LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS == true
#endif // defined(LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS) 

	// disable forced host migration in P2P games
	// this is using the XNet QoS probes to select a preferred host with a possible better connection
	// which is not really available anymore with cartographer, since the QoS probes are not that accurate anymore
	if (!Memory::IsDedicatedServer())
	{
		PatchCall(Memory::GetAddress(0x1D97DD, 0x1BEE59), is_network_observer_mode_managed);
	}
}