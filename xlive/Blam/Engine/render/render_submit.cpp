#include "stdafx.h"
#include "render_submit.h"

#include "render.h"
#include "render_layers.h"
#include "render_primitive.h"
#include "render_section_visibility.h"

#include "objects/objects.h"
#include "rasterizer/dx9/rasterizer_dx9.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"

/* prototypes */

void __cdecl render_submit_transparent_geometry(s_model_group_submit_data* model_data);
void __cdecl render_submit_transparent_hologram_geometry(uint32* a1);

/* public code */

void render_submit_apply_patches(void)
{
	// replace marker call to interpolated ones
	PatchCall(Memory::GetAddress(0x19BAFC), object_get_markers_by_string_id);

	WritePointer(Memory::GetAddress(0x19B525 + 3), render_submit_transparent_geometry);
	WritePointer(Memory::GetAddress(0x19B7BA + 3), render_submit_transparent_hologram_geometry);
	return;
}

/* private */

void __cdecl render_submit_transparent_geometry(s_model_group_submit_data* model_data)
{
	uint32 v1 = 0x40000;

	s_scenario_fog_result* g_fog_result = global_fog_result_get();
	if (g_fog_result->patchy_fog_tag_index != NONE && !g_fog_result->sort_behind_transparents)
	{
		real32 result = (g_fog_result->field_88.lower > g_fog_result->field_88.upper ? g_fog_result->field_88.lower : g_fog_result->field_88.upper);

		v1 = (result > 0.f ? 0xC0000 : v1);
	}

	s_render_layer_globals* render_layer_globals = render_layer_globals_get();
	render_layer_globals->transparent_submit_in_progress = true;

	c_render_primitive_list* primitive_list = render_primitive_get_by_primitive_list_type(0);
	primitive_list->m_render_layer_flags_backup = primitive_list->m_render_layer_flags;
	primitive_list->m_render_layer_flags.clear();
	primitive_list->m_field_C = primitive_list->m_field_4;

	// Copy render primary viewport to active camo
	// Make sure to create a rect so that only the content in the viewport is copied over rather than the whole screen
	{
		RECT rect;
		rectangle2d_to_rect(&global_window_parameters_get()->camera.viewport_bounds, &rect);

		rasterizer_dx9_device_get_interface()->StretchRect(
			rasterizer_dx9_main_globals_get()->global_d3d_surface_render_primary,
			&rect,
			rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_active_camo),
			NULL,
			D3DTEXF_LINEAR);
	}


	for (uint8 i = 0; i < model_data->count; i++)
	{
		render_section_visibility_compute(0, NONE, model_data->model_group_index[i], 0x11818E, v1, INT16_MAX+1, 0x40000, 0);
	}

	bool* g_rasterizer_dx9_disable_stencil = rasterizer_dx9_disable_stencil_get();
	if (render_layer_globals->first_person_render_in_progress)
	{
		*g_rasterizer_dx9_disable_stencil = false;
		rasterizer_dx9_set_stencil_mode(2);
		*g_rasterizer_dx9_disable_stencil = true;
	}

	render_scene_geometry(_collection_type_0, _render_layer_lightmap_indirect);
	render_scene_geometry(_collection_type_0, _render_layer_enviroment_map);
	render_scene_geometry(_collection_type_0, _render_layer_decal);
	render_scene_geometry(_collection_type_0, _render_layer_selfillumination);
	render_scene_geometry(_collection_type_0, _render_layer_overlay);

	if (render_layer_globals->first_person_render_in_progress)
	{
		*g_rasterizer_dx9_disable_stencil = false;
		rasterizer_dx9_set_stencil_mode(0);
		*g_rasterizer_dx9_disable_stencil = true;
	}

	render_scene_geometry(_collection_type_0, _render_layer_fog);
	render_scene_geometry(_collection_type_0, _render_layer_active_camo);

	if (g_fog_result->patchy_fog_tag_index != NONE && !g_fog_result->field_96)
	{
		real32 result = (g_fog_result->field_88.lower > g_fog_result->field_88.upper ? g_fog_result->field_88.lower : g_fog_result->field_88.upper);

		if (result > 0.f)
		{
			render_scene_geometry(_collection_type_0, _render_layer_active_camo_stencil_modulate);
		}
	}

	render_scene_geometry(_collection_type_0, _render_layer_transparent);
	primitive_list->m_render_layer_flags = primitive_list->m_render_layer_flags_backup;
	primitive_list->m_field_C = 0;

	render_layer_globals->transparent_submit_in_progress = false;
	return;
}

void __cdecl render_submit_transparent_hologram_geometry(uint32* a1)
{
	const uint32 model_group_index = a1[0];
	const uint32 flags = a1[1];
	c_render_primitive_list* primitive_list = render_primitive_get_by_primitive_list_type(0);
	primitive_list->m_render_layer_flags_backup = primitive_list->m_render_layer_flags;
	primitive_list->m_render_layer_flags.clear();
	primitive_list->m_field_C = primitive_list->m_field_4;

	// Copy render primary viewport to active camo
	// Make sure to create a rect so that only the content in the viewport is copied over rather than the whole screen
	{
		RECT rect;
		rectangle2d_to_rect(&global_window_parameters_get()->camera.viewport_bounds, &rect);

		IDirect3DSurface9* main_mip_surface = rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_active_camo);
		IDirect3DSurface9* render_target_surface = rasterizer_dx9_get_render_target_surface(*rasterizer_dx9_main_render_target_get(), 0);

		rasterizer_dx9_device_get_interface()->StretchRect(
			render_target_surface,
			&rect,
			main_mip_surface,
			NULL,
			D3DTEXF_LINEAR);
	}

	render_section_visibility_compute(0, flags | ~48, (uint8)model_group_index, NONE, 0, NONE, 0, true);

	render_scene_geometry(_collection_type_0, _render_layer_texture_accumulate);
	render_scene_geometry(_collection_type_0, _render_layer_lightmap_indirect);
	render_scene_geometry(_collection_type_0, _render_layer_enviroment_map);
	render_scene_geometry(_collection_type_0, _render_layer_selfillumination);
	render_scene_geometry(_collection_type_0, _render_layer_overlay);
	render_scene_geometry(_collection_type_0, _render_layer_fog);
	render_scene_geometry(_collection_type_0, _render_layer_transparent);
	render_scene_geometry(_collection_type_0, _render_layer_hologram);

	primitive_list->m_render_layer_flags = primitive_list->m_render_layer_flags_backup;
	primitive_list->m_field_C = 0;
	return;
}
