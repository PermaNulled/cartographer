#include "stdafx.h"
#include "main_game_time.h"

#include "game/game.h"
#include "game/game_time.h"
#include "math/math.h"
#include "shell/shell_windows.h"
#include "game/player_control.h"

bool g_main_game_time_frame_limiter_enabled = false;

real32 main_time_get_max_frame_time()
{
	time_globals* time_globals = time_globals::get();
	real32 result = time_globals->tick_length - (real32)(time_globals->game_ticks_leftover / (real32)time_globals->ticks_per_second);
	return MAX(result, 0.0f);
}

void __cdecl compute_target_tick_count(float dt, float* out_time_delta, int* out_target_tick_count)
{
	typedef void(__cdecl* compute_target_tick_count_t)(float, float*, int*);
	auto p_compute_target_tick_count = Memory::GetAddress<compute_target_tick_count_t>(0x7C1BF);

	typedef bool(__cdecl* game_is_not_paused_t)();
	auto p_game_is_not_paused = Memory::GetAddress<game_is_not_paused_t>(0x497EA);

	p_compute_target_tick_count(dt, out_time_delta, out_target_tick_count);
}

uint64 __cdecl main_time_get_absolute_milliseconds()
{
	uint64 milliseconds = system_milliseconds();
	s_main_time_globals* main_time_globals = s_main_time_globals::get();

	// copy the high part
	milliseconds |= (main_time_globals->last_milliseconds >> 32) << 32;

	if (milliseconds < main_time_globals->last_milliseconds)
	{
		// adjust the time, a complete cycle of system_milliseconds has passed
		milliseconds += UINT_MAX + 1llu;
	}

	ASSERT(milliseconds >= main_time_globals->last_milliseconds);
	return milliseconds;
}

real32 __cdecl main_time_update_hook(bool fixed_time_step, real32 fixed_time_delta)
{
	/*typedef real32(__cdecl* main_time_update_t)(bool, real32);
	auto p_main_time_update = Memory::GetAddress<main_time_update_t>(0x28814);*/

	typedef void(__cdecl* t_shell_update)();
	auto p_shell_update = Memory::GetAddress<t_shell_update>(0x7902, 0xBA18);

	int game_time;
	real32 dt_sec = 0.0f;
	uint64 time_now_msec;

	s_main_time_globals* main_time_globals = s_main_time_globals::get();
	game_time = game_in_progress() ? get_game_time_ticks() : 0;

	// TranslateMessage()
	// TODO move to function and cleanup
	p_shell_update();

	if (fixed_time_step)
	{
		dt_sec = fixed_time_delta;
		if (time_globals::available())
			time_globals::get()->game_ticks_leftover = 0.0f;

		time_now_msec = main_time_globals->last_milliseconds + (long long)(fixed_time_delta * 1000.);
	}
	else
	{
		time_now_msec = main_time_get_absolute_milliseconds();
		dt_sec = (real32)((real64)(time_now_msec - main_time_globals->last_milliseconds) / 1000.);

		// don't run the frame limiter when time step is fixed, because the code doesn't support it
		// in case of fixed time step, frame limiter should be handled by the other frame limiter
		if (g_main_game_time_frame_limiter_enabled || game_is_minimized())
		{
			if (time_globals::available())
			{
				// if there's game tick leftover time (i.e the actual game tick update executed faster than the actual engine's fixed time step)
				// FIXED by interpolation: 
				// limit the framerate to get back in sync with the renderer to prevent ghosting and jagged movement
				real32 frame_time = main_time_get_max_frame_time();
				while (frame_time > dt_sec)
				{
					uint32 yield_time_msec = 0;
					real32 fMsSleep = (real32)(frame_time - dt_sec) * 1000.f;

					if (fMsSleep >= 1.0f)
					{
						yield_time_msec = (int)fMsSleep;

						// TODO FIXME to reduce stuttering, spend some of the time to sleep by CPU spinning,
						// Sleep is not precise since Windows is not a RTOS
						Sleep(yield_time_msec);
					}


					time_now_msec = main_time_get_absolute_milliseconds();
					dt_sec = (real32)((real64)(time_now_msec - main_time_globals->last_milliseconds) / 1000.);
				}
			}
			else
			{
				Sleep(15u);

				time_now_msec = main_time_get_absolute_milliseconds();
				dt_sec = (real32)((real64)(time_now_msec - main_time_globals->last_milliseconds) / 1000.);
			}
		}
	}

	dt_sec = PIN(dt_sec, 0.0f, 10.f);
	main_time_globals->last_milliseconds = time_now_msec;
	main_time_globals->last_game_time = game_time;
	main_time_globals->field_16[0] = *Memory::GetAddress<__int64*>(0xA3E440);
	main_time_globals->field_16[1] = *Memory::GetAddress<__int64*>(0xA3E440);

	player_control_update_dt(dt_sec);

	//LOG_TRACE_GAME("main_time_update() - timeDeltaSeconds: {}", timeDeltaSeconds);
	return dt_sec;
}

void main_game_time_apply_patches()
{
	if (!Memory::IsDedicatedServer())
	{
		PatchCall(Memory::GetAddress(0x39BE3, 0xC03E), main_time_update_hook);
		PatchCall(Memory::GetAddress(0x39C0D), main_time_update_hook);
		//NopFill(Memory::GetAddress(0x2728E7), 5);

		// PatchCall(Memory::GetAddress(0x39D04), compute_target_tick_count);

		//NopFill(Memory::GetAddress(0x39BDA), 2);
		//NopFill(Memory::GetAddress(0x39DF0), 8);

		//NopFill(Memory::GetAddress(0x39DE1), 5);
	}
}

