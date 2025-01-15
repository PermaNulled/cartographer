#include "stdafx.h"
#include "screen_cartographer_account_manager.h"

#include "screen_cartographer_account_manager_strings.h"

#include "screen_cartographer_errors.h"
#include "screen_virtual_keyboard.h"

#include "main/game_preferences.h"
#include "memory/data.h"
#include "interface/user_interface_memory.h"
#include "interface/screens/screen_xbox_live_task_progress_dialog.h"
#include "text/unicode.h"

#include "H2MOD/GUI/ImGui_Integration/ImGui_Handler.h"

#include "H2MOD/Modules/Accounts/Accounts.h"
#include "H2MOD/Modules/Accounts/AccountLogin.h"
#include "H2MOD/Modules/Accounts/AccountCreate.h"
#include "H2MOD/Modules/CustomMenu/CustomMenuGlobals.h"
#include "H2MOD/Modules/OnScreenDebug/OnscreenDebug.h"
#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/Utils/Utils.h"

/* enums */

enum e_cartographer_screen_type_account_list_string_table
{
	_screen_type_account_list_header_text = 0,
	_screen_type_account_list_subheader_text,

	// last indexes are reserved for buttons
	k_screen_type_account_list_button_text_index,
	_screen_type_account_list_button_play_offline_text = k_screen_type_account_list_button_text_index,
	_screen_type_account_list_button_create_account_text,
	_screen_type_account_list_button_add_account_text,
	_screen_type_account_list_button_remove_account_text,
	_screen_type_account_list_button_cancel_remove_account_text,
	_screen_type_account_list_button_unnamed_account,

	k_screen_type_account_list_unknown_text
};

enum e_cartographer_account_create_list_items : uint16
{
	_item_cartographer_account_create_username = 0,
	_item_cartographer_account_create_email,
	_item_cartographer_account_create_password,
	_item_cartographer_account_create_confirm_password,
	_item_cartographer_account_create_process,

	k_total_no_of_cartographer_account_create_list_items,
};


enum e_cartographer_screen_type_account_create_string_table
{
	_screen_type_account_create_header_text = 0,
	_screen_type_account_create_subheader_text,

	k_screen_type_account_create_button_text_index,
	_screen_type_account_create_button_placeholder_username_text = k_screen_type_account_list_button_text_index,
	_screen_type_account_create_button_placeholder_email_text,
	_screen_type_account_create_button_placeholder_password_text,
	_screen_type_account_create_button_placeholder_confirm_password_text,
	_screen_type_account_create_button_create_account,

	k_screen_type_account_create_unknown_text
};

enum e_cartographer_account_add_list_items : uint16
{
	_item_cartographer_account_add_username = 0,
	_item_cartographer_account_add_password,
	_item_cartographer_account_add_remember,
	_item_cartographer_account_add_login,

	k_total_no_of_cartographer_account_add_list_items,
};

enum e_cartographer_screen_type_account_add_string_table
{
	_screen_type_account_add_header_text = 0,
	_screen_type_account_add_subheader_text,

	k_screen_type_account_add_button_text_index,
	_screen_type_account_add_button_placeholder_username_text = k_screen_type_account_add_button_text_index,
	_screen_type_account_add_button_placeholder_password_text,
	_screen_type_account_add_button_remember_me_text,
	_screen_type_account_add_button_login_text,

	k_cartographer_screen_type_account_add_string_table_count
};

/* structures */

struct s_cartographer_account_create_data
{
	char user_name[XUSER_NAME_SIZE];
	char email[128];
	char password[128];
	e_cartographer_error_id server_error_code;
};

struct s_cartographer_account_login_data
{
	char email_or_username[128];
	char password[128];
};

/* global externs */

extern bool g_force_cartographer_update;
extern void* ui_load_cartographer_update_notice_menu();

/* constants */

const char *const k_cartographer_account_manager_list_name = "cartographer account manager list";

const wchar_t *const *const k_screen_label_table_eng[k_cartographer_account_manager_screen_type_count] =
{
	k_screen_type_list_table_eng,
	NULL,
	k_screen_type_list_create_account_eng,
	k_screen_type_list_add_account_eng
};

const wchar_t *const *const *const k_screen_label_table[k_language_count] =
{
	k_screen_label_table_eng,
	k_screen_label_table_eng,
	k_screen_label_table_eng,
	k_screen_label_table_eng,
	k_screen_label_table_eng,
	k_screen_label_table_eng,
	k_screen_label_table_eng,
	k_screen_label_table_eng,
	k_screen_label_table_eng
};

/* prototypes */

void* ui_load_cartographer_invalid_login_token(void);
void sign_in_xbox_task_progress_cb(c_screen_xbox_live_task_progress_dialog* dialog);
DWORD WINAPI thread_account_login_proc_cb(LPVOID lParam);
static DWORD WINAPI thread_account_create_proc_cb(LPVOID lParam);
static const wchar_t* get_cartographer_account_manager_label(e_cartographer_account_manager_screen_type screen_type, int32 label_id);

/*
Censors the password string entered if it's not the default password label and returns the pointer to the string.
If password is the default label then return the pointer to password
*/
static const wchar_t* cartographer_account_manager_set_password_label(const wchar_t* password, wchar_t* password_censored_buffer);

/* globals */

static bool g_account_manager_remove_mode;
bool c_cartographer_account_manager_menu::g_accounting_go_back_to_list = false;
int32 c_cartographer_account_manager_menu::g_accounting_active_handle_count = 0;

s_cartographer_account_login_data g_account_add_login_data;
s_cartographer_account_create_data g_account_create_data;

e_cartographer_account_manager_screen_type g_open_cartographer_account_manager_context = _cartographer_account_manager_screen_type_none;

int g_account_manager_master_login_code;
HANDLE g_account_manager_login_thread_handle = NULL;
HANDLE g_account_manager_thread_handle = NULL;

/* public code */

c_cartographer_account_manager_edit_list::c_cartographer_account_manager_edit_list(uint16 _flags, int32 _button_count, int32 _default_selected_button, e_cartographer_account_manager_screen_type _screen_type) :
	c_list_widget(_flags),
	m_slot_2(this, &c_cartographer_account_manager_edit_list::handle_item_pressed_event)
{
	this->m_cartographer_screen_type = _screen_type;
	this->m_default_focused_item = _default_selected_button;

	s_data_array* account_list_data = ui_list_data_new(k_cartographer_account_manager_list_name, _button_count, sizeof(datum));
	this->m_list_data = account_list_data;

	data_make_valid(account_list_data);

	for (int32 i = 0; i < this->m_list_data->datum_max_elements; i++) {
		datum_new(this->m_list_data);
	}

	linker_type2.link(&this->m_slot_2);

	const wchar_t* placeholder_username, *placeholder_email, *placeholder_password, *placeholder_confirm_password;
	switch (m_cartographer_screen_type)
	{
	case _cartographer_account_manager_screen_type_none:
		break;
	case _cartographer_account_manager_screen_type_list:
	case _cartographer_account_manager_screen_type_list_remove_account:
		break;
	case _cartographer_account_manager_screen_type_create_account:
		csmemset(&m_account_create, 0, sizeof(m_account_create));

		placeholder_username = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_create_button_placeholder_username_text);
		placeholder_email = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_create_button_placeholder_email_text);
		placeholder_password = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_create_button_placeholder_password_text);
		placeholder_confirm_password = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_create_button_placeholder_confirm_password_text);
		wcsncpy(m_account_create.user_name, placeholder_username, XUSER_NAME_SIZE);
		wcsncpy(m_account_create.email, placeholder_email, ARRAYSIZE(m_account_create.email));
		wcsncpy(m_account_create.password, placeholder_password, ARRAYSIZE(m_account_create.password));
		wcsncpy(m_account_create.password_confirmation, placeholder_confirm_password, ARRAYSIZE(m_account_create.password_confirmation));
		break;
	case _cartographer_account_manager_screen_type_add_account:
		csmemset(&m_account_add, 0, sizeof(m_account_add));

		placeholder_username = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_add_button_placeholder_username_text);
		placeholder_password = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_add_button_placeholder_password_text);
		wcsncpy(m_account_add.email_or_username, placeholder_username, ARRAYSIZE(m_account_add.email_or_username));
		wcsncpy(m_account_add.password, placeholder_password, ARRAYSIZE(m_account_add.password));
		break;
	default:
		break;
	}
}

void c_cartographer_account_manager_edit_list::pre_destroy(void)
{
	switch (m_cartographer_screen_type)
	{
	case _cartographer_account_manager_screen_type_list:
	case _cartographer_account_manager_screen_type_list_remove_account:
		break;
	case _cartographer_account_manager_screen_type_create_account:
		csmemset(&m_account_create, 0, sizeof(m_account_create));
		break;
	case _cartographer_account_manager_screen_type_add_account:
		csmemset(&m_account_add, 0, sizeof(m_account_add));
		break;
	default:
		break;
	}
}

c_list_item_widget* c_cartographer_account_manager_edit_list::get_list_items(void)
{
	return m_list_item_widgets;
}

int32 c_cartographer_account_manager_edit_list::get_list_items_count(void)
{
	return NUMBEROF(m_list_item_widgets);
}

void c_cartographer_account_manager_edit_list::update_list_items(c_list_item_widget* item, int32 skin_index) 
{
	int32 list_item_index = DATUM_INDEX_TO_ABSOLUTE_INDEX(item->get_last_data_index());

	if (item->get_last_data_index() == NONE)
	{
		return;
	}

	c_text_widget* text_widget = item->try_find_text_widget(_default_list_skin_text_main);
	if (text_widget)
	{
		int32 button_count_allocated = m_list_data->datum_max_elements;
		const wchar_t* button_label = nullptr;

		// + 1 for the unicode character (nullptr included in original string size)
		wchar_t remember_me_text[NUMBEROF(k_remember_me_string_eng) + 1];
		wchar_t username_wide[XUSER_NAME_SIZE];
		wchar_t password_censored_buffer_wide[k_cartographer_account_email_and_password_max_length];

		switch (m_cartographer_screen_type)
		{
		case _cartographer_account_manager_screen_type_list:
		case _cartographer_account_manager_screen_type_list_remove_account:
		{
			// play offline, add account, create account
			int32 default_button_count = 3;
			if (button_count_allocated > default_button_count)
			{
				// we have accounts in the list
				if (list_item_index > button_count_allocated - 1 - 4)
				{
					// button text
					int32 label_index = 4 - (button_count_allocated - list_item_index);
					if (account_removal_mode()
						&& label_index == 3)
					{
						// go to next, which is cancel remove
						label_index += 1;
					}

					button_label = get_cartographer_account_manager_label(_cartographer_account_manager_screen_type_list, k_screen_type_account_list_button_text_index + label_index);
				}
				else
				{
					if (H2AccountArrayUsername[list_item_index] != NULL)
					{
						swprintf(username_wide, XUSER_NAME_SIZE, L"%S", H2AccountArrayUsername[list_item_index]);
						button_label = username_wide;
					}
					else
					{
						button_label = get_cartographer_account_manager_label(_cartographer_account_manager_screen_type_list, _screen_type_account_list_button_unnamed_account);
					}
				}
			}
			else
			{
				// just 3 buttons, play offline, create account and add
				const int32 label_index = 3 - (button_count_allocated - list_item_index);
				button_label = get_cartographer_account_manager_label(_cartographer_account_manager_screen_type_list, k_screen_type_account_list_button_text_index + label_index);
			}
		}
		break;
		case _cartographer_account_manager_screen_type_create_account:
			switch (k_screen_type_account_create_button_text_index + list_item_index)
			{
			case _screen_type_account_create_button_placeholder_username_text:
				button_label = m_account_create.user_name;
				break;
			case _screen_type_account_create_button_placeholder_email_text:
				button_label = m_account_create.email;
				break;
			case _screen_type_account_create_button_placeholder_password_text:
				button_label = cartographer_account_manager_set_password_label(m_account_create.password, password_censored_buffer_wide);
				break;
			case _screen_type_account_create_button_placeholder_confirm_password_text:
				button_label = cartographer_account_manager_set_password_label(m_account_create.password_confirmation, password_censored_buffer_wide);
				break;
			default:
				button_label = get_cartographer_account_manager_label(m_cartographer_screen_type, k_screen_type_account_create_button_text_index + list_item_index);
				break;
			}
			break;
		case _cartographer_account_manager_screen_type_add_account:
			switch (k_screen_type_account_add_button_text_index + list_item_index)
			{
			case _screen_type_account_add_button_placeholder_username_text:
				button_label = m_account_add.email_or_username;
				break;
			case _screen_type_account_add_button_placeholder_password_text:
				button_label = cartographer_account_manager_set_password_label(m_account_add.password, password_censored_buffer_wide);
				break;
			case _screen_type_account_add_button_remember_me_text:
				button_label = get_cartographer_account_manager_label(m_cartographer_screen_type, k_screen_type_account_add_button_text_index + list_item_index);
				usnzprintf(remember_me_text, NUMBEROF(remember_me_text), button_label, m_account_add.remember_me ? L'☑' : L'☐');
				button_label = remember_me_text;
				break;
			case _screen_type_account_add_button_login_text:
				button_label = get_cartographer_account_manager_label(m_cartographer_screen_type, k_screen_type_account_add_button_text_index + list_item_index);
				break;
			}
			break;
		default:
			break;
		}

		ASSERT(button_label != NULL);
		text_widget->set_text(button_label);
	}
}

void c_cartographer_account_manager_edit_list::handle_item_pressed_event(s_event_record** pevent, int32* pitem_index)
{
	switch (m_cartographer_screen_type)
	{
	case _cartographer_account_manager_screen_type_list:
	case _cartographer_account_manager_screen_type_list_remove_account:
		handle_item_pressed_event_for_listed_accounts(pevent, pitem_index);
		break;
	case _cartographer_account_manager_screen_type_create_account:
		handle_item_pressed_event_for_create_account(pevent, pitem_index);
		break;
	case _cartographer_account_manager_screen_type_add_account:
		handle_item_pressed_event_for_add_account(pevent, pitem_index);
		break;
	default:
		break;
	}

	return;
}


void c_cartographer_account_manager_edit_list::handle_item_pressed_event_for_add_account(s_event_record** event_record, int32* a3)
{
	int16 button_id = DATUM_INDEX_TO_ABSOLUTE_INDEX(*a3);

	e_user_interface_render_window	parent_render_window = this->get_parent_render_window();
	e_user_interface_channel_type	parent_screen_ui_channel = this->get_parent_channel();

	if (button_id == _item_cartographer_account_add_username)
	{
		ui_load_virtual_keyboard(m_account_add.email_or_username, ARRAYSIZE(m_account_add.email_or_username), _vkbd_custom_context_email_or_username);
	}
	else if (button_id == _item_cartographer_account_add_password)
	{
		ui_load_virtual_keyboard(m_account_add.password, ARRAYSIZE(m_account_add.password), _vkbd_custom_context_password);
	}
	else if (button_id == _item_cartographer_account_add_remember)
	{
		m_account_add.remember_me = !m_account_add.remember_me;
		AccountEdit_remember = m_account_add.remember_me;
	}
	else if (button_id == _item_cartographer_account_add_login) // login button id
	{
		if (g_account_manager_login_thread_handle == NULL) {

			c_cartographer_account_manager_menu::g_accounting_go_back_to_list = false;
			c_cartographer_account_manager_menu::update_accounting_active_handle(true);
			snprintf(g_account_add_login_data.email_or_username, ARRAYSIZE(g_account_add_login_data.email_or_username), "%S", m_account_add.email_or_username);
			snprintf(g_account_add_login_data.password, ARRAYSIZE(g_account_add_login_data.password), "%S", m_account_add.password);

			g_account_manager_login_thread_handle = CreateThread(NULL, 0, thread_account_login_proc_cb, (LPVOID)NONE, 0, NULL);
			c_screen_xbox_live_task_progress_dialog::add_task(sign_in_xbox_task_progress_cb);

			user_interface_back_out_from_channel(parent_screen_ui_channel, parent_render_window);
		}
	}
}

void c_cartographer_account_manager_edit_list::handle_item_pressed_event_for_listed_accounts(s_event_record** event_record, int32* a3)
{
	const int32 button_id = DATUM_INDEX_TO_ABSOLUTE_INDEX(*a3);

	e_user_interface_render_window	parent_render_window = this->get_parent_render_window();
	e_user_interface_channel_type	parent_screen_ui_channel = this->get_parent_channel();

	if (button_id == H2AccountCount + 1) // create account
	{
		if (!this->account_removal_mode())
		{
			c_cartographer_account_manager_menu::load_for_account_create_context();
		}
	}
	else if (button_id == H2AccountCount + 2) // add account
	{
		if (!this->account_removal_mode())
		{
			c_cartographer_account_manager_menu::load_for_account_add_context();
		}
	}
	else if (button_id == H2AccountCount + 3) // account removal mode switch
	{
		//remove next selected account.
		g_account_manager_remove_mode = !this->account_removal_mode();
		user_interface_back_out_from_channel(parent_screen_ui_channel, parent_render_window);
		g_account_manager_remove_mode ? c_cartographer_account_manager_menu::load_for_account_remove_from_list_context() : c_cartographer_account_manager_menu::load_for_account_list_context();
	}
	else if (H2AccountCount > 0 && button_id >= 0 && button_id < H2AccountCount) // account selection (either for deletion or login)
	{
		if (this->account_removal_mode())
		{
			H2AccountAccountRemove(button_id);
			// re-open the menu
			g_account_manager_remove_mode = false; // reset global accountRemove state
			H2AccountLastUsed = 0;
			c_cartographer_account_manager_menu::load_for_account_list_context();
			user_interface_back_out_from_channel(parent_screen_ui_channel, parent_render_window);
		}
		else if (g_account_manager_login_thread_handle == NULL)
		{
			c_cartographer_account_manager_menu::g_accounting_go_back_to_list = false;
			c_cartographer_account_manager_menu::update_accounting_active_handle(true);
			
			g_account_manager_login_thread_handle = CreateThread(NULL, 0, thread_account_login_proc_cb, (LPVOID)button_id, 0, NULL);
			c_screen_xbox_live_task_progress_dialog::add_task(sign_in_xbox_task_progress_cb);
			
			user_interface_back_out_from_channel(parent_screen_ui_channel, parent_render_window);
		}
	}
	// play offline button
	else if (button_id == H2AccountCount)
	{
		BYTE abEnet[6];
		BYTE abOnline[20];
		XNetRandom(abEnet, sizeof(abEnet));
		XNetRandom(abOnline, sizeof(abOnline));
		g_account_manager_remove_mode = false;
		if (ConfigureUserDetails("[Username]", "12345678901234567890123456789012", rand(), 0, H2Config_ip_lan, ByteToHexStr(abEnet, 6).c_str(), ByteToHexStr(abOnline, 20).c_str(), false))
		{
			//show select profile gui
			extern int notify_xlive_ui;
			notify_xlive_ui = 0;
			H2Config_master_ip = inet_addr("127.0.0.1");
			H2Config_master_port_relay = 2001;
			XUserSignInSetStatusChanged(0);
			user_interface_back_out_from_channel(parent_screen_ui_channel, parent_render_window);
		}
	}
	return;
}


void __cdecl create_account_xbox_task_progress_cb(c_screen_xbox_live_task_progress_dialog* dialog)
{
	dialog->set_display_text_raw(L"Processing account creation request, please wait...");

	if (g_account_manager_thread_handle == NULL)
	{
		dialog->close_task();
		c_cartographer_error_menu::load_by_error_id(g_account_create_data.server_error_code);

		SecureZeroMemory(g_account_create_data.user_name, sizeof(g_account_create_data.user_name));
		SecureZeroMemory(g_account_create_data.email, sizeof(g_account_create_data.email));
		SecureZeroMemory(g_account_create_data.password, sizeof(g_account_create_data.password));
		csmemset(&g_account_create_data, 0, sizeof(g_account_create_data));
	}
}

void c_cartographer_account_manager_edit_list::handle_item_pressed_event_for_create_account(s_event_record** event_record, int32* a3)
{
	const int32 button_id = DATUM_INDEX_TO_ABSOLUTE_INDEX(*a3);

	e_user_interface_render_window	parent_render_window = this->get_parent_render_window();
	e_user_interface_channel_type	parent_screen_ui_channel = this->get_parent_channel();

	if (button_id == _item_cartographer_account_create_username)
	{
		ui_load_virtual_keyboard(m_account_create.user_name, /* wide characters count */ ARRAYSIZE(m_account_create.user_name), _vkbd_custom_context_username);
	}
	else if (button_id == _item_cartographer_account_create_email)
	{
		ui_load_virtual_keyboard(m_account_create.email, ARRAYSIZE(m_account_create.email), _vkbd_custom_context_email);
	}
	else if (button_id == _item_cartographer_account_create_password)
	{
		ui_load_virtual_keyboard(m_account_create.password, ARRAYSIZE(m_account_create.password), _vkbd_custom_context_password);
	}
	else if (button_id == _item_cartographer_account_create_confirm_password)
	{
		ui_load_virtual_keyboard(m_account_create.password_confirmation, ARRAYSIZE(m_account_create.password_confirmation), _vkbd_custom_context_password);
	}
	else if (button_id == _item_cartographer_account_create_process)
	{
		if (ustrncmp(m_account_create.password, m_account_create.password_confirmation, k_cartographer_account_email_and_password_max_length) != 0)
		{
			c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_account_create_password_confirmation_mismatch);
		}
		else if (g_account_manager_thread_handle == NULL)
		{
			c_cartographer_account_manager_menu::g_accounting_go_back_to_list = false;
			c_cartographer_account_manager_menu::update_accounting_active_handle(true);

			snprintf(g_account_create_data.user_name, ARRAYSIZE(g_account_create_data.user_name), "%S", m_account_create.user_name);
			snprintf(g_account_create_data.email, ARRAYSIZE(g_account_create_data.email), "%S", m_account_create.email);
			snprintf(g_account_create_data.password, ARRAYSIZE(g_account_create_data.password), "%S", m_account_create.password);

			SecureZeroMemory(m_account_create.user_name, sizeof(m_account_create.user_name));
			SecureZeroMemory(m_account_create.email, sizeof(m_account_create.email));
			SecureZeroMemory(m_account_create.password, sizeof(m_account_create.password));
			SecureZeroMemory(m_account_create.password_confirmation, sizeof(m_account_create.password_confirmation));

			user_interface_back_out_from_channel(parent_screen_ui_channel, parent_render_window);
			g_account_manager_thread_handle = CreateThread(NULL, 0, thread_account_create_proc_cb, (LPVOID)0, 0, NULL);
			c_screen_xbox_live_task_progress_dialog::add_task(create_account_xbox_task_progress_cb);
		}
	}
	return;
}

c_cartographer_account_manager_menu::c_cartographer_account_manager_menu(e_user_interface_channel_type _ui_channel, e_user_interface_render_window _window_index, uint16 _flags, e_cartographer_account_manager_screen_type _screen_type, int32 _button_count, int32 _selected_button) :
	c_screen_with_menu(_screen_brightness_level, _ui_channel, _window_index, _flags, &this->m_account_edit_list),
	m_account_edit_list(_flags, _button_count, _selected_button, _screen_type)
{
	m_cartographer_screen_type = _screen_type;
	return;
}

void c_cartographer_account_manager_menu::pre_destroy()
{
	switch (m_cartographer_screen_type)
	{
	case _cartographer_account_manager_screen_type_list:
	case _cartographer_account_manager_screen_type_list_remove_account:
		break;
	case _cartographer_account_manager_screen_type_create_account:
		if (c_cartographer_account_manager_menu::g_accounting_go_back_to_list
			&& c_cartographer_account_manager_menu::is_accounting_active_handle())
		{
			load_for_account_list_context();
			c_cartographer_account_manager_menu::g_accounting_go_back_to_list = false;
		}
		break;

	case _cartographer_account_manager_screen_type_add_account:
		if (c_cartographer_account_manager_menu::g_accounting_go_back_to_list
			&& c_cartographer_account_manager_menu::is_accounting_active_handle())
		{
			load_for_account_list_context();
			c_cartographer_account_manager_menu::g_accounting_go_back_to_list = false;
		}
		break;
	default:
		break;
	}
	
	update_accounting_active_handle(false);
	return;
}

// c_screen_with_menu specific interface
void c_cartographer_account_manager_menu::initialize(s_screen_parameters* screen_parameters)
{
	c_screen_with_menu::initialize(screen_parameters);

	const wchar_t* header_text = NULL;
	const wchar_t* subheader_text = NULL;

	switch (m_cartographer_screen_type)
	{
	case _cartographer_account_manager_screen_type_list:
	case _cartographer_account_manager_screen_type_list_remove_account:
		header_text = get_cartographer_account_manager_label(_cartographer_account_manager_screen_type_list, _screen_type_account_list_header_text);
		subheader_text = get_cartographer_account_manager_label(_cartographer_account_manager_screen_type_list, _screen_type_account_list_subheader_text);
		break;
	case _cartographer_account_manager_screen_type_create_account:
		header_text = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_create_header_text);
		subheader_text = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_create_subheader_text);
		break;
	case _cartographer_account_manager_screen_type_add_account:
		header_text = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_add_header_text);
		subheader_text = get_cartographer_account_manager_label(m_cartographer_screen_type, _screen_type_account_add_subheader_text);
		break;
	default:
		break;
	}

	ASSERT(header_text != NULL && subheader_text != NULL);

	m_header_text.set_text(header_text);
	c_text_widget* subheader_text_widget = get_screen_subheader_text();
	if (subheader_text_widget)
	{
		subheader_text_widget->set_text(subheader_text);
	}
	return;
}

void c_cartographer_account_manager_menu::post_initialize(void)
{
	m_account_edit_list.set_focused_item_index(m_account_edit_list.m_default_focused_item != NONE ? m_account_edit_list.m_default_focused_item : 0);
	return c_screen_with_menu::post_initialize();
}

const void* c_cartographer_account_manager_menu::load_proc(void) const
{
	return c_cartographer_account_manager_menu::load_default_context;
}


bool c_cartographer_account_manager_menu::is_accounting_active_handle(void)
{
	return c_cartographer_account_manager_menu::g_accounting_active_handle_count > 0;
}

void c_cartographer_account_manager_menu::update_accounting_active_handle(bool active) {
	c_cartographer_account_manager_menu::g_accounting_active_handle_count += active ? 1 : -1;
	addDebugText("Accounting active: %d", c_cartographer_account_manager_menu::g_accounting_active_handle_count);
	if (c_cartographer_account_manager_menu::g_accounting_active_handle_count <= 0) {
		SaveH2Accounts();
	}
	return;
}

void c_cartographer_account_manager_menu::set_menu_open_context(e_cartographer_account_manager_screen_type screen_type)
{
	g_open_cartographer_account_manager_context = screen_type;
}


void* __cdecl c_cartographer_account_manager_menu::load(s_screen_parameters* parameters)
{
	ASSERT(g_open_cartographer_account_manager_context != _cartographer_account_manager_screen_type_none);

	c_cartographer_account_manager_menu* account_list_menu = nullptr;
	uint8* ui_buffer = (uint8*)ui_pool_allocate_space(sizeof(c_cartographer_account_manager_menu), 0);

	int32 button_count = 0;
	int32 selected_button_index = 0;

	c_cartographer_account_manager_menu::update_accounting_active_handle(true);
	switch (g_open_cartographer_account_manager_context)
	{
	case _cartographer_account_manager_screen_type_list:
		button_count = H2AccountCount + (H2AccountCount > 0 ? 4 : 3);
		selected_button_index = H2AccountLastUsed;
		break;
	case _cartographer_account_manager_screen_type_list_remove_account:
		button_count = H2AccountCount + (H2AccountCount > 0 ? 4 : 3);
		selected_button_index = 0;
		break;
	case _cartographer_account_manager_screen_type_create_account:
		button_count = 5;
		selected_button_index = 0;
		c_cartographer_account_manager_menu::g_accounting_go_back_to_list = true;
		break;
	case _cartographer_account_manager_screen_type_add_account:
		button_count = 4;
		selected_button_index = 0;
		c_cartographer_account_manager_menu::g_accounting_go_back_to_list = true;
		break;
	default:
		break;
	}

	if (ui_buffer) {
		account_list_menu = new (ui_buffer) c_cartographer_account_manager_menu(
			parameters->m_channel_type, parameters->m_window_index, parameters->user_flags, g_open_cartographer_account_manager_context, button_count, selected_button_index);
		account_list_menu->m_allocated = true;
	}

	g_open_cartographer_account_manager_context = _cartographer_account_manager_screen_type_none;

	user_interface_register_screen_to_channel(account_list_menu, parameters);
	return account_list_menu;
}

void* c_cartographer_account_manager_menu::load_default_context(s_screen_parameters* parameters)
{
	set_menu_open_context(_cartographer_account_manager_screen_type_list);
	return load(parameters);
}

c_cartographer_account_manager_menu* c_cartographer_account_manager_menu::load_for_account_list_context()
{
	set_menu_open_context(_cartographer_account_manager_screen_type_list);
	return (c_cartographer_account_manager_menu*)ui_custom_cartographer_load_menu(c_cartographer_account_manager_menu::load, 3);
}

c_cartographer_account_manager_menu* c_cartographer_account_manager_menu::load_for_account_remove_from_list_context()
{
	set_menu_open_context(_cartographer_account_manager_screen_type_list_remove_account);
	return (c_cartographer_account_manager_menu*)ui_custom_cartographer_load_menu(c_cartographer_account_manager_menu::load, 3);
}

c_cartographer_account_manager_menu* c_cartographer_account_manager_menu::load_for_account_create_context()
{
	set_menu_open_context(_cartographer_account_manager_screen_type_create_account);
	return (c_cartographer_account_manager_menu*)ui_custom_cartographer_load_menu(c_cartographer_account_manager_menu::load);
}

c_cartographer_account_manager_menu* c_cartographer_account_manager_menu::load_for_account_add_context()
{
	set_menu_open_context(_cartographer_account_manager_screen_type_add_account);
	return (c_cartographer_account_manager_menu*)ui_custom_cartographer_load_menu(c_cartographer_account_manager_menu::load);
}

void cartographer_account_manager_open_list(void)
{
	const bool is_accounting_active_handle = c_cartographer_account_manager_menu::is_accounting_active_handle();
	if (!is_accounting_active_handle && ReadH2Accounts())
	{
		c_cartographer_account_manager_menu::load_for_account_list_context();
	}
	else
	{
		if (!is_accounting_active_handle)
		{
			c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_login_account_already_in_use);
		}
	}
	return;
}

/* private code */

void* ui_load_cartographer_invalid_login_token(void)
{
	c_cartographer_account_manager_menu::g_accounting_go_back_to_list = true;
	c_cartographer_account_manager_menu::update_accounting_active_handle(true);
	return c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_invalid_login_token);
}


void sign_in_xbox_task_progress_cb(c_screen_xbox_live_task_progress_dialog* dialog)
{
	dialog->set_display_text_raw(L"Signing into Project Cartographer, please wait...");

	// if the g_account_manager_login_thread_handle handle is NULL, it means that the login thread has ended
	if (g_account_manager_login_thread_handle == NULL)
	{
		dialog->close_task();

		if (g_account_manager_master_login_code < 0)
		{
			switch (g_account_manager_master_login_code)
			{
			case ERROR_CODE_CURL_HANDLE:
			case ERROR_CODE_CURL_SOCKET_FAILED:
			case ERROR_CODE_ACCOUNT_DATA:
			case ERROR_CODE_INVALID_PARAM:
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_internal_error);
				return;

			case ERROR_CODE_CURL_EASY_PERF:
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_account_login_connection_failed);
				return;

			case ERROR_CODE_INVALID_VERSION:
				ui_load_cartographer_update_notice_menu();
				return;

			case ERROR_CODE_INVALID_LOGIN_TOKEN:
				ui_load_cartographer_invalid_login_token();
				return;

			case ERROR_CODE_INVALID_LOGIN_ID:
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_login_invalid_account_id);
				return;

			case ERROR_CODE_INVALID_PASSWORD:
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_login_incorrect_password);
				return;

			case ERROR_CODE_MACHINE_BANNED:
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_login_machine_banned);
				return;

			case ERROR_CODE_ACCOUNT_BANNED:
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_login_account_banned);
				return;

			case ERROR_CODE_ACCOUNT_DISABLED:
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_login_account_disabled);
				return;

			case ERROR_CODE_MACHINE_SERIAL_INSUFFICIENT:
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_login_insufficient_machine_identifiers);
				return;

			default:
				//unknown error!
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_unknown_unhandled_error);
				return;
			}
		}
		else {
			if (g_account_manager_master_login_code == SUCCESS_CODE_MACHINE_SERIAL_INSUFFICIENT) 
			{
				c_cartographer_error_menu::load_by_error_id(_cartographer_error_id_login_insufficient_machine_identifiers);
				return;
			}
			else 
			{
				if (g_force_cartographer_update)
				{
					//Just in case save the config to prevent a high iq individual from getting themsevles in an update loop.
					SaveH2Config();
					g_force_cartographer_update = false;
					ui_load_cartographer_update_notice_menu();
					return;
				}
				ImGuiHandler::ToggleWindow(k_motd_window_name);
				// extern int notify_xlive_ui;
				// notify_xlive_ui = 0;
				// sub_248beb_nak_deconstructor_Login_Warn();
				// GSCustomMenuCall_Login_Warn();
				return;
			}
		}
	}
}

DWORD WINAPI thread_account_login_proc_cb(LPVOID lParam)
{
	int button_id = (int)lParam;

	//gotta delay it a little to make sure the menu's decide to render correctly.
	Sleep(200L);

	if (button_id == -1) 
	{
		if (HandleGuiLogin(NULL, g_account_add_login_data.email_or_username, g_account_add_login_data.password, &g_account_manager_master_login_code)) 
		{
			H2AccountLastUsed = 0;
		}
		SecureZeroMemory(g_account_add_login_data.email_or_username, sizeof(g_account_add_login_data.email_or_username));
		SecureZeroMemory(g_account_add_login_data.password, sizeof(g_account_add_login_data.password));
	}
	else 
	{
		//login to account
		if (HandleGuiLogin(H2AccountArrayLoginToken[button_id], NULL, NULL, &g_account_manager_master_login_code))
		{
			H2AccountLastUsed = button_id;
		}
	}

	c_cartographer_account_manager_menu::update_accounting_active_handle(false);

	g_account_manager_login_thread_handle = NULL;
	return 0;
}

static DWORD WINAPI thread_account_create_proc_cb(LPVOID lParam)
{
	//gotta delay it a little to make sure the menu's decide to render correctly.
	Sleep(200L);

	//submit account creation.
	if (HandleGuiAccountCreate(g_account_create_data.user_name, g_account_create_data.email, g_account_create_data.password, &g_account_create_data.server_error_code))
	{
	}

	c_cartographer_account_manager_menu::update_accounting_active_handle(false);

	g_account_manager_thread_handle = NULL;
	return 0;
}

static const wchar_t* get_cartographer_account_manager_label(e_cartographer_account_manager_screen_type screen_type, int32 label_id)
{
	const e_language language = get_current_language();
	return k_screen_label_table[language][screen_type][label_id];
}

static const wchar_t* cartographer_account_manager_set_password_label(const wchar_t* password, wchar_t* password_censored_text)
{
	const wchar_t* default_password_labels[2];
	default_password_labels[0] = get_cartographer_account_manager_label(_cartographer_account_manager_screen_type_create_account, _screen_type_account_create_button_placeholder_password_text);
	default_password_labels[1] = get_cartographer_account_manager_label(_cartographer_account_manager_screen_type_create_account, _screen_type_account_create_button_placeholder_confirm_password_text);

	bool is_default_password_label = false;
	for (int32 i = 0; i < ARRAYSIZE(default_password_labels); i++)
	{
		const size_t default_password_label_length = wcslen(default_password_labels[i]);

		// Check if the password passed is the default password level (k_password_string)
		is_default_password_label = ustrncmp(password, default_password_labels[i], default_password_label_length) == 0;
		if (is_default_password_label)
		{
			break;
		}
	}

	if (!is_default_password_label)
	{
		// Populate password_censored_text with the '*' character for each character entered
		const size_t length = ustrnlen(password, k_cartographer_account_email_and_password_max_length);

		size_t i = 0;
		for (; i < length; ++i)
		{
			password_censored_text[i] = L'*';
		}
		password_censored_text[i] = L'\0';
	}

	return is_default_password_label ? password : password_censored_text;
}
