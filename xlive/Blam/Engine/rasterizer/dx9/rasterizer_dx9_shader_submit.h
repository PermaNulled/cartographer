#pragma once

/* public code */

void __cdecl rasterizer_dx9_set_sampler_state(uint32 sampler, D3DSAMPLERSTATETYPE state, uint32 value);

D3DBLEND __cdecl rasterizer_dx9_get_render_state(D3DRENDERSTATETYPE state);