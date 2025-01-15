#include "stdafx.h"
#include "interface.h"

#include "cutscene/cinematics.h"
#include "game/game_globals.h"
#include "rasterizer/dx9/rasterizer_dx9_dynamic_geometry.h"
#include "rasterizer/dx9/rasterizer_dx9_dynavobgeom.h"
#include "rasterizer/rasterizer_globals.h"

/* constants */

#define k_splitscreen_line_colour D3DCOLOR_ARGB(255, 0, 0, 0);

/* globals */

/* prototypes */

e_display_type get_display_split_type(void);
void set_display_type(e_display_type display_type);
void render_splitscreen_line(void);

/* public code */

void apply_interface_hooks(void)
{
	PatchCall(Memory::GetAddress(0x227A17), render_splitscreen_line);
	return;
}

datum interface_get_tag_index(e_interface_tag interface_tag_index)
{
	VALID_INDEX(interface_tag_index, NUMBER_OF_INTERFACE_TAGS);
	return game_globals_get_interface_tag_reference(interface_tag_index).index;
}

void __cdecl interface_draw_bitmap(
	bitmap_data* bitmap,
	const real_point2d* position,
	const real_rectangle2d* rectangle,
	real32 size,
	real32 rotation,
	real32 alpha)
{
	real_rectangle2d default_rectangle = { 0.f, 1.f, 0.f, 1.f };
	const real_rectangle2d* rectangle_used = (rectangle ? rectangle : &default_rectangle);

	real32 sin_rotation = sine(rotation);
	real32 cos_rotation = cosine(rotation);

	// This originally was { 1, 2, 0, 3 } in vista which is the incorrect order of vertices
	uint8 g_interface_vertex_order[4] = { 3, 2, 0, 1 };
	
	s_dynamic_vertex_data vertex_data[4];

	for (uint8 i = 0; i < 4; i++)
	{
		int8 vertex_selection_bitflag = i + 1;
		bool right_vertices = TEST_BIT(vertex_selection_bitflag, 1);
		bool top_vertices = i > 1;

		real32 bounds_x = (right_vertices ? rectangle_used->x1 : rectangle_used->x0);
		real32 bounds_y = (top_vertices ? rectangle_used->y1 : rectangle_used->y0);

		real32 v1 = ((bitmap->width * bounds_x) - bitmap->registration_point.x) * size;
		real32 v2 = ((bitmap->height * bounds_y) - bitmap->registration_point.y) * size;
		
		real32 pos_x = (v1 * cos_rotation + position->x) - (v2 * sin_rotation);
		real32 pos_y = (v1 * sin_rotation) + (v2 * cos_rotation) + position->y;
		
		uint8 vertex_index = g_interface_vertex_order[i];

		vertex_data[vertex_index].pos_x = pos_x;
		vertex_data[vertex_index].pos_y = pos_y;
		vertex_data[vertex_index].bounds_x = bounds_x;
		vertex_data[vertex_index].bounds_y = bounds_y;
		vertex_data[vertex_index].color = { D3DCOLOR_ARGB((uint8)(alpha * 255.f), 255, 255, 255) };	// Set color to white with alpha from parameter
	}

	s_dynamic_geometry_parameters parameters;
	csmemset(&parameters, 0, sizeof(parameters));

	parameters.height_ratio = 1.f;
	parameters.width_ratio = 1.f;
	parameters.vs_constants_second_4.j = 1.f;
	parameters.vs_constants_second_4.i = 1.f;
	parameters.meter_parameters = NULL;
	parameters.point_interpolation = false;
	parameters.blend_function = _framebuffer_blend_function_alpha_multiply_add;
	parameters.map[0] = bitmap;
	rasterizer_dx9_dynamic_screen_geometry_draw(&parameters, vertex_data);
	return;
}

/* private code */

e_display_type get_display_split_type(void)
{
	return *Memory::GetAddress<e_display_type*>(0x4E6970);
}

void set_display_type(e_display_type display_type)
{
	WriteValue<e_display_type>(Memory::GetAddress(0xA3E460), display_type);
}

// Renders the lines inbetween windows in splitscreen
// Updated function so it works when the resolution isnt 640x480
void render_splitscreen_line(void)
{
	const int32 player_window_count = get_player_window_count();
	ASSERT(IN_RANGE(player_window_count, 1, 4));

	const s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
	const int16 resolution_x = (int16)rasterizer_globals->resolution_x;
	const int16 resolution_y = (int16)rasterizer_globals->resolution_y;

	// We calculate the size of the line by dividing our resolution by the height the original game ran at
	const int16 line_size = resolution_y / 480;		
	
	rectangle2d line;
	pixel32 color = k_splitscreen_line_colour;
	if (player_window_count > 1)
	{
		if (get_display_split_type() == _display_split_type_horizontal)
		{
			line.top = (resolution_y / 2) - line_size;
			line.left = 0;
			line.bottom = (resolution_y / 2) + line_size;
			line.right = resolution_x;
			cinematics_draw_line(&line, color);

			// Draw horizontal line between players
			if (player_window_count > 2)
			{
				line.left = (resolution_x / 2) - line_size;
				line.bottom = resolution_y;
				line.right = (resolution_x / 2) + line_size;
				line.top = (player_window_count == 3 ? resolution_y / 2 : 0);
			}
		}
		else
		{
			line.top = 0;
			line.left = (resolution_x / 2) - line_size;
			line.bottom = resolution_y;
			line.right = (resolution_x / 2) + line_size;
			cinematics_draw_line(&line, color);

			// Draw vertical line between players
			if (player_window_count > 2)
			{
				line.top = (resolution_y / 2) - line_size;
				line.bottom = (resolution_y / 2) + line_size;
				line.right = resolution_x;
				line.left = (player_window_count == 3 ? resolution_x / 2 : 0);
			}
		}
		cinematics_draw_line(&line, color);
	}
	return;
}
