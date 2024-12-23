#pragma once

#include "networking/session/network_session.h"
#include "networking/transport/network_observer.h"

enum e_game_life_cycle : int32
{
	_life_cycle_none,
	_life_cycle_pre_game,
	_life_cycle_start_game,
	_life_cycle_in_game,
	_life_cycle_post_game,
	_life_cycle_joining,
	_life_cycle_matchmaking,
	k_life_cycle_count
};

e_game_life_cycle get_game_life_cycle(void);
typedef bool (*life_cycle_update)(void* life_cycle_handler);
typedef void (*life_cycle_initialize)(void* life_cycle_handler_requested, void* life_cycle_handler_current, int32 unk_1, void* unk_2);
typedef void (*life_cycle_dispose)(void* life_cycle_handler_current, void* life_cycle_handler_requested);

class c_game_life_cycle_manager;

class c_game_life_cycle_handler_functions
{
public:
	life_cycle_update update;
	life_cycle_initialize initialize;
	life_cycle_dispose dispose;
};

class c_game_life_cycle_handler
{
public:
	c_game_life_cycle_handler_functions* functions;
	e_game_life_cycle life_cycle;
	c_game_life_cycle_manager* life_cycle_manager;
	bool field_C;
	void initialize(c_game_life_cycle_manager* life_cycle_manager, e_game_life_cycle life_cycle, bool a3);
};

struct c_game_life_cycle_handler_none : c_game_life_cycle_handler
{
	int8 gap[15];
};
ASSERT_STRUCT_SIZE(c_game_life_cycle_handler_none, 0x20);

struct c_game_life_cycle_handler_pre_game : c_game_life_cycle_handler
{
	int32 count_down_network_time;
	int32 count_down_network_session;
	int32 field_18;
	int8 gap_1C[15];
};
ASSERT_STRUCT_SIZE(c_game_life_cycle_handler_pre_game, 0x2C);

struct c_game_life_cycle_handler_start_game : c_game_life_cycle_handler
{
	int8 field_10;
	int8 field_11;
	int8 gap_12[25];
};
ASSERT_STRUCT_SIZE(c_game_life_cycle_handler_start_game, 0x2C);

struct c_game_life_cycle_handler_in_game : c_game_life_cycle_handler
{
	int64 rand_bytes;
	int8 gap_18[15];
};
ASSERT_STRUCT_SIZE(c_game_life_cycle_handler_in_game, 0x28);

struct c_game_life_cycle_handler_post_game : c_game_life_cycle_handler
{
	int8 gap[15];
};
ASSERT_STRUCT_SIZE(c_game_life_cycle_handler_post_game, 0x20);

class c_game_life_cycle_handler_joining : c_game_life_cycle_handler
{
public:
	bool field_10;
	int8 field_11;
	bool field_12;
	int32 field_14;
	XNKID joining_xnkid;
	XNKEY joining_xnkey;
	XNADDR joining_xnaddr;
	int32 field_54;
	int32 joining_user_count;
	s_player_identifier player_identifiers[k_number_of_users];
	wchar_t player_names[k_number_of_users][XUSER_NAME_SIZE];
	int8 gapFC[128];
	int32 field_17C;
	bool field_180;
	int8 field_181;
	int8 gap_182[2];
	int32 field_184;
	int32 field_188;
	int32 field_18C;
	bool field_190;
	int8 field_191;
	bool field_192;
	int8 gap_193[5];
	int32 field_198;
	int8 gap_19C[3804];
	int32 field_1078;
	int8 gap_107C[4];
	int32 join_attempt_result_code;
	static void __cdecl check_joining_capability();
};
ASSERT_STRUCT_SIZE(c_game_life_cycle_handler_joining, 0x1084);

struct  c_game_life_cycle_handler_match_making : c_game_life_cycle_handler
{
	void* some_function_pointer;
	int32 field_14;
};
ASSERT_STRUCT_SIZE(c_game_life_cycle_handler_match_making, 0x18);

class c_game_life_cycle_manager
{
public:
	e_game_life_cycle m_state;
	c_game_life_cycle_handler* m_life_cycle_handlers[k_life_cycle_count];
	void* m_network_session_manager;
	c_network_session* m_active_squad_session;
	c_network_session* m_secondary_squad_session;
	void* m_network_message_gateway;
	c_network_observer* m_network_observer;
	bool m_state_change_active;
	bool m_update_active;
	bool m_update_requested;

	e_game_life_cycle m_requested_life_cycle;
	int32 m_entry_data_size;
	uint8 m_entry_data[4];

	c_game_life_cycle_manager(
		void* network_message_gateway, 
		c_network_observer* network_observer, 
		void* network_session_manager, 
		c_network_session* squad_session_one,
		c_network_session* squad_session_two)
	{
		csmemset(m_life_cycle_handlers, 0, sizeof(m_life_cycle_handlers));
		this->m_state = _life_cycle_none;
		this->m_network_message_gateway = network_message_gateway;
		this->m_network_observer = network_observer;
		this->m_network_session_manager = network_session_manager;
		this->m_active_squad_session = squad_session_one;
		this->m_secondary_squad_session = squad_session_two;
		this->m_update_active = false;
		this->m_state_change_active = false;
		this->m_update_requested = false;
		this->m_requested_life_cycle = _life_cycle_none;
	}

	static c_game_life_cycle_manager* get();
	

	e_game_life_cycle get_life_cycle() const;
	bool get_active_session(c_network_session** out_session);
	
	bool state_is_joining() const;
	bool state_is_in_game() const;
	void request_state_change(e_game_life_cycle requested_state, int a3, void* a4);
};
ASSERT_STRUCT_SIZE(c_game_life_cycle_manager, 68);

bool game_life_cycle_initialized();
bool network_life_cycle_in_squad_session(c_network_session** out_network_session);