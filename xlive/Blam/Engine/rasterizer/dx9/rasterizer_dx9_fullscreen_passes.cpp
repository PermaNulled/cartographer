#include "stdafx.h"
#include "rasterizer_dx9_fullscreen_passes.h"

#include "rasterizer_dx9.h"
#include "rasterizer_dx9_main.h"
#include "rasterizer_dx9_screen_effect.h"
#include "rasterizer_dx9_shader_submit.h"

#include "rasterizer/rasterizer_globals.h"
#include "main/main_screenshot.h"

/* prototypes */

real32 rasterizer_dx9_fullscreen_effect_calculate_position_z(real32 z_far, int32 a2);

/* public code */

void rasterizer_dx9_fullscreen_passes_apply_patches(void)
{
    // splitscreen bloom fullscreen passes fix
    WritePointer(Memory::GetAddress(0x27D8C5) + 1, rasterizer_fullscreen_effects_build_vertex_buffer_color_ctx_cb);
    WritePointer(Memory::GetAddress(0x27D8BB) + 1, rasterizer_dx9_build_default_vertex_buffer);
    return;
}

void rasterizer_dx9_fullscreen_calculate_position(const real_vector4d* location, real32 z_far, real_vector4d* output)
{
    ASSERT(location);
    ASSERT(output);

    real32 z = (z_far != 0.f ? rasterizer_dx9_fullscreen_effect_calculate_position_z(z_far, 1) : 1.f);

    output->i = location->i * 2.f - 1.f;    // x
    output->j = -(location->j * 2.f - 1.f); // y
    output->k = z;                          // z
    output->l = 1.f;                        // scale
    return;
}

void rasterizer_dx9_fullscreen_calculate_screen_coordinates(const real_rectangle2d* bounds, const real_point2d* location, real_point2d* output)
{
    ASSERT(bounds);
    ASSERT(location);
    ASSERT(output);
    
    const real32 width = bounds->x1 - bounds->x0;
    const real32 height = bounds->y1 - bounds->y0;

    output[0].u = (0.5f / width) + location->u;
    output[0].v = (0.5f / height) + location->v;
    output[1].u = 0.f;
    output[1].v = 1.f;
    return;
}

void rasterizer_dx9_fullscreen_calculate_texcoords(const real_rectangle2d* bounds, const real_point2d* location, real_point2d* output, int32 texture_stage)
{
    ASSERT(bounds);
    ASSERT(location);
    ASSERT(output);
    
    /*
    const uint32 width = rasterizer_get_width();
    const uint32 height = rasterizer_get_height();
    const uint32 bounds_width = bounds->x1 - bounds->x0;
    const uint32 bounds_height = bounds->y1 - bounds->y0;

    output[0].u = ((bounds_width * location->x) / width) + (bounds->x0 / width);
    output[0].v = ((bounds_height * location->y) / height) + (bounds->y0 / height);
    output[1].u = 0.f;
    output[1].v = 1.f;
    */

    // texcoords normalized device coordinates (origin in 0,0 bottom left, max 1,1 top right)
    // set the tex coords based on the texture set to be drawn on the screen
	const uint32 bounds_width = (uint32)(bounds->x1 - bounds->x0);
	const uint32 bounds_height = (uint32)(bounds->y1 - bounds->y0);

    uint32 tex_width;
    uint32 tex_height;
    rasterizer_dx9_staged_texture_surface_size(texture_stage, &tex_width, &tex_height);

    // explanation: locations are per vertex, this draw call will take 4 vertices
    // this function will get called 4 times, for each single vertex
    // the location of the first vertex is at 0, 0. and according to texcoords normalized device coordinates, the origin is in bottom left corner of the texture surface
    // then it goes counter-clockwise until it forms a rectangle, so 0,0 -> 1,0 (right down) -> 1,1 (right top) -> 0,1 (left top) -> (end) 0,0 (left bottom)
    // if the location is 1, for each component we scale it with the bounds width/lenght and offset that with the verticies located on the left
    // if it is 0, then the vertex is either at the left of the screen, or at the bottom, depending on which component is 0
    output[0].u = (0.5f / bounds_width) + ((location->u * bounds_width) / tex_width) + (bounds->x0 / tex_width);
    output[0].v = (0.5f / bounds_height) + ((location->v * bounds_height) / tex_height) + (bounds->y0 / tex_height);
    output[1].u = 0.f;
    output[1].v = 1.f;
    return;
}

void __cdecl rasterizer_dx9_render_fullscreen_overlay_geometry(
    real_rectangle2d* a1,
    bool(__cdecl* a2)(void*),
    bool(__cdecl* a3)(
        int32 output_type,
        real_rectangle2d* bounds,
        real_vector4d* location,
        void* output,
        void* ctx),
    int32(__cdecl* a4)(int32),
    int32 a5,
    int16 a6,
    bool a7)
{
    INVOKE(0x27D4EF, 0x0, rasterizer_dx9_render_fullscreen_overlay_geometry, a1, a2, a3, a4, a5, a6, a7);
    return;
}

bool __cdecl rasterizer_dx9_build_default_vertex_buffer(
    int32 output_type,
    real_rectangle2d* bounds,
    real_vector4d* location,
    void* output,
    void* ctx)
{
    bool result = true;

    switch (output_type)
    {
    case _vertex_output_type_position:
        rasterizer_dx9_fullscreen_calculate_position(location, 1.0, (real_vector4d*)output);
        break;
    case _vertex_output_type_texcoord:
        rasterizer_dx9_fullscreen_calculate_screen_coordinates(bounds, (real_point2d*)location, (real_point2d*)output);
        break;
    case _vertex_output_type_color:
        *(pixel32*)output = global_white_pixel32;
        break;
    default:
        DISPLAY_ASSERT("unreachable");
        result = false;
    }

    return result;
}

void __cdecl rasterizer_dx9_apply_gamma_and_brightness(e_rasterizer_target rasterizer_target)
{
    IDirect3DSurface9* backbuffer = NULL;
    const s_frame* global_window_parameters = global_window_parameters_get();
    s_rasterizer_dx9_main_globals* dx9_globals = rasterizer_dx9_main_globals_get();

    IDirect3DDevice9Ex* global_d3d_device = rasterizer_dx9_device_get_interface();

    if (!global_window_parameters->is_texture_camera || *hs_texture_camera_view_get())
    {
        if (screenshot_in_progress())
        {
            backbuffer = dx9_globals->global_d3d_surface_screenshot;
            backbuffer->AddRef();
        }
        else
        {
            global_d3d_device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backbuffer);
        }

        const s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
        if (rasterizer_globals->fullscreen_parameters.brightness == 0.f && rasterizer_globals->fullscreen_parameters.gamma == 1.f)
        {
            IDirect3DSurface9* target_surface = rasterizer_dx9_get_render_target_surface(rasterizer_target, 0);

            RECT rect;
            rectangle2d_to_rect(&global_window_parameters->camera.viewport_bounds, &rect);

            rasterizer_set_render_target_internal_hook_set_viewport(backbuffer, (IDirect3DSurface9*)NONE, true);
            global_d3d_device->StretchRect(target_surface, &rect, backbuffer, &rect, D3DTEXF_NONE);
        }
        else
        {
            rasterizer_dx9_set_sampler_state(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            rasterizer_dx9_set_sampler_state(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            rasterizer_dx9_set_sampler_state(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
            rasterizer_dx9_set_sampler_state(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
            rasterizer_dx9_set_sampler_state(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
            rasterizer_dx9_set_sampler_state(0, D3DSAMP_MAXANISOTROPY, 1);
            rasterizer_dx9_set_sampler_state(0, D3DSAMP_MIPMAPLODBIAS, 0);
            rasterizer_dx9_set_sampler_state(0, D3DSAMP_MAXMIPLEVEL, 0);
            rasterizer_dx9_set_render_state(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA);
            rasterizer_dx9_set_render_state(D3DRS_ZENABLE, D3DZB_FALSE);
            rasterizer_dx9_set_render_state(D3DRS_DEPTHBIAS, 0);
            rasterizer_dx9_set_render_state(D3DRS_ALPHATESTENABLE, FALSE);
            rasterizer_dx9_set_render_state(D3DRS_CULLMODE, D3DCULL_NONE);
            if (!*rasterizer_dx9_disable_stencil_get())
            {
                rasterizer_dx9_set_render_state(D3DRS_STENCILENABLE, FALSE);
            }

            rasterizer_dx9_set_blend_render_state(_framebuffer_blend_function_none);
            rasterizer_set_render_target_internal_hook_set_viewport(backbuffer, (IDirect3DSurface9*)NONE, true);
            rasterizer_dx9_set_target_as_texture(0, rasterizer_target);

            c_rasterizer_constant_4f_cache<32>* main_pixel_shader_constants = rasterizer_get_main_pixel_shader_cache();
            if (main_pixel_shader_constants->test_cache(0, (real_vector4d*)&rasterizer_globals->fullscreen_parameters, 1))
            {
                global_d3d_device->SetPixelShaderConstantF(0, (real32*)&rasterizer_globals->fullscreen_parameters, 1);
            }

            rasterizer_dx9_set_screen_effect_pixel_shader(3);
            rasterizer_dx9_render_fullscreen_overlay_geometry(0, 0, rasterizer_dx9_patchy_fog_apply_from_stencil_build_vertex_buffer, 0, 0, 1, 1);
        }
        if (backbuffer)
        {
            backbuffer->Release();
        }
    }
    else
    {
        e_rasterizer_target rasterizer_target_dst = rasterizer_dx9_get_overlay_destination_target();
        if (rasterizer_target_dst != NONE)
        {
            RECT rect;
            rectangle2d_to_rect(&global_window_parameters->camera.viewport_bounds, &rect);

            IDirect3DSurface9* dst_surface = rasterizer_dx9_get_render_target_surface(rasterizer_target_dst, 0);
            IDirect3DSurface9* src_surface = rasterizer_dx9_get_render_target_surface(rasterizer_target, 0);
            global_d3d_device->StretchRect(src_surface, &rect, dst_surface, NULL, D3DTEXF_POINT);
        }
    }
    return;
}

bool __cdecl rasterizer_fullscreen_effects_build_vertex_buffer_color_ctx_cb(
    int32 output_type,
    real_rectangle2d* bounds,
    real_vector4d* location,
    void* output,
    void* ctx
)
{
    bool result = true;

    switch (output_type)
    {
    case _vertex_output_type_position:
        rasterizer_dx9_fullscreen_calculate_position(location, 1.f, (real_vector4d*)output);
        break;
    case _vertex_output_type_texcoord:
        rasterizer_dx9_fullscreen_calculate_screen_coordinates(bounds, (real_point2d*)location, (real_point2d*)output);
        break;
    case _vertex_output_type_color:
        *(pixel32*)output = *(pixel32*)ctx;
        break;
    default:
        DISPLAY_ASSERT("unreachable");
        result = false;
        break;
    }

    return result;
}

/* private code */

real32 rasterizer_dx9_fullscreen_effect_calculate_position_z(real32 z_far, int32 a2)
{
    const s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();

    real32 z = (a2 != 2 ? z_far : z_far / rasterizer_globals->clipping_parameters.z_far);

    return PIN(z, 0.f, 1.f);
}
