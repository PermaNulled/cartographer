#include "stdafx.h"
#include "input_xinput.h"
#include "input_windows.h"
#include "input_abstraction.h"
#include "controllers.h"

#include "interface/user_interface_controller.h"
#include "shell/shell_windows.h"
#include "render/render.h"

extern input_device** g_xinput_devices;
extern s_input_abstraction_globals* input_abstraction_globals;

/* globals */

bool g_should_offset_gamepad_indices = false;
bool g_notified_to_change_mapping = false;
uint32 input_device_change_delay_timer = NULL;
s_input_globals* input_globals;
bool* g_input_windows_request_terminate;

/* constants */

XINPUT_VIBRATION g_vibration_state[k_number_of_controllers]{};
real32 g_rumble_factor = 1.0f;

/* prototypes */

int compare_device_compatibility(const void* p1, const void* p2);
int compare_device_ports(const void* p1, const void* p2);
void input_windows_update_device_mapping();
void input_windows_restore_device_mapping();

/* public code */

void __cdecl input_initialize()
{
	INVOKE(0x2FD23, 0x0, input_initialize);
}

void __cdecl input_dispose()
{
	INVOKE(0x2E309, 0x0, input_dispose);
}

void __cdecl input_update()
{
	INVOKE(0x2F9AC, 0x0, input_update);
}

void __cdecl input_update_gamepads(uint32 duration_ms)
{
	//INVOKE(0x2E7A4, 0x0, input_update_gamepads, duration_ms);

	/*
	* Reimplemented function removes dependency on input_globals.main_controller_index
	* Actually Updates input_globals.gamepad_states[index] buffer from device state
	*/

	bool input_handled = false;
	for (uint16 gamepad_index = _controller_index_0; gamepad_index < k_number_of_controllers; gamepad_index++)
	{
		if (input_has_gamepad(gamepad_index, nullptr))
		{
			s_gamepad_input_button_state* gamepad_state = input_get_gamepad_state(gamepad_index);

			if (input_xinput_update_gamepad(gamepad_index, duration_ms, gamepad_state))
			{
				//handled successfully for any device
				input_handled = true;
			}
		}
	}

	HWND g_window_handle = *shell_windows_get_hwnd();

	if (input_handled
		&& g_window_handle == GetFocus()
		&& g_window_handle == GetForegroundWindow()
		&& !game_is_minimized())
	{
		if ((input_globals->field7D8 & 1) == 0)
		{
			input_globals->field7D8 |= 1u;
			//v26 = 0;
			input_globals->field7D0 = system_milliseconds();
			//v26 = 0xFFFFFFFF;
		}
		uint32 time = system_milliseconds();
		if (time - input_globals->field7D0 > 15000 || time - input_globals->field7D0 < 0)
		{
			input_globals->field7D0 = time;
			INPUT input;
			//csmemset(&input, 0, sizeof(input));
			input.type = INPUT_KEYBOARD;
			input.ki.wVk = 0;
			input.ki.wScan = 0;
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			input.ki.time= 0;
			input.ki.dwExtraInfo = 0;
			SendInput(1, &input, sizeof(input));
		}
	}

	// we want device changes to happen smoothly, not abruptly
	if (g_notified_to_change_mapping && input_device_change_delay_timer > k_maximum_delay_for_split_inputs)
	{
		if (g_should_offset_gamepad_indices)
		{
			input_windows_update_device_mapping();
		}
		else
		{
			input_windows_restore_device_mapping();
		}
		g_notified_to_change_mapping = false;
		input_device_change_delay_timer = 0;
	}

	TEST_N_DEF(PC1);
}

void __cdecl input_update_mouse(DIMOUSESTATE2* mouse_state, uint32 duration_ms)
{
	INVOKE(0x2E60C, 0x0, input_update_mouse, mouse_state, duration_ms);
}

bool __cdecl input_has_gamepad(uint16 gamepad_index, bool* a2)
{
	return INVOKE(0x2F3CD, 0x0, input_has_gamepad, gamepad_index, a2);
}

bool __cdecl input_has_gamepad_plugged(uint16 gamepad_index)
{
	//return INVOKE_TYPE(0x2E186, 0x0, bool(__cdecl*)(uint16), gamepad_index);
	return input_globals->gamepad_states[gamepad_index].connected;
}

bool __cdecl input_gamepad_just_left(uint16 gamepad_index)
{
	if (VALID_INDEX(gamepad_index, k_number_of_controllers))
	{
		return input_globals->gamepad_states[gamepad_index].m_device_just_left;
	}
	return false;
}

uint8 __cdecl input_get_connected_gamepads_count()
{
	uint8 count = 0;
	for (e_controller_index controller = first_controller();
		controller != k_no_controller;
		controller = next_controller(controller))
	{
		if (input_has_gamepad_plugged(controller))
			count++;
	}
	return count;
}

s_gamepad_input_state* __cdecl input_get_gamepad(uint16 gamepad_index)
{
	//s_gamepad_input_state* global = Memory::GetAddress<s_gamepad_input_state*>(0x47A5C8);
	//return &global[gamepad_index];
	return &input_globals->gamepad_states[gamepad_index];
}

s_gamepad_input_button_state* __cdecl input_get_gamepad_state(uint16 gamepad_index)
{
	return INVOKE(0x2F433, 0x0, input_get_gamepad_state, gamepad_index);
}

DIMOUSESTATE2* __cdecl input_get_mouse_state()
{
	//return INVOKE(0x2E404, 0x0, input_get_mouse_state);
	if (!input_globals->mouse_dinput_device)
		return nullptr;
	if (!input_globals->input_suppressed)
		return &input_globals->mouse_state;

	return &input_globals->suppressed_mouse_state;
}

uint16* __cdecl input_get_mouse_button_state()
{
	if (!input_globals->input_suppressed)
		return input_globals->mouse_buttons;

	return nullptr;
}

bool __cdecl input_get_key(s_key_state* keystate)
{
	return INVOKE(0x2E3CB, 0x0, input_get_key, keystate);
}

void __cdecl input_update_main_device_state()
{
	//INVOKE(0x2E709, 0x0, input_update_main_device_state);

	/*
	* Reimplemented function removes dependency on input_globals.main_controller_index
	* Updates input_globals.gamepad_states[index].connected
	* Allows game to actually detect multiple controllers as multi inputs rather single input
	*/

	uint8 device_index = _controller_index_0;
	do
	{
		input_device* device = g_xinput_devices[device_index];
		XINPUT_STATE state;
		s_gamepad_input_state* gamepad = input_get_gamepad(device_index);
		uint32 error_code = ERROR_DEVICE_NOT_CONNECTED;


		if (!device
			|| (error_code = device->XGetState(&state)) == ERROR_SEVERITY_SUCCESS
			|| error_code == ERROR_DEVICE_NOT_CONNECTED)
		{

			bool dev_connected = gamepad->connected;
			bool success = error_code == ERROR_SEVERITY_SUCCESS;
			bool initially_not_connected = !gamepad->connected;
			bool dev_state_joined = initially_not_connected && success;
			bool dev_state_left = dev_connected && !success;

			gamepad->connected = success;
			gamepad->m_device_just_joined = dev_state_joined;
			gamepad->m_device_just_left = dev_state_left;
		}

		uint32 device_flags = 0;
		if (gamepad->m_device_just_left)
			SET_FLAG(device_flags, 0, true);
		if (gamepad->m_device_just_joined)
			SET_FLAG(device_flags, 13, true);

		input_abstraction_handle_device_change(device_flags);
		device_index++;

	} while (device_index < k_number_of_controllers);
}


int32* hs_debug_simulate_gamepad_global_get(void)
{
	return Memory::GetAddress<int32*>(0x47A71C);
}

bool* input_suppress_global_get(void)
{
	return Memory::GetAddress<bool*>(0x479F52);
}

void __cdecl input_set_gamepad_rumbler_state(int16 gamepad_index, uint16 left, uint16 right)
{
	ASSERT(VALID_INDEX(gamepad_index, k_number_of_controllers));

	XINPUT_VIBRATION state = { left, right };
	const XINPUT_VIBRATION state_none = { 0, 0 };

	state.wLeftMotorSpeed = (WORD)(state.wLeftMotorSpeed * g_rumble_factor);
	state.wRightMotorSpeed = (WORD)(state.wRightMotorSpeed * g_rumble_factor);

	bool enabled = user_interface_controller_get_rumble_enabled((e_controller_index)gamepad_index);
	g_vibration_state[gamepad_index] = (enabled ? state : state_none);
	return;
}

uint8 __cdecl input_windows_key_frames_down(int16 key)
{
	return INVOKE(0x2EF86, 0x0, input_windows_key_frames_down, key);
}

uint16 __cdecl input_windows_key_msec_down(int16 key)
{
	return INVOKE(0x2F030, 0x0, input_windows_key_msec_down, key);
}

bool __cdecl input_windows_drive_letter_test(int32 memory_unit, int8* drive_letter)
{
	ASSERT(memory_unit < k_number_of_memory_units);
	return INVOKE(0x2E463, 0, input_windows_drive_letter_test, memory_unit, drive_letter);
}

bool input_windows_processing_device_change()
{
	return g_notified_to_change_mapping;
}

bool input_windows_has_split_device_active()
{
	return g_should_offset_gamepad_indices;
}

void input_windows_notify_change_device_mapping()
{
	g_notified_to_change_mapping = true;
	g_should_offset_gamepad_indices = !g_should_offset_gamepad_indices;
}

int compare_device_compatibility(const void* p1, const void* p2)
{
	input_device* device1 = *(input_device**)p1;
	input_device* device2 = *(input_device**)p2;

	XINPUT_STATE state_temp;
	uint32 error_code1, error_code2;
	error_code1 = device1->XGetState(&state_temp);
	error_code2 = device2->XGetState(&state_temp);


	if (error_code1 == ERROR_SEVERITY_SUCCESS && error_code2 == ERROR_DEVICE_NOT_CONNECTED)
		return -1;
	else if (error_code2 == ERROR_SEVERITY_SUCCESS && error_code1 == ERROR_DEVICE_NOT_CONNECTED)
		return 1;

	return 0;
}

int compare_device_ports(const void* p1, const void* p2)
{
	xinput_device* device1 = *(xinput_device**)p1;
	xinput_device* device2 = *(xinput_device**)p2;

	return device1->get_port() - device2->get_port();
}

void input_windows_update_device_mapping()
{
	input_device* g_new_xinput_order[k_number_of_controllers] = {};
	for (uint16 gamepad_index = _controller_index_0; gamepad_index < k_number_of_controllers; gamepad_index++)
	{
		g_new_xinput_order[gamepad_index] = g_xinput_devices[gamepad_index];
	}

	//sort based on connectivity, 
	//most active /first connected gamepads will be first in the array
	qsort(g_new_xinput_order, NUMBEROF(g_new_xinput_order), sizeof(input_device*), compare_device_compatibility);

	// we also rotate the controller array 
	// so that k_windows_device_controller_index does not have most active controller

	if (g_should_offset_gamepad_indices)
	{
		g_xinput_devices[_controller_index_0] = g_new_xinput_order[_controller_index_3];
		g_xinput_devices[_controller_index_1] = g_new_xinput_order[_controller_index_0];
		g_xinput_devices[_controller_index_2] = g_new_xinput_order[_controller_index_1];
		g_xinput_devices[_controller_index_3] = g_new_xinput_order[_controller_index_2];
	}

}

void input_windows_restore_device_mapping()
{
	input_device* g_new_xinput_order[k_number_of_controllers] = {};
	for (uint16 gamepad_index = _controller_index_0; gamepad_index < k_number_of_controllers; gamepad_index++)
	{
		g_new_xinput_order[gamepad_index] = g_xinput_devices[gamepad_index];
	}

	//sort based on dwUserIndex (port)
	//this is set upon input_initialize()
	qsort(g_new_xinput_order, NUMBEROF(g_new_xinput_order), sizeof(input_device*), compare_device_ports);


	for (uint16 gamepad_index = _controller_index_0; gamepad_index < k_number_of_controllers; gamepad_index++)
	{
		g_xinput_devices[gamepad_index] = g_new_xinput_order[gamepad_index];
	}
}

bool input_has_gamepad_hook(int16 gamepad_index, bool* a1)
{
	if (g_should_offset_gamepad_indices && gamepad_index == k_windows_device_controller_index)
	{
		if (a1)
		{
			*a1 = true;
		}
		return true;
	}

	return input_has_gamepad(gamepad_index, a1);
}

void input_stop_removed_controller_handler_from_panicking()
{
	PatchCall(Memory::GetAddress(0x208D3C), input_has_gamepad_hook);
	PatchCall(Memory::GetAddress(0x2084B3), input_has_gamepad_hook);
	PatchCall(Memory::GetAddress(0x20844A), input_has_gamepad_hook);
}


void input_windows_apply_patches(void)
{
	input_globals = Memory::GetAddress<s_input_globals*>(0x479F50);
	g_input_windows_request_terminate = Memory::GetAddress<bool*>(0x971291);

	PatchCall(Memory::GetAddress(0x9020F), input_set_gamepad_rumbler_state);    // Replace call in rumble_clear_all_now

	PatchCall(Memory::GetAddress(0x2FA62), input_update_main_device_state);		// Replace call in input_update
	PatchCall(Memory::GetAddress(0x2FC2F), input_update_main_device_state);		// Replace call in input_update
	PatchCall(Memory::GetAddress(0x2FBD2), input_update_gamepads);				// Replace call in input_update

	input_stop_removed_controller_handler_from_panicking();
	return;
}