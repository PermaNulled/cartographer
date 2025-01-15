#pragma once
#include "math/real_math.h"
#include "shaders/shader_postprocess_definitions.h"
#include "shaders/shader_passes.h"

// TODO: finish reversing

/* structures */

struct s_shader_submission_bitmap_info
{
	datum bitmap_group;
	int32 field_4;
	int32 width;
	int32 height;
	real32 field_10;
	real32 field_14;
	real32 field_18;
	int8 field_1C[36];
};
ASSERT_STRUCT_SIZE(s_shader_submission_bitmap_info, 64)

/* classes */

class c_shader_submission_interface_new
{
public:
	void stage_texture(int16 stage, e_shader_pass_texture_source_extern bitmap_extern_index, uint32* res_x, uint32* res_y) const;
private:
	int32 field_0;
	int32 field_4;
	datum m_extern_shader_index;
	s_shader_postprocess_definition_new* m_shader_postprocess_definition;
	s_shader_template_postprocess_definition_new* m_template_postprocess_definition;
	s_shader_template_postprocess_level_of_detail_new* m_levels_of_detail;
	s_shader_template_postprocess_pass_new* m_passes;
	s_shader_template_postprocess_implementation_new* m_implementations;
	s_shader_pass_postprocess_definition_new* m_shader_pass_postprocess_definition;
	s_shader_pass_postprocess_implementation_new* m_shader_pass_implementation;
	real32 m_field_28;
	real32 m_field_2C;
	int8 m_dirty_value_externs[8];
	int8 m_dirty_color_externs[8];
	int8 m_dirty_switch_externs[4];
	int8 m_updated_value_externs[8];
	int8 m_updated_color_externs[8];
	int8 m_field_54[4];
	int8 m_updated_switch_externs[4];
	real32 field_5C[43];
	int8 field_108[501];
	int32 m_switch_externs[8];
	s_shader_submission_bitmap_info m_bitmap_info[64];
	s_shader_submission_bitmap_info m_staged_texture_info[8];
	int8 field_1520[248];
	bool m_pixel_shader_being_set;
	int8 pad_1619[3];
	int8 field_161C[16];
	real_vector4d m_vertex_constants[16];
	real_vector4d m_vertex_shader_constants[16];
	int32 field_182C;
	int32 m_vertex_shader_constant_count;
	int8 field_1834[8];
	IDirect3DPixelShader9* m_pixel_shader_handle;
	int32 m_pixel_shader_index;
	int32 field_1844;
	real_vector4d field_1848[32];
	int32 m_level_of_detail;
	int32 m_shader_layer;
	int32 m_shader_pass;
	int32 m_implementation;
};

/* public code */

void rasterizer_dx9_shader_submit_new_apply_patches(void);

int32* convolution_targets_get(void);

void __cdecl rasterizer_shader_level_of_detail_bias_update(void);

void __cdecl rasterizer_shader_submit(datum shader_index, int32 a2, int32 render_layer, int32 a4, int32 a5, real32 a6);

// these functions modify some flags but cannot ascertain what they do.
void __cdecl rasterizer_flags_unknown_function_1();
void __cdecl rasterizer_flags_unknown_function_2(int32* a1);

// referenced before submitting a shader
int32* rasterizer_unknown_shader_submit_unknown_value_get();