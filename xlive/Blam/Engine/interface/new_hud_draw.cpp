#include "stdafx.h"
#include "new_hud_draw.h"

#include "hud.h"
#include "hud_definitions.h"
#include "new_hud.h"
#include "new_hud_definitions.h"

#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "camera/camera.h"
#include "game/players.h"
#include "rasterizer/dx9/rasterizer_dx9_shader_submit_new.h"
#include "render/render.h"
#include "saved_games/cartographer_player_profile.h"
#include "text/draw_string.h"
#include "text/unicode.h"

/* globals */

static const pixel32 g_draw_hud_bitmap_widget_shield_pixel_colors[9]
{
	D3DCOLOR_ARGB(0, 0, 0, 0),
	D3DCOLOR_ARGB(0, 255, 0, 0),
	D3DCOLOR_ARGB(0, 0, 255, 0),
	D3DCOLOR_ARGB(0, 255, 255, 0),
	D3DCOLOR_ARGB(0, 127, 0, 255),
	D3DCOLOR_ARGB(0, 69, 5, 154),
	D3DCOLOR_ARGB(0, 156, 70, 193),
	D3DCOLOR_ARGB(0, 00, 85, 170),
	D3DCOLOR_ARGB(0, 0,120,240),
};

// storage for bitmaps that contain crosshairs
uint32 g_draw_hud_crosshair_bitmap_cache_count;
datum* g_draw_hud_crosshair_bitmap_cache;

// k_number_of_users, we have 4 bits left to spare in this value
uint8 g_draw_hud_user_draw_player_indicators_mask;

/* private code */

s_draw_hud_widget_input_results* global_hud_draw_widget_function_results_get()
{
	return Memory::GetAddress<s_draw_hud_widget_input_results*>(0x976680);
}

real_rgb_color* global_hud_draw_widget_special_hud_type_color_primary_get()
{
	return Memory::GetAddress<real_rgb_color*>(0x976690);
}
real_rgb_color* global_hud_draw_widget_special_hud_type_secondary_color_get()
{
	return Memory::GetAddress<real_rgb_color*>(0x97669C);
}
real_rgb_color* global_hud_draw_widget_special_hud_type_tertiary_color_get()
{
	return Memory::GetAddress<real_rgb_color*>(0x9766A8);
}

real_rgb_color* global_hud_draw_text_widget_primary_color_get()
{
	return Memory::GetAddress<real_rgb_color*>(0x976650);
}

void draw_hud_get_bitmap_data(uint32 local_render_user_index, s_hud_bitmap_widget_definition* bitmap_widget, real_rectangle2d* bounds, int16* out_bitmap_index, int16* out_width_pixels, int16* out_height_pixels)
{
	ASSERT(bitmap_widget);
	ASSERT(bounds);
	ASSERT(out_bitmap_index);
	ASSERT(out_width_pixels);
	ASSERT(out_height_pixels);

	// If returned bitmap index is NONE the draw call will exit early.
	*out_bitmap_index = NONE;

	int32 sequence_index = 0;

	switch (get_screen_split_type(local_render_user_index))
	{
	case _screen_split_type_full:
		sequence_index = bitmap_widget->fullscreen_sequence_index;
		break;
	case _screen_split_type_half:
		sequence_index = bitmap_widget->halfscreen_sequence_index;
		break;
	case _screen_split_type_quarter:
		sequence_index = bitmap_widget->quarterscreen_sequence_index;
		break;
	default:
		sequence_index = bitmap_widget->fullscreen_sequence_index;
	}

	if (bitmap_widget->shader.index == NONE || bitmap_widget->bitmap.index == NONE || sequence_index < 0)
		return;

	bitmap_group* bitmap = (bitmap_group*)tag_get_fast(bitmap_widget->bitmap.index);

	if (sequence_index >= bitmap->sequences.count)
	{
		if (sequence_index || bitmap->sequences.count)
			return;

		bitmap_data* bitmap_data = bitmap->bitmaps[0];

		*out_width_pixels = bitmap_data->width;
		*out_height_pixels = bitmap_data->height;
		*out_bitmap_index = 0;

		bounds->y1 = 1.f;
		bounds->x1 = 1.f;
		bounds->y0 = 0;
		bounds->x0 = 0;

		return;
	}

	bitmap_group_sequence* bitmap_sequence = bitmap->sequences[sequence_index];

	if (bitmap_sequence->sprites.count <= 0)
	{
		if (bitmap_sequence->bitmap_count <= 0)
			return;

		int16 bitmap_index = bitmap_sequence->first_bitmap_index;
		if (bitmap_sequence->bitmap_count > 1 && player_user_is_elite_or_dervish(local_render_user_index))
			bitmap_index += 1;

		*out_width_pixels = bitmap->bitmaps[bitmap_index]->width;
		*out_height_pixels = bitmap->bitmaps[bitmap_index]->height;
		*out_bitmap_index = bitmap_index;
		bounds->y1 = 1.f;
		bounds->x1 = 1.f;
		bounds->y0 = 0;
		bounds->x0 = 0;

		return;
	}

	int8 sprite_index = 0;
	if (bitmap_sequence->sprites.count > 1 && player_user_is_elite_or_dervish(local_render_user_index))
		sprite_index = 1;

	bitmap_group_sprite* bitmap_sprite = bitmap_sequence->sprites[sprite_index];
	bitmap_data* bitmap_data = bitmap->bitmaps[bitmap_sprite->bitmap_index];

	*out_width_pixels = bitmap_data->width;
	*out_height_pixels = bitmap_data->height;
	*out_bitmap_index = bitmap_sprite->bitmap_index;
	bounds->y0 = bitmap_sprite->bounds.y0;
	bounds->y1 = bitmap_sprite->bounds.y1;
	bounds->x0 = bitmap_sprite->bounds.x0;
	bounds->x1 = bitmap_sprite->bounds.x1;
}

real32 __cdecl draw_hud_widget_get_value(uint32 unused, string_id input_name)
{
	return INVOKE(0x22211A, 0, draw_hud_widget_get_value, unused, input_name);
}

void hud_widget_effect_evaluate(uint32 local_render_user_index, s_new_hud_temporary_user_state* user_state, s_hud_widget_effect_definition* widget_effect, real_point2d* out_offset, real_point2d* out_scale, real32* out_theta)
{
	if (out_theta && widget_effect->flags.test(hud_widget_effect_flag_apply_theta))
	{
		const real32 theta_value = draw_hud_widget_get_value(NONE, widget_effect->theta.input_name);
		const real32 theta_result = widget_effect->theta.function.evaluate(theta_value, 1.f);
		*out_theta += widget_effect->theta.function.unknown_post_evaluate_function(theta_result);
	}
	if (out_offset && widget_effect->flags.test(hud_widget_effect_flag_apply_offset))
	{
		ASSERT(out_scale);

		const real32 horizontal_value = draw_hud_widget_get_value(NONE, widget_effect->horizontal_offset.input_name);
		const real32 vertical_value = draw_hud_widget_get_value(NONE, widget_effect->vertical_offset.input_name);

		const real32 horizontal_result = widget_effect->horizontal_offset.function.evaluate(horizontal_value, 1.f);
		const real32 vertical_result = widget_effect->vertical_offset.function.evaluate(vertical_value, 1.f);

		out_offset->x += out_scale->x * widget_effect->horizontal_offset.function.unknown_post_evaluate_function(horizontal_result);
		out_offset->y += out_scale->y * widget_effect->vertical_offset.function.unknown_post_evaluate_function(vertical_result);
	}
	if (out_scale && widget_effect->flags.test(hud_widget_effect_flag_apply_scale))
	{
		const real32 horizontal_value = draw_hud_widget_get_value(NONE, widget_effect->horizontal_scale.input_name);
		const real32 vertical_value = draw_hud_widget_get_value(NONE, widget_effect->vertical_scale.input_name);

		const real32 horizontal_result = widget_effect->horizontal_scale.function.evaluate(horizontal_value, 1.f);
		const real32 vertical_result = widget_effect->vertical_scale.function.evaluate(vertical_value, 1.f);

		out_scale->x *= widget_effect->horizontal_scale.function.unknown_post_evaluate_function(horizontal_result);
		out_scale->y *= widget_effect->vertical_scale.function.unknown_post_evaluate_function(vertical_result);
	}
}

void hud_widget_anchor_calculate_point(e_hud_anchor anchor, real_point2d* out_point)
{
	INVOKE(0x223969, 0, hud_widget_anchor_calculate_point, anchor, out_point);
	return;
}

bool draw_hud_bitmap_is_crosshair(datum bitmap_datum)
{
	for(uint32 i = 0; i < g_draw_hud_crosshair_bitmap_cache_count; i++)
		if (g_draw_hud_crosshair_bitmap_cache[i] == bitmap_datum)
			return true;

	return false;
}

void __cdecl draw_hud_bitmap_widget(int32 local_render_user_index, s_new_hud_temporary_user_state* user_state, s_hud_bitmap_widget_definition* bitmap_widget, real32* widget_function_results)
{
	if (bitmap_widget->bitmap.index == NONE || bitmap_widget->shader.index == NONE)
		return;

	real_rectangle2d bitmap_bounds{};
	int16 bitmap_index = NONE;
	int16 bitmap_width = 0;
	int16 bitmap_height = 0;

	draw_hud_get_bitmap_data(local_render_user_index, bitmap_widget, &bitmap_bounds, &bitmap_index, &bitmap_width, &bitmap_height);

	if (bitmap_index == NONE)
		return;

	s_draw_hud_widget_input_results* hud_input_results = global_hud_draw_widget_function_results_get();

	memcpy(hud_input_results, widget_function_results, sizeof(real32) * 4);

	real_point2d offset_result{ 0, 0 };
	real_point2d scale_result{ 1.f, 1.f };
	real32 theta_result = 0;

	if (bitmap_widget->effect.count > 0)
		hud_widget_effect_evaluate(local_render_user_index, user_state, bitmap_widget->effect[0], &offset_result, &scale_result, &theta_result);

	bitmap_width = (int16)(bitmap_width * scale_result.x);
	bitmap_height = (int16)(bitmap_height * scale_result.y);

	real32 hud_scale = *get_primary_hud_scale();

	if (bitmap_widget->anchor == _hud_anchor_crosshair)
	{
		if(draw_hud_bitmap_is_crosshair(bitmap_widget->bitmap.index))
		{
			s_saved_game_cartographer_player_profile* profile_settings = cartographer_player_profile_get_by_user_index(local_render_user_index);
			hud_scale = *get_secondary_hud_scale() * profile_settings->crosshair_scale;
		}
		else
			hud_scale = *get_secondary_hud_scale();
	}

	real_point2d anchor_point{};

	hud_widget_anchor_calculate_point(bitmap_widget->anchor, &anchor_point);

	point2d screen_offset;
	real_point2d registration_point;

	switch (get_screen_split_type(local_render_user_index))
	{
	case _screen_split_type_half:
		screen_offset = bitmap_widget->halfscreen_offset;
		registration_point = bitmap_widget->halfscreen_registration_point;
		break;
	case _screen_split_type_quarter:
		screen_offset = bitmap_widget->quarterscreen_offset;
		registration_point = bitmap_widget->quarterscreen_registration_point;
		break;
	default:
	case _screen_split_type_full:
		screen_offset = bitmap_widget->fullscreen_offset;
		registration_point = bitmap_widget->fullscreen_registration_point;
		break;
	}

	real32 calc_registration_x = ((bitmap_bounds.x1 - bitmap_bounds.x0) * registration_point.x) * (real32)bitmap_width;
	real32 calc_offset_x = (real32)screen_offset.x + offset_result.x;

	real32 calc_registration_y = ((bitmap_bounds.y1 - bitmap_bounds.y0) * registration_point.y) * (real32)bitmap_height;
	real32 calc_offset_y = (real32)screen_offset.y + offset_result.y;

	real32 final_point_x = (((calc_offset_x - calc_registration_x) + bitmap_bounds.x0) * hud_scale) + anchor_point.x;
	real32 final_point_y = (((calc_offset_y - calc_registration_y) + bitmap_bounds.y0) * hud_scale) + anchor_point.y;

	real_point2d final_location{ final_point_x, final_point_y };
	real_point2d bitmap_size{ (real32)bitmap_width, (real32)bitmap_height };

	if (bitmap_widget->flags.test(bitmap_widget_flag_flip_horizontally))
	{
		real32 bounds_left = bitmap_bounds.x0;
		bitmap_bounds.x0 = bitmap_bounds.x1;
		bitmap_bounds.x1 = bounds_left;
	}

	if (bitmap_widget->flags.test(bitmap_widget_flag_flip_vertically))
	{
		real32 bounds_top = bitmap_bounds.y0;
		bitmap_bounds.y0 = bitmap_bounds.y1;
		bitmap_bounds.y1 = bounds_top;
	}

	bool special_draw_case = false;

	switch (bitmap_widget->special_hud_type)
	{
	case special_hud_type_sb_player_emblem:
		new_hud_engine_globals_set_drawing_player_index(user_state->local_player_datum);
		*global_hud_draw_widget_special_hud_type_color_primary_get() = user_state->local_player_color;
		break;
	case special_hud_type_sb_other_player_emblem:
		new_hud_engine_globals_set_drawing_player_index(user_state->other_player_datum);
		*global_hud_draw_widget_special_hud_type_color_primary_get() = user_state->other_player_color;
		break;
	case special_hud_type_sb_player_score_meter:
		*global_hud_draw_widget_special_hud_type_color_primary_get() = user_state->local_player_color;
		*global_hud_draw_widget_special_hud_type_secondary_color_get() = user_state->local_player_color;
		break;
	case special_hud_type_sb_other_player_score_meter:
		*global_hud_draw_widget_special_hud_type_color_primary_get() = user_state->other_player_color;
		*global_hud_draw_widget_special_hud_type_secondary_color_get() = user_state->other_player_color;
		break;
	case special_hud_type_unit_shield_meter:
		{
			special_draw_case = true;

			s_new_hud_globals_player_info* player_info = new_hud_engine_globals_get_player_data(local_render_user_index);
			for (int32 shield_layer_level = 0; shield_layer_level < 5; shield_layer_level++)
			{
				real32 shield_vitality = user_state->unit_current_shield_vitality - (real32)shield_layer_level;
				shield_vitality = PIN(shield_vitality, 0.0f, 1.0f);

				real32 player_unk_0 = player_info->field_0 - (real32)shield_layer_level;
				player_unk_0 = PIN(player_unk_0, 0.0f, 1.0f);

				bool shield_damaged = false;
				real32 shield_damage_color_intensity;

				if (player_unk_0 <= shield_vitality)
				{
					shield_damaged = false;
					shield_damage_color_intensity = 0.0f;
				}
				else
				{
					shield_damaged = true;

					if (player_info->field_4 >= 0.0f)
					{
						shield_damage_color_intensity = PIN(1.0f - player_info->field_4, 0.0f, 1.0f);
					}
					else
					{
						shield_damage_color_intensity = 0.0f;
					}
				}

				real_rgb_color shield_meter_damage_color = *global_real_rgb_white;
				shield_meter_damage_color.red *= shield_damage_color_intensity;
				shield_meter_damage_color.green *= shield_damage_color_intensity;
				shield_meter_damage_color.blue *= shield_damage_color_intensity;

				if (!shield_damaged)
					player_unk_0 = shield_vitality;

				if (player_unk_0 <= 0.0f && shield_vitality <= 0.0f)
					break;

				global_hud_draw_widget_function_results_get()->result_1 = player_unk_0;
				global_hud_draw_widget_function_results_get()->result_2 = shield_vitality;
				*global_hud_draw_widget_special_hud_type_color_primary_get() = shield_meter_damage_color;

				if (shield_layer_level != 0)
				{
					pixel32_to_real_rgb_color(g_draw_hud_bitmap_widget_shield_pixel_colors[shield_layer_level], global_hud_draw_widget_special_hud_type_secondary_color_get());
					pixel32_to_real_rgb_color(g_draw_hud_bitmap_widget_shield_pixel_colors[shield_layer_level], global_hud_draw_widget_special_hud_type_tertiary_color_get());
				}
				else if (player_user_is_elite_or_dervish(local_render_user_index))
				{
					pixel32_to_real_rgb_color(g_draw_hud_bitmap_widget_shield_pixel_colors[5], global_hud_draw_widget_special_hud_type_secondary_color_get());
					pixel32_to_real_rgb_color(g_draw_hud_bitmap_widget_shield_pixel_colors[6], global_hud_draw_widget_special_hud_type_tertiary_color_get());
				}
				else
				{
					pixel32_to_real_rgb_color(g_draw_hud_bitmap_widget_shield_pixel_colors[7], global_hud_draw_widget_special_hud_type_secondary_color_get());
					pixel32_to_real_rgb_color(g_draw_hud_bitmap_widget_shield_pixel_colors[8], global_hud_draw_widget_special_hud_type_tertiary_color_get());
				}

				render_ingame_user_interface_hud_element_hook(
					final_location.x,
					final_location.y,
					bitmap_width,
					bitmap_height,
					hud_scale,
					theta_result,
					bitmap_widget->bitmap.index,
					bitmap_index,
					&bitmap_bounds,
					bitmap_widget->shader.index);
			}
			break;
		}
	case special_hud_type_territory_meter:
		{
			special_draw_case = true;

			if (user_state->territories_count == 0)
				return;

			*global_hud_draw_widget_special_hud_type_secondary_color_get() = *global_real_rgb_white;

			real32 distance_per_territory = ((float)bitmap_width + 1) * hud_scale;
			real32 base_location = final_location.x - ((user_state->territories_count - 1) * (distance_per_territory * 0.5f));

			for (uint32 index = 0; index < user_state->territories_count; ++index)
			{
				pixel32 territory_color = user_state->territory_pixel_color[index];
				pixel32_to_real_rgb_color(territory_color, global_hud_draw_widget_special_hud_type_color_primary_get());

				real32 territory_progress = user_state->territory_control_progress[index];
				global_hud_draw_widget_function_results_get()->result_1 = territory_progress;

				real32 adjusted_location_x = base_location + distance_per_territory * index;

				render_ingame_user_interface_hud_element_hook(
					adjusted_location_x,
					final_location.y,
					bitmap_width,
					bitmap_height,
					hud_scale,
					theta_result,
					bitmap_widget->bitmap.index,
					bitmap_index,
					&bitmap_bounds,
					bitmap_widget->shader.index);
			}

			break;
		}
	}


	if (!special_draw_case)
	{
		if (!bitmap_widget->flags.test(bitmap_widget_flag_scope_stretch)) {
			render_ingame_user_interface_hud_element_hook(
				final_location.x,
				final_location.y,
				bitmap_width,
				bitmap_height,
				hud_scale,
				theta_result,
				bitmap_widget->bitmap.index,
				bitmap_index,
				&bitmap_bounds,
				bitmap_widget->shader.index);
		}
		else
		{
			final_location.x = final_location.x - ((bitmap_size.x * hud_scale) * 9.f);
			final_location.y = final_location.y - ((bitmap_size.y * hud_scale) * 9.f);

			bitmap_bounds.y0 = bitmap_bounds.y0 - ((bitmap_bounds.y1 - bitmap_bounds.y0) * 9.f);
			bitmap_bounds.x0 = bitmap_bounds.x0 - ((bitmap_bounds.x1 - bitmap_bounds.x0) * 9.f);

			hud_scale = hud_scale * 10;

			render_ingame_user_interface_hud_element_hook(
				final_location.x,
				final_location.y,
				(int16)bitmap_size.x,
				(int16)bitmap_size.y,
				hud_scale,
				theta_result,
				bitmap_widget->bitmap.index,
				bitmap_index,
				&bitmap_bounds,
				bitmap_widget->shader.index);
		}
		if (bitmap_widget->flags.test(bitmap_widget_flag_scope_mirror_horizontally))
		{
			real_rectangle2d flipped_bounds{};
			flipped_bounds.y0 = bitmap_bounds.y0;
			flipped_bounds.y1 = bitmap_bounds.y1;
			flipped_bounds.x0 = bitmap_bounds.x1;
			flipped_bounds.x1 = bitmap_bounds.x0;

			render_ingame_user_interface_hud_element_hook(
				(bitmap_size.x * hud_scale) + final_location.x,
				final_location.y,
				(int16)bitmap_size.x,
				(int16)bitmap_size.y,
				hud_scale,
				theta_result,
				bitmap_widget->bitmap.index,
				bitmap_index,
				&flipped_bounds,
				bitmap_widget->shader.index);
		}
		if (bitmap_widget->flags.test(bitmap_widget_flag_scope_mirror_vertically))
		{
			real_rectangle2d flipped_bounds{};

			flipped_bounds.y0 = bitmap_bounds.y1;
			flipped_bounds.y1 = bitmap_bounds.y0;
			flipped_bounds.x0 = bitmap_bounds.x0;
			flipped_bounds.x1 = bitmap_bounds.x1;


			render_ingame_user_interface_hud_element_hook(
				final_location.x,
				bitmap_size.y * hud_scale + final_location.y,
				(int16)bitmap_size.x,
				(int16)bitmap_size.y,
				hud_scale,
				theta_result,
				bitmap_widget->bitmap.index,
				bitmap_index,
				&flipped_bounds,
				bitmap_widget->shader.index);
		}
		if (bitmap_widget->flags.test(bitmap_widget_flag_scope_mirror_horizontally) &&
			bitmap_widget->flags.test(bitmap_widget_flag_scope_mirror_vertically))
		{
			real_rectangle2d flipped_bounds{};
			flipped_bounds.y0 = bitmap_bounds.y1;
			flipped_bounds.y1 = bitmap_bounds.y0;
			flipped_bounds.x0 = bitmap_bounds.x1;
			flipped_bounds.x1 = bitmap_bounds.x0;
			render_ingame_user_interface_hud_element_hook(
				(bitmap_size.x * hud_scale) + final_location.x,
				bitmap_size.y * hud_scale + final_location.y,
				(int16)bitmap_size.x,
				(int16)bitmap_size.y,
				hud_scale,
				theta_result,
				bitmap_widget->bitmap.index,
				bitmap_index,
				&flipped_bounds,
				bitmap_widget->shader.index);
		}
	}
}

uint32 draw_hud_text_get_split_screen_font_type(e_hud_anchor anchor)
{
	return INVOKE(0x224030, 0, draw_hud_text_get_split_screen_font_type, anchor);
}

void __cdecl draw_hud_resolve_string_id_to_value(string_id string, wchar_t* out_string)
{
	INVOKE(0x224B26, 0, draw_hud_resolve_string_id_to_value, string, out_string);
}

void __cdecl draw_hud_fixup_private_characters(wchar_t* string)
{
	INVOKE(0x2309D6, 0, draw_hud_fixup_private_characters, string);
}

int32 draw_hud_get_draw_string_font_index(int32 font_index)
{
	if (!font_index)
		return 6;
	if (font_index == 1)
		return 8;
	return NONE;
}

void draw_hud_text_get_string(s_draw_hud_widget_input_results* widget_function_results, s_hud_text_widget_definition* text_widget, s_new_hud_temporary_user_state* user_state, wchar_t* out_string, int32* out_font)
{
	s_draw_hud_widget_input_results* hud_input_results = global_hud_draw_widget_function_results_get();
	memcpy(hud_input_results, widget_function_results, sizeof(int32) * 4);

	if(text_widget->flags.test(text_widget_flag_string_is_a_number))
	{
		int32 value = (int32)draw_hud_widget_get_value(-1, text_widget->string);
		if (text_widget->flags.test(text_widget_flag_force_2digit_number))
		{
			if (value < 0)
			{
				usnzprintf(out_string, 512, L"0%d", 0);
			}
			else if (value > 99)
			{
				usnzprintf(out_string, 512, L"%d", 99);
			}
			else if (value < 10)
			{
				usnzprintf(out_string, 512, L"0%d", value);
			}
			else
			{
				usnzprintf(out_string, 512, L"%d", value);
			}
		}
		else if (text_widget->flags.test(text_widget_flag_force_3digit_number))
		{
			if (value < 0)
			{
				usnzprintf(out_string, 512, L"00%d", 0);
			}
			else if (value > 999)
			{
				usnzprintf(out_string, 512, L"%d", 999);
			}
			else if (value < 10)
			{
				usnzprintf(out_string, 512, L"00%d", value);
			}
			else if (value < 100)
			{
				usnzprintf(out_string, 512, L"0%d", value);
			}
			else
			{
				usnzprintf(out_string, 512, L"%d", value);
			}
		}
		else
		{
			usnzprintf(out_string, 512, L"%d", value);
		}
	}
	else if(text_widget->flags.test(text_widget_flag_talking_player_hack))
	{
		if(user_state->player_talking && user_state->player_index != NONE)
		{
			s_player* player = (s_player*)datum_get(s_player::get_data(), user_state->player_index);
			usnzprintf(out_string, 512, L"%s", player->properties[0].player_name);
		}
	}
	else
	{
		draw_hud_resolve_string_id_to_value(text_widget->string, out_string);
		draw_hud_fixup_private_characters(out_string);
	}

	*out_font = font_index_invalid;
	uint32 split_screen_font_type = draw_hud_text_get_split_screen_font_type(text_widget->anchor);
	switch(split_screen_font_type)
	{
	case 1:
		*out_font = draw_hud_get_draw_string_font_index(text_widget->halfscreen_font_index);
		break;
	case 2:
		*out_font = draw_hud_get_draw_string_font_index(text_widget->quarterscreen_font_index);
		break;
	default:
		*out_font = draw_hud_get_draw_string_font_index(text_widget->fullscreen_font_index);
		break;
	}
}


void __cdecl draw_hud_text_widget(uint32 local_render_user_index, s_new_hud_temporary_user_state* user_state, s_hud_text_widget_definition* text_widget, s_draw_hud_widget_input_results* widget_function_results)
{
	if (!text_widget->string != 0 || text_widget->shader.index == NONE)
		return;

	wchar_t widget_string[512]{};
	int32 draw_string_font_index = NONE;

	draw_hud_text_get_string(widget_function_results, text_widget, user_state, widget_string, &draw_string_font_index);
	draw_string_set_draw_mode(draw_string_font_index, NONE, 0,0, global_real_argb_white, global_real_argb_black, false);
	if(draw_string_set_string(widget_string))
	{
		rasterizer_flags_unknown_function_1();
		rasterizer_flags_unknown_function_2(0);
		rasterizer_flags_unknown_function_2(rasterizer_unknown_shader_submit_unknown_value_get());
		pixel32_to_real_rgb_color(get_hud_globals()->help_text_default_color, global_hud_draw_text_widget_primary_color_get());
		rasterizer_shader_submit(text_widget->shader.index, 0, 1, 0, 0, 100.f);
		draw_string_set_position(0, 0);

		real_point2d offset_result {0,0};
		real_point2d scale_result {1.f,1.f};

		if(text_widget->effect.count > 0)
			hud_widget_effect_evaluate(local_render_user_index, user_state, text_widget->effect[0], &offset_result, &scale_result, nullptr);

		real_point2d anchor_point;

		hud_widget_anchor_calculate_point(text_widget->anchor, &anchor_point);

		point2d screen_offset;

		switch (get_screen_split_type(local_render_user_index))
		{
		case _screen_split_type_half:
			screen_offset = text_widget->halfscreen_offset;
			break;
		case _screen_split_type_quarter:
			screen_offset = text_widget->quarterscreen_offset;
			break;
		default:
		case _screen_split_type_full:
			screen_offset = text_widget->fullscreen_offset;
			break;
		}

		real32 final_location_x = (((screen_offset.x + offset_result.x) * *get_primary_hud_scale()) * scale_result.x + anchor_point.x) - get_global_camera()->viewport_bounds.left;
		real32 final_location_y = (((screen_offset.y + offset_result.y) * *get_primary_hud_scale()) * scale_result.y + anchor_point.y) - get_global_camera()->viewport_bounds.top;

		real_point2d final_location{ final_location_x, final_location_y };

		rectangle2d text_bounds{ 0, 0, 1000, 1000 };
		rectangle2d draw_string_bounds;
		int32 draw_string_unk_2;
		draw_string_calculate_bounds(&text_bounds, widget_string, &draw_string_bounds, &draw_string_unk_2, 1.f);

		int32 text_width = rectangle2d_width(&draw_string_bounds);
		real32 calc_text_width = (text_width * *get_primary_hud_scale()) * scale_result.x;

		int32 text_height = rectangle2d_height(&draw_string_bounds);
		real32 calc_text_height = (text_height * *get_primary_hud_scale()) * scale_result.y;

		int32 ceil_text_width = (int32)ceil(calc_text_width);
		int32 ceil_text_height = (int32)ceil(calc_text_height);

		draw_string_set_player_color(global_real_argb_white);
		draw_string_set_shadow_color(global_real_argb_black);

		switch(text_widget->justification)
		{
		case text_justification_center:
			text_bounds.left = (int16)(final_location.x - (real32)(ceil_text_width >> 1));
			text_bounds.top = (int16)final_location.y;
			text_bounds.right = (int16)(final_location.x + (real32)(ceil_text_width - (ceil_text_width >> 1)));
			text_bounds.bottom = (int16)(ceil_text_height + final_location.y);
			break;
		case text_justification_right:
			text_bounds.left = (int16)(final_location.x - ceil_text_width);
			text_bounds.top = (int16)final_location.y;
			text_bounds.right = (int16)final_location.x;
			text_bounds.bottom = (int16)(final_location_y + ceil_text_height);
			break;
		default:
			text_bounds.left = (int16)final_location.x;
			text_bounds.top = (int16)final_location.y;
			text_bounds.right = (int16)(ceil_text_width + final_location.x);
			text_bounds.bottom = (int16)(ceil_text_height + final_location.y);
			break;
		}

		draw_string_draw(&text_bounds, widget_string, *get_primary_hud_scale());
	}
}

void __cdecl draw_hud_player_indicators(uint32 local_render_user_index)
{
	typedef void(__cdecl* game_mode_engine_draw_team_indicators_t)(int);
	auto p_game_mode_engine_draw_team_indicators = Memory::GetAddress<game_mode_engine_draw_team_indicators_t>(0x6AFA4);

	if (TEST_BIT(g_draw_hud_user_draw_player_indicators_mask, local_render_user_index))
		p_game_mode_engine_draw_team_indicators(local_render_user_index);
}
/* public code */

void hud_draw_on_map_load()
{
	if (g_draw_hud_crosshair_bitmap_cache)
		free(g_draw_hud_crosshair_bitmap_cache);

	g_draw_hud_crosshair_bitmap_cache_count = 0;

	// initial loop to find how many bitmaps there are
	tag_iterator it;
	tag_iterator_new(&it, _tag_group_bitmap);
	while (tag_iterator_next(&it) != NONE)
	{
		if (strstr(tag_get_name(it.current_tag_index), "new_hud\\crosshairs"))
		{
			g_draw_hud_crosshair_bitmap_cache_count++;
		}
	}

	// allocate a storage for the datums
	g_draw_hud_crosshair_bitmap_cache = (datum*)calloc(g_draw_hud_crosshair_bitmap_cache_count, sizeof(datum));

	// do a second loop to store all the actual datums
	size_t i = 0;
	tag_iterator_new(&it, _tag_group_bitmap);
	while (tag_iterator_next(&it) != NONE)
	{
		if (strstr(tag_get_name(it.current_tag_index), "new_hud\\crosshairs") && i < g_draw_hud_crosshair_bitmap_cache_count)
		{
			g_draw_hud_crosshair_bitmap_cache[i++] = it.current_tag_index;
		}
		// Exit when we've cached all the bitmap datums
		else if (i >= g_draw_hud_crosshair_bitmap_cache_count) {
			break;
		}
	}

	return;
}

datum hud_bitmap_tag_index_get(void)
{
	return *Memory::GetAddress<datum*>(0x976678);
}

int16 hud_bitmap_data_index_get(void)
{
	return *Memory::GetAddress<int16*>(0x97667C);
}

void hud_player_indicators_draw_enabled_set(int32 user_index, bool enabled)
{
	SET_FLAG(g_draw_hud_user_draw_player_indicators_mask, user_index, enabled);
	return;
}

void hud_player_indicators_draw_reset(void)
{
	g_draw_hud_user_draw_player_indicators_mask = FLAG(k_number_of_users) - 1;	// Set all bits representing local users to 1
	return;
}

void __cdecl draw_hud_layer(void)
{
	INVOKE(0x22657B, 0x0, draw_hud_layer);
	return;
}

void new_hud_draw_apply_patches(void)
{
	hud_player_indicators_draw_reset();

	PatchCall(Memory::GetAddress(0x226702), draw_hud_player_indicators);
	PatchCall(Memory::GetAddress(0x224F46), draw_hud_bitmap_widget);
	PatchCall(Memory::GetAddress(0x224FDA), draw_hud_text_widget);
	return;
}
