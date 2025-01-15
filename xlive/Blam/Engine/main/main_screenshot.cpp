#include "stdafx.h"
#include "main_screenshot.h"

#include "bitmaps/bitmaps.h"
#include "bitmaps/bitmap_utilities.h"
#include "bitmaps/targa_file.h"
#include "bitmaps/tiff_file.h"
#include "cache/pc_geometry_cache.h"
#include "cache/pc_texture_cache.h"
#include "cseries/cseries_strings.h"
#include "game/game_time.h"
#include "main/main.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"
#include "rasterizer/rasterizer_main.h"
#include "render/render.h"
#include "tag_files/files_windows.h"

/* constants */

#define k_screenshot_cubemap_size 256
#define k_screenshot_cubemap_resolution_multiplier_width 4
#define k_screenshot_cubemap_resolution_multiplier_height 3

/* prototypes */

// Returns true if we should continue recording the movie
static bool movie_should_continue(void);

// Attempts to render a bloom screenshot and returns true if it's been rendered
static bool render_bloom_screenshot(int16 width, int16 height, window_bound* window_bound);

/* 
Takes in the horizontal tile number and vertical tile number of the cubemap and retrieves information
we need to know about the tile in order to construct the screenshot
*/
static void screenshot_cubemap_retrieve_tile_info(
	bool* is_horizontal_tile,
	int32* angle,
	bool* is_vertical_tile,
	int32 horizontal_tile_num,
	int32 vertical_tile_num,
	int32* face_index);

/*
Takes in the angle, width, and height and populates the x, y and rotation fields for calculating cubemap pixels locations when the camera rotated
*/
static void screenshot_cubemap_retrieve_rotation_info(
	int32* x,
	int32* y,
	int32 angle,
	int32* rotation,
	int32 height,
	int32 width);

/* dumps camera info to a text file */
static void __cdecl dump_camera_to_file(const char* camera_name, const render_camera* camera);

static void __cdecl screenshot_cubemap_set_camera_rotation(int32 face_index, int32 resolution, const real_point3d* location, render_camera* camera);

static void screenshot_calculate_bloom(bitmap_data* bitmap, int32 horizontal_tile, int32 vertical_tile);

/* public code */

void main_screenshot_apply_patches(void)
{
	PatchCall(Memory::GetAddress(0x26FFD1), screenshot_render);
	return;
}

s_screenshot_globals* get_screenshot_globals(void)
{
	return Memory::GetAddress<s_screenshot_globals*>(0xA55630);
}

s_movie_globals* movie_globals_get(void)
{
	return Memory::GetAddress<s_movie_globals*>(0xA55744);
}

bool screenshot_in_progress(void)
{
	return get_screenshot_globals()->taking_screenshot;
}

bool cubemap_screenshot_in_progress(void)
{
	const s_screenshot_globals* screenshot_globals = get_screenshot_globals();
	return screenshot_globals->taking_screenshot && screenshot_globals->cubemap_screenshot;
}

void screenshot_cubemap(const char* name)
{
	s_screenshot_globals* screenshot_globals = get_screenshot_globals();
	screenshot_globals->take_screenshot = true;
	screenshot_globals->capture_cubemap = true;
	screenshot_globals->cubemap_screenshot = true;
	screenshot_globals->camera_name_set = false;
	csstrnzcpy(screenshot_globals->camera_name, name, NUMBEROF(screenshot_globals->camera_name));
	return;
}

// TODO: maybe fix screenshots while in splitscreen?
bool __cdecl screenshot_render(window_bound* window)
{
	ASSERT(window);

	bool result = true;
	s_screenshot_globals* screenshot_globals = get_screenshot_globals();
	s_movie_globals* movie_globals = movie_globals_get();

	if (screenshot_globals->take_screenshot || movie_globals->in_progress)
	{
		s_rasterizer_globals* rasterizer_globals = rasterizer_globals_get();
		s_rasterizer_dx9_main_globals* rasterizer_dx9_globals = rasterizer_dx9_main_globals_get();
		IDirect3DDevice9Ex* global_d3d_device = rasterizer_dx9_device_get_interface();

		int16 screen_width = rectangle2d_width(&rasterizer_globals->screen_bounds);
		int16 screen_height = rectangle2d_height(&rasterizer_globals->screen_bounds);


		const HRESULT hr = global_d3d_device->CreateRenderTarget(screen_width, screen_height, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, true, &rasterizer_dx9_globals->global_d3d_surface_screenshot, NULL);
		if (FAILED(hr))
		{
			result = false;
			return result;
		}

		// Continue if we meet the movie checks
		if (!movie_globals->in_progress || movie_should_continue())
		{
			const window_bound window_copy = *window;
			const int16 tile_count = 1 << screenshot_globals->tile_count_jittered;
			screenshot_globals->taking_screenshot = true;

			int32 horizontal_tiles;
			int32 vertical_tiles;

			bool tiled;
			
			bitmap_data* bitmap_0;
			bitmap_data* bitmap_1;
			bitmap_data* bitmap_secondary;

			if (screenshot_globals->capture_cubemap)
			{
				screen_width = k_screenshot_cubemap_size;
				screen_height = k_screenshot_cubemap_size;
				horizontal_tiles = k_screenshot_cubemap_resolution_multiplier_width;
				vertical_tiles = k_screenshot_cubemap_resolution_multiplier_height;
				tiled = true;
				bitmap_0 = bitmap_2d_new(k_screenshot_cubemap_size, k_screenshot_cubemap_size, 0, _bitmap_format_x8r8g8b8, (e_bitmap_data_flags)0);
				bitmap_1 = bitmap_2d_new(k_screenshot_cubemap_size, k_screenshot_cubemap_size, 0, _bitmap_format_x8r8g8b8, (e_bitmap_data_flags)0);
			}
			else
			{
				screen_width /= tile_count;
				screen_height /= tile_count;
				horizontal_tiles = screenshot_globals->resolution_multiplier;
				vertical_tiles = screenshot_globals->resolution_multiplier;
				bitmap_0 = bitmap_2d_new(screen_width, screen_height, 0, _bitmap_format_x8r8g8b8, (e_bitmap_data_flags)0);
				if (screenshot_globals->web_map)
				{
					ASSERT(screenshot_globals->take_screenshot);
					bitmap_secondary = bitmap_2d_new(screen_width, screen_height, 0, _bitmap_format_a8r8g8b8, (e_bitmap_data_flags)0);
				}
				
				if (screenshot_globals->camera_name_set)
				{
					tiled = false;
					bitmap_1 = bitmap_2d_new(
						(int16)(screen_width * horizontal_tiles),
						(int16)(screen_height * vertical_tiles),
						0,
						_bitmap_format_x8r8g8b8,
						(e_bitmap_data_flags)0);
				}
				else
				{
					tiled = screenshot_globals->resolution_multiplier > 1;
					if (screenshot_globals->resolution_multiplier <= 1)
					{
						bitmap_1 = bitmap_0;
					}
					else
					{
						bitmap_1 = bitmap_2d_new(screen_width, screen_height, 0, _bitmap_format_x8r8g8b8, (e_bitmap_data_flags)0);
					}
				}
			}

			if (bitmap_0 &&
				bitmap_0->base_address &&
				bitmap_1 &&
				bitmap_1->base_address &&
				(!screenshot_globals->web_map || bitmap_secondary))
			{
				geometry_cache_block_for_one_frame(2);
				texture_cache_block_for_one_frame(2);
				bool rendered_bloom_screenshot = render_bloom_screenshot(bitmap_0->width, bitmap_0->height, window);

				char file_path[512];
				char file_path_depth[512];
				if (movie_globals->in_progress)
				{
					snprintf(file_path, NUMBEROF(file_path), "movie\\frame%06d.tif", movie_globals->recording_frame_index);
				}
				else if (screenshot_globals->save_as_tga)
				{
					snprintf(file_path, NUMBEROF(file_path), "%s.tga", screenshot_globals->camera_name);
					snprintf(file_path_depth, NUMBEROF(file_path_depth), "%s_depth.tga", screenshot_globals->camera_name);
				}
				else
				{
					snprintf(file_path, NUMBEROF(file_path), "%s.tif", screenshot_globals->camera_name);
					snprintf(file_path_depth, NUMBEROF(file_path_depth), "%s_depth.tif", screenshot_globals->camera_name);
				}

				s_file_reference file_reference_normal;
				s_file_reference file_reference_depth;

				file_reference_create_from_path(&file_reference_normal, file_path, false);
				file_create_parent_directories_if_not_present(&file_reference_normal);

				if (screenshot_globals->web_map)
				{
					file_reference_create_from_path(&file_reference_depth, file_path_depth, false);
					file_create_parent_directories_if_not_present(&file_reference_depth);
				}

				void* tiff_data;
				if (tiled && !screenshot_globals->save_as_tga)
				{
					tiff_export_begin_tiled(&file_reference_normal,
						bitmap_0->width * horizontal_tiles,
						bitmap_0->height * vertical_tiles,
						bitmap_0->width,
						bitmap_0->height,
						&tiff_data);
				}

				// If we're capturing a cubemap then we need to make sure tiled is true
				ASSERT(screenshot_globals->capture_cubemap ? tiled : true);

				for (int32 vertical_tile_num = 0; vertical_tile_num < vertical_tiles; ++vertical_tile_num)
				{
					int32 unk_num = 1;
					for (int32 horizontal_tile_num = 0; horizontal_tile_num < horizontal_tiles; ++horizontal_tile_num)
					{
						uint32 frame_render_type = 4;
						bool should_render_frame = true;
						bool is_horizontal_tile = false;
						bool is_vertical_tile = false;
						int32 angle = 0;
						if (screenshot_globals->capture_cubemap)
						{
							int32 face_index = NONE;
							screenshot_cubemap_retrieve_tile_info(
								&is_horizontal_tile,
								&angle,
								&is_vertical_tile,
								horizontal_tile_num,
								vertical_tile_num,
								&face_index);

							if (face_index == NONE)
							{
								should_render_frame = false;
							}
							else
							{
								screenshot_cubemap_set_camera_rotation(face_index, k_screenshot_cubemap_size, &window->render_camera.point, &window->rasterizer_camera);
								window->render_camera = window->rasterizer_camera;
							}
							frame_render_type = 7;
						}
						else if (screenshot_globals->resolution_multiplier > 1)
						{
							const real32 screen_size_per_tile = 1.f / (real32)screenshot_globals->resolution_multiplier;
							if (tiled)
							{
								const int32 vertical_offset = screenshot_globals->resolution_multiplier - vertical_tile_num - 1;
								window->rasterizer_camera.tiled = true;
								window->rasterizer_camera.unk_floats[0] = (unk_num * screen_size_per_tile) - 1.f;
								window->rasterizer_camera.unk_floats[1] = ((2 * vertical_offset + 1) * screen_size_per_tile) - 1.f;
								window->rasterizer_camera.unk_floats[2] = screen_size_per_tile;
							}
							else
							{
								window->rasterizer_camera.bool_2 = true;
								window->rasterizer_camera.frustum_multiplier_x = screen_size_per_tile * horizontal_tile_num;
								window->rasterizer_camera.frustum_multiplier_y = screen_size_per_tile * vertical_tile_num;
							}
							frame_render_type = rendered_bloom_screenshot ? 7 : 4;
						}

						if (!tiled)
						{
							window->render_camera = window->rasterizer_camera;
						}

						if (tile_count > 1)
						{
							window->render_camera.viewport_bounds.left /= tile_count;
							window->render_camera.viewport_bounds.right /= tile_count;
							window->render_camera.viewport_bounds.top /= tile_count;
							window->render_camera.viewport_bounds.bottom /= tile_count;
							window->render_camera.window_bounds.left /= tile_count;
							window->render_camera.window_bounds.right /= tile_count;
							window->render_camera.window_bounds.top /= tile_count;
							window->render_camera.window_bounds.bottom /= tile_count;
							window->rasterizer_camera.viewport_bounds.left /= tile_count;
							window->rasterizer_camera.viewport_bounds.right /= tile_count;
							window->rasterizer_camera.viewport_bounds.top /= tile_count;
							window->rasterizer_camera.viewport_bounds.bottom /= tile_count;
							window->rasterizer_camera.window_bounds.left /= tile_count;
							window->rasterizer_camera.window_bounds.right /= tile_count;
							window->rasterizer_camera.window_bounds.top /= tile_count;
							window->rasterizer_camera.window_bounds.bottom /= tile_count;
						}

						main_loop_pregame(0, 0);
						if (should_render_frame)
						{
							render_frame(frame_render_type, 1, 1, 0, window);
							rasterizer_present_frame_wrapper(bitmap_0);
							if (screenshot_globals->web_map)
							{
								rasterizer_present_frame_screenshot_wrapper(bitmap_secondary);
							}

							if (screenshot_globals->dump_camera_data)
							{
								dump_camera_to_file(screenshot_globals->camera_name, &window->render_camera);
							}
						}
						else
						{
							bitmap_clear_data(bitmap_0, 0);
						}

						if (rendered_bloom_screenshot && tiled)
						{
							if (!screenshot_globals->capture_cubemap)
							{
								main_loop_pregame(0, 0);
								screenshot_calculate_bloom(bitmap_0, horizontal_tile_num, vertical_tile_num);
							}
						}
						
						const bitmap_data* bitmap_tile = screenshot_globals->capture_cubemap ? bitmap_1 : bitmap_0;
						// TODO: better names for everything in this section
						if (screenshot_globals->capture_cubemap)
						{
							ASSERT(screen_width == screen_height);
							int32 cube_x[2];
							int32 cube_y[2];
							int32 rotation[2];
							screenshot_cubemap_retrieve_rotation_info(
								cube_x,
								cube_y,
								angle,
								rotation,
								screen_height,
								screen_width);

							if (screen_height > 0)
							{
								int32 adjusted_x = screen_width - rotation[0] - 1;
								int32 adjusted_y = screen_height - rotation[1] - 1;
								int32 rotated_x_width = rotation[0];
								int32 rotated_y_width = rotation[1];

								for (int16 y = 0; y < screen_height; ++y)
								{
									if (screen_width > 0)
									{
										int32 rotated_x = rotated_x_width;
										int32 rotated_y = rotated_y_width;
										int32 horizontal_tile_x = adjusted_x;
										int32 vertical_tile_y = adjusted_y;
										for (int16 x = 0; x < screen_width; ++x)
										{
											ASSERT(rotated_x >= 0 && rotated_x < screen_width);
											ASSERT(rotated_y >= 0 && rotated_y < screen_height);

											const int32 final_x = is_horizontal_tile ? horizontal_tile_x : rotated_x;
											const int32 final_y = is_vertical_tile ? vertical_tile_y : rotated_y;
											uint32* address_0 = bitmap_2d_address(bitmap_0, x, y, 0);
											uint32* address_1 = bitmap_2d_address(bitmap_1, (int16)final_x, (int16)final_y, 0);
											*address_1 = *address_0;	// Copy data from bitmap 0 to 1

											rotated_x += cube_y[0];
											rotated_y += cube_x[0];
											horizontal_tile_x += -cube_y[0];
											vertical_tile_y += -cube_x[0];
										}
									}

									rotated_x_width += cube_y[1];
									rotated_y_width += cube_x[1];
									adjusted_x += -cube_y[1];
									adjusted_y += -cube_x[1];
								}
							}
						}


						if (tiled)
						{
							if (!screenshot_globals->save_as_tga)
							{
								main_loop_pregame(0, 0);
								tiff_export_add_tile_from_surface(tiff_data, bitmap_tile, horizontal_tile_num * bitmap_0->width, vertical_tile_num* bitmap_0->height);
							}
						}
						// If the bitmaps aren't the same then we copy the address data over from bitmap_0 to bitmap_1
						else if (bitmap_1 != bitmap_0)
						{
							for (int16 y = 0; y < screen_height; ++y)
							{
								for (int16 x = 0; x < screen_width; ++x)
								{
									uint32* address_0 = bitmap_2d_address(bitmap_0, x, y, 0);
									uint32* address_1 = bitmap_2d_address(
										bitmap_1,
										(int16)(horizontal_tile_num + (screenshot_globals->resolution_multiplier * x)),
										(int16)(vertical_tile_num + (screenshot_globals->resolution_multiplier * y)),
										0);
									*address_1 = *address_0;
								}
							}
						}

						// Restore the window to it's original state
						*window = window_copy;

						main_loop_pregame(0, 0);
						unk_num += 2;
					}
				}
				
				// Bitmap exporting

				if (screenshot_globals->save_as_tga)
				{
					targa_export(&file_reference_normal, bitmap_0);
				}
				else if (tiled)
				{
					tiff_export_end_tiled(tiff_data);
				}
				else
				{
					tiff_export_raw(&file_reference_normal, bitmap_1);
				}

				if (screenshot_globals->web_map)
				{
					targa_export(&file_reference_depth, bitmap_secondary);
				}

				// Bitmap cleanup

				if (bitmap_1 != bitmap_0)
				{
					bitmap_data_free(bitmap_1);
				}

				bitmap_data_free(bitmap_0);

				if (screenshot_globals->web_map)
				{
					bitmap_data_free(bitmap_secondary);
				}

				if (rendered_bloom_screenshot)
				{
					bitmap_data_free(screenshot_globals->bloom_bitmap);
					screenshot_globals->bloom_bitmap = NULL;
				}
			}
		}

		// Globals cleanup

		if (movie_globals->in_progress)
		{
			++movie_globals->recording_frame_index;
			if (movie_globals->recording_stop_tick > get_game_time_ticks())
				result = false;
			else
				movie_globals->in_progress = false;
		}
		screenshot_globals->taking_screenshot = false;
		screenshot_globals->take_screenshot = false;
		screenshot_globals->web_map = false;
		screenshot_globals->save_as_tga = false;
		screenshot_globals->dump_camera_data = false;
		screenshot_globals->capture_cubemap = false;
		screenshot_globals->cubemap_screenshot = false;

		// Cleanup rasterizer target

		if (rasterizer_dx9_globals->global_d3d_surface_screenshot)
		{
			rasterizer_dx9_globals->global_d3d_surface_screenshot->Release();
			rasterizer_dx9_globals->global_d3d_surface_screenshot = NULL;
		}
	}

	return result;
}

/* private code */

static bool movie_should_continue(void)
{
	const s_movie_globals* movie_globals = movie_globals_get();
	return movie_globals->in_progress && (!movie_globals->field_1C || movie_globals->recording_frame_index % movie_globals->field_14 == movie_globals->field_10);
}

static bool render_bloom_screenshot(int16 width, int16 height, window_bound* window_bound)
{
	bool result = false;
	s_screenshot_globals* screenshot_globals = get_screenshot_globals();

	if (screenshot_globals->resolution_multiplier > 1 && !screenshot_globals->camera_name_set)
	{
		bitmap_data* bitmap = bitmap_2d_new(width, height, 0, _bitmap_format_x8r8g8b8, (e_bitmap_data_flags)0);
		screenshot_globals->bloom_bitmap = bitmap;

		if (bitmap && bitmap->base_address)
		{
			result = true;
			render_frame(6, 1, 1, 0, window_bound);
			rasterizer_present_frame_wrapper(screenshot_globals->bloom_bitmap);
		}
	}

	return result;
}

static void screenshot_cubemap_retrieve_tile_info(
	bool* is_horizontal_tile,
	int32* angle,
	bool* is_vertical_tile,
	int32 horizontal_tile_num,
	int32 vertical_tile_num,
	int32* face_index)
{
	*is_horizontal_tile = true;
	if (horizontal_tile_num)
	{
		if (horizontal_tile_num == 2)
		{
			if (vertical_tile_num == 1)
			{
				*face_index = 1;
				*angle = -90;
			}
		}
		else if (horizontal_tile_num == 1)
		{
			if (vertical_tile_num == 1)
			{
				*face_index = 2;
				*angle = 0;
			}
		}
		else if (horizontal_tile_num == 3 && vertical_tile_num == 1)
		{
			*face_index = 3;
			*angle = 0;
		}
	}
	else if (vertical_tile_num == 1)
	{
		*face_index = 0;
		*angle = 90;
	}
	else if (vertical_tile_num)
	{
		if (vertical_tile_num == 2)
		{
			*is_vertical_tile = true;
			*is_horizontal_tile = false;
			*face_index = 5;
			*angle = -90;
		}
	}
	else
	{
		*is_vertical_tile = true;
		*is_horizontal_tile = false;
		*face_index = 4;
		*angle = -90;
	}
	return;
}

static void screenshot_cubemap_retrieve_rotation_info(
	int32* x,
	int32* y,
	int32 angle,
	int32* rotation,
	int32 height,
	int32 width)
{
	if (angle)
	{
		switch (angle)
		{
		case 90:
			rotation[1] = height - 1;
			rotation[0] = 0;
			y[0] = 0;
			y[1] = 1;
			x[1] = 0;
			x[0] = -1;
			break;
		case -90:
			rotation[0] = width - 1;
			rotation[1] = 0;
			y[0] = 0;
			y[1] = -1;
			x[1] = 0;
			x[0] = 1;
			break;
		case 180:
			rotation[1] = width - 1;
			rotation[0] = height - 1;
			y[1] = 0;
			y[0] = -1;
			x[0] = 0;
			x[1] = -1;
			break;
		default:
			DISPLAY_ASSERT("unreachable");
		}
	}
	else
	{
		rotation[0] = 0;
		rotation[1] = 0;
		y[1] = 0;
		y[0] = 1;
		x[0] = 0;
		x[1] = 1;
	}
	return;
}

static void __cdecl dump_camera_to_file(const char* camera_name, const render_camera* camera)
{
	INVOKE(0x2735CA, 0x0, dump_camera_to_file, camera_name, camera);
	return;
}

static void __cdecl screenshot_cubemap_set_camera_rotation(int32 face_index, int32 resolution, const real_point3d* location, render_camera* camera)
{
	INVOKE(0x2788CB, 0x0, screenshot_cubemap_set_camera_rotation, face_index, resolution, location, camera);
	return;
}

static void screenshot_calculate_bloom(bitmap_data* screenshot_bitmap, int32 horizontal_tile, int32 vertical_tile)
{
	s_screenshot_globals* screenshot_globals = get_screenshot_globals();

	ASSERT(screenshot_globals->bloom_bitmap);

	const int16 width = screenshot_globals->bloom_bitmap->width;
	const int16 height = screenshot_globals->bloom_bitmap->height;
	
	ASSERT(screenshot_bitmap->width == width);
	ASSERT(screenshot_bitmap->height == height);

	screenshot_globals->bloom_bitmap->format = (screenshot_globals->bloom_bitmap->format == _bitmap_format_x8r8g8b8 ? _bitmap_format_a8r8g8b8 : screenshot_globals->bloom_bitmap->format);
	screenshot_bitmap->format = (screenshot_bitmap->format == _bitmap_format_x8r8g8b8 ? _bitmap_format_a8r8g8b8 : screenshot_bitmap->format);

	bitmap_data* height_map = bitmap_2d_height_map(
		screenshot_globals->bloom_bitmap,
		(real32)((real32)horizontal_tile * (real32)width) / (real32)screenshot_globals->resolution_multiplier,
		(real32)((real32)vertical_tile * (real32)height) / (real32)screenshot_globals->resolution_multiplier,
		(real32)width / (real32)screenshot_globals->resolution_multiplier,
		(real32)height / (real32)screenshot_globals->resolution_multiplier,
		width,
		height);

	if (height_map && height_map->base_address)
	{
		bitmap_copy(height_map, screenshot_bitmap);
		bitmap_data_free(height_map);
	}

	screenshot_globals->bloom_bitmap->format = (screenshot_globals->bloom_bitmap->format == _bitmap_format_a8r8g8b8 ? _bitmap_format_x8r8g8b8 : screenshot_globals->bloom_bitmap->format);
	screenshot_bitmap->format = (screenshot_bitmap->format == _bitmap_format_a8r8g8b8 ? _bitmap_format_x8r8g8b8 : screenshot_bitmap->format);
	return;
}
