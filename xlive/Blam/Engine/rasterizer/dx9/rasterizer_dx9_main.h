#pragma once
#include "rasterizer_dx9_targets.h"

#include "bitmaps/bitmap_group.h"
#include "render/render.h"
#include "rasterizer/rasterizer_globals.h"

/* enums */

enum e_framebuffer_blend_function : int16
{
	_framebuffer_blend_function_alpha_blend = 0,
	_framebuffer_blend_function_multiply = 1,
	_framebuffer_blend_function_double_multiply = 2,
	_framebuffer_blend_function_add = 3,
	_framebuffer_blend_function_subtract = 4,
	_framebuffer_blend_function_component_min = 5,
	_framebuffer_blend_function_component_max = 6,
	_framebuffer_blend_function_alpha_multiply_add = 7,
	_framebuffer_blend_function_constant_color_blend = 8,
	_framebuffer_blend_function_inverse_constant_color_blend = 9,
	_framebuffer_blend_function_none = 10,
	k_shader_framebuffer_blend_function_count = 12
};

/* structures */

struct s_rasterizer_dx9_main_globals
{
    IDirect3D9Ex* global_d3d_interface;
    D3DMULTISAMPLE_TYPE global_d3d_primary_multisampletype;
    uint32 global_d3d_primary_multisamplequality;
    IDirect3DSurface9* global_d3d_surface_render_primary;
    IDirect3DSurface9* global_d3d_surface_render_primary_z;
    IDirect3DSurface9* global_d3d_surface_render_z_as_target_z;
    IDirect3DTexture9* global_d3d_texture_render_resolved;
    IDirect3DSurface9* global_d3d_surface_render_resolved;
    IDirect3DSurface9* global_d3d_surface_screenshot;
    IDirect3DTexture9* global_d3d_texture_render_primary;
    IDirect3DTexture9* global_d3d_texture_backbuffer;
    IDirect3DSurface9* global_d3d_surface_backbuffer;
    uint32 global_d3d_sun_height;
    uint32 global_d3d_sun_width;
    IDirect3DTexture9* global_d3d_texture_sun_glow_primary;
    IDirect3DSurface9* global_d3d_surface_sun_glow_primary;
    IDirect3DTexture9* global_d3d_texture_sun_glow_secondary;
    IDirect3DSurface9* global_d3d_surface_sun_glow_secondary;
    IDirect3DTexture9* global_d3d_texture_motion_sensor;
    IDirect3DSurface9* global_d3d_surface_motion_sensor;

    /* TODO: these cubemap surfaces are unused, eventually remove these once we gain ownership */
    IDirect3DTexture9* global_d3d_texture_cubemap;
    IDirect3DSurface9* global_d3d_surface_cubemap_pos_x;
    IDirect3DSurface9* global_d3d_surface_cubemap_neg_x;
    IDirect3DSurface9* global_d3d_surface_cubemap_pos_y;
    IDirect3DSurface9* global_d3d_surface_cubemap_neg_y;
    IDirect3DSurface9* global_d3d_surface_cubemap_pos_z;
    IDirect3DSurface9* global_d3d_surface_cubemap_neg_z;
    IDirect3DSurface9* global_d3d_surface_cubemap_depth;

    uint32 global_d3d_palette_count;
    IDirect3DDevice9Ex* global_d3d_device;
};

/* globals */


/* prototypes */

void rasterizer_dx9_main_apply_patches(void);

s_rasterizer_dx9_main_globals* rasterizer_dx9_main_globals_get(void);

IDirect3DDevice9Ex* rasterizer_dx9_device_get_interface(void);

datum last_bitmap_tag_index_get(void);

D3DCAPS9* rasterizer_dx9_caps_get(void);

int32* hardware_vertex_processing_get(void);

int32* allow_vsync_get(void);

bool __cdecl rasterizer_initialize(void);

bool __cdecl rasterizer_dx9_reset(bool create_window);

void __cdecl rasterizer_dx9_reset_depth_buffer(void);

void rasterizer_dx9_present(bitmap_data* screenshot_bitmap, bool a2);

void __cdecl rasterizer_dx9_set_texture_direct(int16 stage, datum bitmap_tag_index, int16 bitmap_data_index, real32 a4);

bool __cdecl rasterizer_window_begin(s_frame* preferences);

void __cdecl rasterizer_update_cameras(void);

void __cdecl rasterizer_dx9_set_stencil_mode(int16 mode);

void __cdecl rasterizer_dx9_set_render_state(D3DRENDERSTATETYPE state, DWORD value);

void rasterizer_dx9_set_blend_render_state(e_framebuffer_blend_function framebuffer_blend_function);

void __cdecl rasterizer_get_bloom_brightness(real32* brightness, real32* overbright);

bool __cdecl rasterizer_set_texture_bitmap_data(int16 stage, bitmap_data* texture);

bool rasterizer_dx9_draw_primitive_up(
	D3DPRIMITIVETYPE PrimitiveType,
	uint32 PrimitiveCount,
	const void* pVertexStreamZeroData,
	uint32 VertexStreamZeroStride);

bool __cdecl rasterizer_dx9_create_texture(uint32 width, uint32 height, int32 levels, uint32 usage, e_bitmap_data_format format, bool linear, IDirect3DTexture9** texture);

void rasterizer_dx9_texture_stage_dimensions(int16 stage, uint32 width, uint32 height);

bool __cdecl rasterizer_dx9_device_set_texture(uint32 stage, IDirect3DTexture9* texture);

void __cdecl rasterizer_dx9_set_texture(uint16 stage, e_bitmap_type type, uint32 usage, datum tag_index, int16 bitmap_index, real32 a6);

bool __cdecl rasterizer_dx9_device_initialize(s_rasterizer_parameters* parameters, bool display_blackness);

void __cdecl rasterizer_dx9_initialize_camera_projection(
    bool is_texture_camera,
    const render_camera* camera,
    const render_projection* projection,
    e_rasterizer_target rasterizer_target);

bool __cdecl rasterizer_dx9_initialize(void);

bool __cdecl rasterizer_dx9_vertex_shaders_initialize(void);

bool __cdecl rasterizer_dx9_render_scene_start(const s_render_scene_parameters* parameters);

bool __cdecl rasterizer_dx9_render_scene_end(void);

void __cdecl rasterizer_dx9_clear_render_target(uint32 flags, pixel32 color, real32 z, bool stencil);
