#pragma once
#include "rasterizer_dx9_dynamic_geometry.h"

/* prototypes */

void __cdecl rasterizer_dx9_create_dynavobgeom_pixel_shaders(void);

void rasterizer_dx9_dynamic_screen_geometry_draw(const s_dynamic_geometry_parameters* parameters, const s_dynamic_vertex_data* vertex_data);
