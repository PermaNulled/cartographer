#include "stdafx.h"

#include "Halloween.h"
#include "MapObjectPlacements/Halloween/Coagulation.h"
#include "MapObjectPlacements/Halloween/Lockout.h"

#include "Blam/Cache/TagGroups/model_definition.hpp"
#include "Blam/Cache/TagGroups/render_model_definition.hpp"
#include "Blam/Cache/TagGroups/scenario_definition.hpp"
#include "Blam/Cache/TagGroups/scenario_lightmap_definition.hpp"
#include "Blam/Cache/TagGroups/scenario_structure_bsp_definition.hpp"
#include "Blam/Cache/TagGroups/scenery_definition.hpp"
#include "Blam/Engine/Game/GameGlobals.h"
#include "Blam/Engine/Players/Players.h"
#include "Blam/Engine/Objects/Objects.h"
#include "H2MOD/Modules/EventHandler/EventHandler.hpp"
#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Tags/MetaExtender.h"
#include "H2MOD/Tags/MetaLoader/tag_loader.h"

datum lbitm_datum;
datum sky_datum;
datum candle_datum;
datum candle_fire_datum;
datum large_candle_datum;
datum pump_datum;

void halloween_game_life_cycle_update(e_game_life_cycle state)
{
	if (state == _life_cycle_in_game)
	{
		if (H2Config_spooky_boy) {
			*Memory::GetAddress<s_player::e_character_type*>(0x51A67C) = s_player::e_character_type::Skeleton;
		}

		typedef void(__cdecl t_set_orientation)(real_vector3d* forward, real_vector3d* up, const real_vector3d* orient);
		auto set_orientation = Memory::GetAddress<t_set_orientation*>(0x3347B);

		object_placement_data placement;

		auto pump = tags::get_tag<blam_tag::tag_group_type::scenery, s_scenery_group_definition>(pump_datum, true);
		auto pump_hmlt = tags::get_tag<blam_tag::tag_group_type::model, s_model_group_definition>(pump->objectTag.model.TagIndex, true);

		const s_cache_header* cache_header = tags::get_cache_header();
		if (!strcmp(cache_header->name, "coagulation"))
		{
			for (auto& scen_place : coag_scen_places)
			{
				// Set type of object and variant
				switch (scen_place.type)
				{
				case 0:
					object_placement_data_new(&placement, pump_datum, -1, 0);
					placement.variant_name = pump_hmlt->variants[scen_place.variant_id]->name.get_packed();
					break;
				case 1:
					object_placement_data_new(&placement, candle_datum, -1, 0);
					placement.variant_name = 0;
					break;
				case 2:
					object_placement_data_new(&placement, large_candle_datum, -1, 0);
					placement.variant_name = 0;
					break;
				}
				// Set location orientation and scale
				placement.position = scen_place.position;
				set_orientation(&placement.forward, &placement.up, &scen_place.rotation);
				placement.scale = scen_place.scale;

				// Create the new object
				object_new(&placement);
			}
		}
		else if (!strcmp(cache_header->name, "lockout"))
		{
			for (auto& scen_place : lockout_scen_places)
			{
				// Set type of object and variant
				switch (scen_place.type)
				{
				case 0:
					object_placement_data_new(&placement, pump_datum, -1, 0);
					placement.variant_name = pump_hmlt->variants[scen_place.variant_id]->name.get_packed();
					break;
				case 1:
					object_placement_data_new(&placement, candle_datum, -1, 0);
					placement.variant_name = 0;
					break;
				}
				// Set location orientation and scale
				placement.position = scen_place.position;
				set_orientation(&placement.forward, &placement.up, &scen_place.rotation);
				placement.scale = scen_place.scale;

				// Create the new object
				object_new(&placement);
			}
		}
	}
}

void halloween_event_map_load()
{
	// Load specific tags from shared and modify placements depending on the map being played
	const s_cache_header* cache_header = tags::get_cache_header();
	auto scnr = tags::get_tag_fast<s_scenario_group_definition>(tags::get_tags_header()->scenario_datum);
	auto sbps = tags::get_tag_fast< s_scenario_structure_bsp_group_definition>(scnr->structure_bsps[0]->structure_bsp.TagIndex);
	if (!strcmp(cache_header->name, "coagulation"))
	{
		lbitm_datum = tag_loader::Get_tag_datum("scenarios\\multi\\halo\\coagulation\\coagulation_coagulation_lightmap_truecolor_bitmaps", blam_tag::tag_group_type::bitmap, "carto_shared");
		sky_datum = tag_loader::Get_tag_datum("scenarios\\skies\\multi\\halo\\coagulation\\coagulation_night", blam_tag::tag_group_type::sky, "carto_shared");
		candle_fire_datum = tag_loader::Get_tag_datum("scenarios\\objects\\multi\\carto_shared\\jack_o_lantern\\candle\\candle_fire", blam_tag::tag_group_type::scenery, "carto_shared");
		candle_datum = tag_loader::Get_tag_datum("scenarios\\objects\\multi\\carto_shared\\jack_o_lantern\\candle\\candle", blam_tag::tag_group_type::scenery, "carto_shared");
		pump_datum = tag_loader::Get_tag_datum("scenarios\\objects\\multi\\carto_shared\\jack_o_lantern\\jack_o_lantern", blam_tag::tag_group_type::scenery, "carto_shared");
		large_candle_datum = tag_loader::Get_tag_datum("scenarios\\objects\\multi\\carto_shared\\jack_o_lantern\\candle\\candle_big_light", blam_tag::tag_group_type::scenery, "carto_shared");

		tag_loader::Load_tag(pump_datum, true, "carto_shared");
		tag_loader::Load_tag(candle_datum, true, "carto_shared");
		tag_loader::Load_tag(lbitm_datum, true, "carto_shared");
		tag_loader::Load_tag(sky_datum, true, "carto_shared");
		tag_loader::Load_tag(large_candle_datum, true, "carto_shared");
		tag_loader::Push_Back();

		// OG Halo 2 Coag lightmap
		datum ltmp_datum = tags::find_tag(blam_tag::tag_group_type::scenariostructurelightmap,
			"scenarios\\multi\\halo\\coagulation\\coagulation_coagulation_lightmap");

		candle_datum = tag_loader::ResolveNewDatum(candle_datum);
		candle_fire_datum = tag_loader::ResolveNewDatum(candle_fire_datum);
		pump_datum = tag_loader::ResolveNewDatum(pump_datum);
		large_candle_datum = tag_loader::ResolveNewDatum(large_candle_datum);
		lbitm_datum = tag_loader::ResolveNewDatum(lbitm_datum);
		sky_datum = tag_loader::ResolveNewDatum(sky_datum);

		LOG_INFO_GAME("{:x}", candle_datum);
		LOG_INFO_GAME("{:x}", candle_fire_datum);
		LOG_INFO_GAME("{:x}", pump_datum);
		LOG_INFO_GAME("{:x}", lbitm_datum);
		LOG_INFO_GAME("{:x}", sky_datum);

		if (!DATUM_IS_NONE(sky_datum))
		{
			scnr->skies[0]->sky.TagIndex = sky_datum;
		}
		if (!DATUM_IS_NONE(ltmp_datum) && !DATUM_IS_NONE(lbitm_datum))
		{
			auto ltmp = tags::get_tag_fast<s_scenario_structure_lightmap_group_definition>(ltmp_datum);
			ltmp->lightmap_groups[0]->bitmap_group.TagIndex = lbitm_datum;
			sbps->decorators_block.size = 0;
			sbps->decorators_block.data = 0;
		}

		// Add items to scenario
		if (!DATUM_IS_NONE(candle_datum) && !DATUM_IS_NONE(pump_datum) && !DATUM_IS_NONE(large_candle_datum))
		{
			EventHandler::register_callback(halloween_game_life_cycle_update, EventType::gamelifecycle_change, EventExecutionType::execute_after, true);
			// We execute this after a bluescreen since our new objects arent recreated automatically
			EventHandler::register_callback(halloween_game_life_cycle_update, EventType::blue_screen, EventExecutionType::execute_after, true);
		}
	}
	else if (!strcmp(cache_header->name, "lockout"))
	{
		candle_datum = tag_loader::Get_tag_datum("scenarios\\objects\\multi\\carto_shared\\jack_o_lantern\\candle\\candle", blam_tag::tag_group_type::scenery, "carto_shared");
		candle_fire_datum = tag_loader::Get_tag_datum("scenarios\\objects\\multi\\carto_shared\\jack_o_lantern\\candle\\candle_fire", blam_tag::tag_group_type::scenery, "carto_shared");
		pump_datum = tag_loader::Get_tag_datum("scenarios\\objects\\multi\\carto_shared\\jack_o_lantern\\jack_o_lantern", blam_tag::tag_group_type::scenery, "carto_shared");

		tag_loader::Load_tag(pump_datum, true, "carto_shared");
		tag_loader::Load_tag(candle_datum, true, "carto_shared");
		tag_loader::Push_Back();

		candle_datum = tag_loader::ResolveNewDatum(candle_datum);
		candle_fire_datum = tag_loader::ResolveNewDatum(candle_fire_datum);
		pump_datum = tag_loader::ResolveNewDatum(pump_datum);

		LOG_INFO_GAME("{:x}", candle_datum);
		LOG_INFO_GAME("{:x}", candle_fire_datum);
		LOG_INFO_GAME("{:x}", pump_datum);

		if (!DATUM_IS_NONE(candle_datum) && !DATUM_IS_NONE(pump_datum))
		{
			EventHandler::register_callback(halloween_game_life_cycle_update, EventType::gamelifecycle_change, EventExecutionType::execute_after, true);
			// We execute this after a bluescreen since our new objects arent recreated automatically
			EventHandler::register_callback(halloween_game_life_cycle_update, EventType::blue_screen, EventExecutionType::execute_after, true);
		}
	}
}
