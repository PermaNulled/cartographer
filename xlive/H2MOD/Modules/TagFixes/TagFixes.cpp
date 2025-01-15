#include "stdafx.h"
#include "TagFixes.h"

#include "shaders/shader_definitions.h"
#include "units/biped_definitions.h"

#include "H2MOD/Modules/Shell/Config.h"
#include "objects/light_definitions.h"

// ### TODO Cleanup
extern bool g_xbox_tickrate_enabled;

namespace TagFixes
{
	namespace
	{
		void fix_shader_template_nvidia(const char *template_name, const char* bitmap_name, int32 bitmap_idx)
		{
			datum bitmap_to_fix = tag_loaded(_tag_group_bitmap, bitmap_name);
			datum borked_template = tag_loaded(_tag_group_shader_template, template_name);
			LOG_DEBUG_FUNC("bitmap {0}, borked_template {1}", bitmap_to_fix, borked_template);
			if (bitmap_to_fix == NONE || borked_template == NONE)
				return;
			LOG_DEBUG_FUNC("Fixing: template {}, bitmap {}", template_name, bitmap_name);

			tag_iterator iterator;
			tag_iterator_new(&iterator, _tag_group_shader);

			while (tag_iterator_next(&iterator) != NONE)
			{
				char* shader = (char*)tag_get_fast(iterator.current_tag_index);
				if(shader != nullptr)
				{
					tag_reference* shader_template = reinterpret_cast<tag_reference*>(shader);
					if(shader_template->index == borked_template)
					{
						tag_block<int> *shader_post = reinterpret_cast<tag_block<int>*>(shader + 0x20);
						if(shader_post->count > 0)
						{
							auto shader_post_data = cache_get_tag_data(shader_post->data);
							auto shader_post_bitmap = reinterpret_cast<tag_block<int>*>(shader_post_data + 0x4);
							if(shader_post_bitmap->count >= bitmap_idx + 1)
							{
								auto bitmap_data = cache_get_tag_data( (shader_post_bitmap->data + (bitmap_idx * 0xC)));
								datum* bitmap = reinterpret_cast<datum*>(bitmap_data);
								if(*bitmap == bitmap_to_fix)
									*bitmap = NONE;
							}
						}
					}
				}
			}
		}
		void fix_shaders_nvidia()
		{
			fix_shader_template_nvidia(
				"shaders\\shader_templates\\opaque\\tex_bump_alpha_test_single_pass",
				"shaders\\default_bitmaps\\bitmaps\\alpha_white",
				4
			);

			fix_shader_template_nvidia(
				"shaders\\shader_templates\\opaque\\tex_bump_alpha_test",
				"shaders\\default_bitmaps\\bitmaps\\gray_50_percent",
				1
			);
		}

		void font_table_fix()
		{
			WriteValue<int>(Memory::GetAddress(0x464940), 0);
			WriteValue<int>(Memory::GetAddress(0x46494C), 0);
			WriteValue<int>(Memory::GetAddress(0x464958), 0);
			WriteValue<int>(Memory::GetAddress(0x464964), 0);
		}

		void shader_lod_max()
		{
			tag_iterator iterator;
			tag_iterator_new(&iterator, _tag_group_shader);

			while (tag_iterator_next(&iterator) != NONE)
			{
				s_shader_definition* shader = (s_shader_definition*)tag_get_fast(iterator.current_tag_index);
				shader->shader_lod_bias = _shader_lod_bias_never;
			}
		}

		void light_framerate_killer()
		{
			tag_iterator iterator;
			tag_iterator_new(&iterator, _tag_group_light);

			while (tag_iterator_next(&iterator) != NONE)
			{
				light_definition* light = (light_definition*)tag_get_fast(iterator.current_tag_index);
				// Disabled since it caused issues where certain lights wouldnt render randomly
				// TODO figure out why it does this at some other point in time
				// light->flags.set(_light_definition_light_framerate_killer, true);

				light->flags.set(_light_definition_multiplayer_override, true);
			}
		}

		void fall_damage_fix()
		{
			tag_iterator iterator;
			tag_iterator_new(&iterator, _tag_group_biped);

			while (tag_iterator_next(&iterator) != NONE)
			{
				biped_definition* biped = (biped_definition*)tag_get_fast(iterator.current_tag_index);
				biped->object.abs_acceleration.lower *= 2.f;
				biped->object.abs_acceleration.upper *= 2.f;
			}
		}
	}

	void OnMapLoad()
	{
		if (!Memory::IsDedicatedServer()) 
		{
			fix_shaders_nvidia();
			font_table_fix();
			if (H2Config_shader_lod_max)
			{
				shader_lod_max();
			}
			if (H2Config_light_suppressor)
			{
				light_framerate_killer();
			}
		}
		if (!g_xbox_tickrate_enabled)
		{
			// both client/server
			fall_damage_fix();
		}
	}
}
