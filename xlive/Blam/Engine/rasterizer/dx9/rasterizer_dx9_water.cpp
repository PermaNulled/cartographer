#include "stdafx.h"
#include "rasterizer_dx9_water.h"

#include "rasterizer_dx9_main.h"
#include "rasterizer_dx9_targets.h"

#include "camera/camera.h"
#include "structures/structures.h"

/* prototypes */

bool __cdecl rasterizer_dx9_water_update_refraction_render_surface_with_main_rendered_surface(void);

/* public code */

bool* rasterizer_water_refraction_surface_updated_get(void)
{
    return Memory::GetAddress<bool*>(0x4F5048);
}

void rasterizer_dx9_water_apply_patches(void)
{
    PatchCall(Memory::GetAddress(0x1A07B5, 0x0), rasterizer_dx9_water_update_refraction_render_surface_with_main_rendered_surface);
    PatchCall(Memory::GetAddress(0x28158F, 0x0), rasterizer_dx9_water_update_refraction_render_surface_with_main_rendered_surface);
    return;
}

/* private code */

bool __cdecl rasterizer_dx9_water_update_refraction_render_surface_with_main_rendered_surface(void)
{
    bool* rasterizer_water_refraction_surface_updated = rasterizer_water_refraction_surface_updated_get();

    if (!*rasterizer_water_refraction_surface_updated)
    {
        rasterizer_dx9_reset_depth_buffer();
        if (get_global_structure_bsp()->water_definitions.count > 0)
        {
            IDirect3DSurface9* surface = rasterizer_dx9_target_get_main_mip_surface(_rasterizer_target_20);

            e_rasterizer_target render_target = *rasterizer_dx9_main_render_target_get();
            IDirect3DSurface9* render_surface = rasterizer_dx9_get_render_target_surface(render_target, 0);

            D3DVIEWPORT9 d3d_viewport; 
            rasterizer_dx9_device_get_interface()->GetViewport(&d3d_viewport);

            RECT rect = { d3d_viewport.X, d3d_viewport.Y, d3d_viewport.X + d3d_viewport.Width, d3d_viewport.Y + d3d_viewport.Height };
            rasterizer_dx9_device_get_interface()->StretchRect(render_surface, &rect, surface, NULL, D3DTEXF_LINEAR);
        }
        *rasterizer_water_refraction_surface_updated = true;
        rasterizer_dx9_reset_depth_buffer();
    }
    return true;
}