#include "stdafx.h"
#include "user_interface_utilities.h"

#include "hud.h"
#include "user_interface_controller.h"
#include "render/render.h"

datum __cdecl user_interface_get_widget_tag_index_from_screen_id(e_user_interface_screen_id screen_id)
{
	return INVOKE(0x20C701, 0x0, user_interface_get_widget_tag_index_from_screen_id, screen_id);
}

bool __cdecl user_interface_exit_to_dash()
{
	return INVOKE(0x212F7F, 0x0, user_interface_exit_to_dash);
}

bool __cdecl user_interface_save_map_and_exit()
{
	return INVOKE(0x212F68, 0x0, user_interface_save_map_and_exit);
}

void user_interface_transition_to_offline()
{
	for (e_controller_index controller = first_controller();
		controller != k_no_controller;
		controller = next_controller(controller))
	{
		if (user_interface_controller_is_player_profile_valid(controller))
		{
			user_interface_controller_xbox_live_account_set_signed_in(controller, false);
			user_interface_controller_update_player_name(controller);
			user_interface_controller_update_network_properties(controller);
		}
	}
}

void user_interface_get_cursor_position(point2d* out_position)
{
	INVOKE(0x2EC6D, 0, user_interface_get_cursor_position, out_position);
}

typedef void(__cdecl* user_interface_get_cursor_position_scale_t)(point2d* out_position);
user_interface_get_cursor_position_scale_t p_user_interface_get_cursor_position;

void __cdecl user_interface_get_cursor_position_scaled(point2d* out_position)
{
	if (out_position)
	{
		point2d temp_position;
		// Get the current cursor position
		user_interface_get_cursor_position(&temp_position);

		window_bound* bounds = get_user_window_bounds(0);

		temp_position.x = (int16)(temp_position.x - bounds->rasterizer_camera.viewport_bounds.right / 2.f);
		temp_position.y = (int16)((bounds->rasterizer_camera.viewport_bounds.bottom / 2.f) - temp_position.y);

		temp_position.x = (int16)(temp_position.x / *get_ui_scale());
		temp_position.y = (int16)(temp_position.y / *get_ui_scale());

		// Convert to integer coordinates
		out_position->x = temp_position.x;
		out_position->y = temp_position.y;
	}
	return;
}

void user_interface_utilities_apply_patches()
{
	DETOUR_ATTACH(p_user_interface_get_cursor_position, Memory::GetAddress<user_interface_get_cursor_position_scale_t>(0x21D620), user_interface_get_cursor_position_scaled);
	return;
}
