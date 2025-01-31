#include "stdafx.h"
#include "screen_cartographer_menus.h"

#include "main/game_preferences.h"
#include "interface/user_interface_memory.h"

#include "H2MOD/GUI/ImGui_Integration/imgui_handler.h"
#include "H2MOD/Modules/CustomMenu/CustomMenuGlobals.h"
#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Modules/Updater/Updater.h"


/* macro defines */

#define k_cartographer_guide_list_name "cartographer guide list"
#define k_cartographer_credits_list_name "cartographer credits list"
#define k_cartographer_update_list_name "cartographer update list"
#define k_cartographer_update_notice_list_name "cartographer update notice list"

/* enums */

enum e_cartographer_guide_list_items : uint16
{
	_item_cartographer_guide_advanced_settings = 0,
	_item_cartographer_guide_website,
	_item_cartographer_guide_credits,
	_item_cartographer_guide_update,

	k_total_no_of_cartographer_guide_list_items,
};

enum e_cartographer_guide_string_table
{
	_cartographer_guide_text_none = NONE,
	_cartographer_guide_text_header,
	_cartographer_guide_text_subheader,

	k_cartographer_guide_button_label_index_start,
	_cartographer_guide_button_advanced_settings_text = k_cartographer_guide_button_label_index_start,
	_cartographer_guide_button_text_website,
	_cartographer_guide_button_text_credits,
	_cartographer_guide_button_text_update,

	k_cartographer_guide_end,
};

enum e_cartographer_credits_string_table
{
	_cartographer_credits_text_none = NONE,
	_cartographer_credits_text_header,
	_cartographer_credits_text_subheader,

	k_cartographer_credits_button_text_index,
	_cartographer_credits_text_button_1 = k_cartographer_credits_button_text_index,
	_cartographer_credits_text_button_2,
	_cartographer_credits_text_button_3,
	_cartographer_credits_text_button_4,
	_cartographer_credits_text_button_5,
	_cartographer_credits_text_button_6,
	_cartographer_credits_text_button_7,
	_cartographer_credits_text_button_8,
	_cartographer_credits_text_button_9,
	_cartographer_credits_text_button_10,
	_cartographer_credits_text_button_11,
	_cartographer_credits_text_button_12,
	_cartographer_credits_text_button_13,
	_cartographer_credits_text_button_14,
	_cartographer_credits_text_button_15,
	_cartographer_credits_text_button_16,

	k_cartographer_credits_end,
	k_total_no_of_cartographer_credits_list_items = k_cartographer_credits_end - k_cartographer_credits_button_text_index,
};

enum e_cartographer_update_list_items : uint16
{
	_item_cartographer_update_status = 0,
	_item_cartographer_update_cancel,

	k_total_no_of_cartographer_update_list_items,
};

enum e_cartographer_update_string_table
{
	_cartographer_update_text_none = NONE,
	_cartographer_update_text_header,
	_cartographer_update_text_subheader,

	k_cartographer_update_button_label_index_start,
	_cartographer_update_text_button_check_for_updates = k_cartographer_update_button_label_index_start,
	_cartographer_update_text_button_status_checking_for_updates,
	_cartographer_update_text_button_download_updates,
	_cartographer_update_text_button_status_downloading_update,
	_cartographer_update_text_button_install_update,
	_cartographer_update_text_button_status_installing_update,
	_cartographer_update_text_button_status_failed_to_run_updater_app,
	_cartographer_update_text_button_status_download_the_following,
	_cartographer_update_text_button_status_install_the_following,
	_cartographer_update_text_button_status_up_to_date,
	_cartographer_update_text_button_status_cancel,

	k_cartographer_update_text_end,
};

enum e_cartographer_update_notice_list_items : uint16
{
	_item_cartographer_update_notice_yes = 0,
	_item_cartographer_update_notice_no,

	k_total_no_of_cartographer_update_notice_list_items,
};

enum e_cartographer_update_notice_string_table
{
	_cartographer_update_notice_text_none = NONE,
	_cartographer_update_notice_text_header,
	_cartographer_update_notice_text_subheader,

	k_cartographer_update_notice_button_label_index_start,
	_cartographer_update_text_button_notice_yes = k_cartographer_update_notice_button_label_index_start,
	_cartographer_update_text_button_notice_no,

	k_cartographer_update_notice_end,
};

static void get_guide_label(int32 label_id, wchar_t** out_label)
{
	wchar_t* label_table[k_language_count][k_cartographer_guide_end] = {};

	label_table[_language_english][_cartographer_guide_text_header] = L"Project Cartographer Guide";
	label_table[_language_english][_cartographer_guide_text_subheader] = L"Press the %S Key to open this guide from anywhere.";
	label_table[_language_english][_cartographer_guide_button_advanced_settings_text] = L"Advanced Settings";
	label_table[_language_english][_cartographer_guide_button_text_website] = L"Website";
	label_table[_language_english][_cartographer_guide_button_text_credits] = L"Credits";
	label_table[_language_english][_cartographer_guide_button_text_update] = L"Update";

	*out_label = label_table[_language_english][label_id];
}

static void get_credits_label(int32 label_id, wchar_t** out_label)
{
	wchar_t* label_table[k_language_count][k_cartographer_credits_end] = {};

	label_table[_language_english][_cartographer_credits_text_header] = L"Credits";
	label_table[_language_english][_cartographer_credits_text_subheader] = L"Praise the Following:";
	label_table[_language_english][_cartographer_credits_text_button_1] = L"--- The Devs ---";
	label_table[_language_english][_cartographer_credits_text_button_2] = L"pnill";
	label_table[_language_english][_cartographer_credits_text_button_3] = L"Glitchy Scripts";
	label_table[_language_english][_cartographer_credits_text_button_4] = L"Himanshu01";
	label_table[_language_english][_cartographer_credits_text_button_5] = L"nukeulater";
	label_table[_language_english][_cartographer_credits_text_button_6] = L"Berthalamew";
	label_table[_language_english][_cartographer_credits_text_button_7] = L"Rude Yoshi";
	label_table[_language_english][_cartographer_credits_text_button_8] = L"num005";
	label_table[_language_english][_cartographer_credits_text_button_9] = L"Supersniper";
	label_table[_language_english][_cartographer_credits_text_button_10] = L"--- Content Creators ---";
	label_table[_language_english][_cartographer_credits_text_button_11] = L"General_101";
	label_table[_language_english][_cartographer_credits_text_button_12] = L"--- Additional 7hanks ---";
	label_table[_language_english][_cartographer_credits_text_button_13] = L"Project Cartographer Staff";
	label_table[_language_english][_cartographer_credits_text_button_14] = L"Dev Preview Members";
	label_table[_language_english][_cartographer_credits_text_button_15] = L"And the many many more";
	label_table[_language_english][_cartographer_credits_text_button_16] = L"from the Halo 2 Community!";

	*out_label = label_table[_language_english][label_id];
}

static void get_update_label(int32 label_id, wchar_t** out_label)
{
	wchar_t* label_table[k_language_count][k_cartographer_update_text_end] = {};

	label_table[_language_english][_cartographer_update_text_header] = L"Update!";
	label_table[_language_english][_cartographer_update_text_subheader] = L"Update Project Cartographer";
	label_table[_language_english][_cartographer_update_text_button_check_for_updates] = L"Check for Update";
	label_table[_language_english][_cartographer_update_text_button_status_checking_for_updates] = L"Checking For Update...";
	label_table[_language_english][_cartographer_update_text_button_download_updates] = L"Download Update";
	label_table[_language_english][_cartographer_update_text_button_status_downloading_update] = L"Downloading Update...";
	label_table[_language_english][_cartographer_update_text_button_install_update] = L"Install Update";
	label_table[_language_english][_cartographer_update_text_button_status_installing_update] = L"Installing Update...";
	label_table[_language_english][_cartographer_update_text_button_status_failed_to_run_updater_app] = L"Failed to run updater app!";
	label_table[_language_english][_cartographer_update_text_button_status_cancel] = L"Cancel";

	*out_label = label_table[_language_english][label_id];
}

static void get_update_notice_label(int32 label_id, wchar_t** out_label)
{
	wchar_t* label_table[k_language_count][k_cartographer_update_notice_end] = {};

	label_table[_language_english][_cartographer_update_notice_text_header] = L"Outdated Version!";
	label_table[_language_english][_cartographer_update_notice_text_subheader] = L"You are using an outdated version of Project Cartographer! Would you like to go install the latest version?";
	label_table[_language_english][_cartographer_update_text_button_notice_yes] = L"Yes";
	label_table[_language_english][_cartographer_update_text_button_notice_no] = L"No";

	*out_label = label_table[_language_english][label_id];
}


// guide

c_cartographer_guide_edit_list::c_cartographer_guide_edit_list(uint16 _flags) :
	c_list_widget(_flags),
	m_slot_2(this, &c_cartographer_guide_edit_list::handle_item_pressed_event)
{
	s_data_array* account_list_data = ui_list_data_new(k_cartographer_guide_list_name, k_total_no_of_cartographer_guide_list_items, sizeof(datum));
	this->m_list_data = account_list_data;

	data_make_valid(account_list_data);

	for (int32 i = 0; i < this->m_list_data->datum_max_elements; i++) {
		datum_new(this->m_list_data);
	}

	linker_type2.link(&this->m_slot_2);
}

c_list_item_widget* c_cartographer_guide_edit_list::get_list_items()
{
	return m_list_item_widgets;
}

int32 c_cartographer_guide_edit_list::get_list_items_count()
{
	return NUMBEROF(m_list_item_widgets);
}

void c_cartographer_guide_edit_list::update_list_items(c_list_item_widget* item, int32 skin_index)
{
	int16 list_item_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(item->get_last_data_index());

	c_text_widget* text_widget = item->try_find_text_widget(_default_list_skin_text_main);
	if (text_widget)
	{
		wchar_t* button_label = nullptr;
		get_guide_label(k_cartographer_guide_button_label_index_start + (int32)list_item_index, &button_label);

		ASSERT(button_label != nullptr);
		text_widget->set_text(button_label);
	}
}

void c_cartographer_guide_edit_list::handle_item_pressed_event(s_event_record** pevent, int32* pitem_index)
{
	int16 button_id = DATUM_INDEX_TO_ABSOLUTE_INDEX(*pitem_index);

	if (button_id == _item_cartographer_guide_advanced_settings) 
	{
		ImGuiHandler::ImAdvancedSettings::set_controller_index(_controller_index_0);
		ImGuiHandler::ToggleWindow(k_advanced_settings_window_name);
	}
	else if (button_id == _item_cartographer_guide_website) 
	{
		ShellExecuteA(NULL, "open", k_cartographer_url, NULL, NULL, SW_SHOWDEFAULT);
	}
	else if (button_id == _item_cartographer_guide_credits) 
	{
		ui_load_cartographer_credits_menu();
	}
	else if (button_id == _item_cartographer_guide_update) 
	{
		ui_load_cartographer_update_menu();
	}
}

c_cartographer_guide_menu::c_cartographer_guide_menu(e_user_interface_channel_type _ui_channel, e_user_interface_render_window _window_index, uint16 _flags) :
	c_screen_with_menu(_screen_brightness_level, _ui_channel, _window_index, _flags, &m_guide_edit_list),
	m_guide_edit_list(_flags)
{
}


void* c_cartographer_guide_menu::load(s_screen_parameters* parameters)
{
	c_cartographer_guide_menu* guide_menu = nullptr;
	uint8* ui_buffer = (uint8*)ui_pool_allocate_space(sizeof(c_cartographer_guide_menu), 0);

	if (ui_buffer) {
		guide_menu = new (ui_buffer) c_cartographer_guide_menu(
			parameters->m_channel_type, parameters->m_window_index, parameters->user_flags);
		guide_menu->m_allocated = true;
	}

	user_interface_register_screen_to_channel(guide_menu, parameters);
	return guide_menu;
}

void c_cartographer_guide_menu::initialize(s_screen_parameters* screen_parameters)
{
	c_screen_with_menu::initialize(screen_parameters);

	//update header and subheader labels

	wchar_t* header_text = nullptr;
	wchar_t* subheader_text = nullptr;
	wchar_t text_buffer[512];

	get_guide_label(_cartographer_guide_text_header, &header_text);
	get_guide_label(_cartographer_guide_text_subheader, &subheader_text);

	ASSERT(header_text != nullptr && subheader_text != nullptr);

	m_header_text.set_text(header_text);
	c_text_widget* subheader_text_widget = get_screen_subheader_text();
	if (subheader_text_widget)
	{
		extern int H2Config_hotkeyIdGuide;
		extern std::string GetVKeyCodeString(int vkey);
		swprintf(text_buffer, ARRAYSIZE(text_buffer), subheader_text, GetVKeyCodeString(H2Config_hotkeyIdGuide).c_str());
		subheader_text = text_buffer;

		subheader_text_widget->set_text(subheader_text);
	}
}

const void* c_cartographer_guide_menu::load_proc(void) const
{
	return c_cartographer_guide_menu::load;
}

// credits

c_cartographer_credits_edit_list::c_cartographer_credits_edit_list(uint16 _flags) :
	c_list_widget(_flags),
	m_slot_2(this, &c_cartographer_credits_edit_list::handle_item_pressed_event)
{
	s_data_array* account_list_data = ui_list_data_new(k_cartographer_credits_list_name, k_total_no_of_cartographer_credits_list_items, sizeof(datum));
	this->m_list_data = account_list_data;

	data_make_valid(account_list_data);

	for (int32 i = 0; i < this->m_list_data->datum_max_elements; i++) {
		datum_new(this->m_list_data);
	}

	linker_type2.link(&this->m_slot_2);
}

c_list_item_widget* c_cartographer_credits_edit_list::get_list_items()
{
	return m_list_item_widgets;
}

int32 c_cartographer_credits_edit_list::get_list_items_count()
{
	return NUMBEROF(m_list_item_widgets);
}

void c_cartographer_credits_edit_list::update_list_items(c_list_item_widget* item, int32 skin_index)
{
	datum list_item_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(item->get_last_data_index());

	c_text_widget* text_widget = (c_text_widget*)item->try_find_text_widget(_default_list_skin_text_main);
	if (text_widget)
	{
		wchar_t* button_label = nullptr;
		get_credits_label(k_cartographer_credits_button_text_index + (int32)list_item_index, &button_label);

		ASSERT(button_label != nullptr);
		text_widget->set_text(button_label);
	}
}

void c_cartographer_credits_edit_list::handle_item_pressed_event(s_event_record** event_record, int32* a3)
{
	return;
}

c_cartographer_credits_menu::c_cartographer_credits_menu(e_user_interface_channel_type _ui_channel, e_user_interface_render_window _window_index, uint16 _flags) :
	c_screen_with_menu(_screen_brightness_level, _ui_channel, _window_index, _flags, &m_credits_edit_list),
	m_credits_edit_list(_flags)
{
}

void* c_cartographer_credits_menu::load(s_screen_parameters* parameters)
{
	c_cartographer_credits_menu* credits_menu = nullptr;
	uint8* ui_buffer = (uint8*)ui_pool_allocate_space(sizeof(c_cartographer_credits_menu), 0);

	if (ui_buffer) {
		credits_menu = new (ui_buffer) c_cartographer_credits_menu(
			parameters->m_channel_type, parameters->m_window_index, parameters->user_flags);
		credits_menu->m_allocated = true;
	}

	user_interface_register_screen_to_channel(credits_menu, parameters);
	return credits_menu;
}

const void* c_cartographer_credits_menu::load_proc(void) const
{
	return c_cartographer_credits_menu::load;
}

void c_cartographer_credits_menu::initialize(s_screen_parameters* screen_parameters)
{
	c_screen_with_menu::initialize(screen_parameters);

	wchar_t* header_text = nullptr;
	wchar_t* subheader_text = nullptr;

	get_credits_label(_cartographer_credits_text_header, &header_text);
	get_credits_label(_cartographer_credits_text_subheader, &subheader_text);

	ASSERT(header_text != nullptr && subheader_text != nullptr);

	m_header_text.set_text(header_text);
	c_text_widget* subheader_text_widget = get_screen_subheader_text();
	if (subheader_text_widget)
	{
		subheader_text_widget->set_text(subheader_text);
	}
}

// update

c_cartographer_update_edit_list::c_cartographer_update_edit_list(uint16 _flags) :
	c_list_widget(_flags),
	m_slot_2(this, &c_cartographer_update_edit_list::handle_item_pressed_event)
{
	s_data_array* update_list_data = ui_list_data_new(k_cartographer_update_list_name, k_total_no_of_cartographer_update_list_items, sizeof(datum));
	this->m_list_data = update_list_data;

	data_make_valid(update_list_data);

	for (int32 i = 0; i < this->m_list_data->datum_max_elements; i++) {
		datum_new(this->m_list_data);
	}

	linker_type2.link(&this->m_slot_2);

	// ### TODO FIXME this is pretty hacked up, cleanup the system at some point
	m_keep_screen_open = true;
	m_update_status = _cartographer_update_status_pending_check_for_update;

	// check the updater state, and set the current state accordingly
	if (updater_has_files_to_download)
	{
		m_update_status = _cartographer_update_status_pending_download_update;
	}

	if (updater_has_files_to_install)
	{
		m_update_status = _cartographer_update_status_pending_install_update;
	}
}

void c_cartographer_update_edit_list::pre_destroy()
{
	if (m_keep_screen_open)
	{
		ui_load_cartographer_update_menu();
	}
}

c_list_item_widget* c_cartographer_update_edit_list::get_list_items()
{
	return m_list_item_widgets;
}

int32 c_cartographer_update_edit_list::get_list_items_count()
{
	return NUMBEROF(m_list_item_widgets);
}

void c_cartographer_update_edit_list::update_list_items(c_list_item_widget* item, int32 skin_index)
{
	int16 list_item_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(item->get_last_data_index());

	c_text_widget* text_widget = (c_text_widget*)item->try_find_text_widget(_default_list_skin_text_main);
	if (text_widget)
	{
		wchar_t* button_label = nullptr;

		switch (list_item_index)
		{
		case _item_cartographer_update_status:
			get_update_label(k_cartographer_update_button_label_index_start + m_update_status, &button_label);
			text_widget->get_interface()->set_pulsating((m_update_status % 2) == 0);
			break;
		case _item_cartographer_update_cancel:
			get_update_label(_cartographer_update_text_button_status_cancel, &button_label);
			break;
		default:
			break;
		}

		ASSERT(button_label != nullptr);
		text_widget->set_text(button_label);
	}
}

void c_cartographer_update_edit_list::set_status(e_cartographer_update_status status)
{
}


void c_cartographer_update_edit_list::update_updater_status()
{
	if (m_update_status < _cartographer_update_status_pending_download_update && updater_has_files_to_download)
	{
		m_update_status = _cartographer_update_status_pending_download_update;
	}

	if (m_update_status < _cartographer_update_status_pending_install_update && updater_has_files_to_install)
	{
		m_update_status = _cartographer_update_status_pending_install_update;
	}
}


void c_cartographer_update_edit_list::handle_item_pressed_event(s_event_record** pevent, int32* pitem_index)
{
	int32 button_id = DATUM_INDEX_TO_ABSOLUTE_INDEX(*pitem_index);

	e_user_interface_render_window	parent_render_window = this->get_parent_render_window();
	e_user_interface_channel_type	parent_screen_ui_channel = this->get_parent_channel();

	if (button_id == _item_cartographer_update_status)
	{
		switch (m_update_status)
		{
		case _cartographer_update_status_pending_check_for_update:
			GSDownloadCheck();
			m_update_status = _cartographer_update_status_checking_for_update;
			break;
		case _cartographer_update_status_pending_download_update:
			if (updater_has_files_to_download) {
				GSDownloadDL();
				m_update_status = _cartographer_update_status_downloading_update;
			}
			break;
		case _cartographer_update_status_pending_install_update:
			if (updater_has_files_to_install) {
				GSDownloadInstall();
				m_update_status = _cartographer_update_status_installing_update;
			}
			break;

		case _cartographer_update_status_checking_for_update:
		case _cartographer_update_status_downloading_update:
		default:
			break;
		}
	}
	else if (button_id == _item_cartographer_update_cancel)
	{
		GSDownloadCancel();
		m_keep_screen_open = false;

		user_interface_back_out_from_channel(parent_screen_ui_channel, parent_render_window);
	}

	return;
}


c_cartographer_update_menu::c_cartographer_update_menu(e_user_interface_channel_type _ui_channel, e_user_interface_render_window _window_index, uint16 _flags) :
	c_screen_with_menu(_screen_brightness_level, _ui_channel, _window_index, _flags, &m_update_edit_list),
	m_update_edit_list(_flags)
{
	GSDownloadInit();
}

void c_cartographer_update_menu::update()
{
	m_update_edit_list.update_updater_status();
	c_screen_with_menu::update();
}

const void* c_cartographer_update_menu::load_proc(void) const
{
	return c_cartographer_update_menu::load;
}

void c_cartographer_update_menu::initialize(s_screen_parameters* screen_parameters)
{
	c_screen_with_menu::initialize(screen_parameters);

	wchar_t* header_text = nullptr;
	wchar_t* subheader_text = nullptr;

	get_update_label(_cartographer_update_text_header, &header_text);
	get_update_label(_cartographer_update_text_subheader, &subheader_text);

	ASSERT(header_text != nullptr && subheader_text != nullptr);

	m_header_text.set_text(header_text);
	c_text_widget* subheader_text_widget = get_screen_subheader_text();
	if (subheader_text_widget)
	{
		subheader_text_widget->set_text(subheader_text);
	}
}

void c_cartographer_update_menu::set_update_status(e_cartographer_update_status status)
{
	m_update_edit_list.set_status(status);
}

void* c_cartographer_update_menu::load(s_screen_parameters* parameters)
{
	c_cartographer_update_menu* update_menu = nullptr;
	uint8* ui_buffer = (uint8*)ui_pool_allocate_space(sizeof(c_cartographer_update_menu), 0);

	if (ui_buffer) {
		update_menu = new (ui_buffer) c_cartographer_update_menu(
			parameters->m_channel_type, parameters->m_window_index, parameters->user_flags);
		update_menu->m_allocated = true;
	}

	user_interface_register_screen_to_channel(update_menu, parameters);
	return update_menu;
}


// update notice

c_cartographer_update_notice_edit_list::c_cartographer_update_notice_edit_list(uint16 _flags) :
	c_list_widget(_flags),
	m_slot_2(this, &c_cartographer_update_notice_edit_list::handle_item_pressed_event)
{
	s_data_array* account_list_data = ui_list_data_new(k_cartographer_update_notice_list_name, k_total_no_of_cartographer_update_notice_list_items, sizeof(datum));
	this->m_list_data = account_list_data;

	data_make_valid(account_list_data);

	for (int32 i = 0; i < this->m_list_data->datum_max_elements; i++) {
		datum_new(this->m_list_data);
	}

	linker_type2.link(&this->m_slot_2);
}

c_list_item_widget* c_cartographer_update_notice_edit_list::get_list_items()
{
	return m_list_item_widgets;
}

int32 c_cartographer_update_notice_edit_list::get_list_items_count()
{
	return NUMBEROF(m_list_item_widgets);
}

void c_cartographer_update_notice_edit_list::update_list_items(c_list_item_widget* item, int32 skin_index)
{
	int16 list_item_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(item->get_last_data_index());

	c_text_widget* text_widget = (c_text_widget*)item->try_find_text_widget(_default_list_skin_text_main);
	if (text_widget)
	{
		wchar_t* button_label = nullptr;
		get_update_notice_label(k_cartographer_update_notice_button_label_index_start + (int32)list_item_index, &button_label);

		ASSERT(button_label != nullptr);
		text_widget->set_text(button_label);
	}
}

void c_cartographer_update_notice_edit_list::handle_item_pressed_event(s_event_record** pevent, int32* pitem_index)
{
	int16 button_id = DATUM_INDEX_TO_ABSOLUTE_INDEX(*pitem_index);

	e_user_interface_render_window	parent_render_window = this->get_parent_render_window();
	e_user_interface_channel_type	parent_screen_ui_channel = this->get_parent_channel();

	if (button_id == _item_cartographer_update_notice_yes) 
	{
		ui_load_cartographer_update_menu();
	}
	else if (button_id == _item_cartographer_update_notice_no)
	{
		user_interface_back_out_from_channel(parent_screen_ui_channel, parent_render_window);
	}

	return;
}


c_cartographer_update_notice_menu::c_cartographer_update_notice_menu(e_user_interface_channel_type _ui_channel, e_user_interface_render_window _window_index, uint16 _flags) :
	c_screen_with_menu(_screen_brightness_level, _ui_channel, _window_index, _flags, &m_update_notice_edit_list),
	m_update_notice_edit_list(_flags)
{
}

const void* c_cartographer_update_notice_menu::load_proc(void) const
{
	return c_cartographer_update_notice_menu::load;
}

void c_cartographer_update_notice_menu::initialize(s_screen_parameters* screen_parameters)
{
	c_screen_with_menu::initialize(screen_parameters);

	wchar_t* header_text = nullptr;
	wchar_t* subheader_text = nullptr;

	get_update_notice_label(_cartographer_update_notice_text_header, &header_text);
	get_update_notice_label(_cartographer_update_notice_text_subheader, &subheader_text);

	ASSERT(header_text != nullptr && subheader_text != nullptr);

	m_header_text.set_text(header_text);
	c_text_widget* subheader_text_widget = get_screen_subheader_text();
	if (subheader_text_widget)
	{
		subheader_text_widget->set_text(subheader_text);
	}
}

void* c_cartographer_update_notice_menu::load(s_screen_parameters* parameters)
{
	c_cartographer_update_notice_menu* update_notice_menu = nullptr;
	uint8* ui_buffer = (uint8*)ui_pool_allocate_space(sizeof(c_cartographer_update_notice_menu), 0);

	if (ui_buffer) {
		update_notice_menu = new (ui_buffer) c_cartographer_update_notice_menu(
			parameters->m_channel_type, parameters->m_window_index, parameters->user_flags);
		update_notice_menu->m_allocated = true;
	}

	user_interface_register_screen_to_channel(update_notice_menu, parameters);
	return update_notice_menu;
}

void* ui_load_cartographer_guide_menu()
{
	return ui_custom_cartographer_load_menu(c_cartographer_guide_menu::load);
}

void* ui_load_cartographer_update_notice_menu()
{
	return ui_custom_cartographer_load_menu(c_cartographer_update_notice_menu::load);
}

void* ui_load_cartographer_credits_menu()
{
	return ui_custom_cartographer_load_menu(c_cartographer_credits_menu::load);
}

void* ui_load_cartographer_update_menu()
{
	return ui_custom_cartographer_load_menu(c_cartographer_update_menu::load);
}


