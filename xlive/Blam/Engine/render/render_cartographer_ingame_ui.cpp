#include "stdafx.h"
#include "render_cartographer_ingame_ui.h"

#include "Blam/Engine/game/game.h"
#include "Blam/Engine/rasterizer/rasterizer_globals.h"
#include "Blam/Engine/shell/shell_windows.h"
#include "Blam/Engine/text/draw_string.h"
#include "Blam/Engine/text/font_cache.h"
#include "Blam/Engine/text/unicode.h"

#include "H2MOD/Modules/Accounts/AccountLogin.h"
#include "H2MOD/Modules/Achievements/Achievements.h"
#include "H2MOD/Modules/Shell/Config.h"

// define this to enable queueing a test message in render_cartographer_achievements
// #define ACHIVEMENT_RENDER_DEBUG_ENABLED

const int32 k_status_text_font = 0;

// defined in XLiveRendering.cpp
extern char* buildText;

void render_cartographer_status_text()
{
	rectangle2d bounds;
	draw_string_reset();

	rasterizer_get_frame_bounds(&bounds);

	bool game_is_main_menu = game_is_ui_shell();
	bool paused_or_in_menus = *Memory::GetAddress<bool*>(0x47A568) != 0;

	wchar_t build_string_buffer[256];
	utf8_string_to_wchar_string(buildText, build_string_buffer, 256);

	wchar_t master_state_string_buffer[256];
	utf8_string_to_wchar_string(GetMasterStateStr(), master_state_string_buffer, 256);

	real_argb_color text_color_console = **Memory::GetAddress<real_argb_color**>(0x41277C);
	if (game_is_main_menu)
	{
		text_color_console.alpha = .5f;
	}

	int32 line_height = get_text_size_from_font_cache(k_status_text_font);

	if (game_is_main_menu || (!game_is_main_menu && paused_or_in_menus)) {
		draw_string_set_font(k_status_text_font);
		draw_string_set_color(&text_color_console);
		draw_string_set_shadow_color(*Memory::GetAddress<real_argb_color**>(0x412780));

		if (H2Config_anti_cheat_enabled)
		{
			_draw_string_set_style_justification_flags_wrap_horizontally(0, 1, 0, false);
			draw_string_render(&bounds, L"Anti-cheat is enabled");
			_draw_string_set_style_justification_flags_wrap_horizontally(0, 0, 0, false);
		}

		draw_string_render(&bounds, build_string_buffer);
		bounds.top += line_height;
		draw_string_render(&bounds, master_state_string_buffer);
	}
}

const uint64 k_cheevo_display_lifetime = (5 * k_shell_time_msec_denominator);
const uint32 k_cheevo_title_font = 10;
const uint32 k_cheevo_message_font = 1;

// there should be a better way of doing this. right now,
// this works on frame-by-frame logic (text conversion, ugly STL map)
// but you could easily hook up XAchievements to enqueue achievement
// toast notifications.
void render_cartographer_achievements()
{
	static int64 x_cheevo_timer = 0;
	int64 time_now = shell_time_now_msec();

#ifdef ACHIVEMENT_RENDER_DEBUG_ENABLED
	static bool x_test = false;
	if (x_test)
	{
		AchievementMap.insert({ "Why are we still here? Just to suffer?|I can feel my leg, my arm... even my fingers.", false });
		x_test = false;
	}
#endif

	if (!AchievementMap.empty())
	{
		// hover over this type to die instantly
		auto it = AchievementMap.begin();
		if (it->second == false)
		{
			it->second = true;
			x_cheevo_timer = time_now + k_cheevo_display_lifetime;
		}

		if (x_cheevo_timer - time_now < 0)
		{
			AchievementMap.erase(it);
		}
		else
		{
			rectangle2d bounds;
			wchar_t cheevo_message[256];
			int32 widget_total_height = get_text_size_from_font_cache(k_cheevo_title_font) + (get_text_size_from_font_cache(k_cheevo_message_font) * 2);
			real_argb_color text_color = **Memory::GetAddress<real_argb_color**>(0x41277C);
			text_color.alpha = (float)(x_cheevo_timer - time_now) / k_cheevo_display_lifetime;

			utf8_string_to_wchar_string(it->first.c_str(), cheevo_message, 256);
			wchar_t* divider_position = wcschr(cheevo_message, '|');
			if (divider_position)
			{
				*divider_position = '\0';
			}

			draw_string_reset();
			draw_string_set_font(k_cheevo_title_font);
			_draw_string_set_style_justification_flags_wrap_horizontally(0, 2, 0, false);
			draw_string_set_color(&text_color);
			draw_string_set_shadow_color(*Memory::GetAddress<real_argb_color**>(0x412780));

			rasterizer_get_screen_bounds(&bounds);
			bounds.top += rectangle2d_height(&bounds) / 3 - (widget_total_height / 2);
			draw_string_render(&bounds, L"Achievement Unlocked");
			bounds.top += get_text_size_from_font_cache(k_cheevo_title_font);

			draw_string_set_font(k_cheevo_message_font);
			draw_string_render(&bounds, cheevo_message);
			bounds.top += get_text_size_from_font_cache(k_cheevo_message_font);
			draw_string_render(&bounds, divider_position ? divider_position + 1 : L"<invalid achievement description>");
		}
	}
}

// defined in Modules\Updater\Updater.cpp
extern char* autoUpdateText;
extern long long sizeOfDownload;
extern long long sizeOfDownloaded;

void render_cartographer_update()
{
	//if (autoUpdateText)
	//{
	//	rectangle2d bounds;
	//	wchar_t update_message[256];
	//
	//	if (sizeOfDownload > 0)
	//	{
	//		float percent_complate = 100.f * ((float)sizeOfDownloaded / sizeOfDownload);
	//		swprintf_s(update_message, 256, L"%S (%.1f)", autoUpdateText, percent_complate);
	//	}
	//	else
	//	{
	//		utf8_string_to_wchar_string(autoUpdateText, update_message, 256);
	//	}
	//
	//	draw_string_reset();
	//	draw_string_set_font(k_cheevo_title_font);
	//	_draw_string_set_style_justification_flags_wrap_horizontally(0, 2, 0, false);
	//	draw_string_set_shadow_color(*Memory::GetAddress<real_argb_color**>(0x412780));
	//
	//	rasterizer_get_screen_bounds(&bounds);
	//	bounds.top += rectangle2d_height(&bounds) / 4;
	//	draw_string_render(&bounds, update_message);
	//}
}