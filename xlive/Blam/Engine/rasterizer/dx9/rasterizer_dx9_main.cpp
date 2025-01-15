#include "stdafx.h"
#include "rasterizer_dx9_main.h"

#include "rasterizer_dx9_9on12.h"

#include "rasterizer_dx9.h"
#include "rasterizer_dx9_bitmaps.h"
#include "rasterizer_dx9_debug.h"
#include "rasterizer_dx9_dof.h"
#include "rasterizer_dx9_dynavobgeom.h"
#include "rasterizer_dx9_errors.h"
#include "rasterizer_dx9_motion_sensor.h"
#include "rasterizer_dx9_screen_effect.h"
#include "rasterizer_dx9_submit.h"
#include "rasterizer_dx9_shader_submit.h"
#include "rasterizer_dx9_targets.h"
#include "rasterizer_dx9_text.h"
#include "rasterizer_dx9_vertex_buffers.h"
#include "rasterizer_dx9_widgets.h"

#include "bitmaps/bitmaps.h"
#include "bink/wmv_playback.h"
#include "cache/pc_geometry_cache.h"
#include "cache/pc_texture_cache.h"
#include "cseries/debug_memory.h"
#include "game/game.h"
#include "main/main.h"
#include "networking/network_configuration.h"
#include "rasterizer/rasterizer_cinematics.h"
#include "rasterizer/rasterizer_dynamic_reflect.h"
#include "rasterizer/rasterizer_globals.h"
#include "rasterizer/rasterizer_loading.h"
#include "rasterizer/rasterizer_memory.h"
#include "rasterizer/rasterizer_occlusion.h"
#ifdef RASTERIZER_PROFILE_ENABLED
#include "rasterizer/rasterizer_profile.h"
#endif
#include "rasterizer/rasterizer_settings.h"
#include "rasterizer/rasterizer_stipple.h"
#include "rasterizer/rasterizer_text.h"
#include "rasterizer/rasterizer_transparent_geometry.h"
#include "render/render_weather.h"
#include "saved_games/game_state.h"
#include "shell/shell.h"
#include "shell/shell_windows.h"

#include "H2MOD/GUI/XLiveRendering.h"
#include "H2MOD/Modules/Shell/Config.h"
#include <dwmapi.h>

/* typedefs */

typedef void(__cdecl* rasterizer_dx9_set_texture_stage_t)(int16, datum, int16, real32);
typedef bool(__cdecl* rasterizer_dx9_initialize_t)(void);

/* constants */

const D3DBLEND k_blend_operation[k_shader_framebuffer_blend_function_count] =
{
  D3DBLEND_ZERO,
  D3DBLEND_ZERO,
  D3DBLEND_ZERO,
  D3DBLEND_ZERO,
  D3DBLEND_SRCCOLOR,
  D3DBLEND_INVSRCCOLOR,
  D3DBLEND_SRCALPHA,
  D3DBLEND_ZERO,
  D3DBLEND_ZERO,
  D3DBLEND_ZERO,
  (D3DBLEND)NONE,
  (D3DBLEND)NONE
};

const D3DBLEND k_dst_blend[k_shader_framebuffer_blend_function_count] =
{
  D3DBLEND_INVSRCALPHA,
  D3DBLEND_ZERO,
  D3DBLEND_SRCCOLOR,
  D3DBLEND_ONE,
  D3DBLEND_ONE,
  D3DBLEND_ONE,
  D3DBLEND_ONE,
  D3DBLEND_INVSRCALPHA,
  (D3DBLEND)0,
  (D3DBLEND)0,
  (D3DBLEND)NONE,
  (D3DBLEND)NONE
};

const D3DBLEND k_src_blend[k_shader_framebuffer_blend_function_count] =
{
  D3DBLEND_SRCALPHA,
  D3DBLEND_DESTCOLOR,
  D3DBLEND_DESTCOLOR,
  D3DBLEND_ONE,
  D3DBLEND_ONE,
  D3DBLEND_ONE,
  D3DBLEND_ONE,
  D3DBLEND_ONE,
  (D3DBLEND)0,
  (D3DBLEND)0,
  (D3DBLEND)NONE,
  (D3DBLEND)NONE
};

const uint32 k_rasterizer_dx9_texture_usage[] =
{
    0,
    D3DUSAGE_RENDERTARGET,
    D3DUSAGE_DEPTHSTENCIL,
    0
};

const D3DPOOL k_rasterizer_dx9_texture_pool[] =
{
    D3DPOOL_MANAGED,
    D3DPOOL_DEFAULT,
    D3DPOOL_DEFAULT,
    D3DPOOL_MANAGED
};

const uint32 k_rasterizer_dx9_vertex_usage[] =
{
    D3DUSAGE_WRITEONLY,
    D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
    D3DUSAGE_WRITEONLY
};

const D3DPOOL k_rasterizer_dx9_vertex_pool[] =
{
    D3DPOOL_MANAGED,
    D3DPOOL_MANAGED,
    D3DPOOL_MANAGED
};

const uint32 k_rasterizer_dx9ex_texture_usage[] =
{
    0,
    D3DUSAGE_RENDERTARGET,
    D3DUSAGE_DEPTHSTENCIL,
    0
};

const D3DPOOL k_rasterizer_dx9ex_texture_pool[] =
{
    D3DPOOL_DEFAULT,
    D3DPOOL_DEFAULT,
    D3DPOOL_DEFAULT,
    D3DPOOL_DEFAULT
};

const uint32 k_rasterizer_dx9ex_vertex_usage[] =
{
    D3DUSAGE_WRITEONLY,
    D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
    D3DUSAGE_WRITEONLY
};

const D3DPOOL k_rasterizer_dx9ex_vertex_pool[] =
{
    D3DPOOL_DEFAULT,
    D3DPOOL_DEFAULT,
    D3DPOOL_DEFAULT
};

const char* global_d3d_vs_prime_source = "float4 main(float4 pos : POSITION) : POSITION { return pos; }";

/* globals */

rasterizer_dx9_set_texture_stage_t p_rasterizer_dx9_set_texture_stage;
rasterizer_dx9_initialize_t p_rasterizer_dx9_initialize;

datum g_last_bitmap_tag_index = 0;

const D3DVERTEXELEMENT9 global_d3d_vd_source[] = 
{
    { 0, 0, D3DDECLTYPE_SHORT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    D3DDECL_END()
};
s_rasterizer_parameters g_rasterizer_parameters = {};

/* prototypes */

static PALETTEENTRY* g_d3d_palettes_get(void);

static IDirect3DVertexDeclaration9** global_d3d_vd_prime_get(void);
static IDirect3DVertexShader9** global_d3d_vs_prime_get(void);

static const uint32** g_d3d9_texture_usage_get(void);
static const D3DPOOL** g_d3d9_texture_pool_get(void);
static const uint32** g_d3d9_vertex_usage_get(void);
static const D3DPOOL** g_d3d9_vertex_pool_get(void);

static void __cdecl rasterizer_set_stream_source(void);
static void __cdecl debug_frame_usage_draw(void);
static void __cdecl rasterizer_present_backbuffer(void);
static void __cdecl rasterizer_cache_bitmaps(void);

// Used for debugging the vertex declaration
static bool rasterizer_dx9_get_vertex_declaration_format(D3DVERTEXELEMENT9* vertex_elements, UINT* vertex_element_count);

// Used for debugging the vertex declaration
static bool __cdecl DrawPrimitiveUP_hook_get_vertex_decl(
    D3DPRIMITIVETYPE PrimitiveType,
    UINT PrimitiveCount,
    const void* pVertexStreamZeroData,
    UINT VertexStreamZeroStride);

static void __cdecl display_blackness_window(void);

static HWND __cdecl rasterizer_dx9_create_main_window(void);

// Initializes the prime vertex shader
static void rasterizer_dx9_prime_shader_initialize(void);

// Returns true if we can use d3d9ex
static bool rasterizer_dx9_should_use_d3d9ex(void);

// Returns true if we retrieved the d3d9ex proc
static bool rasterizer_dx9_get_create_ex_proc(void);

// Returns true if we've successfully created the device interface
static bool rasterizer_dx9_create_device_interface(void);

// Returns true if we're using an amd or ati gpu
static bool rasterizer_dx9_is_amd_or_ati_card(void);

/* public code */

void rasterizer_dx9_main_apply_patches(void)
{
    // debugging, get vertex decls currently set
    //PatchCall(Memory::GetAddress(0x2220CA), DrawPrimitiveUP_hook_get_vertex_decl);
    //PatchCall(Memory::GetAddress(0x27D746), DrawPrimitiveUP_hook_get_vertex_decl);

    DETOUR_ATTACH(p_rasterizer_dx9_set_texture_stage, Memory::GetAddress<rasterizer_dx9_set_texture_stage_t>(0x25F600, 0x0), rasterizer_dx9_set_texture_direct);

    // Patch initialize code with our own to detect sm3 support
    PatchCall(Memory::GetAddress(0x263526), rasterizer_dx9_device_initialize);
    PatchCall(Memory::GetAddress(0x26375D), rasterizer_dx9_device_initialize);

    // Redirect dx9 initialization function so we can use d3d9on12 and do other cool stuff
    DETOUR_ATTACH(p_rasterizer_dx9_initialize, Memory::GetAddress<rasterizer_dx9_initialize_t>(0x263359, 0x0), rasterizer_dx9_initialize);

    // disable gamma correction by using D3D9::SetGammaRamp, TODO: implement a shader to take care of this, because D3D9::SetGammaRamp function seems to have 2 issues:
    // 1) it's very heavy on NVIDIA/Intel (not sure about AMD) GPUs (or there is something wrong with the drivers), causing stuttering on maps that override gamma (like Warlock, Turf, Backwash)
    // 2) it doesn't apply the gamma override when playing in windowed mode (thus why some people like using windowed mode, because it doesn't cause stuttering on these maps)

    // maybe we could find a way to use the gamma shader built in by converting the override gamma ramp to something that shader could understand
    BYTE SetGammaRampSkipBytes[] = { 0xE9, 0x94, 0x00, 0x00, 0x00, 0x90 };
    NopFill(Memory::GetAddress(0x26192F), 15);
    WriteBytes(Memory::GetAddress(0x26193E), SetGammaRampSkipBytes, sizeof(SetGammaRampSkipBytes));
    return;
}

s_rasterizer_dx9_main_globals* rasterizer_dx9_main_globals_get(void)
{
    return Memory::GetAddress<s_rasterizer_dx9_main_globals*>(0xA3C640);
}

IDirect3DDevice9Ex* rasterizer_dx9_device_get_interface(void)
{
    return rasterizer_dx9_main_globals_get()->global_d3d_device;
}

datum last_bitmap_tag_index_get(void)
{
    return g_last_bitmap_tag_index;
}

D3DCAPS9* rasterizer_dx9_caps_get(void)
{
    return Memory::GetAddress<D3DCAPS9*>(0x9DA900);
}

int32* hardware_vertex_processing_get(void)
{
    return Memory::GetAddress<int32*>(0x9DA8B0);
}

int32* allow_vsync_get(void)
{
    return Memory::GetAddress<int32*>(0x9DA8C8);
}

bool __cdecl rasterizer_initialize(void)
{
    return INVOKE(0x0263359, 0x0, rasterizer_initialize);
}

bool __cdecl rasterizer_dx9_reset(bool create_window)
{
    return INVOKE(0x26370F, 0x0, rasterizer_dx9_reset, create_window);
}

void __cdecl rasterizer_dx9_reset_depth_buffer(void)
{
    return INVOKE(0x269835, 0x0, rasterizer_dx9_reset_depth_buffer);
}

void rasterizer_dx9_present(bitmap_data* screenshot_bitmap, bool a2)
{
    bool result = true;
    if (!media_foundation_player_running())
    {
        s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();
        s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
        rasterizer_globals->rasterizer_draw_on_main_back_buffer = false;
        if (screenshot_bitmap && screenshot_bitmap->base_address)
        {
            rectangle2d screen_bounds = rasterizer_globals->screen_bounds;

            const int16 calc_width = screen_bounds.left + screenshot_bitmap->width;
            const int16 calc_height = screen_bounds.top + screenshot_bitmap->height;
            screen_bounds.right = (screen_bounds.right > calc_width ? calc_width : screen_bounds.right);
            screen_bounds.bottom = (screen_bounds.bottom > calc_height ? calc_height : screen_bounds.bottom);

            int16 screenshot_height = rectangle2d_height(&screen_bounds);
            int32 screenshot_width = rectangle2d_width(&screen_bounds);

            D3DLOCKED_RECT locked_rect;
            if ((screenshot_bitmap->format == _bitmap_format_a8r8g8b8 || screenshot_bitmap->format == _bitmap_format_x8r8g8b8)
                && !screenshot_bitmap->mipmap_count
                && SUCCEEDED(dx9_globals->global_d3d_surface_screenshot->LockRect(&locked_rect, NULL, D3DLOCK_READONLY)))
            {
                if (locked_rect.pBits)
                {
                    int32 screenshot_pitch = screenshot_width * (bitmap_format_get_bits_per_pixel(screenshot_bitmap->format) / 8);
                    for (int16 row_index = 0; row_index < screenshot_height; ++row_index)
                    {
                        uint32* base_address = bitmap_2d_address(screenshot_bitmap, 0, row_index, 0);
                        uint8* data = (uint8*)locked_rect.pBits;
                        csmemcpy(base_address, &data[row_index * locked_rect.Pitch], screenshot_pitch);
                    }
                    result = SUCCEEDED(dx9_globals->global_d3d_surface_screenshot->UnlockRect());
                }
            }
            else
            {
                result = false;
            }
            rasterizer_dx9_set_render_target_internal(dx9_globals->global_d3d_surface_render_primary, (IDirect3DSurface9*)NONE, true);
            rasterizer_dx9_clear_render_target(0, global_white_pixel32, 0.0f, false);
        }

        rasterizer_set_stream_source();
        HRESULT present_result = dx9_globals->global_d3d_device->Present(NULL, NULL, *Memory::GetAddress<HWND*>(0x46D9C8), NULL);
        if (result && SUCCEEDED(present_result))
        {

#ifdef _DEBUG
            debug_frame_usage_draw();
            rasterizer_present_backbuffer();
#endif
            ++rasterizer_globals->display_parameters.frame_presented_count;
            // nullsub_69();
            if (!loading_screen_in_progress())
            {
                rasterizer_cache_bitmaps();
            }
        }
        else
        {
            rasterizer_globals->reset_screen = true;
        }
    }
}

void __cdecl rasterizer_dx9_set_texture_direct(int16 stage, datum bitmap_tag_index, int16 bitmap_data_index, real32 a4)
{
    g_last_bitmap_tag_index = bitmap_tag_index;
    p_rasterizer_dx9_set_texture_stage(stage, bitmap_tag_index, bitmap_data_index, a4);
    return;
}

bool __cdecl rasterizer_window_begin(s_frame* preferences)
{
    return INVOKE(0x2620CF, 0x0, rasterizer_window_begin, preferences);
}

void __cdecl rasterizer_update_cameras(void)
{
    return INVOKE(0x26268D, 0x0, rasterizer_update_cameras);
}

void __cdecl rasterizer_dx9_set_stencil_mode(int16 mode)
{
    return INVOKE(0x2603D2, 0x0, rasterizer_dx9_set_stencil_mode, mode);
}

void __cdecl rasterizer_dx9_set_render_state(D3DRENDERSTATETYPE state, DWORD value)
{
    return INVOKE(0x26F8E2, 0x0, rasterizer_dx9_set_render_state, state, value);
}

void rasterizer_dx9_set_blend_render_state(e_framebuffer_blend_function framebuffer_blend_function)
{
    bool alpha_blend_enabled = framebuffer_blend_function != _framebuffer_blend_function_none;
    rasterizer_dx9_set_render_state(D3DRS_ALPHABLENDENABLE, alpha_blend_enabled);
    if (alpha_blend_enabled)
    {
        rasterizer_dx9_set_render_state(D3DRS_SRCBLEND, k_src_blend[framebuffer_blend_function]);
        rasterizer_dx9_set_render_state(D3DRS_DESTBLEND, k_dst_blend[framebuffer_blend_function]);
        rasterizer_dx9_set_render_state(D3DRS_BLENDOP, k_blend_operation[framebuffer_blend_function]);
    }
    return;
}

void __cdecl rasterizer_get_bloom_brightness(real32* brightness, real32* overbright)
{
    INVOKE(0x25F17D, 0x0, rasterizer_get_bloom_brightness, brightness, overbright);
    return;
}


bool __cdecl rasterizer_set_texture_bitmap_data(int16 stage, bitmap_data* texture)
{
    return INVOKE(0x25F5AC, 0x0, rasterizer_set_texture_bitmap_data, stage, texture);
}

bool rasterizer_dx9_draw_primitive_up(
    D3DPRIMITIVETYPE PrimitiveType,
    uint32 PrimitiveCount,
    const void* pVertexStreamZeroData,
    uint32 VertexStreamZeroStride)
{
    return rasterizer_dx9_device_get_interface()->DrawPrimitiveUP(
        PrimitiveType,
        PrimitiveCount,
        pVertexStreamZeroData,
        VertexStreamZeroStride) >= 0;
}

bool __cdecl rasterizer_dx9_create_texture(uint32 width, uint32 height, int32 levels, uint32 usage, e_bitmap_data_format format, bool linear, IDirect3DTexture9** texture)
{
    return INVOKE(0x260820, 0x0, rasterizer_dx9_create_texture, width, height, levels, usage, format, linear, texture);
}

void rasterizer_dx9_texture_stage_dimensions(int16 stage, uint32 width, uint32 height)
{
    s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
    rasterizer_globals->bitmaps.textures_staged_width[stage] = width;
    rasterizer_globals->bitmaps.textures_staged_height[stage] = height;
    return;
}

bool __cdecl rasterizer_dx9_device_set_texture(uint32 stage, IDirect3DTexture9* texture)
{
    return INVOKE(0x26EBC7, 0x0, rasterizer_dx9_device_set_texture, stage, texture);
}

void __cdecl rasterizer_dx9_set_texture(uint16 stage, e_bitmap_type type, uint32 usage, datum tag_index, int16 bitmap_index, real32 a6)
{
    INVOKE(0x25F74C, 0x0, rasterizer_dx9_set_texture, stage, type, usage, tag_index, bitmap_index, a6);
    return;
}

bool __cdecl rasterizer_dx9_device_initialize(s_rasterizer_parameters* parameters, bool display_blackness)
{    
    s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
    s_rasterizer_dx9_main_globals* rasterizer_dx9_main_globals = rasterizer_dx9_main_globals_get();

    uint32 monitor_count = 0;
    if (shell_command_line_flag_is_set(_shell_command_line_flag_monitor_count))
    {
        const uint32 flag_value = (uint32)shell_command_line_flag_get(_shell_command_line_flag_monitor_count);
        monitor_count = flag_value < rasterizer_dx9_main_globals->global_d3d_interface->GetAdapterCount() ? flag_value : monitor_count;
    }

    ASSERT(parameters);

    csmemmove(&rasterizer_globals->clipping_parameters.parameters, parameters, sizeof(s_rasterizer_parameters));

    const bool is_fullscreen = rasterizer_globals->display_parameters.window_mode == _rasterizer_window_mode_real_fullscreen;

    D3DPRESENT_PARAMETERS d3d_present_parameters = {};
    d3d_present_parameters.hDeviceWindow = *shell_windows_get_hwnd();
    rasterizer_globals->display_parameters.backbuffer_format = D3DFMT_A8R8G8B8;
    d3d_present_parameters.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3d_present_parameters.Windowed = !is_fullscreen;
    d3d_present_parameters.AutoDepthStencilFormat = rasterizer_globals->display_parameters.depthstencil_format;
    d3d_present_parameters.BackBufferWidth = rectangle2d_width(&rasterizer_globals->screen_bounds);
    d3d_present_parameters.Flags = 0;
    d3d_present_parameters.EnableAutoDepthStencil = false;
    d3d_present_parameters.BackBufferHeight = rectangle2d_height(&rasterizer_globals->screen_bounds);
    d3d_present_parameters.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3d_present_parameters.MultiSampleQuality = 0;
    d3d_present_parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3d_present_parameters.FullScreen_RefreshRateInHz = is_fullscreen ? rasterizer_globals->display_parameters.refresh_rate : d3d_present_parameters.FullScreen_RefreshRateInHz;
	d3d_present_parameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    // ### FIXME: make VSYNC configurable (UI required) !!
    if (shell_command_line_flag_is_set(_shell_command_line_flag_novsync) 
        /* || *rasterizer_low_level_texture_detail_get() */ // low detail textures checked for no vsync
        || !*allow_vsync_get())    
    {
		d3d_present_parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    bool succeeded = true;
    HRESULT device_create_hr;
    D3DDISPLAYMODEEX fs_disp_mode;
    if (rasterizer_dx9_main_globals->global_d3d_device)
    {
        HRESULT hr = XLiveOnResetDevice(&d3d_present_parameters);
        succeeded = SUCCEEDED(hr);
        if (!succeeded)
        {
            rasterizer_dx9_errors_log(hr, "XLiveOnResetDevice(&d3d_present_parameters)");
        }

        if (rasterizer_globals->use_d3d9_ex)
        {
            if (!is_fullscreen)
            {
                hr = rasterizer_dx9_main_globals->global_d3d_device->ResetEx(&d3d_present_parameters, NULL);
                succeeded &= SUCCEEDED(hr);
                if (!succeeded)
                {
                    rasterizer_dx9_errors_log(hr, "global_d3d_device->ResetEx(&d3d_present_parameters, NULL)");
                }
            }
            else
            {
                fs_disp_mode.Size = sizeof(D3DDISPLAYMODEEX);
                fs_disp_mode.Width = d3d_present_parameters.BackBufferWidth;
                fs_disp_mode.Height = d3d_present_parameters.BackBufferHeight;
                fs_disp_mode.RefreshRate = d3d_present_parameters.FullScreen_RefreshRateInHz;
                fs_disp_mode.Format = rasterizer_globals->display_parameters.backbuffer_format;
                fs_disp_mode.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
                hr = rasterizer_dx9_main_globals->global_d3d_device->ResetEx(&d3d_present_parameters, &fs_disp_mode);
                succeeded &= SUCCEEDED(hr);
                if (!succeeded)
                {
                    rasterizer_dx9_errors_log(hr, "global_d3d_device->ResetEx(&d3d_present_parameters, &fs_disp_mode)");
                }
            }
        }
        else
        {
            if (display_blackness)
            {
                display_blackness_window();
            }
            HRESULT hr = rasterizer_dx9_main_globals->global_d3d_device->Reset(&d3d_present_parameters);
            succeeded &= SUCCEEDED(hr);
            if (!succeeded)
            {
                rasterizer_dx9_errors_log(hr, "global_d3d_device->Reset(&d3d_present_parameters)");
            }

            if (!d3d_present_parameters.SwapEffect)
            {
                DwmEnableComposition(0);
            }
        }
    }
    else
    {
        const DWORD behavior_flags = 
            (*hardware_vertex_processing_get() != 0 ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING) |
            D3DCREATE_FPU_PRESERVE |
            D3DCREATE_MULTITHREADED |
            D3DCREATE_DISABLE_DRIVER_MANAGEMENT;

        if (rasterizer_globals->use_d3d9_ex)
        {
            fs_disp_mode.Size = sizeof(D3DDISPLAYMODEEX);
            fs_disp_mode.Width = d3d_present_parameters.BackBufferWidth;
            fs_disp_mode.Height = d3d_present_parameters.BackBufferHeight;
            fs_disp_mode.RefreshRate = d3d_present_parameters.FullScreen_RefreshRateInHz;
            fs_disp_mode.Format = rasterizer_globals->display_parameters.backbuffer_format;
            fs_disp_mode.ScanLineOrdering = D3DSCANLINEORDERING_PROGRESSIVE;
            D3DDISPLAYMODEEX* p_fs_disp_mode = is_fullscreen ? &fs_disp_mode : NULL;
            
            device_create_hr = rasterizer_dx9_main_globals->global_d3d_interface->CreateDeviceEx(
                monitor_count,
                D3DDEVTYPE_HAL,
                d3d_present_parameters.hDeviceWindow,
                behavior_flags,
                &d3d_present_parameters,
                p_fs_disp_mode,
                &rasterizer_dx9_main_globals->global_d3d_device);

            succeeded &= SUCCEEDED(device_create_hr);
            if (!succeeded)
            {
                rasterizer_dx9_errors_log(device_create_hr, "CreateDeviceEx");
            }
        }
        else
        {
            device_create_hr = rasterizer_dx9_main_globals->global_d3d_interface->CreateDevice(
                monitor_count,
                D3DDEVTYPE_HAL,
                d3d_present_parameters.hDeviceWindow,
                behavior_flags,
                &d3d_present_parameters,
                (IDirect3DDevice9**)&rasterizer_dx9_main_globals->global_d3d_device);

            succeeded &= SUCCEEDED(device_create_hr);
            if (!succeeded)
            {
                rasterizer_dx9_errors_log(device_create_hr, "CreateDevice");
            }
        }

        if (g_rasterizer_dx9on12_enabled)
        {
            HRESULT d3d9on12_query_interface_hr = rasterizer_dx9_main_globals->global_d3d_device->QueryInterface(IID_PPV_ARGS(&g_d3d9on12_device));
            if (FAILED(d3d9on12_query_interface_hr))
            {
                rasterizer_dx9_errors_log(d3d9on12_query_interface_hr, "QueryInterface IDirect3DDevice9On12");
            }
        }
        
    }

    // TODO: implement
    //debug_end_tracking_nonheap_allocation()

    if (rasterizer_dx9_main_globals->global_d3d_device)
    {
        if (*should_initilize_xlive_get() && !*xlive_initilized_get())
        {
            const int32 network_adapter_index = network_adapter_index_get();
            XLIVE_INITIALIZE_INFO xlive_init_info;
            xlive_init_info.cbSize = sizeof(XLIVE_INITIALIZE_INFO);
            xlive_init_info.pD3D = rasterizer_dx9_main_globals->global_d3d_device;
            xlive_init_info.pD3DPP = &d3d_present_parameters;
            xlive_init_info.langID = 0;
            xlive_init_info.wLivePortOverride = 0;
            xlive_init_info.dwFlags = 0;
            xlive_init_info.pszAdapterName = NULL;

            if (network_adapter_index != NONE)
            {
                xlive_init_info.dwFlags = 1 << 0;
                xlive_init_info.pszAdapterName = (PCHAR)network_adapter_name_get(network_adapter_index);
            }

            const HRESULT hr = XLiveInitialize(&xlive_init_info);
            succeeded &= SUCCEEDED(hr);
            if (succeeded)
            {
                *xlive_initilized_get() = true;
                if (d3d_present_parameters.hDeviceWindow && is_fullscreen)
                {
                    rasterizer_settings_update_window_position();
                }
            }
            else
            {
                *fatal_error_id_get() = hr == 0x80040317 ? 1018 : 105;
                rasterizer_dx9_errors_log(hr, "XLiveInitialize");
            }
        }
    }

    SYSTEM_DEBUG_MEMORY("after CreateDevice/ResetDevice");

    if (succeeded)
    {
        D3DCAPS9* caps = rasterizer_dx9_caps_get();
        HRESULT hr = rasterizer_dx9_device_get_interface()->GetDeviceCaps(caps);
        if (SUCCEEDED(hr))
        {
            if (!g_rasterizer_dx9_driver_globals.disable_amd_or_ati_patches)
            {
                g_rasterizer_dx9_driver_globals.using_amd_or_ati_gpu = rasterizer_dx9_is_amd_or_ati_card();
            }

            // Tells the game to make use of MRT and shader model 3 if the gpu supports it
            rasterizer_globals_get()->d3d9_sm3_supported =
                caps->MaxVertexShader30InstructionSlots >= 512 &&
                caps->MaxPixelShader30InstructionSlots >= 512 &&
                caps->NumSimultaneousRTs >= 2 &&
                !H2Config_force_off_sm3 &&                          // Force disable sm3 if setting is set in H2Config
                !g_rasterizer_dx9_driver_globals.using_amd_or_ati_gpu;
        }
        else
        {
            rasterizer_dx9_errors_log(hr, "global_d3d_device->GetDeviceCaps(&global_d3d_caps)");
        }
    }
    else
    {
        error(3, "### ERROR %s() failed", __FUNCTION__);
    }

    return succeeded;
}

void __cdecl rasterizer_dx9_initialize_camera_projection(
    bool is_texture_camera,
    const render_camera* camera,
    const render_projection* projection,
    e_rasterizer_target rasterizer_target)
{
    INVOKE(0x261B53, 0x0, rasterizer_dx9_initialize_camera_projection, is_texture_camera, camera, projection, rasterizer_target);
    return;
}

bool __cdecl rasterizer_dx9_initialize(void)
{
    bool result = true;

    s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
    s_rasterizer_dx9_main_globals* rasterizer_dx9_main_globals = rasterizer_dx9_main_globals_get();

    if (!rasterizer_globals->rasterizer_initialized && shell_tool_type() != _shell_tool_type_editing_tools)
    {
        rasterizer_globals->use_d3d9_ex = rasterizer_dx9_should_use_d3d9ex();
        
        if (result)
        {
            result = rasterizer_dx9_get_create_ex_proc();
        }

        if (result)
        {
            result = rasterizer_dx9_create_device_interface();
        }

        if (result)
        {
            *g_d3d9_texture_usage_get() = rasterizer_globals->use_d3d9_ex ? k_rasterizer_dx9ex_texture_usage : k_rasterizer_dx9_texture_usage;
            *g_d3d9_texture_pool_get() = rasterizer_globals->use_d3d9_ex ? k_rasterizer_dx9ex_texture_pool : k_rasterizer_dx9_texture_pool;
            *g_d3d9_vertex_usage_get() = rasterizer_globals->use_d3d9_ex ? k_rasterizer_dx9ex_vertex_usage : k_rasterizer_dx9_vertex_usage;
            *g_d3d9_vertex_pool_get() = rasterizer_globals->use_d3d9_ex ? k_rasterizer_dx9ex_vertex_pool : k_rasterizer_dx9_vertex_pool;

            if (shell_command_line_flag_is_set(_shell_command_line_flag_disable_hardware_vertex_processing))
            {
                *hardware_vertex_processing_get() = false;
            }
        }

        if (result)
        {
            rasterizer_settings_set_default_settings();
            rasterizer_settings_create_registry_keys(true);

            // Force windowed if we're in the editor
            if (game_in_editor())
            {
                rasterizer_globals->display_parameters.window_mode = _rasterizer_window_mode_windowed;
            }

            if (shell_command_line_flag_is_set(_shell_command_line_flag_windowed))
            {
                rasterizer_globals->display_parameters.window_mode = _rasterizer_window_mode_windowed;
                const e_rasterizer_window_mode mode = _rasterizer_window_mode_windowed;
                rasterizer_settings_set_display_mode(&mode);
            }

            if (rasterizer_globals->display_parameters.window_mode == _rasterizer_window_mode_real_fullscreen)
            {
                rasterizer_globals->display_parameters.window_mode = _rasterizer_window_mode_funky_fullscreen;
            }

            HWND hwnd = rasterizer_dx9_create_main_window();
            result = hwnd != 0;
        }

        result &= rasterizer_dx9_device_initialize(&g_rasterizer_parameters, false);

        if (result)
        {
            rasterizer_dx9_initialize_state_cache();
            rasterizer_dx9_reset_depth_buffer();
            rasterizer_stipple_initialize();
            rasterizer_dx9_gpu_frontend_initialize();
            rasterizer_dx9_create_dynavobgeom_pixel_shaders();
            rasterizer_dx9_create_screen_effect_pixel_shaders();
            rasterizer_dx9_create_text_pixel_shaders();
            rasterizer_dx9_create_widget_pixel_shaders();
            rasterizer_dx9_create_blur_pixel_shaders();
            rasterizer_dx9_create_motion_sensor_shaders();
            c_particle_system_lite::initialize();
            
            csmemset(g_d3d_palettes_get(), 0, sizeof(int32) * 32);

            rasterizer_dx9_set_render_state(D3DRS_ZENABLE, TRUE);
            rasterizer_dx9_set_render_state(D3DRS_ZWRITEENABLE, TRUE);
            rasterizer_dx9_set_render_state(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
            rasterizer_dx9_set_render_state(D3DRS_DEPTHBIAS, 0);
            rasterizer_dx9_set_render_state(D3DRS_ALPHATESTENABLE, FALSE);
            rasterizer_dx9_set_render_state(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
            rasterizer_dx9_set_render_state(D3DRS_ALPHAREF, 0);
            rasterizer_dx9_set_render_state(D3DRS_ALPHABLENDENABLE, FALSE);
            rasterizer_dx9_set_render_state(D3DRS_SRCBLEND, D3DBLEND_ONE);
            rasterizer_dx9_set_render_state(D3DRS_DESTBLEND, D3DBLEND_ZERO);
            rasterizer_dx9_set_render_state(D3DRS_BLENDOP, D3DBLENDOP_ADD);
            rasterizer_dx9_set_render_state(D3DRS_FOGENABLE, TRUE);
            rasterizer_dx9_set_render_state(D3DRS_LIGHTING, FALSE);
            rasterizer_dx9_set_render_state(D3DRS_SPECULARENABLE, TRUE);
        }

        if (result)
        {
            rasterizer_memory_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_memory_initialize());

            result &= rasterizer_dx9_primary_targets_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_dx9_primary_targets_initialize());


            result &= rasterizer_dx9_vertex_buffer_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_dx9_vertex_buffer_initialize());
        

            result &= rasterizer_dx9_secondary_targets_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_dx9_secondary_targets_initialize());
        

            result &= rasterizer_dx9_targets_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_dx9_targets_initialize());
        

            result &= rasterizer_dx9_default_bitmaps_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_dx9_default_bitmaps_initialize());


            result &= rasterizer_transparent_geometry_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_transparent_geometry_initialize());

        
            result &= rasterizer_dx9_vertex_shaders_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_dx9_vertex_shaders_initialize());


            result &= rasterizer_dx9_debug_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_dx9_debug_initialize());


            result &= rasterizer_text_cache_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_text_cache_initialize());


    #ifdef RASTERIZER_PROFILE_ENABLED

            result &= rasterizer_profile_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_profile_initialize());
    #endif

            result &= rasterizer_dynamic_reflect_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_dynamic_reflect_initialize());
        }

        if (result)
        {
            rasterizer_dx9_prime_shader_initialize();

            rasterizer_cinematics_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_cinematics_initialize());

            geometry_cache_new();
            SYSTEM_DEBUG_MEMORY(geometry_cache_new());

            texture_cache_new();
            SYSTEM_DEBUG_MEMORY(texture_cache_new());

            rasterizer_occlusion_initialize();
            SYSTEM_DEBUG_MEMORY(rasterizer_occlusion_initialize);

            *rasterizer_bloom_globals_get() = (s_rasterizer_bloom_globals*)game_state_malloc("rasterizer bloom globals", 0, sizeof(s_rasterizer_bloom_globals));
            rasterizer_bloom_globals_initialize();
        }

        rasterizer_globals->next_bitmap_index = 0;
        rasterizer_globals->bitmap_data_count = 0;
        rasterizer_globals->rasterizer_initialized = result;

        if (!result)
        {
            error(3, "### ERROR failed to initialize rasterizer");
        }
    }

    return result;
}

bool __cdecl rasterizer_dx9_vertex_shaders_initialize(void)
{
    return INVOKE(0x285061, 0x0, rasterizer_dx9_vertex_shaders_initialize);
}

bool __cdecl rasterizer_dx9_render_scene_start(const s_render_scene_parameters* parameters) 
{
    return INVOKE(0x262105, 0x0, rasterizer_dx9_render_scene_start, parameters);
}

bool __cdecl rasterizer_dx9_render_scene_end(void)
{
    return INVOKE(0x262215, 0x0, rasterizer_dx9_render_scene_end);
}

void __cdecl rasterizer_dx9_clear_render_target(uint32 flags, D3DCOLOR color, real32 z, bool stencil)
{
    INVOKE(0x25FC2A, 0x0, rasterizer_dx9_clear_render_target, flags, color, z, stencil);
    return;
}

/* private code */

static PALETTEENTRY* g_d3d_palettes_get(void)
{
    return Memory::GetAddress<PALETTEENTRY*>(0x9DAA30);
}

static IDirect3DVertexDeclaration9** global_d3d_vd_prime_get(void)
{
    return Memory::GetAddress<IDirect3DVertexDeclaration9**>(0xA3C78C);
}

static IDirect3DVertexShader9** global_d3d_vs_prime_get(void)
{
    return Memory::GetAddress<IDirect3DVertexShader9**>(0xA3C790);
}

static const uint32** g_d3d9_texture_usage_get(void)
{
    return Memory::GetAddress<const uint32**>(0xA3C628);
}

static const D3DPOOL** g_d3d9_texture_pool_get(void)
{
    return Memory::GetAddress<const D3DPOOL**>(0xA3C62C);
}

static const uint32** g_d3d9_vertex_usage_get(void)
{
    return Memory::GetAddress<const uint32**>(0xA3C630);
}

static const D3DPOOL** g_d3d9_vertex_pool_get(void)
{
    return Memory::GetAddress<const D3DPOOL**>(0xA3C634);
}

static void __cdecl rasterizer_set_stream_source(void)
{
    INVOKE(0x26F4D3, 0x0, rasterizer_set_stream_source);
    return;
}

static void __cdecl debug_frame_usage_draw(void)
{
    INVOKE(0x25ECAA, 0x0, debug_frame_usage_draw);
    return;
}

static void __cdecl rasterizer_present_backbuffer(void)
{
    INVOKE(0x25F277, 0x0, rasterizer_present_backbuffer);
    return;
}

static void __cdecl rasterizer_cache_bitmaps(void)
{
    INVOKE(0x261720, 0x0, rasterizer_cache_bitmaps);
    return;
}

static bool rasterizer_dx9_get_vertex_declaration_format(D3DVERTEXELEMENT9* vertex_elements, UINT* vertex_element_count)
{
    IDirect3DDevice9Ex* global_d3d_device = rasterizer_dx9_device_get_interface();

    IDirect3DVertexDeclaration9* vertex_dcl;
    global_d3d_device->GetVertexDeclaration(&vertex_dcl);
    bool res = SUCCEEDED(vertex_dcl->GetDeclaration(vertex_elements, vertex_element_count));

    vertex_dcl->Release();

    return res;
}

static bool __cdecl DrawPrimitiveUP_hook_get_vertex_decl(
    D3DPRIMITIVETYPE PrimitiveType,
    UINT PrimitiveCount,
    const void* pVertexStreamZeroData,
    UINT VertexStreamZeroStride)
{
    IDirect3DDevice9Ex* global_d3d_device = rasterizer_dx9_device_get_interface();

    D3DVERTEXELEMENT9 vertex_elements[MAXD3DDECLLENGTH];
    UINT vertex_element_couunt;

    if (rasterizer_dx9_get_vertex_declaration_format(vertex_elements, &vertex_element_couunt))
    {
    }

    return SUCCEEDED(global_d3d_device->DrawPrimitiveUP(PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride));
}

static void __cdecl display_blackness_window(void)
{
    INVOKE(0x25E5A8, 0x0, display_blackness_window);
    return;
}

static HWND __cdecl rasterizer_dx9_create_main_window(void)
{
    return INVOKE(0x26101B, 0x0, rasterizer_dx9_create_main_window);
}

static void rasterizer_dx9_prime_shader_initialize(void)
{
    IDirect3DDevice9Ex* global_d3d_device = rasterizer_dx9_device_get_interface();
    HRESULT hr = global_d3d_device->CreateVertexDeclaration(global_d3d_vd_source, global_d3d_vd_prime_get());
    
    bool succeeded = SUCCEEDED(hr);
    if (!succeeded)
    {
        rasterizer_dx9_errors_log(hr, "global_d3d_device->CreateVertexDeclaration(global_d3d_vd_source, &global_d3d_vd_prime)");
    }

    s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
    const char* shader_version = rasterizer_globals->d3d9_sm3_supported ? "vs_3_0" : "vs_2_0";

    LPD3DXBUFFER pShader;
    hr = D3DXCompileShader(
        global_d3d_vs_prime_source,
        strlen(global_d3d_vs_prime_source),
        NULL,
        NULL,
        "main",
        shader_version,
        0,
        &pShader,
        NULL,
        NULL
    );

    succeeded &= SUCCEEDED(hr);

    if (!succeeded)
    {
        rasterizer_dx9_errors_log(hr, "D3DXCompileShader(global_d3d_vs_prime_source, strlen(global_d3d_vs_prime_source), NULL, NULL, \"main\", shader_version, 0, &pShader, NULL, NULL)");
    }

    hr = global_d3d_device->CreateVertexShader((DWORD*)pShader->GetBufferPointer(), global_d3d_vs_prime_get());
    succeeded &= SUCCEEDED(hr);
    if (!succeeded)
    {
        rasterizer_dx9_errors_log(hr, "global_d3d_device->CreateVertexShader((DWORD*)pShader->GetBufferPointer(), &global_d3d_vs_prime)");
    }

    if (pShader)
    {
        pShader->Release();
    }

    return;
}

static bool rasterizer_dx9_should_use_d3d9ex(void)
{
    bool result = false;
    
    // OSVERSIONINFOW in H2Sapien while OSVERSIONINFOA in the base game
    // Possibly because it was changed as a part of the H2TOOLSET macro when PI studios was upgrading the Halo 2 Editing Kit
    OSVERSIONINFOW version_information;
    version_information.dwOSVersionInfoSize = sizeof(version_information);
    if (GetVersionEx(&version_information))
    {
        if (version_information.dwMajorVersion < 6)
        {
            error(2, "### ERROR this machine isn't vista!");
            
            // By default it quits but let's ignore this for now...
            /*
            main_quit();
            result = false;
            */
        }
        else
        {
            // Enable d3d9ex if we don't force it off in H2Config and if dx9on12 is not enabled
            // Using d3d9ex while dx9on12 is enabled crashes the game on Nvidia cards
            result = !H2Config_force_off_d3d9ex && !g_rasterizer_dx9on12_enabled;
        }
    }

    return result;
}

static bool rasterizer_dx9_get_create_ex_proc(void)
{
    bool result = true;

    const HMODULE module = LoadLibrary(L"d3d9");
    if (module)
    {
        s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
        rasterizer_globals->d3d9_create_ex_proc = (decltype(Direct3DCreate9Ex)*)GetProcAddress(module, "Direct3DCreate9Ex");
        if (!rasterizer_globals->d3d9_create_ex_proc)
        {
            error(2, "### ERROR failed to obtain Direct3DCreate9Ex function pointer");
            /*
            main_quit();
            result = false;
            */
        }
    }
    else
    {
        error(2, "### ERROR failed to dynamically load d3d9.dll");
        main_quit();
        result = false;
    }
    return result;
}

static bool rasterizer_dx9_create_device_interface(void)
{
    bool result = true;

    s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
    s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();

    HRESULT hr;
    if (g_rasterizer_dx9on12_enabled)
    {
        hr = rasterizer_dx9_create_through_d3d9on12(&dx9_globals->global_d3d_interface, false /*use_warp*/);
        if (FAILED(hr))
        {
            error(2, "### ERROR failed to create D3D object through d3d9on12");
            main_quit();
            result = false;
        }
    }
    else if (rasterizer_globals->use_d3d9_ex)
    {
        hr = rasterizer_globals->d3d9_create_ex_proc(D3D_SDK_VERSION, &dx9_globals->global_d3d_interface);
        if (FAILED(hr))
        {
            error(2, "### ERROR failed to create D3D object with the Ex version of D3D");
            main_quit();
            result = false;
        }
    }
    else
    {
        dx9_globals->global_d3d_interface = (IDirect3D9Ex*)Direct3DCreate9(D3D_SDK_VERSION);
    }

    if (dx9_globals->global_d3d_interface == NULL)
    {
        error(3, "### ERROR failed to create D3D object");
        main_quit();
        result = false;
    }

    return result;
}

static bool rasterizer_dx9_is_amd_or_ati_card(void)
{
    bool result = false;
    s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();

    D3DADAPTER_IDENTIFIER9 identifier;
    dx9_globals->global_d3d_interface->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &identifier);
    
    if (StrStrIA(identifier.Description, "amd") != NULL || 
        StrStrIA(identifier.Description, "ati") != NULL ||
        StrStrIA(identifier.Description, "radeon") != NULL)
    {
        LOG_INFO_GAME("AMD or ATI card in use.");
        result = true;
    }

    return result;
}
