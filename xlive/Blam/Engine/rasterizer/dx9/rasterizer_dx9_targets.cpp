#include "stdafx.h"
#include "rasterizer_dx9_targets.h"

#include "rasterizer_dx9_main.h"

#include "camera/camera.h"
#include "main/main_screenshot.h"
#include "rasterizer/rasterizer_globals.h"


/* constants */

// This is no longer used
// #define USE_CUBEMAP_TARGETS

/* typedefs */

typedef void(__cdecl* rasterizer_dx9_set_target_t)(e_rasterizer_target, int32, bool);
typedef void(__cdecl* rasterizer_dx9_set_target_as_texture_t)(int16, e_rasterizer_target);
typedef bool(__cdecl* rasterizer_dx9_set_render_target_internal_t)(IDirect3DSurface9*, IDirect3DSurface9*, bool);

typedef bool(__cdecl* rasterizer_dx9_init_t)(void);
typedef void(__cdecl* rasterizer_dx9_dispose_t)(void);
typedef void(__cdecl* rasterizer_dx9_get_render_target_surface_t)(e_rasterizer_target, int16);

/* globals */

rasterizer_dx9_set_target_t p_rasterizer_dx9_set_target;
rasterizer_dx9_set_target_as_texture_t p_rasterizer_dx9_set_target_as_texture;
rasterizer_dx9_set_render_target_internal_t p_rasterizer_dx9_set_render_target_internal;

rasterizer_dx9_init_t p_rasterizer_dx9_primary_targets_initialize;
rasterizer_dx9_init_t p_rasterizer_dx9_secondary_targets_initialize;
rasterizer_dx9_dispose_t p_rasterizer_dx9_primary_targets_dispose;
rasterizer_dx9_dispose_t p_rasterizer_dx9_secondary_targets_dispose;
rasterizer_dx9_get_render_target_surface_t p_rasterizer_dx9_get_render_target_surface;

IDirect3DTexture9* g_d3d_texture_render_z_as_target_z = NULL;

real32 g_sun_size = 0.2f;

bool g_d3d_target_use_depth = false;
bool g_dx9_dont_draw_to_depth_target_if_mrt_is_used = false;

/* prototypes */

IDirect3DPixelShader9** global_d3d_shader_white_source_get(void);

uint32* motion_sensor_texture_size_get(void);
IDirect3DTexture9* rasterizer_dx9_get_target_texture(e_rasterizer_target rasterizer_target);
IDirect3DSurface9* __cdecl rasterizer_dx9_get_target_mip_surface(e_rasterizer_target target, int16 mip);
void __cdecl rasterizer_set_render_target_internal_hook_set_main_render_surface(IDirect3DSurface9* target, IDirect3DSurface9* z_stencil, bool a3);

/* public code */

void rasterizer_dx9_targets_apply_patches(void)
{
    DETOUR_ATTACH(p_rasterizer_dx9_set_target, Memory::GetAddress<decltype(rasterizer_dx9_set_target)*>(0x25FDC0), rasterizer_dx9_set_target);
    DETOUR_ATTACH(p_rasterizer_dx9_set_target_as_texture, Memory::GetAddress<rasterizer_dx9_set_target_as_texture_t>(0x260256), rasterizer_dx9_set_target_as_texture);
    DETOUR_ATTACH(p_rasterizer_dx9_set_render_target_internal, Memory::GetAddress<rasterizer_dx9_set_render_target_internal_t>(0x26EBF8), rasterizer_dx9_set_render_target_internal);

    // fix the missing motion sensor blip
    PatchCall(Memory::GetAddress(0x284BF3), rasterizer_set_render_target_internal_hook_set_main_render_surface);

    DETOUR_ATTACH(p_rasterizer_dx9_primary_targets_initialize, Memory::GetAddress<rasterizer_dx9_init_t>(0x25F902), rasterizer_dx9_primary_targets_initialize);
    DETOUR_ATTACH(p_rasterizer_dx9_secondary_targets_initialize, Memory::GetAddress<rasterizer_dx9_init_t>(0x2628EB), rasterizer_dx9_secondary_targets_initialize);
    DETOUR_ATTACH(p_rasterizer_dx9_primary_targets_dispose, Memory::GetAddress<rasterizer_dx9_dispose_t>(0x25FAC1), rasterizer_dx9_primary_targets_dispose);
    DETOUR_ATTACH(p_rasterizer_dx9_secondary_targets_dispose, Memory::GetAddress<rasterizer_dx9_dispose_t>(0x262B00), rasterizer_dx9_secondary_targets_dispose);
    DETOUR_ATTACH(p_rasterizer_dx9_get_render_target_surface, Memory::GetAddress<rasterizer_dx9_get_render_target_surface_t>(0x25FB67), rasterizer_dx9_get_render_target_surface);

    // split-screen bloom fixes
    PatchCall(Memory::GetAddress(0x26C5E2), rasterizer_set_render_target_internal_hook_set_viewport);
    PatchCall(Memory::GetAddress(0x26C761), rasterizer_set_render_target_internal_hook_set_viewport);
    // discard unnecessary set render target call(s)
    NopFill(Memory::GetAddress(0x26C81A), 5);

    // motion sensor
    PatchCall(Memory::GetAddress(0x284BF3), rasterizer_set_render_target_internal_hook_set_viewport);

    return;
}

uint32* cubemap_target_size_get(void)
{
    return Memory::GetAddress<uint32*>(0x41F674);
}

e_rasterizer_target* rasterizer_dx9_main_render_target_get(void)
{
	return Memory::GetAddress<e_rasterizer_target*>(0xA3E228);
}

s_rasterizer_target* rasterizer_dx9_texture_target_get(e_rasterizer_target texture_target)
{
	ASSERT(VALID_INDEX(texture_target, k_rasterizer_target_count));
	return &Memory::GetAddress<s_rasterizer_target*>(0xA8D658)[texture_target];
}

void rasterizer_dx9_copy_target(e_rasterizer_target src_target, e_rasterizer_target dst_target)
{
    IDirect3DSurface9* dst_surface = rasterizer_dx9_target_get_main_mip_surface(dst_target);
    IDirect3DSurface9* src_surface = rasterizer_dx9_target_get_main_mip_surface(src_target);
    rasterizer_dx9_device_get_interface()->StretchRect(src_surface, NULL, dst_surface, NULL, D3DTEXF_POINT);
    return;
}

void __cdecl rasterizer_dx9_staged_texture_surface_size(int32 texture_stage, uint32* out_width, uint32* out_height)
{
    return INVOKE(0x25F58D, 0x0, rasterizer_dx9_staged_texture_surface_size, texture_stage, out_width, out_height);
}

IDirect3DSurface9* rasterizer_dx9_target_get_main_mip_surface(e_rasterizer_target rasterizer_target)
{
    s_rasterizer_target* target = rasterizer_dx9_texture_target_get(rasterizer_target);
    return ((target->d3d_texture_storage != NULL && !target->unk_bool_20 && target->mip_count > 0) ? target->d3d_surface_storage[0] : NULL);
}

bool __cdecl rasterizer_dx9_set_render_target_internal(IDirect3DSurface9* target, IDirect3DSurface9* z_stencil, bool use_depth)
{
    IDirect3DDevice9Ex* global_d3d_device = rasterizer_dx9_device_get_interface();

    IDirect3DSurface9** last_target = rasterizer_dx9_last_target_get();
    IDirect3DSurface9** last_z_target = rasterizer_dx9_last_z_target_get();
    IDirect3DSurface9** last_z_stencil = rasterizer_dx9_last_z_stencil_get();

    bool valid = true;
    if (target != (IDirect3DSurface9*)NONE && *last_target != target)
    {
        valid = SUCCEEDED(global_d3d_device->SetRenderTarget(0, target));
        *last_target = target;
    }

    bool z_target_set = false;
    if (*global_rasterizer_pixel_shader_index_get() == 2)
    {
        const s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();
        if (target == dx9_globals->global_d3d_surface_render_primary)
        {
            // Set the target to scratch if we don't want to draw to the depth target for specific render layers
            *last_z_target = g_dx9_dont_draw_to_depth_target_if_mrt_is_used ? NULL : dx9_globals->global_d3d_surface_render_z_as_target_z;

            const HRESULT hr = global_d3d_device->SetRenderTarget(1, *last_z_target);
            valid = valid && SUCCEEDED(hr);
            z_target_set = true;
        }
    }

    if (!z_target_set && *last_z_target != NULL)
    {
        *last_z_target = NULL;
        HRESULT hr = global_d3d_device->SetRenderTarget(1, NULL);
        valid &= SUCCEEDED(hr);
    }

    bool result;
    g_d3d_target_use_depth = use_depth;
    if (z_stencil == (IDirect3DSurface9*)NONE || *last_z_stencil == z_stencil)
    {
        result = valid;
    }
    else
    {
        HRESULT hr = global_d3d_device->SetDepthStencilSurface(z_stencil);
        *last_z_stencil = z_stencil;
        result = valid && SUCCEEDED(hr); 
    }

    return result;
}

void __cdecl rasterizer_set_render_target_internal_hook_set_viewport(IDirect3DSurface9* target, IDirect3DSurface9* z_stencil, bool a3)
{
	const render_camera* global_camera = get_global_camera();

    // NOTE: *ONLY* use this function for entire screen surfaces (e.g the backbuffer, a render target)

	rasterizer_dx9_set_render_target_internal(target, z_stencil, a3);
	D3DVIEWPORT9 vp =
    {
		(DWORD)global_camera->viewport_bounds.left,
        (DWORD)global_camera->viewport_bounds.top,
        (DWORD)rectangle2d_width(&global_camera->viewport_bounds),
        (DWORD)rectangle2d_height(&global_camera->viewport_bounds),
        0.f,
        1.f
	};
	// set the viewport, after setting the main render target
	// to note that the viewport will always gets reset when a new render target is set
    rasterizer_dx9_device_get_interface()->SetViewport(&vp);
	return;
}

e_rasterizer_target __cdecl rasterizer_dx9_get_overlay_destination_target(void)
{
	return INVOKE(0x19DAFB, 0x0, rasterizer_dx9_get_overlay_destination_target);
}

IDirect3DSurface9* __cdecl rasterizer_dx9_get_render_target_surface(e_rasterizer_target rasterizer_target, int16 mipmap_index)
{
    const s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();
    IDirect3DSurface9* result;

    switch (rasterizer_target)
    {
    case _rasterizer_target_render_primary:
    case _rasterizer_target_frontbuffer:
        result = dx9_globals->global_d3d_surface_render_primary;
        break;
    case _rasterizer_target_sun_glow_primary:
        result = dx9_globals->global_d3d_surface_sun_glow_primary;
        break;
    case _rasterizer_target_sun_glow_secondary:
        result = dx9_globals->global_d3d_surface_sun_glow_secondary;
        break;
    case _rasterizer_target_motion_sensor:
        result = dx9_globals->global_d3d_surface_motion_sensor;
        break;
#ifdef USE_CUBEMAP_TARGETS
    case _rasterizer_target_cubemap_pos_x:
        result = dx9_globals->global_d3d_surface_cubemap_pos_x;
        break;
    case _rasterizer_target_cubemap_neg_x:
        result = dx9_globals->global_d3d_surface_cubemap_neg_x;
        break;
    case _rasterizer_target_cubemap_pos_y:
        result = dx9_globals->global_d3d_surface_cubemap_pos_y;
        break;
    case _rasterizer_target_cubemap_neg_y:
        result = dx9_globals->global_d3d_surface_cubemap_neg_y;
        break;
    case _rasterizer_target_cubemap_pos_z:
        result = dx9_globals->global_d3d_surface_cubemap_pos_z;
        break;
    case _rasterizer_target_cubemap_neg_z:
        result = dx9_globals->global_d3d_surface_cubemap_neg_z;
        break;
#endif
    case _rasterizer_target_resolved:
        result = dx9_globals->global_d3d_surface_render_resolved;
        break;
    default:
        result = rasterizer_dx9_get_target_mip_surface(rasterizer_target, mipmap_index);
        break;
    }
    return result;
}

bool __cdecl rasterizer_dx9_set_target_as_texture(int16 stage, e_rasterizer_target rasterizer_target)
{   
    const s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();

    IDirect3DTexture9* d3d_texture = NULL;
    uint32 width;
    uint32 height;

    ASSERT(VALID_INDEX(stage, k_max_texture_stages));
    rasterizer_target_get_resolution(rasterizer_target, &width, &height);
    rasterizer_dx9_texture_stage_dimensions(stage, width, height);

    switch (rasterizer_target)
    {
    case _rasterizer_target_render_primary:
    case _rasterizer_target_resolved:
        d3d_texture = dx9_globals->global_d3d_texture_render_resolved;
        break;
    case _rasterizer_target_texaccum:
        d3d_texture = rasterizer_dx9_get_target_texture(_rasterizer_target_texaccum);
        break;
    case _rasterizer_target_backbuffer:
        d3d_texture = dx9_globals->global_d3d_texture_backbuffer;
        break;
    case _rasterizer_target_frontbuffer:
        DISPLAY_ASSERT("###ERROR### attempt to set front buffer as a texture");
        break;
    case _rasterizer_target_bloom_primary:
    case _rasterizer_target_bloom_secondary:
    case _rasterizer_target_shadow_buffer:
    case _rasterizer_target_shadow:
    case _rasterizer_target_shadow_scratch1:
    case _rasterizer_target_shadow_scratch2:
    case _rasterizer_target_gamma_remap_lut:
    case _rasterizer_target_water_reflection:
    case _rasterizer_target_water_refraction:
    case _rasterizer_target_convolution_scratch1:
    case _rasterizer_target_convolution_scratch2:
    case _rasterizer_target_scratch:
        d3d_texture = rasterizer_dx9_get_target_texture(rasterizer_target);
        break;
    case _rasterizer_target_shadow_alias_swizzled:
        d3d_texture = rasterizer_dx9_get_target_texture(_rasterizer_target_shadow_alias_swizzled);
        break;
    case _rasterizer_target_dynamic_gamma:
        DISPLAY_ASSERT("_rasterizer_target_dynamic_gamma is no longer available");
        break;
    case _rasterizer_target_sun_glow_primary:
        d3d_texture = dx9_globals->global_d3d_texture_sun_glow_primary;
        break;
    case _rasterizer_target_sun_glow_secondary:
        d3d_texture = dx9_globals->global_d3d_texture_sun_glow_secondary;
        break;
    case _rasterizer_target_cinematic:
        d3d_texture = rasterizer_dx9_get_target_texture(_rasterizer_target_cinematic);
        break;
    case _rasterizer_target_active_camo:
        d3d_texture = rasterizer_dx9_get_target_texture(_rasterizer_target_active_camo);
        break;
    case _rasterizer_target_texture_camera:
        d3d_texture = rasterizer_dx9_get_target_texture(_rasterizer_target_texture_camera);
        break;
    case _rasterizer_target_particle_distortion:
        d3d_texture = rasterizer_dx9_get_target_texture(_rasterizer_target_particle_distortion);
        break;
    case _rasterizer_target_z_a8b8g8r8:
        d3d_texture = g_d3d_texture_render_z_as_target_z;
        break;
    case _rasterizer_target_motion_sensor:
        d3d_texture = dx9_globals->global_d3d_texture_motion_sensor;
        break;
#ifdef USE_CUBEMAP_TARGETS
    case _rasterizer_target_cubemap_pos_x:
    case _rasterizer_target_cubemap_neg_x:
    case _rasterizer_target_cubemap_pos_y:
    case _rasterizer_target_cubemap_neg_y:
    case _rasterizer_target_cubemap_pos_z:
    case _rasterizer_target_cubemap_neg_z:
        target_texture = dx9_globals->global_d3d_texture_cubemap;
        break;
#endif
    default:
        DISPLAY_ASSERT("unreachable");
        break;
    }

    return rasterizer_dx9_device_set_texture(stage, d3d_texture);
}

bool rasterizer_dx9_set_target_as_texture_internal(int16 stage, e_rasterizer_target rasterizer_target, IDirect3DTexture9* d3d_texture)
{
    uint32 resolution_x;
    uint32 resolution_y;
    rasterizer_target_get_resolution(rasterizer_target, &resolution_x, &resolution_y);
    rasterizer_dx9_texture_stage_dimensions(stage, resolution_x, resolution_y);    
    return rasterizer_dx9_device_set_texture(stage, d3d_texture);
}

void __cdecl rasterizer_dx9_set_target(e_rasterizer_target rasterizer_target, int32 mipmap_index, bool use_depth)
{
    const s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
    const s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();

    IDirect3DSurface9* d3d_surface = NULL;
    IDirect3DSurface9* d3d_depth_buffer_stencil = NULL;
    bool valid_target = true;
    bool depth_provided = false;

    int32 g_ps_index = *global_rasterizer_pixel_shader_index_get();
    rasterizer_target = (g_ps_index == 1 ? _rasterizer_target_backbuffer : rasterizer_target);
    switch (rasterizer_target)
    {
    case _rasterizer_target_render_primary:
        depth_provided = true;
        if (rasterizer_globals->rasterizer_draw_on_main_back_buffer)
        {
            if (screenshot_in_progress())
            {
                d3d_surface = dx9_globals->global_d3d_surface_screenshot;
            }
            else
            {
                dx9_globals->global_d3d_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &d3d_surface);
                d3d_surface->Release();
            }

            d3d_depth_buffer_stencil = NULL;
        }
        else
        {
            d3d_surface = dx9_globals->global_d3d_surface_render_primary;
            d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_render_primary_z;
        }
        break;
    case _rasterizer_target_texaccum:
    case _rasterizer_target_bloom_primary:
    case _rasterizer_target_bloom_secondary:
    case _rasterizer_target_gamma_remap_lut:
    case _rasterizer_target_scratch:
        d3d_surface = rasterizer_dx9_target_get_main_mip_surface(rasterizer_target);
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_render_primary_z;
        break;
    case _rasterizer_target_frontbuffer:
        DISPLAY_ASSERT("###ERROR### attempt to set front buffer as the render target");
        valid_target = false;
        break;
    case _rasterizer_target_backbuffer:
        d3d_surface = dx9_globals->global_d3d_surface_backbuffer;
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_render_z_as_target_z;
        depth_provided = true;
        break;
    case _rasterizer_target_shadow_buffer:
        d3d_surface = rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_shadow_buffer);
        d3d_depth_buffer_stencil = rasterizer_dx9_get_target_mip_surface(_rasterizer_target_shadow_buffer, 1);
        break;
    case _rasterizer_target_shadow:
    case _rasterizer_target_shadow_scratch1:
    case _rasterizer_target_shadow_scratch2:
    case _rasterizer_target_convolution_scratch1:
    case _rasterizer_target_convolution_scratch2:
        d3d_surface = rasterizer_dx9_target_get_main_mip_surface(rasterizer_target);
        break;
    case _rasterizer_target_shadow_alias_swizzled:
        d3d_surface = rasterizer_dx9_get_target_mip_surface(_rasterizer_target_shadow_alias_swizzled, (int16)mipmap_index);
        break;
    case _rasterizer_target_dynamic_gamma:
        DISPLAY_ASSERT("_rasterizer_target_dynamic_gamma is no longer available");
        valid_target = false;
        break;
    case _rasterizer_target_sun_glow_primary:
        d3d_surface = dx9_globals->global_d3d_surface_sun_glow_primary;
        break;
    case _rasterizer_target_sun_glow_secondary:
        d3d_surface = dx9_globals->global_d3d_surface_sun_glow_secondary;
        break;
    case _rasterizer_target_cinematic:
        d3d_surface = rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_cinematic);
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_render_primary_z;
        break;
    case _rasterizer_target_active_camo:
        d3d_surface = rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_active_camo);
        break;
    case _rasterizer_target_texture_camera:
        d3d_surface = rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_texture_camera);
        break;
    case _rasterizer_target_water_reflection:
        d3d_surface = rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_water_reflection);
        break;
    case _rasterizer_target_water_refraction:
        d3d_surface = rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_water_refraction);
        break;
    case _rasterizer_target_particle_distortion:
        d3d_surface = rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_particle_distortion);
        break;
    case _rasterizer_target_motion_sensor:
        d3d_surface = dx9_globals->global_d3d_surface_motion_sensor;
        break;
#ifdef USE_CUBEMAP_TARGETS
    case _rasterizer_target_cubemap_pos_x:
        d3d_surface = dx9_globals->global_d3d_surface_cubemap_pos_x;
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_cubemap_depth;
        break;
    case _rasterizer_target_cubemap_neg_x:
        d3d_surface = dx9_globals->global_d3d_surface_cubemap_neg_x;
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_cubemap_depth;
        break;
    case _rasterizer_target_cubemap_pos_y:
        d3d_surface = dx9_globals->global_d3d_surface_cubemap_pos_y;
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_cubemap_depth;
        break;
    case _rasterizer_target_cubemap_neg_y:
        d3d_surface = dx9_globals->global_d3d_surface_cubemap_neg_y;
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_cubemap_depth;
        break;
    case _rasterizer_target_cubemap_pos_z:
        d3d_surface = dx9_globals->global_d3d_surface_cubemap_pos_z;
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_cubemap_depth;
        break;
    case _rasterizer_target_cubemap_neg_z:
        d3d_surface = dx9_globals->global_d3d_surface_cubemap_neg_z;
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_cubemap_depth;
        break;
#endif
    case _rasterizer_target_resolved:
        d3d_surface = dx9_globals->global_d3d_surface_render_resolved;
        d3d_depth_buffer_stencil = dx9_globals->global_d3d_surface_render_primary_z;
        depth_provided = true;
        break;
    default:
        DISPLAY_ASSERT("unreachable");
        valid_target = false;
    }

    if (valid_target && d3d_surface)
    {
        const s_frame* global_window_parameters = global_window_parameters_get();
        real32 viewport_scale;

        D3DVIEWPORT9 viewport;
        // These are always the same
        viewport.MinZ = 0.f;
        viewport.MaxZ = 1.f;

        rasterizer_dx9_set_render_target_internal(d3d_surface, (use_depth ? d3d_depth_buffer_stencil : NULL), depth_provided);

        // Change viewport parameters based on circumstances

        // If we're rendering a cubemap
        if (global_window_parameters->render_type == 2)
        {
            uint32 g_cubemap_resolution = *cubemap_target_size_get();
            viewport.X = 0;
            viewport.Y = 0;
            viewport.Width = g_cubemap_resolution;
            viewport.Height = g_cubemap_resolution;
        }
        else if (
            rasterizer_target == _rasterizer_target_render_primary ||
            rasterizer_target == _rasterizer_target_resolved ||
            rasterizer_target == _rasterizer_target_2 ||
            rasterizer_target == _rasterizer_target_texaccum ||
            rasterizer_target == _rasterizer_target_cinematic ||
            rasterizer_target == _rasterizer_target_backbuffer ||   // For when the depth is rendered to the backbuffer
            rasterizer_target == rasterizer_dx9_get_overlay_destination_target())
        {
            const int16 viewport_width = rectangle2d_width(&global_window_parameters->camera.viewport_bounds);
            const int16 viewport_height = rectangle2d_height(&global_window_parameters->camera.viewport_bounds);
            if (rasterizer_target == rasterizer_dx9_get_overlay_destination_target())
            {
                if (global_window_parameters->is_texture_camera)
                {
                    viewport.X = 0;
                    viewport.Y = 0;
                    viewport.Width = viewport_width;
                    viewport.Height = viewport_height;
                }
            }
            else if (global_window_parameters->is_texture_camera && !*hs_texture_camera_view_get())
            {
                viewport_scale = PIN(*hs_texture_camera_scale_get(), 0.0625f, 1.f);
                viewport.X = (DWORD)(global_window_parameters->camera.viewport_bounds.left * viewport_scale);
                viewport.Y = (DWORD)(global_window_parameters->camera.viewport_bounds.top * viewport_scale);
                viewport.Width = (DWORD)(viewport_width * viewport_scale);
                viewport.Height = (DWORD)(viewport_height * viewport_scale);
            }
            else
            {
                viewport_scale = 1.0;
                viewport.X = (DWORD)(global_window_parameters->camera.viewport_bounds.left * viewport_scale);
                viewport.Y = (DWORD)(global_window_parameters->camera.viewport_bounds.top * viewport_scale);
                viewport.Width = (DWORD)(viewport_width * viewport_scale);
                viewport.Height = (DWORD)(viewport_height * viewport_scale);
            }
        }
        // By default set the viewport as the desc params
        else 
        {
            D3DSURFACE_DESC desc;
            d3d_surface->GetDesc(&desc);
            viewport.X = 0;
            viewport.Y = 0;
            viewport.Width = desc.Width;
            viewport.Height = desc.Height;
        }
       
        dx9_globals->global_d3d_device->SetViewport(&viewport);
    }
    else
    {
        error(2, "### ERROR %s failed", __FUNCTION__);
    }

    return;
}

bool __cdecl rasterizer_target_get_resolution(e_rasterizer_target rasterizer_target, uint32* resolution_x, uint32* resolution_y)
{
    return INVOKE(0x280413, 0x0, rasterizer_target_get_resolution, rasterizer_target, resolution_x, resolution_y);
}

IDirect3DSurface9** rasterizer_dx9_last_target_get(void)
{
    return Memory::GetAddress<IDirect3DSurface9**>(0xA4B18C);
}

IDirect3DSurface9** rasterizer_dx9_last_z_target_get(void)
{
    return Memory::GetAddress<IDirect3DSurface9**>(0xA4B190);
}

IDirect3DSurface9** rasterizer_dx9_last_z_stencil_get(void)
{
    return Memory::GetAddress<IDirect3DSurface9**>(0xA4B194);
}

bool __cdecl rasterizer_dx9_primary_targets_initialize(void)
{
    s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();
    const s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
    IDirect3DDevice9Ex* global_d3d_device = rasterizer_dx9_device_get_interface();

    const int16 screen_bounds_width = rectangle2d_width(&rasterizer_globals->screen_bounds);
    const int16 screen_bounds_height = rectangle2d_height(&rasterizer_globals->screen_bounds);

    bool success = false;
    const D3DFORMAT backbuffer_fmt = rasterizer_globals->display_parameters.backbuffer_format;

    if (SUCCEEDED(global_d3d_device->CreateTexture(
        screen_bounds_width,
        screen_bounds_height,
        1,
        D3DUSAGE_RENDERTARGET,
        backbuffer_fmt,
        D3DPOOL_DEFAULT,
        &dx9_globals->global_d3d_texture_render_resolved,
        NULL)))
    {
        const D3DMULTISAMPLE_TYPE type = rasterizer_globals->d3d9_sm3_supported ? D3DMULTISAMPLE_NONE : dx9_globals->global_d3d_primary_multisampletype;
        const uint32 quality = rasterizer_globals->d3d9_sm3_supported ? 0 : dx9_globals->global_d3d_primary_multisamplequality;

        const D3DFORMAT depthstencil_fmt = rasterizer_globals->display_parameters.depthstencil_format;

        success =
            SUCCEEDED(dx9_globals->global_d3d_texture_render_resolved->GetSurfaceLevel(0, &dx9_globals->global_d3d_surface_render_resolved)) &&
            SUCCEEDED(global_d3d_device->CreateRenderTarget(
                screen_bounds_width,
                screen_bounds_height,
                backbuffer_fmt,
                type,
                quality,
                false,
                &dx9_globals->global_d3d_surface_render_primary,
                NULL)) &&
            SUCCEEDED(global_d3d_device->CreateDepthStencilSurface(
                screen_bounds_width,
                screen_bounds_height,
                depthstencil_fmt,
                type,
                quality,
                false,
                &dx9_globals->global_d3d_surface_render_primary_z,
                NULL));
    }

    const char white_source[] = "float4 main() : COLOR { return float4(1.0f, 1.0f, 0.0f, 1.0f); }";
    const char* shader_version = rasterizer_globals->d3d9_sm3_supported ? "ps_3_0" : "ps_2_0";


    LPD3DXBUFFER pBuffer;
    const HRESULT hr = D3DXCompileShader(white_source, strlen(white_source), NULL, NULL, "main", shader_version, 0, &pBuffer, NULL, NULL);

    bool result = false;
    if (pBuffer)
    {
        const HRESULT ps_hr = global_d3d_device->CreatePixelShader((const DWORD*)pBuffer->GetBufferPointer(), global_d3d_shader_white_source_get());
        pBuffer->Release();

        result = success && SUCCEEDED(hr) && SUCCEEDED(ps_hr);
    }

    return result;
}

void __cdecl rasterizer_dx9_primary_targets_dispose(void)
{
    s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();

    IDirect3DPixelShader9** global_d3d_shader_white_source = global_d3d_shader_white_source_get();
    if (*global_d3d_shader_white_source)
    {
        (*global_d3d_shader_white_source)->Release();
        *global_d3d_shader_white_source = NULL;
    }
    if (dx9_globals->global_d3d_surface_screenshot)
    {
        dx9_globals->global_d3d_surface_screenshot->Release();
        dx9_globals->global_d3d_surface_screenshot = NULL;
    }

    if (dx9_globals->global_d3d_texture_render_resolved)
    {
        dx9_globals->global_d3d_texture_render_resolved->Release();
        dx9_globals->global_d3d_texture_render_resolved = NULL;
    }
    if (dx9_globals->global_d3d_surface_render_resolved)
    {
        dx9_globals->global_d3d_surface_render_resolved->Release();
        dx9_globals->global_d3d_surface_render_resolved = NULL;
    }

    if (dx9_globals->global_d3d_surface_render_primary)
    {
        dx9_globals->global_d3d_surface_render_primary->Release();
        dx9_globals->global_d3d_surface_render_primary = NULL;
    }
    if (dx9_globals->global_d3d_surface_render_primary_z)
    {
        dx9_globals->global_d3d_surface_render_primary_z->Release();
        dx9_globals->global_d3d_surface_render_primary_z = NULL;
    }
    return;
}

bool __cdecl rasterizer_dx9_secondary_targets_initialize(void)
{
    s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();
    const s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
    IDirect3DDevice9Ex* global_d3d_device = rasterizer_dx9_device_get_interface();

    const int16 screen_bounds_width = rectangle2d_width(&rasterizer_globals->screen_bounds);
    const int16 screen_bounds_height = rectangle2d_height(&rasterizer_globals->screen_bounds);

    bool succeeded = SUCCEEDED(global_d3d_device->CreateTexture(
        screen_bounds_width,
        screen_bounds_height,
        1,
        D3DUSAGE_RENDERTARGET,
        rasterizer_globals->display_parameters.backbuffer_format,
        D3DPOOL_DEFAULT,
        &dx9_globals->global_d3d_texture_backbuffer,
        NULL));

    HRESULT hr;
    if (succeeded)
    {
        if (dx9_globals->global_d3d_texture_backbuffer->GetSurfaceLevel(0, &dx9_globals->global_d3d_surface_backbuffer) >= 0)
        {
            const bool sm3_supported = rasterizer_globals->d3d9_sm3_supported;

            const D3DFORMAT format = (sm3_supported ? rasterizer_globals->display_parameters.backbuffer_format : rasterizer_globals->display_parameters.depthstencil_format);
            const D3DMULTISAMPLE_TYPE type = D3DMULTISAMPLE_NONE;
            const uint32 quality = 0;

            // If shader model 3 is supported we create the z target as a texture rather than a depth stencil surface
            // We do this as the game will render out the z target at the same time as the lightmap indirect stage when shader model 3 is in use
            if (sm3_supported)
            {
                succeeded = SUCCEEDED(global_d3d_device->CreateTexture(
                    screen_bounds_width,
                    screen_bounds_height,
                    1,
                    D3DUSAGE_RENDERTARGET,
                    rasterizer_globals->display_parameters.backbuffer_format,
                    D3DPOOL_DEFAULT,
                    &g_d3d_texture_render_z_as_target_z,
                    NULL));

                // Get texture of the render target from the render target surface
                succeeded &= SUCCEEDED(g_d3d_texture_render_z_as_target_z->GetSurfaceLevel(0, &dx9_globals->global_d3d_surface_render_z_as_target_z));
            }
            else
            {
                hr = global_d3d_device->CreateDepthStencilSurface(
                    screen_bounds_width,
                    screen_bounds_height,
                    format,
                    type,
                    quality,
                    false,
                    &dx9_globals->global_d3d_surface_render_z_as_target_z,
                    NULL);
                succeeded = SUCCEEDED(hr);
            }
        }
        else
        {
            succeeded = false;
        }
    }

    uint32 g_motion_sensor_texture_size = *motion_sensor_texture_size_get();
    rasterizer_dx9_create_texture(g_motion_sensor_texture_size,
        g_motion_sensor_texture_size,
        0,
        1,
        _bitmap_format_a8r8g8b8,
        false,
        &dx9_globals->global_d3d_texture_motion_sensor);

    bool result = false;
    if (dx9_globals->global_d3d_texture_motion_sensor)
    {
        hr = dx9_globals->global_d3d_texture_motion_sensor->GetSurfaceLevel(0, &dx9_globals->global_d3d_surface_motion_sensor);
        result = succeeded && SUCCEEDED(hr) && dx9_globals->global_d3d_surface_motion_sensor != NULL;
    }

    // We calculate the sun glow target width and height based on the viewport resolution
    const s_frame* global_window_parameters = global_window_parameters_get();
    const int16 viewport_width = rectangle2d_width(&global_window_parameters->camera.viewport_bounds);
    const int16 viewport_height = rectangle2d_height(&global_window_parameters->camera.viewport_bounds);

    D3DVIEWPORT9 d3d_viewport;
    dx9_globals->global_d3d_device->GetViewport(&d3d_viewport);

    dx9_globals->global_d3d_sun_width = (uint32)((d3d_viewport.Height * g_sun_size) * rasterizer_globals->sun_width_scale);
    dx9_globals->global_d3d_sun_height = (uint32)(d3d_viewport.Height * g_sun_size);

    result &=
        rasterizer_dx9_create_texture(
            dx9_globals->global_d3d_sun_width,
            dx9_globals->global_d3d_sun_height,
            0,
            D3DUSAGE_RENDERTARGET,
            _bitmap_format_a8r8g8b8,
            0,
            &dx9_globals->global_d3d_texture_sun_glow_primary) &&
        SUCCEEDED(dx9_globals->global_d3d_texture_sun_glow_primary->GetSurfaceLevel(0, &dx9_globals->global_d3d_surface_sun_glow_primary)) &&

        rasterizer_dx9_create_texture(
            dx9_globals->global_d3d_sun_width,
            dx9_globals->global_d3d_sun_height,
            0,
            D3DUSAGE_RENDERTARGET,
            _bitmap_format_a8r8g8b8,
            0,
            &dx9_globals->global_d3d_texture_sun_glow_secondary) &&
        SUCCEEDED(dx9_globals->global_d3d_texture_sun_glow_secondary->GetSurfaceLevel(0, &dx9_globals->global_d3d_surface_sun_glow_secondary));

    // Fail if any of the textures or surfaces are null
    if (!dx9_globals->global_d3d_texture_sun_glow_primary ||
        !dx9_globals->global_d3d_surface_sun_glow_primary ||
        !dx9_globals->global_d3d_texture_sun_glow_secondary ||
        !dx9_globals->global_d3d_surface_sun_glow_secondary)
    {
        result = false;
        error(2, "### ERROR rasterizer_secondary_targets_initialize() failed to create offscreen surface(s)");
    }

    return result;
}

void __cdecl rasterizer_dx9_secondary_targets_dispose(void)
{
    s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();
    if (dx9_globals->global_d3d_surface_motion_sensor)
    {
        dx9_globals->global_d3d_surface_motion_sensor->Release();
        dx9_globals->global_d3d_surface_motion_sensor = NULL;
    }
    if (dx9_globals->global_d3d_texture_motion_sensor)
    {
        dx9_globals->global_d3d_texture_motion_sensor->Release();
        dx9_globals->global_d3d_texture_motion_sensor = NULL;
    }

    if (dx9_globals->global_d3d_surface_sun_glow_primary)
    {
        dx9_globals->global_d3d_surface_sun_glow_primary->Release();
        dx9_globals->global_d3d_surface_sun_glow_primary = NULL;
    }
    if (dx9_globals->global_d3d_surface_sun_glow_secondary)
    {
        dx9_globals->global_d3d_surface_sun_glow_secondary->Release();
        dx9_globals->global_d3d_surface_sun_glow_secondary = NULL;
    }

    if (dx9_globals->global_d3d_texture_sun_glow_primary)
    {
        dx9_globals->global_d3d_texture_sun_glow_primary->Release();
        dx9_globals->global_d3d_texture_sun_glow_primary = NULL;
    }
    if (dx9_globals->global_d3d_texture_sun_glow_secondary)
    {
        dx9_globals->global_d3d_texture_sun_glow_secondary->Release();
        dx9_globals->global_d3d_texture_sun_glow_secondary = NULL;
    }

    if (dx9_globals->global_d3d_surface_backbuffer)
    {
        dx9_globals->global_d3d_surface_backbuffer->Release();
        dx9_globals->global_d3d_surface_backbuffer = NULL;
    }
    if (dx9_globals->global_d3d_texture_backbuffer)
    {
        dx9_globals->global_d3d_texture_backbuffer->Release();
        dx9_globals->global_d3d_texture_backbuffer = NULL;
    }

    if (dx9_globals->global_d3d_surface_render_z_as_target_z)
    {
        dx9_globals->global_d3d_surface_render_z_as_target_z->Release();
        dx9_globals->global_d3d_surface_render_z_as_target_z = NULL;
    }

    if (g_d3d_texture_render_z_as_target_z)
    {
        g_d3d_texture_render_z_as_target_z->Release();
        g_d3d_texture_render_z_as_target_z = NULL;
    }

    return;
}

bool __cdecl rasterizer_dx9_targets_initialize(void)
{
    return INVOKE(0x2804FF, 0x0, rasterizer_dx9_targets_initialize);
}

/* private code */

IDirect3DPixelShader9** global_d3d_shader_white_source_get(void)
{
    return Memory::GetAddress<IDirect3DPixelShader9**>(0x9DAAB0);
}

uint32* motion_sensor_texture_size_get(void)
{
    return Memory::GetAddress<uint32*>(0x41F694);
}

IDirect3DTexture9* rasterizer_dx9_get_target_texture(e_rasterizer_target rasterizer_target)
{
    IDirect3DTexture9* result = NULL;
    if (rasterizer_target != _rasterizer_target_none)
    {
        const s_rasterizer_target* target = rasterizer_dx9_texture_target_get(rasterizer_target);

        // If the bool is false return the texture storage
        // Otherwise, return NULL
        result = (!target->unk_bool_20 ? target->d3d_texture_storage : result);             
    }

    return result;
}

IDirect3DSurface9* __cdecl rasterizer_dx9_get_target_mip_surface(e_rasterizer_target rasterizer_target, int16 mipmap_index)
{
    IDirect3DSurface9* result = NULL;
    const s_rasterizer_target* target = rasterizer_dx9_texture_target_get(rasterizer_target);
    if (target->d3d_texture_storage && !target->unk_bool_20 && VALID_INDEX(mipmap_index, target->mip_count))
    {
        ASSERT(mipmap_index < NUMBEROF(target->d3d_surface_storage));
        result = target->d3d_surface_storage[mipmap_index];
    }

    return result;
}

void __cdecl rasterizer_set_render_target_internal_hook_set_main_render_surface(IDirect3DSurface9* target, IDirect3DSurface9* z_stencil, bool a3)
{
    rasterizer_dx9_set_target(*rasterizer_dx9_main_render_target_get(), 0, true);
    return;
}

