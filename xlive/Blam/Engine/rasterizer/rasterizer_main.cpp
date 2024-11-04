#include "stdafx.h"
#include "rasterizer_main.h"

#include "game/game.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"
#include "shell/shell_windows.h"

#include "H2MOD/Modules/Shell/Config.h"

/* prototypes */

// Used to limit framerate using our implementation
static void __cdecl rasterizer_present_hook(bitmap_data* bitmap);

/* public code */

void rasterizer_main_apply_patches(void)
{
	// present hooks for the frame limiter
	PatchCall(Memory::GetAddress(0x19073C), rasterizer_present_hook);
	return;
}

void rasterizer_present_frame_wrapper(bitmap_data* bitmap)
{
	rasterizer_dx9_present(bitmap, false);
	return;
}

void rasterizer_present_frame_screenshot_wrapper(bitmap_data* bitmap)
{
	rasterizer_dx9_present(bitmap, true);
	return;
}

e_display_type rasterizer_get_display_type(void)
{
	const bool ui_shell = game_is_active() && game_is_ui_shell();
	return ui_shell ? _display_type_4_by_3 : rasterizer_globals_get()->display_parameters.display_type;
}

/* private code */

static void __cdecl rasterizer_present_hook(bitmap_data* bitmap)
{
	rasterizer_dx9_present(bitmap, false);
	shell_windows_throttle_framerate(H2Config_fps_limit);
	return;
}

