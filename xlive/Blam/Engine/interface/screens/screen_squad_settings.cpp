#include "stdafx.h"
#include "screen_squad_settings.h"
#include "screen_single_player_difficulty_select.h"
#include "screen_single_player_level_select.h"

#include "bitmaps/bitmap_group.h"
#include "cache/cache_files.h"
#include "interface/user_interface_controller.h"
#include "interface/user_interface_networking.h"
#include "interface/user_interface_memory.h"
#include "interface/user_interface_bitmap_block.h"
#include "interface/user_interface_globals.h"
#include "main/levels.h"
#include "main/level_definitions.h"
#include "networking/logic/life_cycle_manager.h"
#include "saved_games/game_variant.h"
#include "tag_files/global_string_ids.h"
#include "tag_files/tag_loader/tag_injection.h"

/* macro defines */

#define k_squad_setting_list_name "squad setting list"

/* enums */

enum e_squad_list_items : uint16
{
	//h2v
	//_item_change_map = 0,
	//_item_change_variant,
	//_item_quick_options,
	//_item_party_management,
	//_item_type_4,
	//_item_switch_to_arranged,
	//_item_switch_to_optimatch,
	//_item_change_hopper,

	//h2x
	_item_change_map = 0,
	_item_change_variant,
	_item_change_level,
	_item_change_difficulty,
	_item_quick_options,
	_item_change_playlist = 5, //unused in h2x
	_item_switch_to_coop,
	_item_switch_to_arranged,
	_item_switch_to_optimatch,
	_item_change_hopper,
	
	//h2v addition
	_item_party_management,	
	k_total_no_of_squad_list_items = 0xA
};

enum e_squad_settings_dialog_text_blocks
{
	_squad_settings_dialog_pane_0_text_change_map_help = 0,
	_squad_settings_dialog_pane_0_text_current_map,
	_squad_settings_dialog_pane_0_text_map,
	k_squad_settings_dialog_pane_0_text_count
};

enum e_squad_settings_dialog_bitmap_blocks
{
	_squad_settings_dialog_pane_0_bitmap_fs_dialog = 0,
	_squad_settings_dialog_pane_0_bitmap_1,
	_squad_settings_dialog_pane_0_bitmap_2,
	_squad_settings_dialog_pane_0_bitmap_3,
	_squad_settings_dialog_pane_0_bitmap_xbox_live,
	k_squad_settings_dialog_pane_0_bitmap_count
};

enum e_squad_settings_dialog_local_bitmap_blocks
{
	_squad_settings_dialog_local_bitmap_difficulty_options = 0,
	_squad_settings_dialog_local_bitmap_mp_games,
	k_squad_settings_dialog_local_bitmap_count
};

enum e_local_bitmap_difficulty_options
{
	_difficulty_option_easy = 0,
	_difficulty_option_normal,
	_difficulty_option_heroic,
	_difficulty_option_legendary
};

enum e_local_bitmap_mp_games
{
	_mp_game_type_slayer = 0,
	_mp_game_type_koth,
	_mp_game_type_2,
	_mp_game_type_oddball,
	_mp_game_type_juggernaut,
	_mp_game_type_5,
	_mp_game_type_ctf,
	_mp_game_type_assualt,
	_mp_game_type_territories,
};

enum e_xbox_live_bitmap_type
{
	//h2x
	//_xbox_live_bitmap_type_match_settings=0,
	//_xbox_live_bitmap_type_2,
	//_xbox_live_bitmap_type_quick_options,
	//_xbox_live_bitmap_type_3,
	//_xbox_live_bitmap_type_change_hopper,
	//_xbox_live_bitmap_type_5,
	//_xbox_live_bitmap_type_switch_to_arranged,
	//_xbox_live_bitmap_type_switch_to_coop,

	//h2v
	_xbox_live_bitmap_type_unknown_map = 0,
	_xbox_live_bitmap_type_party_management,
	_xbox_live_bitmap_type_quick_options,
	
	//custom bitmaps additions
	_xbox_live_bitmap_type_switch_to_coop,
	k_number_of_xbox_live_bitmap_types,
};

enum e_xbox_live_menu_bitmap_type
{
	//h2v
	_xbox_live_menu_bitmap_type_0 = 0,
	_xbox_live_menu_bitmap_type_switch_to_optimatch,
	_xbox_live_menu_bitmap_type_2,
	_xbox_live_menu_bitmap_type_3,
	_xbox_live_menu_bitmap_type_4,
	_xbox_live_menu_bitmap_type_5,

	k_number_of_xbox_live_menu_bitmap_types,
};

enum e_settings_variant_bitmap_type
{
	_settings_variant_bitmap_type_default = 0,
	k_number_of_settings_variant_bitmap_types,
};

/* globals */

datum new_xbox_live_bitmap_datum = NONE;
datum xbox_live_menu_bitmap_datum = NONE;
datum variant_bitmap_datum = NONE;

c_squad_settings_list::c_squad_settings_list(int16 user_flags) :
	c_list_widget(user_flags),
	m_slot(this, &c_squad_settings_list::handle_item_pressed_event)
{
	m_list_data = ui_list_data_new(k_squad_setting_list_name, k_total_no_of_squad_list_items, sizeof(s_list_item_datum));
	data_make_valid(m_list_data);

	m_party_mgmt_item_deleted = true;

	// yes this sucks
#define SQUAD_ITEM_GET_NEW() \
		(static_cast<s_list_item_datum*>(datum_get(m_list_data, datum_new(m_list_data))))

	const e_session_protocol active_protocol = user_interface_squad_get_active_protocol();
	switch (active_protocol)
	{
	case _session_protocol_splitscreen_custom:
	case _session_protocol_system_link_custom:
		SQUAD_ITEM_GET_NEW()->item_id = _item_change_map;
		SQUAD_ITEM_GET_NEW()->item_id = _item_change_variant;
		SQUAD_ITEM_GET_NEW()->item_id = _item_quick_options;
		SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_coop;
		break;

	case _session_protocol_xbox_live_custom:
		SQUAD_ITEM_GET_NEW()->item_id = _item_change_map;
		SQUAD_ITEM_GET_NEW()->item_id = _item_change_variant;
		SQUAD_ITEM_GET_NEW()->item_id = _item_quick_options;
		SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_coop;
		SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_optimatch;

		if (user_interface_squad_local_peer_is_leader() && user_interface_squad_get_player_count() > 1)
		{
			SQUAD_ITEM_GET_NEW()->item_id = _item_party_management;
			m_party_mgmt_item_deleted = false;
		}
		else
		{
			m_party_mgmt_item_deleted = true;
		}
		break;


	case _session_protocol_splitscreen_coop:
	case _session_protocol_system_link_coop:
		SQUAD_ITEM_GET_NEW()->item_id = _item_change_level;
		SQUAD_ITEM_GET_NEW()->item_id = _item_change_difficulty;
		SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_arranged;
		break;
	case _session_protocol_xbox_live_coop:
		SQUAD_ITEM_GET_NEW()->item_id = _item_change_level;
		SQUAD_ITEM_GET_NEW()->item_id = _item_change_difficulty;
		SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_arranged;
		SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_optimatch;
		break;
	case _session_protocol_xbox_live_optimatch:
		SQUAD_ITEM_GET_NEW()->item_id = _item_change_hopper;
		SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_coop;
		SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_arranged;
		break;
	}

	// create all list items at once
	//SQUAD_ITEM_GET_NEW()->item_id = _item_change_map;
	//SQUAD_ITEM_GET_NEW()->item_id = _item_change_variant;
	//SQUAD_ITEM_GET_NEW()->item_id = _item_change_level;
	//SQUAD_ITEM_GET_NEW()->item_id = _item_change_difficulty;
	//SQUAD_ITEM_GET_NEW()->item_id = _item_quick_options;
	//SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_coop;
	//SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_arranged;
	//SQUAD_ITEM_GET_NEW()->item_id = _item_switch_to_optimatch;
	//SQUAD_ITEM_GET_NEW()->item_id = _item_change_hopper;
	//SQUAD_ITEM_GET_NEW()->item_id = _item_party_management;

#undef SQUAD_ITEM_GET_NEW

	linker_type2.link(&this->m_slot);
}

uint16 c_squad_settings_list::get_last_item_type()
{
	datum item_idx = this->get_old_data_index();
	s_list_item_datum* item_datum = (s_list_item_datum*)datum_try_and_get(this->m_list_data, item_idx);
	if (item_datum)
		return item_datum->item_id;
	return NONE;
}

bool c_squad_settings_list::party_management_exists() const
{
	return !this->m_party_mgmt_item_deleted;
}

void c_squad_settings_list::party_management_delete_item()
{
	c_list_item_widget* item = this->try_find_item_widget(_item_party_management);
	this->remove_focused_item_datum_from_data_array();

	const datum item_index_first = 0;
	this->set_focused_item_index(item_index_first);
	this->remove_item_from_list(item);
	this->m_party_mgmt_item_deleted = true;
}

bool c_squad_settings_list::handle_event(s_event_record* event)
{
	return INVOKE_TYPE(0x24F979, 0x0, bool(__thiscall*)(c_squad_settings_list*, s_event_record*), this, event);
}

c_list_item_widget* c_squad_settings_list::get_list_items()
{
	//return INVOKE_TYPE(0x24FCF8, 0x0, c_list_item_widget*(__thiscall*)(c_squad_settings_list*), this);
	return this->m_list_items;
}

int32 c_squad_settings_list::get_list_items_count()
{
	//return INVOKE_TYPE(0x24FCFF, 0x0, int32(__thiscall*)(c_squad_settings_list*), this);
	return k_no_of_visible_items_for_squad_settings;
}

void c_squad_settings_list::update_list_items(c_list_item_widget* item, int32 skin_index)
{
	//INVOKE_TYPE(0x24EEEF, 0x0, void(__thiscall*)(c_squad_settings_list*, c_list_item_widget*, int32), this, item, skin_index);

	s_item_text_mapping items_map[k_total_no_of_squad_list_items] =
	{
		{_item_change_map			, _string_id_change_map				},
		{_item_change_variant		, _string_id_change_variant			},
		{_item_change_level			, _string_id_change_level			},
		{_item_change_difficulty	, _string_id_change_difficulty		},
		{_item_quick_options		, _string_id_quick_options			},
		{_item_switch_to_coop		, _string_id_switch_to_coop			},
		{_item_switch_to_arranged	, _string_id_switch_to_arranged		},
		{_item_switch_to_optimatch 	, _string_id_switch_to_optimatch	},
		{_item_change_hopper		, _string_id_change_hopper			},
		{_item_party_management		, _string_id_party_management		}
	};

	this->update_list_items_from_mapping(item, skin_index, 0, items_map, k_total_no_of_squad_list_items);

}

void c_squad_settings_list::handle_item_pressed_event(s_event_record** pevent, datum* pitem_index)
{
	//return INVOKE_TYPE(0x24FA19, 0x0, void(__thiscall*)(c_squad_settings_list*, s_event_record**, long*), this, pevent, pitem_index);

	if (*pitem_index != NONE)
	{
		s_list_item_datum* item = (s_list_item_datum*)datum_try_and_get(m_list_data, *pitem_index);
		e_squad_list_items item_type = (e_squad_list_items)item->item_id;

		switch (item_type)
		{
		case _item_change_map:
			this->handle_item_change_map(pevent);
			break;
		case _item_change_variant:
			this->handle_item_change_variant(pevent);
			break;
		case _item_change_level:
			this->handle_item_change_level(pevent);
			break;
		case _item_change_difficulty:
			this->handle_item_change_difficulty(pevent);
			break;
		case _item_quick_options:
			this->handle_item_quick_options(pevent);
			break;
		case _item_switch_to_coop:
			this->handle_item_switch_to_coop(pevent);
			break;
		case _item_switch_to_arranged:
			this->handle_item_switch_to_arranged(pevent);
			break;
		case _item_switch_to_optimatch:
			this->handle_item_switch_to_optimatch(pevent);
			break;
		case _item_change_hopper:
			this->handle_item_change_hopper(pevent);
			break;
		case _item_party_management:
			this->handle_item_party_management(pevent);
			break;

		}
	}
}

void c_squad_settings_list::handle_item_change_map(s_event_record** pevent)
{
	return INVOKE_TYPE(0x24F9A1, 0x0, void(__thiscall*)(c_squad_settings_list*, s_event_record**), this, pevent);
}
void c_squad_settings_list::handle_item_change_variant(s_event_record** pevent)
{
	return INVOKE_TYPE(0x24F9DD, 0x0, void(__thiscall*)(c_squad_settings_list*, s_event_record**), this, pevent);
}
void c_squad_settings_list::handle_item_change_level(s_event_record** pevent)
{
	s_screen_parameters params;
	params.m_flags = 0;
	params.m_window_index = _window_4;
	params.m_context = 0;
	params.user_flags = FLAG((*pevent)->controller);
	params.m_channel_type = _user_interface_channel_type_dialog;
	params.m_screen_state.field_0 = 0xFFFFFFFF;
	params.m_screen_state.m_last_focused_item_order = 0xFFFFFFFF;
	params.m_screen_state.m_last_focused_item_index = 0xFFFFFFFF;
	params.m_load_function = c_screen_single_player_level_select::load_lobby;
	params.m_load_function(&params);
}
void c_squad_settings_list::handle_item_change_difficulty(s_event_record** pevent)
{
	s_screen_parameters params;
	params.m_flags = 0;
	params.m_window_index = _window_4;
	params.m_context = 0;
	params.user_flags = FLAG((*pevent)->controller);
	params.m_channel_type = _user_interface_channel_type_dialog;
	params.m_screen_state.field_0 = 0xFFFFFFFF;
	params.m_screen_state.m_last_focused_item_order = 0xFFFFFFFF;
	params.m_screen_state.m_last_focused_item_index = 0xFFFFFFFF;
	params.m_load_function = c_screen_single_player_difficulty_select::load_lobby;
	params.m_load_function(&params);
}
void c_squad_settings_list::handle_item_quick_options(s_event_record** pevent)
{
	return INVOKE_TYPE(0x24EF79, 0x0, void(__thiscall*)(c_squad_settings_list*, s_event_record**), this, pevent);
}
void c_squad_settings_list::handle_item_switch_to_coop(s_event_record** pevent)
{
	if (user_interface_globals_is_beta_build())
	{
		screen_error_ok_dialog_show(_user_interface_channel_type_game_error, _ui_error_beta_feature_disabled, _window_4, FLAG((*pevent)->controller), 0, 0);
	}
	else if (network_squad_session_get_session_class() == _network_session_class_system_link
		|| network_squad_session_get_session_class() == _network_session_class_xbox_live)
	{
		screen_error_ok_dialog_with_custom_text(
			_user_interface_channel_type_dialog,
			_ui_error_generic, _window_4,
			FLAG((*pevent)->controller), nullptr, nullptr, L"Access Denied", L"Coming Soon....");
	}
	else
	{
		user_interface_squad_clear_game_settings();
		user_interface_set_desired_multiplayer_mode(0);
		int32 difficulty = user_interface_globals_get_game_difficulty();
		user_interface_globals_set_game_difficulty_real(difficulty);

		user_interface_globals_set_loading_from_persistent_storage(false);
		user_interface_squad_set_campaign_difficulty(difficulty);

		this->get_parent_screen()->start_widget_animation(3);
	}
}
void c_squad_settings_list::handle_item_switch_to_arranged(s_event_record** pevent)
{
	return INVOKE_TYPE(0x24F015, 0x0, void(__thiscall*)(c_squad_settings_list*, s_event_record**), this, pevent);
}
void c_squad_settings_list::handle_item_switch_to_optimatch(s_event_record** pevent)
{
	// maybe someday
	//return INVOKE_TYPE(0x211BA1, 0x0, void(__thiscall*)(c_squad_settings_list*, s_event_record**), this, pevent);

	screen_error_ok_dialog_with_custom_text(
		_user_interface_channel_type_dialog,
		_ui_error_generic, _window_4,
		FLAG((*pevent)->controller), nullptr, nullptr, L"Alert", L"This feature is not currently available");
}
void c_squad_settings_list::handle_item_change_hopper(s_event_record** pevent)
{
	//return INVOKE_TYPE(0x24F68A, 0x0, void(__thiscall*)(c_squad_settings_list*, s_event_record**), this, pevent);
}
void c_squad_settings_list::handle_item_party_management(s_event_record** pevent)
{
	// TODO : figure out why this is broken or invoke a custom menu to handle this
	return INVOKE_TYPE(0x24F5FD, 0x0, void(__thiscall*)(c_squad_settings_list*, s_event_record**), this, pevent);
}



//
// c_screen_squad_settings class starts here
// 



c_screen_squad_settings::c_screen_squad_settings(e_user_interface_channel_type channel_type, e_user_interface_render_window window_index, int16 user_flags) :
	c_screen_widget(_screen_squad_settings, channel_type, window_index, user_flags),
	m_squad_settings_list(user_flags)
{
}

void c_screen_squad_settings::update()
{
	//INVOKE_TYPE(0x24F0EB, 0x0, void(__thiscall*)(c_screen_squad_settings*), this);

	e_squad_list_items item_type = (e_squad_list_items)this->m_squad_settings_list.get_last_item_type();
	c_text_widget* option_help_text_block = this->try_find_screen_text(_squad_settings_dialog_pane_0_text_change_map_help);
	c_text_widget* option_header_text_block = this->try_find_screen_text(_squad_settings_dialog_pane_0_text_current_map);
	c_text_widget* option_value_text_block = this->try_find_screen_text(_squad_settings_dialog_pane_0_text_map);
	c_bitmap_widget* option_bitmap = this->try_find_bitmap_widget(_squad_settings_dialog_pane_0_bitmap_xbox_live);

	if (option_bitmap)
		option_bitmap->assign_new_bitmap_block(nullptr);

	uint32 campaign_id, map_id, custom_map_id;
	bool map_is_set = user_interface_session_get_map(&campaign_id, &map_id, &custom_map_id);

	string_id help_string = _string_id_empty_string;
	string_id header_string = _string_id_empty_string;
	string_id value_string = _string_id_empty_string;
	int16 bitm_index = 0;

	s_game_variant* game_variant = user_interface_session_get_game_variant();
	real_vector2d option_render_scale = { 1.f, 1.f };	// default value

	switch (item_type)
	{
	case _item_change_map:
		help_string = _string_id_change_map_help;
		header_string = _string_id_current_map;
		value_string = _string_id_map;

		if (map_is_set && option_bitmap)
		{
			if (custom_map_id)
			{
				// TODO : fix custom map images once we get them working
				//	if (levels_get_custom_map_ui_level_definition(custom_map_id, (int)&block))
				//		option_bitmap->assign_new_bitmap_block(block);
			}
			else
			{
				s_multiplayer_ui_level_definition* level_definition = levels_get_multiplayer_ui_level(map_id);
				if (level_definition)
				{
					bitmap_data* bitmap_block = bitmap_group_get_bitmap(level_definition->bitmap.index, 0);
					option_bitmap->assign_new_bitmap_block(bitmap_block);
				}
			}
		}
		break;
	case _item_change_variant:
		help_string = _string_id_change_variant_help;
		header_string = _string_id_current_variant;
		value_string = _string_id_variant;


		if (option_bitmap && game_variant && game_variant->variant_game_engine_index)
		{
			switch (game_variant->variant_game_engine_index)
			{
			case _game_engine_type_ctf:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_mp_games, _mp_game_type_ctf);
				break;
			case _game_engine_type_slayer:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_mp_games, _mp_game_type_slayer);
				break;
			case _game_engine_type_oddball:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_mp_games, _mp_game_type_oddball);
				break;
			case _game_engine_type_koth:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_mp_games, _mp_game_type_koth);
				break;
			case _game_engine_type_juggernaut:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_mp_games, _mp_game_type_juggernaut);
				break;
			case _game_engine_type_territories:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_mp_games, _mp_game_type_territories);
				break;
			case _game_engine_type_assault:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_mp_games, _mp_game_type_assualt);
				break;
			}
		}

		break;
	case _item_change_level:
		help_string = _string_id_change_level_help;
		header_string = _string_id_current_level;
		value_string = _string_id_level;

		if (map_is_set && option_bitmap)
		{
			s_campaign_ui_level_definition* level_definition = levels_get_campaign_ui_level(campaign_id, map_id);
			if (level_definition)
			{
				bitmap_data* bitmap_block = bitmap_group_get_bitmap(level_definition->bitmap.index, 0);
				option_bitmap->assign_new_bitmap_block(bitmap_block);
			}
		}

		break;
	case _item_change_difficulty:
		help_string = _string_id_change_difficulty_help;
		header_string = _string_id_current_difficulty;
		value_string = _string_id_difficulty;

		if (option_bitmap)
		{
			switch (user_interface_session_get_campaign_difficulty())
			{
			case _difficulty_option_easy:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_difficulty_options, _difficulty_option_easy);
				break;
			case _difficulty_option_normal:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_difficulty_options, _difficulty_option_normal);
				break;
			case _difficulty_option_heroic:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_difficulty_options, _difficulty_option_heroic);
				break;
			case _difficulty_option_legendary:
				option_bitmap->set_local_bitmap(_squad_settings_dialog_local_bitmap_difficulty_options, _difficulty_option_legendary);
				break;
			}

			option_render_scale = { 0.859375f,0.80859375f };
		}


		break;
	case _item_quick_options:
		help_string = _string_id_quick_options_help;
		header_string = _string_id_current_variant;
		value_string = _string_id_variant;
		bitm_index = _xbox_live_bitmap_type_quick_options;
		break;
	case _item_switch_to_coop:
		help_string = _string_id_switch_to_coop_help;
		header_string = _string_id_switch_to_coop;
		value_string = _string_id_empty_string;
		//bitm_index = 7;
		if (option_bitmap && new_xbox_live_bitmap_datum != NONE)
		{
			bitmap_data* bitmap_block = bitmap_group_get_bitmap(new_xbox_live_bitmap_datum, _xbox_live_bitmap_type_switch_to_coop);
			option_bitmap->assign_new_bitmap_block(bitmap_block);
		}
		break;
	case _item_switch_to_arranged:
		help_string = _string_id_switch_to_custom_game_help;
		header_string = _string_id_switch_to_arranged;
		value_string = _string_id_empty_string;
		//bitm_index = 6;
		if (option_bitmap && variant_bitmap_datum != NONE)
		{
			bitmap_data* bitmap_block = bitmap_group_get_bitmap(variant_bitmap_datum, _settings_variant_bitmap_type_default);
			option_bitmap->assign_new_bitmap_block(bitmap_block);
			//render_scale = { (real32)required_width/(real32)borked_width  ,(real32)required_height /(real32)borked_height };
			option_render_scale = { 0.859375f,0.80859375f };
		}
		break;
	case _item_switch_to_optimatch:
		help_string = _string_id_switch_to_optimatch_help;
		header_string = _string_id_switch_to_optimatch;
		value_string = _string_id_empty_string;
		//bitm_index = 0;
		if (option_bitmap && xbox_live_menu_bitmap_datum != NONE)
		{
			bitmap_data* bitmap_block = bitmap_group_get_bitmap(xbox_live_menu_bitmap_datum, _xbox_live_menu_bitmap_type_switch_to_optimatch);
			option_bitmap->assign_new_bitmap_block(bitmap_block);
		}		

		break;
	case _item_change_hopper:
		help_string = _string_id_change_hopper_help;
		header_string = _string_id_current_hopper;
		value_string = _string_id_hopper;
		bitm_index = 4;
		break;
	case _item_party_management:
		help_string = _string_id_party_management_help;
		header_string = _string_id_party_management_header;
		value_string = _string_id_player_options;
		bitm_index = _xbox_live_bitmap_type_party_management;
		break;
	default:
		help_string = _string_id_empty_string;
		header_string = _string_id_empty_string;
		value_string = _string_id_empty_string;
		bitm_index = _xbox_live_bitmap_type_unknown_map;

	}

	if (option_help_text_block)
		option_help_text_block->set_text_from_string_id(help_string);
	if (option_header_text_block)
		option_header_text_block->set_text_from_string_id(header_string);
	if (option_value_text_block)
		option_value_text_block->set_text_from_string_id(value_string);
	if (option_bitmap)
	{
		option_bitmap->verify_and_update_bitmap_index(bitm_index);
		option_bitmap->set_render_scale(&option_render_scale);
	}

	c_list_item_widget* item = this->m_squad_settings_list.try_find_item_widget(_item_party_management);
	if (item && !this->m_squad_settings_list.party_management_exists() && user_interface_squad_get_player_count() < 2)
	{
		this->m_squad_settings_list.party_management_delete_item();
	}
	c_user_interface_widget::update();

}

bool c_screen_squad_settings::handle_event(s_event_record* event)
{
	return INVOKE_TYPE(0x24FE49, 0x0, bool(__thiscall*)(c_screen_widget*, s_event_record*), this, event);
}

void c_screen_squad_settings::initialize(s_screen_parameters* parameters)
{
	INVOKE_TYPE(0x24F043, 0x0, void(__thiscall*)(c_screen_squad_settings*, s_screen_parameters*), this, parameters);
}

void* c_screen_squad_settings::load(s_screen_parameters* parameters)
{
	//return INVOKE(0x24FE89, 0x0, c_screen_squad_settings::load, parameters);

	c_screen_squad_settings* screen;

	void* pool = ui_pool_allocate_space(sizeof(c_screen_squad_settings), 0);
	if (pool)
	{
		screen = new (pool) c_screen_squad_settings(
			parameters->m_channel_type,
			parameters->m_window_index,
			parameters->user_flags);

		screen->m_allocated = true;
		user_interface_register_screen_to_channel(screen, parameters);
	}
	else
	{
		screen = NULL;
	}

	return screen;
}


const void* c_screen_squad_settings::load_proc(void) const
{
	return &c_screen_squad_settings::load;
}

void c_screen_squad_settings::apply_instance_patches()
{
	//Replace orignal call with custom one inside pregame_lobby
	WriteValue(Memory::GetAddress(0x246356) + 1, c_screen_squad_settings::load);
}

void c_screen_squad_settings::apply_patches_on_map_load()
{
	tag_injection_set_active_map(L"mainmenu_bitmaps");
	if (tag_injection_active_map_verified())
	{
		datum xbox_live_bitmap_datum = tag_injection_load(_tag_group_bitmap, "ui\\screens\\game_shell\\xbox_live\\xbox_live_main_menu\\xbox_live", true);
		if (xbox_live_bitmap_datum != NONE)
		{
			tag_injection_inject();
			new_xbox_live_bitmap_datum = xbox_live_bitmap_datum;
			LOG_DEBUG_FUNC("New xbox live bitmap datum : 0x{:08X} ,", new_xbox_live_bitmap_datum);
		}
		else
		{
			new_xbox_live_bitmap_datum = NONE;
		}
	}

	xbox_live_menu_bitmap_datum = tag_loaded(_tag_group_bitmap, "ui\\screens\\game_shell\\xbox_live\\xbox_live_main_menu\\xbox_live_menu");
	variant_bitmap_datum = tag_loaded(_tag_group_bitmap, "ui\\screens\\game_shell\\settings_screen\\variant_settings\\variant");

	return;
}
