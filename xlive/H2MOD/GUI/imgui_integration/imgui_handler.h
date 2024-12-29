#pragma once
#include "input/controllers.h"

/* constants */

extern const char* k_advanced_settings_window_name;
extern const char* k_weapon_offsets_window_name;
extern const char* k_motd_window_name;
extern const char* k_debug_overlay_window_name;
extern const char* k_message_box_window_name;

/* enums */

enum e_network_stats_display_type : int32
{
	_network_stats_display_none,
	_network_stats_display_basic,
	_network_stats_display_complete,
};

enum e_imgui_window_type
{
	_imgui_window_weapon_offsets = 0,
	_imgui_window_motd = 1,
	_imgui_window_message_box = 2,
	_imgui_window_advanced_settings = 3,
	_imgui_window_console,
	k_imgui_window_type_count
};

namespace ImGuiHandler
{
	extern int g_network_stats_overlay;

	enum s_imgui_images
	{
		patch_notes
	};

	typedef int ImWWindowHandlerFlags;
	enum ImWWindowHandlerFlags_
	{
		_ImWindow_no_input = 1 << 0,
	};

	struct s_imgui_window
	{
		const char* name;
		void(__cdecl *const renderFunc)(bool*);
		void(__cdecl *const openFunc)();
		void(__cdecl *const closeFunc)();
		const ImWWindowHandlerFlags flags;

		bool NoImInput() const
		{
			return (flags & _ImWindow_no_input) != 0;
		}
	};

	enum e_imgui_aspect_ratio : byte
	{
		four_three,
		sixten_nine
	};

	bool ImGuiShouldHandleInput();
	void ImGuiToggleInput(bool state);
	void SetGameInputState(bool enable);
	bool CanDrawImgui();
	void DrawImgui();
	
	bool IsWindowActive(const char* name);
	void ToggleWindow(const char* name);

	void Initalize(HWND hWnd);
	float WidthPercentage(float percent);
	void TextVerticalPad(const char* label);
	bool LoadTextureFromFile(const wchar_t* filename, s_imgui_images image, int* out_width, int* out_height);
	PDIRECT3DTEXTURE9 GetTexture(s_imgui_images image);
	void release_motd_texture();
	e_imgui_aspect_ratio GetAspectRatio(const real_point2d* display_size);
	namespace ImMOTD {
		void Render(bool* p_open);
		void Open();
		void Close();
	}
	namespace ImAdvancedSettings
	{
		void Render(bool* p_open);
		void set_controller_index(e_controller_index controller_index);
		void Open();
		void Close();
	}
	namespace ImMessageBox
	{
		void Render(bool* p_open);
		void SetMessage(const char* message);
		void Open();
		void Close();
	}
	namespace WeaponOffsets
	{
		void Render(bool* p_open);
		void Open();
		void Close();
		void MapLoad();
		void Initialize();
	}
}