#include "stdafx.h"
#include "render_cartographer_ingame_ui.h"

#include "Blam/Engine/game/game.h"
#include "Blam/Engine/rasterizer/rasterizer_globals.h"
#include "Blam/Engine/interface/hud.h"
#include "Blam/Engine/shell/shell_windows.h"
#include "Blam/Engine/text/draw_string.h"
#include "Blam/Engine/text/font_cache.h"
#include "Blam/Engine/text/unicode.h"

#include "H2MOD/Modules/Accounts/AccountLogin.h"
#include "H2MOD/Modules/Achievements/Achievements.h"
#include "H2MOD/Modules/Shell/Config.h"

#include "version_git.h"

// define this to enable queueing a test message in render_cartographer_achievements
// #define ACHIVEMENT_RENDER_DEBUG_ENABLED

const int32 k_status_text_font = 0;

// defined in XLiveRendering.cpp
extern char* buildText;

bool cartographer_test_build_draw_text = true;

void render_cartographer_status_text()
{
	rectangle2d bounds;
	rasterizer_get_frame_bounds(&bounds);

	bool game_is_main_menu = game_is_ui_shell();
	bool paused_or_in_menus = *Memory::GetAddress<bool*>(0x47A568) != 0;

	wchar_t build_string_buffer[256];
	utf8_string_to_wchar_string(buildText, build_string_buffer, NUMBEROF(build_string_buffer));

	wchar_t master_state_string_buffer[256];
	utf8_string_to_wchar_string(GetMasterStateStr(), master_state_string_buffer, NUMBEROF(master_state_string_buffer));

	real_argb_color text_color_console = *global_real_argb_white;
	if (game_is_main_menu)
	{
		text_color_console.alpha = .5f;
	}

	int32 line_height = get_text_size_from_font_cache(k_status_text_font);
	bool paused_or_in_main_menu = game_is_main_menu || paused_or_in_menus;
	if (paused_or_in_main_menu) {
		draw_string_reset();
		draw_string_set_font_and_options(k_status_text_font, 0, 0, 0, &text_color_console, global_real_argb_black, false);

		bounds.bottom = bounds.top + line_height;

		if (H2Config_anti_cheat_enabled)
		{
			draw_string_set_options(0, 1, 0, false);
			draw_string_render(&bounds, L"Anti-cheat is enabled");
			draw_string_set_options(0, 0, 0, false);
		}

		draw_string_render(&bounds, build_string_buffer);
		bounds.top += line_height;
		bounds.bottom = bounds.top + line_height;
		draw_string_render(&bounds, master_state_string_buffer);
	}

#if defined(GEN_GIT_VER_VERSION_STRING)
	if (cartographer_test_build_draw_text)
	{
		rasterizer_get_frame_bounds(&bounds);
		int32 test_build_font = 0;
		line_height = get_text_size_from_font_cache(test_build_font);

		text_color_console.alpha = .55f;
		bounds.top += (1050 * *get_ui_scale());
		bounds.left = bounds.right - (765 * *get_ui_scale());
		bounds.bottom = bounds.top + line_height;
		
		draw_string_reset();
		draw_string_set_font_and_options(test_build_font, 1, 0, 0, &text_color_console, global_real_argb_black, false);

		wchar_t build_git_version_string[256];
		wchar_t build_date[256];
		wchar_t build_time[256];
		wchar_t git_username[256];
		wchar_t git_branch_name[256];

		wchar_t result_text_buffer[1024];

		utf8_string_to_wchar_string(GEN_GIT_VER_VERSION_STRING, build_git_version_string, NUMBEROF(build_git_version_string));
		utf8_string_to_wchar_string(__DATE__, build_date, NUMBEROF(build_date));
		utf8_string_to_wchar_string(__TIME__, build_time, NUMBEROF(build_time));
		utf8_string_to_wchar_string(GET_GIT_VER_USERNAME, git_username, NUMBEROF(git_username));
		utf8_string_to_wchar_string(GET_GIT_VER_BRANCH, git_branch_name, NUMBEROF(git_branch_name));

		swprintf(result_text_buffer, NUMBEROF(result_text_buffer), L"%s %s", build_date, build_time);
		draw_string_render(&bounds, result_text_buffer);
		bounds.top += line_height;
		bounds.bottom = bounds.top + line_height;
		swprintf(result_text_buffer, NUMBEROF(result_text_buffer), L"%s %s branch: %s", build_git_version_string, git_username, git_branch_name);
		draw_string_render(&bounds, result_text_buffer);
	}
#endif
}

const uint64 k_cheevo_display_lifetime = (5 * k_shell_time_msec_denominator);
const uint32 k_cheevo_title_font = 10;
const uint32 k_cheevo_message_font = 1;

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
			real_argb_color text_color = *global_real_argb_white;
			text_color.alpha = (float)(x_cheevo_timer - time_now) / k_cheevo_display_lifetime;

			// there should be a better way of doing this instead of every frame!
			utf8_string_to_wchar_string(it->first.c_str(), cheevo_message, NUMBEROF(cheevo_message));
			wchar_t* divider_position = wcschr(cheevo_message, '|');
			if (divider_position)
			{
				*divider_position = '\0';
			}

			draw_string_reset();
			draw_string_set_font_and_options(k_cheevo_title_font, 0, 2, 0, &text_color, global_real_argb_black, false);

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

const uint32 k_update_status_font = 5;

void render_cartographer_update()
{
	if (autoUpdateText)
	{
		rectangle2d bounds;
		wchar_t update_message_buffer[256];
		wchar_t* last_line = update_message_buffer;
		c_static_wchar_string128 lines[16];
		int32 line_count = 0;
		int32 update_message_length = 0;

		utf8_string_to_wchar_string(autoUpdateText, update_message_buffer, NUMBEROF(update_message_buffer));
		update_message_length = ustrnlen(update_message_buffer, NUMBEROF(update_message_buffer));
	
		for (int32 character_index = 0; character_index < NUMBEROF(update_message_buffer) && character_index < update_message_length && line_count < NUMBEROF(lines); character_index++)
		{
			wchar_t* character = &update_message_buffer[character_index];
			if (*character == '\n')
			{
				*character = '\0';
				lines[line_count++].set(last_line);
				last_line = &update_message_buffer[character_index + 1];
			}
		}

		draw_string_reset();
		draw_string_set_font_and_options(k_update_status_font, 0, 0, 0, global_real_argb_white, global_real_argb_black, false);
	
		rasterizer_get_frame_bounds(&bounds);
		bounds.top += rectangle2d_height(&bounds) * .1f;

		for (int32 line_index = 0; line_index < line_count; line_index++)
		{
			draw_string_render(&bounds, lines[line_index].get_string());
			bounds.top += get_text_size_from_font_cache(k_update_status_font);
		}

		if (sizeOfDownload > 0)
		{
			wchar_t update_message_buffer[256];
			float percent_complate = 100.f * ((float)sizeOfDownloaded / sizeOfDownload);
			swprintf_s(update_message_buffer, NUMBEROF(update_message_buffer), L"(progress: %.2f%%)", percent_complate);
			draw_string_render(&bounds, update_message_buffer);
		}
	}
}
