#pragma once

#include "transport.h"
#include "network_observer.h"

enum e_network_channel_closure_reason
{
	_network_channel_reason_none = 0,
	_network_channel_reason_link_destroyed = 1,
	_network_channel_reason_link_refused_listen = 2,
	_network_channel_reason_channel_deleted = 3,
	_network_channel_reason_connect_timeout = 4,
	_network_channel_reason_connect_refused = 5,
	_network_channel_reason_connect_reinitiate = 6,
	_network_channel_reason_establish_timeout = 7,
	_network_channel_reason_address_change = 8,
	_network_channel_reason_destination_unreachable = 9,
	_network_channel_reason_remote_closure = 10,
	_network_channel_reason_connection_overflow = 11,
	_network_channel_reason_message_overflow = 12,
	_network_channel_reason_security_lost = 13,
	_network_channel_reason_observer_released = 14,
	_network_channel_reason_observer_refused = 15,
	_network_channel_reason_observer_timeout = 16,
	_network_channel_reason_observer_reset = 17,

	k_network_channel_reason_count
};

enum e_network_channel_state
{
	_network_channel_state_2 = 2,
	_network_channel_state_5 = 5,
};

struct alignas(8) s_network_channel
{
	void* m_network_link;
	void* m_network_message_gateway;
	void* network_message_handler;
	void* network_channel_config;
	int32 network_connection_index;
	int32 network_message_queue_index;
	void* simulation_gatekeeper;
	bool simulation_data_available;
	int8 field_1D;
	int8 gap_1E[2];
	int32 field_20;
	int32 field_24;
	int32 field_28;
	int32 field_2C;
	int32 field_30;
	int32 field_34;
	int32 field_38;
	void* simulation_interface;
	c_network_observer* network_observer;
	int32 channel_index;
	int32 channel_flags;
	int32 remote_identifier;
	int32 identifier;
	e_network_channel_state channel_state;
	e_network_channel_closure_reason closure_reason;
	network_address old_address;
	network_address address;
	int8 field_84;
	int8 gap_85[3];
	int32 network_connect_start_timepoint;
	int32 field_8C;
	int32 field_90;
	int32 network_establish_start_timepoint;
	int32 field_98;
	int8 gap_9C[4];
	int64 field_A0;
	int8 gap_A8[48];
	int32 field_D8;
	int8 gap_DC[4];
	int64 field_E0;
	int8 gap[10];

	static s_network_channel* get(int32 channel_index);
	bool get_network_address(network_address* address_out);

	bool has_simulation_interface() const { return simulation_interface != NULL; }
	bool is_simulation_authority() const { return has_simulation_interface() && *(bool*)((uint8*)simulation_interface + 48); }
	bool is_channel_state_5() const { return channel_state == _network_channel_state_5; }
};
ASSERT_STRUCT_SIZE(s_network_channel, 248);

