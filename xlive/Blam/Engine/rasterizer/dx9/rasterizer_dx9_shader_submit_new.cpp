#include "stdafx.h"
#include "rasterizer_dx9_shader_submit_new.h"

#include "rasterizer_dx9_main.h"
#include "rasterizer_dx9_targets.h"

#include "cache/cache_files.h"
#include "game/game_globals.h"
#include "interface/new_hud_draw.h"
#include "objects/light_definitions.h"
#include "rasterizer/rasterizer_globals.h"
#include "rasterizer/rasterizer_lights.h"
#include "render/render_first_person.h"
#include "shaders/shader_definitions.h"

/* globals */

#ifndef NDEBUG
bool g_spherical_light_error_displayed = false;
#endif

bool g_use_secondary_gradient_index = false;
bool g_use_gel_map_bitmap = true;

/* prototypes */

void __fastcall c_shader_submission_interface_new__stage_texture(
    c_shader_submission_interface_new* _this,
    uint32 _edx,
    int16 stage,
    e_shader_pass_texture_source_extern bitmap_extern_index,
    uint32* res_x,
    uint32* res_y
);

/* public code */

void rasterizer_dx9_shader_submit_new_apply_patches(void)
{
    PatchCall(Memory::GetAddress(0x269091), c_shader_submission_interface_new__stage_texture);
    return;
}

void c_shader_submission_interface_new::stage_texture(
    int16 stage,
    e_shader_pass_texture_source_extern bitmap_extern_index,
    uint32* res_x,
    uint32* res_y) const
{
    ASSERT(res_x && res_y);

    light_definition* light_def;
    e_rasterizer_target target;
    s_shader_definition* shader_def;
    s_shader_postprocess_definition_new* pp_def;
    bitmap_group* group;
    uint32 convolution_target_index;
    s_shader_postprocess_bitmap_new* bitmap;
    int16 bitmap_index;
    RECT rect;

    bool continue_staging = true;
    while (continue_staging)
    {
        *res_x = 1;
        *res_y = 1;

        switch (bitmap_extern_index)
        {
        case _shader_pass_texture_source_extern_global_vector_normalization:
            rasterizer_dx9_set_texture_direct(stage, rasterizer_globals_get_data()->vector_normalization.index, 0, 0.f);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_unused_2:
            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_texaccum);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_target_texaccum:
            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_texaccum);
            rasterizer_target_get_resolution(_rasterizer_target_texaccum, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_utility_buffer:
            DISPLAY_ASSERT("shader uses utility buffer but it no longer exists");
            rasterizer_dx9_device_set_texture(stage, NULL);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_target_frame_buffer:
            rasterizer_dx9_set_target_as_texture(stage, *rasterizer_dx9_main_render_target_get());
            rasterizer_target_get_resolution(*rasterizer_dx9_main_render_target_get(), res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_target_z:
            target = rasterizer_globals_get()->d3d9_sm3_supported ? _rasterizer_target_z_a8b8g8r8 : _rasterizer_target_backbuffer;
            rasterizer_dx9_set_target_as_texture(stage, target);
            rasterizer_target_get_resolution(target, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_target_shadow:
            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_shadow);
            rasterizer_target_get_resolution(_rasterizer_target_shadow, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_light_falloff:
            rasterizer_dx9_set_texture_direct(stage, rasterizer_globals_get_data()->distance_attenuation.index, 0, 0.f);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_light_gel:
            light_def = current_light_definition_get();
            if (!light_type_is_spherical(current_lght_type_get()) && light_def)
            {
                if (!light_def->flags.test(_light_definition_texture_camera_gel))
                {
                    if (light_def->shader.index == NONE|| rasterizer_rendering_light_get())
                    {
                        rasterizer_dx9_set_texture(
                            stage,
                            _bitmap_type_2d,
                            1,
                            (g_use_gel_map_bitmap ? light_def->gel_map.index : NONE),
                            0,
                            0.f);
                    }
                    else
                    {
                        rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_shadow_alias_swizzled);
                    }
                    continue_staging = false;
                }
                else
                {
                    bitmap_extern_index = _shader_pass_texture_source_extern_global_texture_camera;
                }
                break;
            }

#ifndef NDEBUG
            if (!g_spherical_light_error_displayed)
            {
                const char* string = "spherical light using gel texture extern! (or no light def?)";
                DISPLAY_ASSERT(string);
                error(2, string);
                g_spherical_light_error_displayed = true;
            }
#endif

            rasterizer_dx9_set_texture(stage, _bitmap_type_2d, 1, NONE, 0, 0.f);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_unused_12:
            if (light_type_is_spherical(current_lght_type_get()))
            {
                rasterizer_dx9_set_texture_direct(stage, rasterizer_globals_get_data()->distance_attenuation.index, 0, 0.f);
            }
            else
            {
                rasterizer_dx9_set_texture(
                    stage,
                    _bitmap_type_2d,
                    1,
                    (g_use_gel_map_bitmap ? current_light_definition_get()->gel_map.index : NONE),
                    0,
                    0.f);
            }
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_shadow_buffer:
            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_shadow_buffer);
            rasterizer_target_get_resolution(_rasterizer_target_shadow_buffer, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_gradient_separate:
            rasterizer_dx9_set_texture_direct(stage, rasterizer_globals_get_data()->gradients.index, g_use_secondary_gradient_index ? 2 : 0, 0.f);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_gradient_product:
            rasterizer_dx9_set_texture_direct(stage, rasterizer_globals_get_data()->gradients.index, (2 * g_use_secondary_gradient_index) + 1, 0.f);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_hud_bitmap:
            rasterizer_dx9_set_texture_direct(stage, hud_bitmap_tag_index_get(), hud_bitmap_data_index_get(), 0.f);
            
            if (hud_bitmap_tag_index_get() != NONE)
            {
                group = (bitmap_group*)tag_get_fast(hud_bitmap_tag_index_get());
                if (VALID_INDEX(hud_bitmap_data_index_get(), group->bitmaps.count))
                {
                    ASSERT(group);
                    const bitmap_data* bitmap = group->bitmaps[hud_bitmap_data_index_get()];
                    *res_x = bitmap->width;
                    *res_y = bitmap->height;
                }
            }

            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_active_camo:
            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_active_camo);
            rasterizer_target_get_resolution(_rasterizer_target_active_camo, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_texture_camera:
            target = rasterizer_dx9_get_overlay_destination_target();
            if (global_window_parameters_get()->is_texture_camera || target == _rasterizer_target_none)
            {
                rasterizer_dx9_set_texture_direct(stage, rasterizer_globals_get_data()->default_textures[_bitmap_type_2d].index, 0, 0.f);
            }
            else
            {
                rasterizer_dx9_set_target_as_texture(stage, target);
                rasterizer_target_get_resolution(target, res_x, res_y);
            }
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_water_reflection:
            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_water_reflection);
            rasterizer_target_get_resolution(_rasterizer_target_water_reflection, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_water_refraction:
            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_water_refraction);
            rasterizer_target_get_resolution(_rasterizer_target_water_refraction, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_aux1:
            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_shadow_alias_swizzled);
            rasterizer_target_get_resolution(_rasterizer_target_shadow_alias_swizzled, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_aux2:
            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_sun_glow_secondary);
            rasterizer_target_get_resolution(_rasterizer_target_sun_glow_secondary, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_particle_distortion:
            rectangle2d_to_rect(&global_window_parameters_get()->camera.viewport_bounds, &rect);

            rasterizer_dx9_device_get_interface()->StretchRect(
                rasterizer_dx9_main_globals_get()->global_d3d_surface_render_primary,
                &rect,
                rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_particle_distortion),
                NULL,
                D3DTEXF_LINEAR);

            rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_particle_distortion);
            rasterizer_target_get_resolution(_rasterizer_target_particle_distortion, res_x, res_y);
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_global_convolution1:
        case _shader_pass_texture_source_extern_global_convolution2:
            convolution_target_index = bitmap_extern_index - _shader_pass_texture_source_extern_global_convolution1;
            ASSERT(IN_RANGE(convolution_target_index, 0, 1));

            target = (e_rasterizer_target)convolution_targets_get()[bitmap_extern_index];

            if (target == _rasterizer_target_convolution_scratch1 ||
                target == _rasterizer_target_convolution_scratch2 ||
                target == _rasterizer_target_active_camo)
            {
                rasterizer_dx9_set_target_as_texture(stage, target);
                rasterizer_target_get_resolution(target, res_x, res_y);
            }
            else
            {
                rasterizer_dx9_set_target_as_texture(stage, _rasterizer_target_texaccum);
            }
            continue_staging = false;
            break;
        case _shader_pass_texture_source_extern_shader_active_camo_bump:
            ASSERT(m_extern_shader_index != NONE);
            
            shader_def = (s_shader_definition*)tag_get_fast(this->m_extern_shader_index);
            pp_def = shader_def->postprocess_definition[0];
            bitmap_index = pp_def->bitmap_property_get(4)->bitmap_index;
            if (bitmap_index == NONE)
            {
                shader_def = (s_shader_definition*)tag_get_fast(rasterizer_globals_get_data()->global_shader.index);
                pp_def = shader_def->postprocess_definition[0];
                bitmap_index = pp_def->bitmap_property_get(4)->bitmap_index;

                ASSERT(bitmap_index != NONE);
            }

            bitmap = pp_def->bitmap_get(bitmap_index);
            rasterizer_dx9_set_texture_direct(stage, bitmap->bitmap_group, (int16)bitmap->bitmap_index, 0.f);
            continue_staging = false;
            break;

        case _shader_pass_texture_source_extern_firstperson_scope:
            target = (*rendering_fp_weapon_get() ? _rasterizer_target_sun_glow_primary : _rasterizer_target_active_camo);
            rasterizer_dx9_set_target_as_texture(stage, target);
            rasterizer_target_get_resolution(target, res_x, res_y);
            continue_staging = false;
            break;

        default:
            DISPLAY_ASSERT("unreachable");
            continue_staging = false;
            break;
        }
    }

    return;
}

int32* convolution_targets_get(void)
{
    return Memory::GetAddress<int32*>(0xA3E388);
}

void __cdecl rasterizer_shader_level_of_detail_bias_update(void)
{
	INVOKE(0x266525, 0x0, rasterizer_shader_level_of_detail_bias_update);
	return;
}

void __cdecl rasterizer_shader_submit(datum shader_index, int32 a2, int32 render_layer, int32 a4, int32 a5, real32 z_far)
{
    INVOKE(0x269E9A, 0, rasterizer_shader_submit, shader_index, a2, render_layer, a4, a5, z_far);
    return;
}

void rasterizer_flags_unknown_function_1()
{
    INVOKE(0x2664EA, 0, rasterizer_flags_unknown_function_1);
}

void rasterizer_flags_unknown_function_2(int32* a1)
{
    INVOKE(0x266415, 0, rasterizer_flags_unknown_function_2, a1);
}

int32* rasterizer_unknown_shader_submit_unknown_value_get()
{
    return Memory::GetAddress<int32*>(0x9765D8);
}

/* private code */

void __fastcall c_shader_submission_interface_new__stage_texture(
    c_shader_submission_interface_new* _this,
    uint32 _edx,
    int16 stage,
    e_shader_pass_texture_source_extern bitmap_extern_index,
    uint32* res_x,
    uint32* res_y
)
{
    _this->stage_texture(stage, bitmap_extern_index, res_x, res_y);
    return;
}
