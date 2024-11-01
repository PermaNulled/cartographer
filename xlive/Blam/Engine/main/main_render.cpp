#include "stdafx.h"
#include "main_render.h"

#include "main_screenshot.h"

#include "cutscene/cinematics.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"
#include "rasterizer/rasterizer_main.h"
#include "render/render_cartographer_ingame_ui.h"

/* globals */

window_bound g_window_bounds[6];

bool g_debug_render_horizontal_splitscreen = false;
bool g_debug_force_all_player_views_to_default = false;

/*  prototypes */

static void main_render_hook(void);

static void compute_window_bounds_to_usercall(
	window_bound* window,
	int32 single_view,
	int32 user_index,
	int32 window_bound_index,
	int32 window_count,
	e_display_split_type display_split_type,
	s_observer_result* observer_result);

/* public code */

void main_render_apply_patches(void)
{
	// this is replacing a nullsub
	PatchCall(Memory::GetAddress(0x19228E), main_render_hook);

	PatchCall(Memory::GetAddress(0x27009A), main_render);
	PatchCall(Memory::GetAddress(0x2700A5), main_render_previous_backbuffer);
	return;
}

void __cdecl main_render(void)
{
	uint32 player_window_count;
	if (cinematic_in_progress())
	{
		player_window_count = 1;
	}
	else if (local_player_count() >= 1)
	{
		player_window_count = MIN(local_player_count(), MAXIMUM_PLAYER_WINDOWS);
	}
	else
	{
		player_window_count = 1;
	}

	uint32 window_count = player_window_count + 1;

	ASSERT(player_window_count <= MAXIMUM_PLAYER_WINDOWS);
	ASSERT(window_count <= MAXIMUM_RENDERED_WINDOWS);

	e_display_split_type display_split_type;
	if (player_window_count <= 1)
	{
		display_split_type = _display_split_type_none;
	}
	else
	{
		const bool use_horizontal_split = g_debug_render_horizontal_splitscreen || rasterizer_get_display_type() == _display_type_4_by_3;
		display_split_type = use_horizontal_split ? _display_split_type_horizontal : _display_split_type_vertical;
	}

	int32 user_index = NONE;
	window_bound* player_windows = &g_window_bounds[1];
	for (uint32 window_num = 0; window_num < window_count; ++window_num)
	{
		s_observer_result* observer_result = NULL;
		if (g_debug_force_all_player_views_to_default)
		{
			break;
		}

		// TODO: another debug global condition here:


		if (++user_index < k_number_of_users)
		{
			while (!players_user_is_active(user_index))
			{
				if (++user_index >= 4)
				{
					user_index = NONE;
				}
			}
		}
		else
		{
			user_index = NONE;
		}

		if (user_index != NONE)
		{
			observer_result = observer_get_camera(user_index);
		}

		compute_window_bounds_to_usercall(&player_windows[window_num], 0, user_index, window_num, player_window_count, display_split_type, observer_result);
	}

	window_bound* window = &g_window_bounds[window_count];
	window->window_bound_index = NONE;
	window->user_index = NONE;
	window->single_view = 1;
	rasterizer_get_screen_and_frame_bounds(
		&window->rasterizer_camera.viewport_bounds,
		&window->rasterizer_camera.window_bounds);
	set_window_camera_values(&window->rasterizer_camera, 0, 0, 0);

	window->render_camera = window->rasterizer_camera;
	if (screenshot_render(&g_window_bounds[1]))
	{
		render_frame(3, window_count, player_window_count, display_split_type, &g_window_bounds[1]);
	}

	return;
}


void __cdecl main_render_previous_backbuffer(int32 a1, int32 a2)
{
	g_window_bounds[0].single_view = 1;
	g_window_bounds[0].window_bound_index = NONE;
	g_window_bounds[0].user_index = NONE;
	rasterizer_get_screen_and_frame_bounds(
		&g_window_bounds[0].rasterizer_camera.viewport_bounds,
		&g_window_bounds[0].rasterizer_camera.window_bounds);

	set_window_camera_values(&g_window_bounds[0].rasterizer_camera, 0, 0, 0);
	
	g_window_bounds[0].render_camera = g_window_bounds[0].rasterizer_camera;
	render_nonplayer_frame(g_window_bounds);


	s_rasterizer_dx9_main_globals* rasterizer_dx9_globals = rasterizer_dx9_main_globals_get();
	IDirect3DSurface9* backbuffer;
	if (screenshot_in_progress())
	{
		backbuffer = rasterizer_dx9_globals->global_d3d_surface_screenshot;
		rasterizer_dx9_globals->global_d3d_surface_screenshot->AddRef();
	}
	else
	{
		rasterizer_dx9_globals->global_d3d_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
	}

	rasterizer_dx9_globals->global_d3d_device->StretchRect(
		rasterizer_dx9_globals->global_d3d_surface_render_primary,
		NULL,
		backbuffer,
		NULL,
		D3DTEXF_NONE);

	if (backbuffer)
	{
		backbuffer->Release();
	}

	return;
}

void __cdecl set_window_camera_values(render_camera* camera, s_observer_result* result, int16* a3, int16* a4)
{
	INVOKE(0x194E75, 0x0, set_window_camera_values, camera, result, a3, a4);
	return;
}

/* private code */

static void main_render_hook(void)
{
	if (!cubemap_screenshot_in_progress())
	{
		render_cartographer_ingame_ui();
	}

	return;
}

static void compute_window_bounds_to_usercall(
	window_bound* window,
	int32 single_view,
	int32 user_index,
	int32 window_bound_index,
	int32 window_count,
	e_display_split_type display_split_type,
	s_observer_result* observer_result)
{
	void* compute_window_bounds = (void*)Memory::GetAddress(0x26FA84);
	__asm
	{
		push observer_result
		push display_split_type
		push window_count
		push window_bound_index
		push window
		mov ecx, user_index
		mov eax, single_view
		call compute_window_bounds
		add esp, 20
	}
	return;
}