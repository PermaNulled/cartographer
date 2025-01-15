#include "stdafx.h"

#include "imgui.h"
#include "imgui_handler.h"

#include "AdvancedSettingsStringTable.h"

#include "cartographer/twizzler/twizzler.h"
#include "cseries/cseries_strings.h"
#include "game/cheats.h"
#include "game/game.h"
#include "input/input_abstraction.h"
#include "interface/new_hud.h"
#include "interface/first_person_weapons.h"
#include "networking/NetworkMessageTypeCollection.h"
#include "rasterizer/rasterizer_globals.h"
#include "saved_games/cartographer_player_profile.h"

#include "H2MOD.h"
#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Modules/SpecialEvents/SpecialEvents.h"

#ifndef NDEBUG
#include "H2MOD/Modules/RenderHooks/RenderHooks.h"
#endif

/* constants */

const char* const k_button_items[] = { "Dpad Up","Dpad Down","Dpad Left","Dpad Right","Start","Back","Left Thumb","Right Thumb","Left Bumper","Right Bumper","A","B","X","Y" };
const char* const k_action_items[] = { "Dpad Up","Dpad Down","Dpad Left","Dpad Right","Start","Back","Crouch","Zoom","Flashlight","Switch Grenades","Jump","Melee","Reload","Switch Weapons" };
const uint16 k_button_values[k_number_of_xinput_buttons] =
{ 
	XINPUT_GAMEPAD_DPAD_UP, 
	XINPUT_GAMEPAD_DPAD_DOWN, 
	XINPUT_GAMEPAD_DPAD_LEFT, 
	XINPUT_GAMEPAD_DPAD_RIGHT, 
	XINPUT_GAMEPAD_START, 
	XINPUT_GAMEPAD_BACK, 
	XINPUT_GAMEPAD_LEFT_THUMB,
	XINPUT_GAMEPAD_RIGHT_THUMB,
	XINPUT_GAMEPAD_LEFT_SHOULDER,
	XINPUT_GAMEPAD_RIGHT_SHOULDER,
	XINPUT_GAMEPAD_A,
	XINPUT_GAMEPAD_B,
	XINPUT_GAMEPAD_X,
	XINPUT_GAMEPAD_Y
};

/* prototypes */

const char* advanced_settings_get_string(e_advanced_string string, const char* id = NULL);

/* globals */

e_language g_language;

c_static_string<256> g_advanced_settings_temp_string;

int g_button_placeholders[14];

e_controller_index current_controller_index = k_no_controller;
s_saved_game_cartographer_player_profile* current_cartographer_profile;

namespace ImGuiHandler {
	namespace ImAdvancedSettings {

		namespace
		{
			bool g_twizzler_checkbox = g_twizzler_status;
			bool should_show_hud = true;
			bool g_showFP = true;
			int g_deadzone = 0;
			int g_aiming = 0;
			int g_shadows = 0;
			int g_water = 0;
			bool g_init = false;

			void DrawDeadzones()
			{
				ImDrawList* draw_list = ImGui::GetForegroundDrawList();
				const ImGuiViewport* viewport = ImGui::GetMainViewport();
				ImVec2 Center(
					viewport->WorkSize.x - 200,
					viewport->WorkSize.y - 200
				);

				draw_list->AddRectFilled(ImVec2(Center.x - 100, Center.y - 100), ImVec2(Center.x + 100, Center.y + 100), ImColor(20, 20, 20));
				draw_list->AddCircleFilled(Center, 100, ImColor(255, 255, 255), 120);
				if (current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_axial 
					|| current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_combined) 
				{
					real32 x_axis = current_cartographer_profile->deadzone_axial.x;
					real32 y_axis = current_cartographer_profile->deadzone_axial.y;
					ImVec2 Y_TopLeft(
						Center.x - 100,
						Center.y - y_axis);
					ImVec2 Y_BottomRight(
						Center.x + 100,
						Center.y + y_axis);
					draw_list->AddRectFilled(Y_TopLeft, Y_BottomRight, ImColor(20, 20, 20, 125));
					ImVec2 X_TopLeft(Center.x - x_axis, Center.y - 100);
					ImVec2 X_BottomRight(Center.x + x_axis, Center.y + 100);
					draw_list->AddRectFilled(X_TopLeft, X_BottomRight, ImColor(20, 20, 20, 125));
				}
				if (current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_radial 
					|| current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_combined) 
				{
					draw_list->AddCircleFilled(Center, current_cartographer_profile->deadzone_radial, ImColor(20, 20, 20, 125), 120);
				}

				s_gamepad_input_button_state* state = input_get_gamepad_state(current_controller_index);
				if (state)
				{
					ImVec2 Thumb_Pos(
						Center.x + THUMBSTICK_POINT_TO_PERCENTAGE(state->thumb_right.x),
						Center.y - THUMBSTICK_POINT_TO_PERCENTAGE(state->thumb_right.y));
					int axial_invalid = 0;
					if (abs(state->thumb_right.x) <= THUMBSTICK_PERCENTAGE_TO_POINT(current_cartographer_profile->deadzone_axial.x))
						axial_invalid++;
					if (abs(state->thumb_right.y) <= THUMBSTICK_PERCENTAGE_TO_POINT(current_cartographer_profile->deadzone_axial.y))
						axial_invalid++;
					bool radial_invalid = false;

					uint32 ar = (int16)THUMBSTICK_PERCENTAGE_TO_POINT(current_cartographer_profile->deadzone_radial);
					ar *= ar;

					uint32 arx = state->thumb_right.x * state->thumb_right.x;
					uint32 ary = state->thumb_right.y * state->thumb_right.y;
					uint32 rh = arx + ary;
					if (rh <= ar)
					{
						radial_invalid = true;
					}
					bool valid = true;
					if (current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_axial
						|| current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_combined)
					{
						if (axial_invalid == 2)
							valid = false;
					}
					if (current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_radial 
						|| current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_combined)
					{
						if (radial_invalid)
							valid = false;
					}

					int8 red = (valid ? 0 : 255);
					int8 green = (valid ? 255 : 0);
					draw_list->AddCircleFilled(Thumb_Pos, 5, ImColor(red, green, 0), 60);
				}
			}
			void HudSettings()
			{
				ImVec2 item_size = ImGui::GetItemRectSize();
				if (ImGui::CollapsingHeader(advanced_settings_get_string(_advanced_string_hud_title)))
				{
					ImVec2 b2_size = ImVec2(WidthPercentage(10), item_size.y);
					//Player FOV
					ImGui::Text(advanced_settings_get_string(_advanced_string_player_field_of_view));
					ImGui::PushItemWidth(WidthPercentage(80));
					int val = current_cartographer_profile->field_of_view;
					ImGui::SliderInt("##PlayerFOV1", &val, 45, 110, "");
					ImGui::SameLine();
					if(ImGui::IsItemEdited())
					{
						current_cartographer_profile->field_of_view = PIN(val, 45, 110);
					}

					ImGui::PushItemWidth(WidthPercentage(10));

					val = current_cartographer_profile->field_of_view;
					ImGui::InputInt("##PlayerFOV2", &val, 0, 110, ImGuiInputTextFlags_::ImGuiInputTextFlags_AutoSelectAll);
					ImGui::SameLine();
					if (ImGui::IsItemEdited()) 
					{
						current_cartographer_profile->field_of_view = PIN(val, 45, 110);
					}
					ImGui::PushItemWidth(WidthPercentage(10));
					if (ImGui::Button(advanced_settings_get_string(_advanced_string_reset, "PlayerFov3"), b2_size))
					{
						current_cartographer_profile->field_of_view = 78;
					}
					ImGui::PopItemWidth();


					//Vehicle FOV
					ImGui::Text(advanced_settings_get_string(_advanced_string_vehicle_field_of_view));
					ImGui::PushItemWidth(WidthPercentage(80));

					val = current_cartographer_profile->vehicle_field_of_view;
					ImGui::SliderInt("##VehicleFOV1", &val, 45, 110, ""); ImGui::SameLine();
					if (ImGui::IsItemEdited())
					{
						current_cartographer_profile->vehicle_field_of_view = PIN(val, 45, 110);
					}

					ImGui::PushItemWidth(WidthPercentage(10));

					val = current_cartographer_profile->vehicle_field_of_view;
					ImGui::InputInt("##VehicleFOV2", &val, 0, 110, ImGuiInputTextFlags_::ImGuiInputTextFlags_AutoSelectAll); ImGui::SameLine();
					if (ImGui::IsItemEdited()) {
						current_cartographer_profile->vehicle_field_of_view = PIN(val, 45, 110);
					}
					ImGui::PushItemWidth(WidthPercentage(10));
					if (ImGui::Button(advanced_settings_get_string(_advanced_string_reset, "VehicleFOV3"), b2_size))
					{
						current_cartographer_profile->vehicle_field_of_view = 78;
					}
					ImGui::PopItemWidth();


					//Crosshair Offset
					ImGui::Text(advanced_settings_get_string(_advanced_string_crosshair_offset));
					ImGui::PushItemWidth(WidthPercentage(80));
					ImGui::SliderFloat("##Crosshair1", &current_cartographer_profile->crosshair_offset, 0.0f, 0.5f, ""); ImGui::SameLine();

					ImGui::PushItemWidth(WidthPercentage(10));
					ImGui::InputFloat("##Crosshair2", &current_cartographer_profile->crosshair_offset, 0, 110, "%.3f"); ImGui::SameLine();
					if (ImGui::IsItemEdited()) {
						current_cartographer_profile->crosshair_offset = PIN(current_cartographer_profile->crosshair_offset, 0.f, 0.5f);
					}
					ImGui::PushItemWidth(WidthPercentage(10));
					if (ImGui::Button(advanced_settings_get_string(_advanced_string_reset, "Crosshair3"), b2_size))
					{
						current_cartographer_profile->crosshair_offset = 0.138f;
					}
					ImGui::PopItemWidth();
					

					//Crosshair Size
					ImGui::Text(advanced_settings_get_string(_advanced_string_crosshair_size));
					ImGui::PushItemWidth(WidthPercentage(80));
					ImGui::SliderFloat("##CrosshairSize1", &current_cartographer_profile->crosshair_scale, 0.0f, 2.0f, "");  ImGui::SameLine();

					ImGui::PushItemWidth(WidthPercentage(10));
					ImGui::InputFloat("##CrosshairSize2", &current_cartographer_profile->crosshair_scale, 0, 110, "%.3f"); ImGui::SameLine();
					if (ImGui::IsItemEdited()) {
						current_cartographer_profile->crosshair_scale = PIN(current_cartographer_profile->crosshair_scale, 0, 2);
					}
					ImGui::PushItemWidth(WidthPercentage(10));
					if (ImGui::Button(advanced_settings_get_string(_advanced_string_reset, "CrosshairSize3"), b2_size))
					{
						current_cartographer_profile->crosshair_scale = 1;
					}
					ImGui::PopItemWidth();


					if (current_controller_index == _controller_index_0)
					{
						ImVec2 b3_size = ImVec2(WidthPercentage(33.3333333333f), item_size.y);
						ImGui::NewLine();
						//Ingame Change Display
						if (ImGui::Button(advanced_settings_get_string(_advanced_string_weaponoffsets, "WeaponOffsets"), b3_size))
						{
							ImGuiHandler::ToggleWindow(k_weapon_offsets_window_name);
						}

						ImGui::Columns(2, NULL, false);

						ImGui::Checkbox(advanced_settings_get_string(_advanced_string_hide_ingame_chat), &H2Config_hide_ingame_chat);
						ImGui::NextColumn();
					}

					ImGui::Checkbox(advanced_settings_get_string(_advanced_string_static_fp), &current_cartographer_profile->static_first_person);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_static_fp_tooltip));
					ImGui::NextColumn();

					if (current_controller_index == _controller_index_0)
					{
						ImGui::Checkbox(advanced_settings_get_string(_advanced_string_show_hud), &should_show_hud);
						if (ImGui::IsItemEdited())
							should_draw_hud_override_set(should_show_hud);
						ImGui::NextColumn();

						ImGui::Checkbox(advanced_settings_get_string(_advanced_string_show_first_person), &g_showFP);
						if (ImGui::IsItemEdited())
							toggle_first_person(g_showFP);
						ImGui::Columns(1);
						ImGui::NewLine();
					}
				}
			}
			void VideoSettings()
			{
				s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
				ImVec2 item_size = ImGui::GetItemRectSize();
				if (ImGui::CollapsingHeader(advanced_settings_get_string(_advanced_string_video_title)))
				{
					//FPS Limit
					ImGui::Columns(2, NULL, false);
					ImGui::Text(advanced_settings_get_string(_advanced_string_fps_limit));
					ImGui::PushItemWidth(WidthPercentage(50));
					int fps_limit = H2Config_fps_limit;
					ImGui::InputInt("##FPS1", &fps_limit, 0, 110);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_fps_limit_tooltip));
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						if (fps_limit > 0)
						{
							fps_limit = MAX(fps_limit, 25);
						}
						else
						{
							fps_limit = 0;
						}

						H2Config_fps_limit = fps_limit;
					}

					ImGui::SameLine();
					if (ImGui::Button(advanced_settings_get_string(_advanced_string_reset, "FPS2"), ImVec2(WidthPercentage(50), item_size.y)))
					{
						H2Config_fps_limit = 60;
					}

					// from the old refresh button
					ImGui::NextColumn();
					ImGui::NextColumn();
					//LOD
					ImGui::Text(advanced_settings_get_string(_advanced_string_lod));
					const char* items[] = { advanced_settings_get_string(_advanced_string_default), advanced_settings_get_string(_advanced_string_lod_1), advanced_settings_get_string(_advanced_string_lod_2), advanced_settings_get_string(_advanced_string_lod_3), advanced_settings_get_string(_advanced_string_lod_4), advanced_settings_get_string(_advanced_string_lod_5), advanced_settings_get_string(_advanced_string_lod_6) };
					ImGui::PushItemWidth(WidthPercentage(100));

					int static_lod_state = H2Config_static_lod_state;

					ImGui::Combo("##LOD", &static_lod_state, items, 7);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_lod_tooltip));

					H2Config_static_lod_state = static_lod_state;

					ImGui::NextColumn();
					ImGui::Text(advanced_settings_get_string(_advanced_string_shadow_title));
					

					const char* s_items[]
					{ 
						advanced_settings_get_string(_advanced_string_tex_L1),
						advanced_settings_get_string(_advanced_string_default),
						advanced_settings_get_string(_advanced_string_tex_L2),
						advanced_settings_get_string(_advanced_string_tex_L3)
					};
					ImGui::PushItemWidth(WidthPercentage(100));
					if (ImGui::Combo("##Shadows", &g_shadows, s_items, 4))
					{
						H2Config_Override_Shadows = (e_override_texture_resolution)g_shadows;
						rasterizer_globals->reset_screen = true;
					}
					ImGui::NextColumn();
					ImGui::Text(advanced_settings_get_string(_advanced_string_water_title));
					ImGui::PushItemWidth(WidthPercentage(100));
					if (ImGui::Combo("##Water", &g_water, s_items, 4))
					{
						H2Config_Override_Water = (e_override_texture_resolution)g_water;
						rasterizer_globals->reset_screen = true;
					}

					ImGui::Columns(1);

					//Force max shader LOD

					ImGui::Checkbox(advanced_settings_get_string(_advanced_string_shader_lod_max), &H2Config_shader_lod_max);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_shader_lod_max_tooltip));

					//Disable Light Suppressor

					ImGui::Checkbox(advanced_settings_get_string(_advanced_string_light_suppressor), &H2Config_light_suppressor);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_light_suppressor_tooltip));
				}
			}
			void MouseKeyboardSettings()
			{
				ImVec2 item_size = ImGui::GetItemRectSize();
				if (ImGui::CollapsingHeader(advanced_settings_get_string(_advanced_string_m_k_title)))
				{
					ImGui::Columns(2, NULL, false);

					//Raw Input
					TextVerticalPad(advanced_settings_get_string(_advanced_string_raw_mouse));
					ImGui::SameLine(ImGui::GetColumnWidth() - 35);
					ImGui::Checkbox("##RawMouse", &current_cartographer_profile->raw_mouse_input);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_raw_mouse_tooltip));
					}
					//Uniform Sensitivity
					ImGui::NextColumn();
					TextVerticalPad(advanced_settings_get_string(_advanced_string_uniform_sensitivity));
					ImGui::SameLine(ImGui::GetColumnWidth() - 35);
					ImGui::Checkbox("##MK_Sep", &current_cartographer_profile->mouse_uniform);
					if (ImGui::IsItemEdited())
					{
						input_abstraction_set_mouse_look_sensitivity(_controller_index_0, current_cartographer_profile->mouse_sensitivity);
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_uniform_sensitivity_tooltip));
					}
					ImGui::Columns(1);
					if (current_cartographer_profile->raw_mouse_input) {
						ImGui::Text(advanced_settings_get_string(_advanced_string_raw_mouse_sensitivity));
						ImGui::PushItemWidth(WidthPercentage(75));
						int g_raw_scale = (int)current_cartographer_profile->raw_mouse_sensitivity;
						ImGui::SliderInt("##RawMouseScale1", &g_raw_scale, 1, 100, ""); ImGui::SameLine();
						if (ImGui::IsItemEdited())
						{
							current_cartographer_profile->raw_mouse_sensitivity = (float)g_raw_scale;
						}
						ImGui::PushItemWidth(WidthPercentage(15));
						ImGui::InputFloat("##RawMouseScale2", &current_cartographer_profile->raw_mouse_sensitivity, 0, 110, "%.1f", ImGuiInputTextFlags_::ImGuiInputTextFlags_AutoSelectAll); ImGui::SameLine();
						if (ImGui::IsItemEdited()) {
							current_cartographer_profile->raw_mouse_sensitivity = PIN(current_cartographer_profile->raw_mouse_sensitivity, 1.f, 100.f);
						}
						ImGui::PushItemWidth(WidthPercentage(10));
						if (ImGui::Button(advanced_settings_get_string(_advanced_string_reset, "RawMouseScale2"), ImVec2(WidthPercentage(10), item_size.y)))
						{
							current_cartographer_profile->raw_mouse_sensitivity = 25.f;
						}
					}
					else
					{
						ImGui::Text(advanced_settings_get_string(_advanced_string_mouse_sensitivity));
						ImGui::PushItemWidth(WidthPercentage(75));
						int g_mouse_sens = (int)current_cartographer_profile->mouse_sensitivity;
						ImGui::SliderInt("##Mousesens1", &g_mouse_sens, 1, 100, ""); ImGui::SameLine();
						if (ImGui::IsItemEdited())
						{
							current_cartographer_profile->mouse_sensitivity = (real32)g_mouse_sens;
							input_abstraction_set_mouse_look_sensitivity(_controller_index_0, current_cartographer_profile->mouse_sensitivity);
						}
						ImGui::PushItemWidth(WidthPercentage(15));

						ImGui::InputFloat("##Mousesens2", &current_cartographer_profile->mouse_sensitivity, 0.f, 110.f, "%.1f", ImGuiInputTextFlags_::ImGuiInputTextFlags_AutoSelectAll); ImGui::SameLine();
						if (ImGui::IsItemEdited()) {

							current_cartographer_profile->mouse_sensitivity = PIN(current_cartographer_profile->mouse_sensitivity, 1.f, 100.f);
							input_abstraction_set_mouse_look_sensitivity(_controller_index_0, current_cartographer_profile->mouse_sensitivity);
						}
						ImGui::PushItemWidth(WidthPercentage(10));
						if (ImGui::Button(advanced_settings_get_string(_advanced_string_reset, "Mousesens3"), ImVec2(WidthPercentage(10), item_size.y)))
						{
							current_cartographer_profile->mouse_sensitivity = 3.0f;
							input_abstraction_set_mouse_look_sensitivity(_controller_index_0, current_cartographer_profile->mouse_sensitivity);
						}
					}
				}
			}
			void ControllerSettings()
			{
				ImVec2 item_size = ImGui::GetItemRectSize();
				if (ImGui::CollapsingHeader(advanced_settings_get_string(_advanced_string_controller_title)))
				{
					DrawDeadzones();
					ImGui::Columns(2, NULL, false);
					//Uniform Sensitivity
					TextVerticalPad(advanced_settings_get_string(_advanced_string_uniform_sensitivity));
					ImGui::SameLine(ImGui::GetColumnWidth() - 35);
					ImGui::Checkbox("##C_Sep", &current_cartographer_profile->mouse_uniform);
					if (ImGui::IsItemEdited())
					{
						input_abstraction_set_mouse_look_sensitivity(_controller_index_0, current_cartographer_profile->mouse_sensitivity);
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_uniform_sensitivity_tooltip));
					}
					ImGui::Columns(1);


					ImGui::Text(advanced_settings_get_string(_advanced_string_controller_sensitivity));
					ImGui::PushItemWidth(WidthPercentage(75));
					int g_controller_sens = (int)current_cartographer_profile->controller_sensitivity;
					ImGui::SliderInt("##Controllersens1", &g_controller_sens, 1, 100, ""); ImGui::SameLine();
					if (ImGui::IsItemEdited())
					{
						current_cartographer_profile->controller_sensitivity = (float)g_controller_sens;
						input_abstraction_set_controller_look_sensitivity(current_controller_index, current_cartographer_profile->controller_sensitivity);
					}
					ImGui::PushItemWidth(WidthPercentage(15));
					ImGui::InputFloat("##Controllersens2", &current_cartographer_profile->controller_sensitivity, 0.f, 110.f, "%.1f", ImGuiInputTextFlags_::ImGuiInputTextFlags_AutoSelectAll); ImGui::SameLine();
					if (ImGui::IsItemEdited()) {
						current_cartographer_profile->controller_sensitivity = PIN(current_cartographer_profile->controller_sensitivity, 1, 100);
						input_abstraction_set_controller_look_sensitivity(current_controller_index, current_cartographer_profile->controller_sensitivity);
					}
					ImGui::PushItemWidth(WidthPercentage(10));
					if (ImGui::Button(advanced_settings_get_string(_advanced_string_reset, "Controllersens3"), ImVec2(WidthPercentage(10), item_size.y)))
					{
						g_controller_sens = 3;
						current_cartographer_profile->controller_sensitivity = 3.0f;
						input_abstraction_set_controller_look_sensitivity(current_controller_index, current_cartographer_profile->controller_sensitivity);
					}
					ImGui::PopItemWidth();

					ImGui::Columns(2, NULL, false);
					ImGui::Text(advanced_settings_get_string(_advanced_string_aiming_type));
					const char* a_items[] = { advanced_settings_get_string(_advanced_string_default), advanced_settings_get_string(_advanced_string_modern) };
					ImGui::PushItemWidth(ImGui::GetColumnWidth());
					if (ImGui::Combo("##C_Aiming_Style", &g_aiming, a_items, 2))
					{
						current_cartographer_profile->controller_modern = g_aiming != 0;
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_aiming_type_tooltip));
					}
					ImGui::PopItemWidth();

					ImGui::NextColumn();

					ImGui::Text(advanced_settings_get_string(_advanced_string_deadzone_type));
					const char* items[] = { advanced_settings_get_string(_advanced_string_axial), advanced_settings_get_string(_advanced_string_radial), advanced_settings_get_string(_advanced_string_both) };
					ImGui::PushItemWidth(ImGui::GetColumnWidth());
					if (ImGui::Combo("##C_Deadzone_Type", &g_deadzone, items, 3))
					{
						current_cartographer_profile->controller_deadzone_type = g_deadzone;
						input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_deadzone_type_tooltip));
					}
					ImGui::PopItemWidth();
					ImGui::Columns(1);

					if (current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_axial 
						|| current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_combined) 
					{
						ImGui::Text(advanced_settings_get_string(_advanced_string_axial_deadzone_X));
						ImGui::PushItemWidth(WidthPercentage(75));
						ImGui::SliderFloat("##C_Deadzone_A_X_1", &current_cartographer_profile->deadzone_axial.x, 0, 100, "");
						if (ImGui::IsItemEdited())
						{
							input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
						}
						ImGui::SameLine();
						ImGui::PushItemWidth(WidthPercentage(13));
						ImGui::InputFloat("##C_Deadzone_A_X_2", &current_cartographer_profile->deadzone_axial.x, 0, 3);
						if (ImGui::IsItemEdited())
						{
							if (current_cartographer_profile->deadzone_axial.x < 0)
								current_cartographer_profile->deadzone_axial.x = 0;
							if (current_cartographer_profile->deadzone_axial.x > 100)
								current_cartographer_profile->deadzone_axial.x = 100;
							input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
						}
						ImGui::SameLine();
						ImGui::PushItemWidth(WidthPercentage(15));
						if (ImGui::Button(advanced_settings_get_string(_advanced_string_default, "C_Deadzone_A_X_3"), ImVec2(WidthPercentage(12), item_size.y)))
						{
							current_cartographer_profile->deadzone_axial.x = k_default_right_thumbstick_deadzone_axial_percentage_x;
							input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
						}
						ImGui::PopItemWidth();
						ImGui::Text(advanced_settings_get_string(_advanced_string_axial_deadzone_Y));
						ImGui::PushItemWidth(WidthPercentage(75));
						ImGui::SliderFloat("##C_Deadzone_A_Y_1", &current_cartographer_profile->deadzone_axial.y, 0, 100, "");
						if (ImGui::IsItemEdited())
						{
							input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
						}
						ImGui::SameLine();
						ImGui::PushItemWidth(WidthPercentage(13));
						ImGui::InputFloat("##C_Deadzone_A_Y_2", &current_cartographer_profile->deadzone_axial.y, 0, 3);
						if (ImGui::IsItemEdited())
						{
							if (current_cartographer_profile->deadzone_axial.y < 0)
								current_cartographer_profile->deadzone_axial.y = 0;
							if (current_cartographer_profile->deadzone_axial.y > 100)
								current_cartographer_profile->deadzone_axial.y = 100;
							input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
						}
						ImGui::SameLine();
						ImGui::PushItemWidth(WidthPercentage(12));
						if (ImGui::Button(advanced_settings_get_string(_advanced_string_default, "C_Deadzone_A_Y_3"), ImVec2(WidthPercentage(12), item_size.y)))
						{
							current_cartographer_profile->deadzone_axial.y = k_default_right_thumbstick_deadzone_axial_percentage_y;
							input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
						}
						ImGui::PopItemWidth();
					}
					if (current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_radial 
						|| current_cartographer_profile->controller_deadzone_type == _controller_deadzone_type_combined) 
					{
						ImGui::Text(advanced_settings_get_string(_advanced_string_radial_deadzone_radius));
						ImGui::PushItemWidth(WidthPercentage(75));
						ImGui::SliderFloat("##C_Deadzone_R_1", &current_cartographer_profile->deadzone_radial, 0, 100, "");
						if (ImGui::IsItemEdited())
						{
							input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
						}
						ImGui::SameLine();
						ImGui::PushItemWidth(WidthPercentage(13));
						ImGui::InputFloat("##C_Deadzone_R_2", &current_cartographer_profile->deadzone_radial, 0, 3);
						if (ImGui::IsItemEdited())
						{
							if (current_cartographer_profile->deadzone_radial < 0)
								current_cartographer_profile->deadzone_radial = 0;
							if (current_cartographer_profile->deadzone_radial > 100)
								current_cartographer_profile->deadzone_radial = 100;
							input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
						}
						ImGui::SameLine();
						ImGui::PushItemWidth(WidthPercentage(12));
						if (ImGui::Button(advanced_settings_get_string(_advanced_string_default, "C_Deadzone_R_R_3"), ImVec2(WidthPercentage(12), item_size.y)))
						{
							current_cartographer_profile->deadzone_radial = k_default_right_thumbstick_deadzone_radial_percentage;
							input_abstraction_set_controller_right_thumb_deadzone(current_controller_index);
						}
						ImGui::PopItemWidth();
					}
					ImGui::NewLine();
					ImGui::Text("Controller Layout");
					ImGui::NewLine();
					ImGui::TextWrapped("To use this you must have your games controller layout SET TO DEFAULT. Changing the drop down for the specific action will remap the button to the new one");
					ImGui::NewLine();
					ImGui::Columns(3, NULL, false);
					for (auto i = 0; i < 14; i++)
					{
						ImGui::Text(k_button_items[i]);
						ImGui::PushItemWidth(ImGui::GetColumnWidth());

						// 2 digit number + null terminator
						char index_string[3];
						csprintf(index_string, NUMBEROF(index_string), "%d", i);

						c_static_string<8> label("##C_L");
						label.append(index_string);
						if (ImGui::Combo(label.get_string(), &g_button_placeholders[i], k_action_items, 14))
						{
							switch (k_button_values[i])
							{
							case XINPUT_GAMEPAD_DPAD_UP:
								current_cartographer_profile->custom_layout.DPAD_UP = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_DPAD_DOWN:
								current_cartographer_profile->custom_layout.DPAD_DOWN = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_DPAD_LEFT:
								current_cartographer_profile->custom_layout.DPAD_LEFT = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_DPAD_RIGHT:
								current_cartographer_profile->custom_layout.DPAD_RIGHT = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_START:
								current_cartographer_profile->custom_layout.START = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_BACK:
								current_cartographer_profile->custom_layout.BACK = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_LEFT_THUMB:
								current_cartographer_profile->custom_layout.LEFT_THUMB = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_RIGHT_THUMB:
								current_cartographer_profile->custom_layout.RIGHT_THUMB = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_LEFT_SHOULDER:
								current_cartographer_profile->custom_layout.LEFT_SHOULDER = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_RIGHT_SHOULDER:
								current_cartographer_profile->custom_layout.RIGHT_SHOULDER = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_A:
								current_cartographer_profile->custom_layout.A = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_B:
								current_cartographer_profile->custom_layout.B = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_X:
								current_cartographer_profile->custom_layout.X = k_button_values[g_button_placeholders[i]];
								break;
							case XINPUT_GAMEPAD_Y:
								current_cartographer_profile->custom_layout.Y = k_button_values[g_button_placeholders[i]];
								break;
							}
						}
						ImGui::PopItemWidth();
						ImGui::NextColumn();
					}
					ImGui::Columns(1);
				}
			}
			void HostSettings()
			{
				if (game_is_campaign() || (network_life_cycle_in_squad_session(NULL) && NetworkSession::LocalPeerIsSessionHost())) {
					if (ImGui::CollapsingHeader(advanced_settings_get_string(_advanced_string_host_campagin_settings)))
					{
						ImGui::Columns(2, NULL, false);
						TextVerticalPad(advanced_settings_get_string(_advanced_string_anti_cheat));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						if (ImGui::Checkbox("##Anti-Cheat", &g_twizzler_checkbox))
						{
							twizzler_set_status(g_twizzler_checkbox);
							for (int i = 0; i < NetworkSession::GetPeerCount(); i++)
							{
								NetworkMessage::SendAntiCheat(i);
							}
						}
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_anti_cheat_tooltip));
						}

						ImGui::NextColumn();

						//XDelay
						TextVerticalPad(advanced_settings_get_string(_advanced_string_disable_x_delay));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						if (ImGui::Checkbox("##XDelay", &H2Config_xDelay))
						{
							H2MOD::RefreshTogglexDelay();
						}
						ImGui::Columns(1);
						ImGui::Separator();
						ImGui::Columns(3, NULL, false);

						// TODO Remove this and replace with proper menu when selecting a map (WIP)
						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_anger));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullAnger", &get_ice_cream_activation()[_skull_type_anger]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_anger_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_assassins));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullAssassins", &get_ice_cream_activation()[_skull_type_assassians]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_assassins_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_black_eye));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullBlackEye", &get_ice_cream_activation()[_skull_type_black_eye]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_black_eye_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_blind));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullBlind", &get_ice_cream_activation()[_skull_type_blind]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_blind_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_catch));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullCatch", &get_ice_cream_activation()[_skull_type_catch]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_catch_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_envy));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullEnvy", &get_ice_cream_activation()[_skull_type_envy]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_envy_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_famine));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullFamine", &get_ice_cream_activation()[_skull_type_famine]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_famine_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_ghost));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullGhost", &get_ice_cream_activation()[_skull_type_ghost]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_ghost_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_grunt));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullGBP", &get_ice_cream_activation()[_skull_type_grunt_birthday_party]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_grunt_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_iron));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullIron", &get_ice_cream_activation()[_skull_type_iron]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_iron_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_iwbyd));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullIWHBYD", &get_ice_cream_activation()[_skull_type_iwhbyd]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_iwbyd_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_mythic));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullMythic", &get_ice_cream_activation()[_skull_type_mythic]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_mythic_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_sputnik));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullSputnik", &get_ice_cream_activation()[_skull_type_sputnik]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_sputnik_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_thunderstorm));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullThunderstorm", &get_ice_cream_activation()[_skull_type_thunderstorm]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_thunderstorm_tooltip));

						ImGui::NextColumn();

						TextVerticalPad(advanced_settings_get_string(_advanced_string_skull_whuppopotamus));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##SkullWhuppopatamus", &get_ice_cream_activation()[_skull_type_whuppopotamus]);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skull_whuppopotamus_tooltip));

						ImGui::Columns(1);
					}
				}
			}
			void GameSettings()
			{
				if (ImGui::CollapsingHeader(advanced_settings_get_string(_advanced_string_game_title)))
				{
					ImGui::Columns(2, NULL, false);

					TextVerticalPad(advanced_settings_get_string(_advanced_string_discord_presence));
					ImGui::SameLine(ImGui::GetColumnWidth() - 35);
					ImGui::Checkbox("##DRP", &H2Config_discord_enable);

					TextVerticalPad(advanced_settings_get_string(_advanced_string_upnp_title));
					ImGui::SameLine(ImGui::GetColumnWidth() - 35);
					ImGui::Checkbox("##upnp", &H2Config_upnp_enable);
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_upnp_tooltip));
					}

					TextVerticalPad(advanced_settings_get_string(_advanced_string_no_events_title));
					ImGui::SameLine(ImGui::GetColumnWidth() - 35);
					ImGui::Checkbox("##no_events", &H2Config_no_events);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_no_events_tooltip));

					if (get_current_special_event() == _special_event_halloween) {
						TextVerticalPad(advanced_settings_get_string(_advanced_string_skeleton_biped));
						ImGui::SameLine(ImGui::GetColumnWidth() - 35);
						ImGui::Checkbox("##spooky_scary", &H2Config_spooky_boy);
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip(advanced_settings_get_string(_advanced_string_skeleton_biped_tooltip));
					}

					// next column elements now
					//Skip Intro
					ImGui::NextColumn();
					TextVerticalPad(advanced_settings_get_string(_advanced_string_disable_intro_videos));
					ImGui::SameLine(ImGui::GetColumnWidth() - 35);
					ImGui::Checkbox("##Intro", &H2Config_skip_intro);

					ImGui::Columns(1);

					ImGui::Text(advanced_settings_get_string(_advanced_string_language));

					const char* lang_items[]
					{
						advanced_settings_get_string(_advanced_string_lang_english),
						advanced_settings_get_string(_advanced_string_lang_japanese),
						advanced_settings_get_string(_advanced_string_lang_german),
						advanced_settings_get_string(_advanced_string_lang_french),
						advanced_settings_get_string(_advanced_string_lang_spanish),
						advanced_settings_get_string(_advanced_string_lang_italian),
						advanced_settings_get_string(_advanced_string_lang_korean),
						advanced_settings_get_string(_advanced_string_lang_chinese),
						advanced_settings_get_string(_advanced_string_lang_native) 
					};

					ImGui::PushItemWidth(ImGui::GetColumnWidth());
					if (ImGui::Combo("##Language_Selection", (int*)&g_language, lang_items, k_language_count))
					{
						H2Config_language.code_main = (g_language == 8 ? -1 : g_language);
						//setCustomLanguage(H2Config_language.code_main, H2Config_language.code_variant);
					}
					ImGui::PopItemWidth();


					ImGui::Columns(1);
					ImGui::NewLine();
				}
			}
		}

		
		void Render(bool* p_open)
		{
			if (!g_init)
			{
				g_deadzone = (int)current_cartographer_profile->controller_deadzone_type;
				g_aiming = (int)current_cartographer_profile->controller_modern;
				g_shadows = (int)H2Config_Override_Shadows;
				g_water = (int)H2Config_Override_Water;
				g_language = (H2Config_language.code_main == -1 ? _language_portuguese : (e_language)H2Config_language.code_main);
				g_init = true;
			}

			bool open = *p_open;
			ImGuiIO& io = ImGui::GetIO();
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGuiWindowFlags window_flags = 0;
			window_flags |= ImGuiWindowFlags_NoCollapse;
			window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			//window_flags |= ImGuiWindowFlags_MenuBar;
			ImGui::SetNextWindowPos(ImVec2(viewport->WorkSize.x * 0.5f, viewport->WorkSize.y * 0.5f), ImGuiCond_::ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 8));
			//ImGui::PushFont(font2);
			ImGui::SetNextWindowSize(ImVec2(650, 530), ImGuiCond_Appearing);
			ImGui::SetNextWindowSizeConstraints(ImVec2(610, 530), ImVec2(1920, 1080));
			if (game_is_ui_shell())
				ImGui::SetNextWindowBgAlpha(1);
			if (ImGui::Begin(advanced_settings_get_string(_advanced_string_title), &open, window_flags))
			{
				HudSettings();
				if (current_controller_index == _controller_index_0)
				{
					VideoSettings();
					MouseKeyboardSettings();
				}
				ControllerSettings();
				if (current_controller_index == _controller_index_0)
				{
					HostSettings();
					GameSettings();
				}

#ifndef NDEBUG
				if (current_controller_index == _controller_index_0)
				{
					if (ImGui::CollapsingHeader("Dev Testing"))
					{
						s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
						char integer_string[16];

						ImGui::Indent();
						if (ImGui::CollapsingHeader("Raster Layers")) {
							ImGui::Columns(4, NULL, false);
							for (auto i = 0; i < 25; i++)
							{
								if (ImGui::Checkbox(_itoa(i, integer_string, 10), &ras_layer_overrides[i]))
								{
									rasterizer_globals->reset_screen = true;
								}
								ImGui::NextColumn();
							}
							ImGui::Columns(1);
						}
						if (ImGui::CollapsingHeader("Render Geometries"))
						{
							ImGui::Columns(4, NULL, false);
							for (auto i = 0; i < 24; i++)
							{
								ImGui::Checkbox(_itoa(i, integer_string, 10), &geo_render_overrides[i]);
								ImGui::NextColumn();
							}
							ImGui::Columns(1);
						}
						static int event_type = H2Config_forced_event;
						if (ImGui::CollapsingHeader("Events"))
						{
							if (ImGui::RadioButton("None", &event_type, _special_event_none))
							{
								H2Config_forced_event = _special_event_none;
							} ImGui::SameLine();
							if (ImGui::RadioButton("Christmas", &event_type, _special_event_christmas))
							{
								H2Config_forced_event = _special_event_christmas;
							} ImGui::SameLine();
							if (ImGui::RadioButton("St Paddys", &event_type, _special_event_st_paddys))
							{
								H2Config_forced_event = _special_event_st_paddys;
							} ImGui::SameLine();
							if (ImGui::RadioButton("Mook Madness", &event_type, _special_event_mook_maddness))
							{
								H2Config_forced_event = _special_event_mook_maddness;
							}

							if (ImGui::RadioButton("Halloween", &event_type, _special_event_halloween))
							{
								H2Config_forced_event = _special_event_halloween;
							}ImGui::SameLine();
							if (ImGui::RadioButton("Birthday", &event_type, _special_event_birthday))
							{
								H2Config_forced_event = _special_event_birthday;
							}

						}
						if (ImGui::CollapsingHeader("WGIT Testing"))
						{
							if (ImGui::Button("Custom Languages"))
							{
								//GSCustomMenuCall_Language();
							}
						}
					}
				}
#endif
			}

			ImGui::PopStyleVar();
			ImGui::End();

			if (!open)
			{
				ImGuiHandler::ToggleWindow(k_advanced_settings_window_name);
			}
		}

		void set_controller_index(e_controller_index controller_index)
		{
			if (cartographer_player_profile_is_signed_in(controller_index))
			{
				current_controller_index = controller_index;
				current_cartographer_profile = cartographer_player_profile_get_by_controller_index(controller_index);
			}
			else
			{
				current_controller_index = k_no_controller;
				current_cartographer_profile = nullptr;
			}
		}

		void Open()
		{
			if (current_controller_index == k_no_controller)
			{
				ImGuiHandler::ToggleWindow(k_advanced_settings_window_name);
				return;
			}

			uint16 Buttons[14];
			current_cartographer_profile->custom_layout.ToArray(Buttons);
			for (auto i = 0; i < 14; i++)
			{
				for (auto j = 0; j < 14; j++)
				{
					if (k_button_values[j] == Buttons[i])
						g_button_placeholders[i] = j;
				}
			}
		}
		void Close()
		{
			g_init = false;
			if (cartographer_player_profile_is_signed_in(current_controller_index))
			{
				cartographer_player_profile_save(current_controller_index);

				if(current_controller_index == _controller_index_0)
					SaveH2Config();
			}
		}
	}
}

/* private code */

const char* advanced_settings_get_string(e_advanced_string string, const char* id)
{
	const e_language language = get_current_language();
	const char* result = k_advanced_settings_string_table[language][string];

	if (id != NULL)
	{
		g_advanced_settings_temp_string.set(result);
		g_advanced_settings_temp_string.append("##");
		g_advanced_settings_temp_string.append(id);
		result = g_advanced_settings_temp_string.get_string();
	}

	return result;
}
