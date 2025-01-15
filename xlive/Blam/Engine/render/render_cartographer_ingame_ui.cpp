#include "stdafx.h"
#include "render_cartographer_ingame_ui.h"

#include "cartographer/twizzler/twizzler.h"
#include "cseries/cseries_strings.h"
#include "game/game.h"
#include "game/game_engine.h"
#include "rasterizer/rasterizer_globals.h"
#include "rasterizer/rasterizer_text.h"
#include "rasterizer/dx9/rasterizer_dx9.h"
#include "shell/shell_windows.h"
#include "networking/network_statistics.h"
#include "text/draw_string.h"
#include "text/font_cache.h"
#include "text/unicode.h"

#include "H2MOD/GUI/imgui_integration/imgui_handler.h"
#include "H2MOD/Modules/Accounts/AccountLogin.h"
#include "H2MOD/Modules/Achievements/Achievements.h"
#include "H2MOD/GUI/XLiveRendering.h"

#include "version_git.h"

/* defines */

// define this to enable queueing a test message in render_cartographer_achievement_message
//#define ACHIVEMENT_RENDER_DEBUG_ENABLED

// define this to render git branch information (if GEN_GIT_VER_VERSION_STRING is defined)
#define CARTOGRAPHER_TEST_BUILD_DRAW_TEXT

/* constants */

enum
{
	k_status_text_font = 0,
	k_update_status_font = 5,
	k_cheevo_title_font = 10,
	k_cheevo_message_font = 1,
	k_netdebug_text_font = 0,
	k_cheevo_display_lifetime = (5 * k_shell_time_msec_denominator),
};

/* prototypes */

static void render_cartographer_status_bar(const char* build_text);
static void render_cartographer_git_build_info(void);
static bool render_cartographer_achievement_message(const char* achivement_message);
static void render_cartographer_update_message(const char* update_text, int64 update_size_bytes, int64 update_downloaded_bytes);
static void render_netdebug_text(void);

/* globals */

/* public code */

void render_cartographer_ingame_ui(void)
{
	// these are global variables defined in legacy files where
	// various d3dx9 rendering functions originally were.

	// defined in Modules\Updater\Updater.cpp
	extern char* autoUpdateText;
	extern long long sizeOfDownload;
	extern long long sizeOfDownloaded;

#ifdef ACHIVEMENT_RENDER_DEBUG_ENABLED
	static bool x_test = false;
	if (x_test)
	{
		AchievementMap.insert({ "achievement title|achievement message", false });
		x_test = false;
	}
#endif

	rasterizer_dx9_perf_event_begin("render cartographer ingame ui", NULL);
	render_cartographer_status_bar(g_cartographer_build_text);
	render_cartographer_update_message(autoUpdateText, sizeOfDownload, sizeOfDownloaded);
	if (!AchievementMap.empty())
	{
		auto it = AchievementMap.begin();
		it->second = true;
		if (!render_cartographer_achievement_message(it->first.c_str()))
		{
			AchievementMap.erase(it);
		}
	}
	render_cartographer_git_build_info();
	render_netdebug_text();
	rasterizer_dx9_perf_event_end("render cartographer ingame ui");

	return;
}

/* private code */

void render_cartographer_status_bar(const char *build_text)
{
	rectangle2d bounds;
	rasterizer_get_frame_bounds(&bounds);

	bool game_is_main_menu = game_is_ui_shell();
	bool paused_or_in_menus = (*Memory::GetAddress<bool*>(0x47A568) != false);

	wchar_t build_string_buffer[256];
	utf8_string_to_wchar_string(build_text, build_string_buffer, NUMBEROF(build_string_buffer));

	wchar_t master_state_string_buffer[256];
	utf8_string_to_wchar_string(GetMasterStateStr(), master_state_string_buffer, NUMBEROF(master_state_string_buffer));

	real_argb_color text_color_console = *global_real_argb_white;
	if (game_is_main_menu)
	{
		text_color_console.alpha = .5f;
	}

	const int16 line_height = get_text_size_from_font_cache(k_status_text_font);
	bool paused_or_in_main_menu = game_is_main_menu || paused_or_in_menus;
	if (paused_or_in_main_menu)
	{
		draw_string_reset();
		draw_string_set_draw_mode(k_status_text_font, 0, 0, 0, &text_color_console, global_real_argb_black, false);

		bounds.bottom = bounds.top + line_height;
		if (g_twizzler_status)
		{
			draw_string_set_format(0, 1, 0, false);
			rasterizer_draw_unicode_string(&bounds, L"Anti-cheat is enabled");
			draw_string_set_format(0, 0, 0, false);
		}
		rasterizer_draw_unicode_string(&bounds, build_string_buffer);
		bounds.top += line_height;
		bounds.bottom = bounds.top + line_height;
		rasterizer_draw_unicode_string(&bounds, master_state_string_buffer);
	}

	return;
}

void render_cartographer_git_build_info(void)
{
#if defined(GEN_GIT_VER_VERSION_STRING) && defined(CARTOGRAPHER_TEST_BUILD_DRAW_TEXT) 
	const s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();

	const int16 line_height = get_text_size_from_font_cache(k_status_text_font);
	real_argb_color text_color_console = *global_real_argb_white;
	text_color_console.alpha = .55f;

	rectangle2d bounds;
	rasterizer_get_frame_bounds(&bounds);
	bounds.top += (int16)(1050 * rasterizer_globals->ui_scale);
	bounds.left = bounds.right - (int16)(765 * rasterizer_globals->ui_scale);
	bounds.bottom = bounds.top + line_height;

	draw_string_reset();
	draw_string_set_draw_mode(k_status_text_font, 0, 0, 0, &text_color_console, global_real_argb_black, false);

	wchar_t result_text_buffer[1024];

	swprintf(result_text_buffer, NUMBEROF(result_text_buffer), L"%S %S", __DATE__, __TIME__);
	rasterizer_draw_unicode_string(&bounds, result_text_buffer);
	bounds.top += line_height;
	bounds.bottom = bounds.top + line_height;
	swprintf(result_text_buffer, NUMBEROF(result_text_buffer), L"%S %S branch: %S", GEN_GIT_VER_VERSION_STRING, GET_GIT_VER_USERNAME, GET_GIT_VER_BRANCH);
	rasterizer_draw_unicode_string(&bounds, result_text_buffer);
#endif
}

bool render_cartographer_achievement_message(const char *achivement_message)
{
	static int64 x_cheevo_timer = 0;
	int64 time_now = shell_time_now_msec();
	bool result = true;

	if (x_cheevo_timer == 0)
	{
		x_cheevo_timer = time_now + k_cheevo_display_lifetime;
	}

	if (x_cheevo_timer - time_now > 0)
	{
		rectangle2d bounds;
		wchar_t cheevo_message[256];
		const int16 widget_total_height = get_text_size_from_font_cache(k_cheevo_title_font) + (get_text_size_from_font_cache(k_cheevo_message_font) * 2);
		real_argb_color text_color = *global_real_argb_white;
		text_color.alpha = (float)(x_cheevo_timer - time_now) / k_cheevo_display_lifetime;

		utf8_string_to_wchar_string(achivement_message, cheevo_message, NUMBEROF(cheevo_message));
		wchar_t* divider_position = wcschr(cheevo_message, '|');
		if (divider_position)
		{
			*divider_position = '\0';
		}

		draw_string_reset();
		draw_string_set_draw_mode(k_cheevo_title_font, 0, 2, 0, &text_color, global_real_argb_black, false);

		rasterizer_get_screen_bounds(&bounds);
		bounds.top += rectangle2d_height(&bounds) / 3 - (widget_total_height / 2);
		rasterizer_draw_unicode_string(&bounds, L"Achievement Unlocked");
		bounds.top += get_text_size_from_font_cache(k_cheevo_title_font);

		draw_string_set_font(k_cheevo_message_font);
		rasterizer_draw_unicode_string(&bounds, cheevo_message);
		bounds.top += get_text_size_from_font_cache(k_cheevo_message_font);
		rasterizer_draw_unicode_string(&bounds, divider_position ? (divider_position + 1) : L"<invalid achievement description>");
	}
	else
	{
		x_cheevo_timer = 0;
		result = false;
	}

	return result;
}

void render_cartographer_update_message(const char* update_text, int64 update_size_bytes, int64 update_downloaded_bytes)
{
	rectangle2d bounds;
	rasterizer_get_frame_bounds(&bounds);
	bounds.top += (int16)(rectangle2d_height(&bounds) * .1f);

	if (update_text != nullptr)
	{
		wchar_t update_message_buffer[256];
		wchar_t* last_line = update_message_buffer;
		c_static_wchar_string<128> lines[16];
		int32 line_count = 0;
		int32 update_message_length = 0;

		utf8_string_to_wchar_string(update_text, update_message_buffer, NUMBEROF(update_message_buffer));
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
		draw_string_set_draw_mode(k_update_status_font, 0, 0, 0, global_real_argb_white, global_real_argb_black, false);

		for (int32 line_index = 0; line_index < line_count; line_index++)
		{
			rasterizer_draw_unicode_string(&bounds, lines[line_index].get_string());
			bounds.top += get_text_size_from_font_cache(k_update_status_font);
		}
	}

	if (update_size_bytes > 0)
	{
		wchar_t update_message_buffer[256];
		real32 percent_complete = 100.f * ((real32)update_downloaded_bytes / update_size_bytes);
		swprintf_s(update_message_buffer, NUMBEROF(update_message_buffer), L"(progress: %.2f%%)", percent_complete);
		rasterizer_draw_unicode_string(&bounds, update_message_buffer);
	}

	return;
}

void render_netdebug_text(void)
{
	if (ImGuiHandler::g_network_stats_overlay != _network_stats_display_none)
	{
		c_network_session* session = NULL;
		if (network_life_cycle_in_squad_session(&session))
		{
			s_simulation_player_netdebug_data netdebug_data_default{};
			s_simulation_player_netdebug_data* netdebug_data = &netdebug_data_default;
			s_observer_channel* observer_channel = NULL;
			int32 observer_channel_index;
			if (!session->is_host())
			{
				s_membership_peer* membership_peer = session->get_peer_membership(session->get_local_peer_index());

				observer_channel_index = session->m_session_peers[session->get_session_host_peer_index()].observer_channel_index;
				if (observer_channel_index != NONE)
				{
					observer_channel = &session->m_network_observer->m_observer_channels[observer_channel_index];

					netdebug_data->client_rtt_msec = (int16)observer_channel->net_rtt;
					netdebug_data->client_packet_rate = (int16)(observer_channel->stream_packet_rate * 10.f);
					netdebug_data->client_throughput = (int16)((observer_channel->throughput_bps * 10.f) / 1024.f);
					netdebug_data->client_packet_loss_percentage = (int16)(observer_channel->field_440.average_values_in_window() * 100.f);

					// NOT UPDATED IN REAL-TIME
					//for (int32 i = 0; i < k_number_of_users; i++)
					//{
					//	if (membership_peer->local_players_indexes[i] != NONE)
					//	{
					//		//netdebug_data = game_engine_get_netdebug_data(membership_peer->local_players_indexes[i]);
					//		break;
					//	}
					//}
				}
			}

			const s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
			const int16 line_height = get_text_size_from_font_cache(k_netdebug_text_font);

			rectangle2d bounds;
			wchar_t netdebug_text[512];
			
			rasterizer_get_frame_bounds(&bounds);
			bounds.top = (int16)(8 * rasterizer_globals->ui_scale);
			bounds.left = bounds.right - (int16)(1070 * rasterizer_globals->ui_scale);
			bounds.bottom = bounds.top + line_height;

			real_argb_color text_color_console = *global_real_argb_white;
			text_color_console.alpha *= (65.f / 100.f);

			swprintf_s(netdebug_text, ARRAYSIZE(netdebug_text),
				L"[up^ rtt: %3d msec, pck rate: %.1f, throughput: %.3f bps, loss: %3d %%]",
				netdebug_data->client_rtt_msec,
				(real32)netdebug_data->client_packet_rate / 10.f,
				((real32)netdebug_data->client_throughput / 10.f) * 1024.f,
				netdebug_data->client_packet_loss_percentage
			);

			draw_string_reset();
			draw_string_set_draw_mode(k_netdebug_text_font, 0, 0, 0, &text_color_console, global_real_argb_black, false);

			draw_string_set_format(0, 0, 0, false);
			rasterizer_draw_unicode_string(&bounds, netdebug_text);
		}
	}
}
