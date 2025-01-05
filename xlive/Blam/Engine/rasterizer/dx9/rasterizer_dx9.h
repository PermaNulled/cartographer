#pragma once
#include "math/color_math.h"
#include "math/real_math.h"

/* structures */

struct s_rasterizer_dx9_driver_globals
{
    bool using_amd_or_ati_gpu;
    bool disable_amd_or_ati_patches;
};

/* classes */

template <size_t size>
class c_rasterizer_constant_4f_cache
{
    real_vector4d values[size];
public:
    bool test_cache(uint32 index, const real_vector4d* constants, uint32 count)
    {
        ASSERT(index < size && index + count <= size);

        bool result = false;

        if (count)
        {
            bool contents_match = false;

            // Loop through values and constants and xor to determine if values are different
            uint32 i = 0;
            for (;
                !(
                    *(uint32*)&values[index + i].i ^ *(uint32*)&constants[i].i |
                    *(uint32*)&values[index + i].j ^ *(uint32*)&constants[i].j |
                    *(uint32*)&values[index + i].k ^ *(uint32*)&constants[i].k |
                    *(uint32*)&values[index + i].l ^ *(uint32*)&constants[i].l
                    );
                )
            {
                if (++i >= count)
                {
                    contents_match = true;
                    break;
                }
            }

            // Copy over the rest of the constants if the values don't match
            if (!contents_match)
            {
                if (count != i)
                {
                    for (i; i < count; ++i)
                    {
                        values[index + i] = constants[i];
                    }

                }
                result = true;
            }
        }

        return result;
    }
};

/* globals */

extern s_rasterizer_dx9_driver_globals g_rasterizer_dx9_driver_globals;

/* public code */

bool* rasterizer_dx9_disable_stencil_get(void);

c_rasterizer_constant_4f_cache<256>* rasterizer_get_main_vertex_shader_cache(void);

c_rasterizer_constant_4f_cache<32>* rasterizer_get_main_pixel_shader_cache(void);

void rasterizer_dx9_perf_event_begin(const char* event_description, real_argb_color* color);

void rasterizer_dx9_perf_event_end(const char* event_description);
