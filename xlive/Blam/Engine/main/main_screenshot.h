#pragma once
#include "bitmaps/bitmap_group.h"
#include "render/render.h"

/* structures */

struct s_screenshot_globals
{
	bool taking_screenshot;
	bool cubemap_screenshot;
	bool take_screenshot;
	bool web_map;
	bool save_as_tga;
	bool dump_camera_data;
	bool capture_cubemap;
	bool camera_name_set;
	int32 resolution_multiplier;
	int8 tile_count_jittered;
	int8 pad[3];
	char camera_name[256];
	bitmap_data* bloom_bitmap;
};
ASSERT_STRUCT_SIZE(s_screenshot_globals, 276);

struct s_movie_globals
{
	bool in_progress;
	int8 pad[3];
	int32 recording_start_tick;
	uint32 recording_stop_tick;
	int32 recording_frame_index;
	int32 field_10;
	int32 field_14;
	real32 recording_dt;
	bool field_1C;
	int8 pad_1[3];
};
ASSERT_STRUCT_SIZE(s_movie_globals, 32);

/* public code */

void main_screenshot_apply_patches(void);

s_screenshot_globals* get_screenshot_globals(void);

s_movie_globals* movie_globals_get(void);

bool screenshot_in_progress(void);

bool cubemap_screenshot_in_progress(void);

void screenshot_cubemap(const char* name);

bool __cdecl screenshot_render(window_bound* window);
