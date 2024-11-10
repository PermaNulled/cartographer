#include "stdafx.h"
#include "screen_error_dialog.h"
#include "interface/user_interface.h"
#include "interface/user_interface_controller.h"


typedef void(__cdecl* ok_cancle_dialog_t)(e_user_interface_channel_type channel_type, e_ui_error_types error_type, e_user_interface_render_window window_index, uint16 user_flags, void* ok_callback, void* fallback, int a7, int a8);
ok_cancle_dialog_t p_ok_cancle_dialog;
void __cdecl ok_cancle_dialog_show_hook(e_user_interface_channel_type channel_type, e_ui_error_types error_type, e_user_interface_render_window window_index, uint16 user_flags, void* ok_callback, void* fallback, int a7, int a8)
{
	//ALL_USERS_MASK
	if ((user_flags & 0xFF) == (uint8)NONE)
	{
		switch (error_type)
		{
		case _ui_error_overwrite_custom_keyboard_mappings:
		case _ui_error_map_download_collision:
		case _ui_error_live_checkpoint_connection_dropped:
		case _ui_error_live_checkpoint_hash_mismatch:
		case _ui_error_tnp_no_source:
		case _ui_error_tnp_disk_read:
		case _ui_error_tnp_no_engine_running:
		case _ui_error_tnp_signature_verification:
		case _ui_error_tnp_drive_removed:
		case _ui_error_tnp_disk_full:
		case _ui_error_tnp_permissions:
		case _ui_error_tnp_unknown:
			//special cases and cant be supported by all controllers
			user_flags = user_interface_controller_get_signed_in_controllers_mask();
			break;
		case _ui_error_confirm_boot_to_dash:
		case _ui_error_confirm_save_and_exit_campaign:
			user_flags = user_interface_controller_get_signed_in_controllers_mask()
				| FLAG(k_windows_device_controller_index); // addtitionally needed for start_screen_introduction
			break;
		case _ui_error_add_to_favorites:
		case _ui_error_remove_from_favorites:
			// these work fine with ALL_USERS_MASK
			break;
		}
	}
	p_ok_cancle_dialog(channel_type, error_type, window_index, user_flags, ok_callback, fallback, a7, a8);
}



void* c_screen_error_dialog_ok::load_for_active_users(s_screen_parameters* parameters)
{
	parameters->user_flags = user_interface_controller_get_signed_in_controllers_mask();
	return INVOKE(0x20E032, 0x0, c_screen_error_dialog_ok::load_for_active_users, parameters);
}

void c_screen_error_dialog_ok::apply_patches()
{
	WritePointer(Memory::GetAddress(0x20E173) + 1, c_screen_error_dialog_ok::load_for_active_users);
}

void c_screen_error_dialog_ok_cancel::apply_patches()
{
	DETOUR_ATTACH(p_ok_cancle_dialog, Memory::GetAddress<ok_cancle_dialog_t>(0x20E243), ok_cancle_dialog_show_hook);	
}
