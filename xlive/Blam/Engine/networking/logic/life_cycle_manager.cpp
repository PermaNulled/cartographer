#include "stdafx.h"
#include "life_cycle_manager.h"

#define g_game_life_cycle_handler c_game_life_cycle_handler::get()

bool game_life_cycle_initialized()
{
	return *Memory::GetAddress<bool*>(0x420FC0, 0x3C40A8);
}

e_game_life_cycle __cdecl get_game_life_cycle()
{
	c_game_life_cycle_manager* life_cycle_manager = c_game_life_cycle_manager::get();

	if (game_life_cycle_initialized())
		return life_cycle_manager->m_state;

	return _life_cycle_none;
}

bool network_life_cycle_in_squad_session(c_network_session** out_active_session)
{
	c_game_life_cycle_manager* life_cycle_manager = c_game_life_cycle_manager::get();

	if (!game_life_cycle_initialized()
		|| life_cycle_manager->m_active_squad_session->disconnected())
		return false;

	if (out_active_session != NULL)
		*out_active_session = life_cycle_manager->m_active_squad_session;

	return true;
}

void c_game_life_cycle_handler::initialize(c_game_life_cycle_manager* life_cycle_manager, e_game_life_cycle life_cycle, bool a3)
{
	this->life_cycle_manager = life_cycle_manager;
	this->life_cycle = life_cycle;
	this->field_C = a3;
	this->life_cycle_manager->m_life_cycle_handlers[this->life_cycle] = this;
}

void __cdecl c_game_life_cycle_handler_joining::check_joining_capability()
{
	INVOKE(0x1AD643, 0x1A65C0, check_joining_capability);
	return;
}

bool c_game_life_cycle_manager::get_active_session(c_network_session** out_session)
{
	bool result = false;
	*out_session = NULL;

	c_game_life_cycle_manager* life_cycle_manager = get();
	if (game_life_cycle_initialized()
		&& IN_RANGE(life_cycle_manager->m_state, _life_cycle_pre_game, _life_cycle_joining)
		&& !m_active_squad_session->disconnected()
		)
	{
		*out_session = m_active_squad_session;
		result = true;
	}

	return result;
}

c_game_life_cycle_manager* c_game_life_cycle_manager::get()
{
	return Memory::GetAddress<c_game_life_cycle_manager*>(0x420FC4, 0x3C40AC);
}

e_game_life_cycle c_game_life_cycle_manager::get_life_cycle() const
{
	e_game_life_cycle result = _life_cycle_none;
	if (game_life_cycle_initialized())
	{
		result = m_state;
	}
	return result;
}

bool c_game_life_cycle_manager::state_is_joining() const
{
	if (!game_life_cycle_initialized())
		return false;
	if (m_state == _life_cycle_joining)
		return true;

	return false;
}

bool c_game_life_cycle_manager::state_is_in_game(void) const
{
	if (!game_life_cycle_initialized())
		return false;
	if (m_state == _life_cycle_in_game)
		return true;

	return false;
}

void c_game_life_cycle_manager::request_state_change(e_game_life_cycle requested_state, int entry_data_size, void* entry_data)
{
	this->m_requested_life_cycle = requested_state;
	this->m_update_requested = true;
	this->m_entry_data_size = entry_data_size;
	csmemset(m_entry_data, 0, sizeof(m_entry_data));
	if (m_entry_data > 0)
	{
		csmemcpy(&m_entry_data, entry_data, this->m_entry_data_size);
	}
}
