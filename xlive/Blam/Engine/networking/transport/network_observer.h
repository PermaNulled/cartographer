#pragma once

// Network Observer
//	- manages network bandwidth based on network conditions

#include "transport.h"
#include "transport_qos.h"

#include "networking/network_statistics.h"

// enables/disables LIVE netcode, so we can use the LIVE serverlist
// true  - LIVE network protocol enabled
// false - System Link network protocol enabled

#if !defined(_CARTOGRAPHER_DLL_CONF)
#define USE_LIVE_NETWORK_PROTOCOL true

#if defined(USE_LIVE_NETWORK_PROTOCOL)
#	if USE_LIVE_NETWORK_PROTOCOL == true
#		define INCREASE_NETWORK_TICKRATE_OBSOLETE 0 // old method of incresing the packet rate, now OBSOLETE
#		define LIVE_NETWORK_PROTOCOL_FORCE_CONSTANT_NETWORK_PARAMETERS 1
#	endif
#endif
#endif

#define k_network_channel_count (16)

// network heap size
#define k_network_preferences_size 108

// default: 1048576
#define k_network_heap_size 10485760

// defaults
#define k_online_netcode_client_rate_real 60.0f
#define k_online_netcode_server_rate_real 60.0f

#define k_online_netcode_client_max_packet_size_bytes 1264
#define k_online_netcode_server_max_packet_size_bytes 1264

// size in bits
#define k_online_netcode_client_max_bandwidth_per_channel ((int32)k_online_netcode_client_rate_real * k_online_netcode_client_max_packet_size_bytes * 8)
#define k_online_netcode_server_max_bandwidth_per_channel ((int32)k_online_netcode_server_rate_real * k_online_netcode_server_max_packet_size_bytes * 8)

struct s_network_observer_configuration;

enum e_observer_channel_state : int32
{
	_observer_channel_state_none = 0,
	_observer_channel_state_pending_transport_layer = 3, // waiting for xnet connection to be established
	_observer_channel_state_pending_game_layer = 4, // xnet layer connected, waiting for game to establish
	_observer_channel_state_connected = 7
};

struct alignas(8) s_observer_channel
{
	int32 state;
	int32 field_4;
	uint8 observer_flags;
	uint8 owner_flags;
	uint16 field_A;
	int32 channel_index;
	int32 field_10;
	XNADDR xnaddr;
	int32 field_38;
	int32 session_index;
	int32 field_40;
	int32 field_44;
	int32 field_48;
	int32 field_4C;
	int32 field_50;
	int32 field_54;
	int32 field_58;
	network_address address;
	datum qos_attempt_index;
	s_qos_attempt_data qos_attempt;
	int32 field_94;
	int32 field_98;
	int32 field_9C;
	uint8 gap_A0[8];
	c_network_time_statistics field_A8;
	uint8 gap_17C[4];
	c_network_window_statistics field_180;
	c_network_time_statistics field_290;
	uint8 gap_364[4];
	c_network_time_statistics field_368;
	uint8 gap_43C[4];
	c_network_window_statistics field_440;
	c_network_window_statistics field_550;
	int32 field_660;
	int32 field_664;
	int32 field_668;
	int32 field_66C;
	int32 field_670;
	int32 field_674;
	int32 field_678;
	int32 field_67C;
	int32 field_680;
	int32 field_684;
	int32 field_688;
	uint8 gap_68C[4];
	int64 field_690;
	int32 unmanaged_stream_bandwidth;
	real32 net_rate_unmanaged_stream;
	int32 unmanaged_stream_window_size;
	bool managed_stream;
	uint8 field_6A5;
	bool simulation_attached;
	bool simulation_authority;
	bool simulation_not_authority;
	uint8 gap_6A9[3];
	int32 net_managed_stream_bandwidth;
	int32 net_managed_stream_window_size;
	real32 net_rate_managed_stream;
	bool field_6B8;
	int8 field_6B9;
	int8 field_6BA;
	int32 field_6BC;
	int32 field_6C0;
	int32 field_6C4;
	int32 net_rtt;
	int32 field_6CC;
	int32 field_6D0;
	int32 field_6D4;
	bool field_6D8;
	int32 field_6DC;
	int32 field_6E0;
	int32 field_6E4;
	int32 field_6E8;
	real32 field_6EC;
	int32 field_6F0;
	int32 field_6F4;
	int32 field_6F8;
	int32 field_6FC;
	int32 field_700;
	int32 field_704;
	int32 field_708;
	bool field_70C;
	int32 field_710;
	int32 field_714;
	real32 field_718;
	int32 field_71C;
	int32 field_720;
	uint8 field_724;
	int8 field_725;
	int8 field_726;
	int8 field_727;
	int32 field_728;
	int32 throughput_bps;
	int32 field_730;
	int32 field_734;
	LONGLONG field_738;
};
ASSERT_STRUCT_SIZE(s_observer_channel, 0x740);

struct alignas(8) c_network_observer
{
public:
	void* vtbl;
	void* m_network_link;
	void* m_network_message_gateway;
	void* m_message_types;
	s_network_observer_configuration* configuration;
	int32 *field_14;
	uint8 gap_18[8];
	XNKID session_id;
	uint8 gap28[32];
	uint8 field_48;
	XNKEY xnkey;
	int32 field_5C;
	uint8 gap_60[4];
	int32 field_64;
	uint8 gap_68[12];
	int32 field_74;
	uint8 gap_78[8];
	s_observer_channel m_observer_channels[k_network_channel_count];
	bool network_observer_enabled;
	int8 field_7481;
	int32 field_7484;
	int32 throughput;
	int32 congestion;
	int32 field_7490;
	bool field_7494;
	uint8 gap_7495[3];
	int32 field_7498;
	int32 field_749C;
	int32 field_74A0;
	int32 field_74A4;
	int32 field_74A8;
	uint8 gap_74AC[4];
	real32 field_74B0;
	uint8 gap_74B4[212];
	int32 field_7588;
	int32 field_758C;
	int32 field_7590;
	int32 field_7594;
	int8 field_7598;
	int32 field_759C;
	int32 bits_per_sec;
	real32 field_75A4;
	real32 packets_per_sec;
	int32 field_75AC;
	int32 field_75B0;
	int32 field_75B4;
	int32 field_75B8;
	int8 field_75BC;
	int8 field_75BD;
	int8 field_75BE;
	int32 field_75C0;
	int32 field_75C4;

	static void apply_patches();
	static void reset_network_observer_bandwidth_preferences();

	bool __thiscall channel_should_send_packet_hook(
		int32 network_channel_index,
		bool a4,
		bool a5,
		int32 a6,
		int32* out_send_sequenced_packet,
		int32* out_force_fill_packet,
		int32* out_packet_size,
		int32* out_voice_size,
		int32 out_voice_chat_data_buffer_size,
		uint8* out_voice_chat_data_buffer);

	bool __thiscall get_bandwidth_results(int32 *out_throughput, real32 *out_satiation, int32 *a4);
	int32 get_observer_channel_state(int32 observer_index) { return m_observer_channels[observer_index].state; };
	void send_message(int32 session_index, int32 observer_index, bool send_out_of_band, int32 type, int32 size, void* data);

private:
	static void force_constant_network_rate();
};
ASSERT_STRUCT_SIZE(c_network_observer, 0x75C8);

struct s_network_observer_configuration
{
	int32 field_0;
	int32 field_4;
	uint8 gap_8[28];
	int32 field_24;
	int32 field_28;
	int32 field_2C;
	int32 field_30;
	uint8 gap_34[20];
	int32 field_48;
	int32 field_4C;
	int32 field_50;
	int32 field_54;
	int32 field_58;
	uint8 gap_5C[16];
	int32 field_6C;
	int32 field_70;
	int32 field_74;
	int32 field_78;
	int32 field_7C;
	int32 field_80;
	int32 field_84;
	int32 field_88;
	int32 field_8C;
	real32 field_90;
	real32 field_94;
	real32 field_98;
	int32 unk_total_flt_array_elements;
	real32 unk_floats_A0[7];
	uint8 gap_BC[36];
	int32 field_E0;
	real32 field_E4;
	real32 field_E8;
	real32 field_EC;
	real32 field_F0;
	int32 field_F4;
	int32 field_F8;
	int32 field_FC;
	int32 field_100;
	real32 field_104;
	int32 field_108;
	int32 field_10C;
	real32 field_110;
	int32 field_114;
	int32 field_118;
	real32 field_11C;
	int32 field_120;
	int32 field_124;
	int32 field_128;
	int32 field_12C;
	int32 field_130;
	int32 field_134;
	int32 field_138;
	int32 field_13C;
	int32 field_140;
	uint8 field_144;
	int32 field_148;
	int32 field_14C;
	int32 field_150;
	int32 field_154;
	int32 max_bits_per_second_single_player;
	int32 max_bits_per_second_full_lobby;
	int32 max_bits_per_second_splitscreen_players;
	int32 field_164;
	int32 field_168;
	int32 field_16C;
	int32 field_170;
	int32 field_174;
	real32 field_178;
	int32 field_17C;
	real32 field_180;
	int32 field_184;
	int32 field_188;
	int32 field_18C;
	real32 field_190;
	int32 field_194;
	real32 field_198;
	int32 field_19C;
	int32 field_1A0;
	int32 field_1A4;
	int32 field_1A8;
	real32 field_1AC;
	int32 field_1B0;
	int32 field_1B4;
	int32 field_1B8;
	int32 field_1BC;
	int32 field_1C0;
	real32 field_1C4;
	real32 field_1C8;
	int32 field_1CC;
	real32 field_1D0;
	int32 field_1D4;
	int32 field_1D8;
	int32 field_1DC;
	int32 field_1E0;
	int32 field_1E4;
	int32 field_1E8;
	int32 field_1EC;
	int32 field_1F0;
	int32 field_1F4;
	int32 field_1F8;
	real32 field_1FC;
	int32 field_200;
};
ASSERT_STRUCT_SIZE(s_network_observer_configuration, 0x204);
