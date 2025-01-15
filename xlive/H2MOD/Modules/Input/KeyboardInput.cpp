#include "stdafx.h"
#include "KeyboardInput.h"

#include "input/controllers.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"

#include "H2MOD/Modules/Shell/Config.h"
#include "H2MOD/GUI/XLiveRendering.h"
#include "H2MOD/GUI/ImGui_Integration/ImGui_Handler.h"
#include "H2MOD/GUI/ImGui_Integration/Console/ImGui_ConsoleImpl.h"
#include "H2MOD/Modules/OnScreenDebug/OnscreenDebug.h"
#include "H2MOD/Utils/Utils.h"

/* structures */

struct s_keyboard_hotkey_data
{
	int* message;
	void(__cdecl* callback)();
};

/* constants */

static BYTE enableKeyboard3[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* globals */

s_keyboard_hotkey_data g_keyboard_hotkey_data[6] = {};
RECT rectScreenOriginal;

void KeyboardInput::ToggleKeyboardInput()
{
	if (H2Config_disable_ingame_keyboard) {
		//Allows to repeat last movement when lose focus in mp, unlocks METHOD E from point after intro vid
		BYTE getFocusB[] = { 0x00 };
		WriteBytes(Memory::GetAddress() + 0x2E3C5, getFocusB, 1);

		//Allows input when not in focus.
		BYTE getFocusE[] = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
		WriteBytes(Memory::GetAddress() + 0x2F9EA, getFocusE, 6);
		WriteBytes(Memory::GetAddress() + 0x2F9FC, getFocusE, 6);
		WriteBytes(Memory::GetAddress() + 0x2FA09, getFocusE, 6);
	}
	else {
		//Reset them all back.
		BYTE getFocusB[] = { 0x01 };
		WriteBytes(Memory::GetAddress() + 0x2E3C5, getFocusB, 1);

		//dont allow input when not in focus.
		BYTE getFocusE[] = { 0x0F, 0x85, 0x02, 0x02, 0x00, 0x00 };
		WriteBytes(Memory::GetAddress() + 0x2F9EA, getFocusE, 6);
		getFocusE[2] = 0xF0;
		getFocusE[3] = 0x01;
		WriteBytes(Memory::GetAddress() + 0x2F9FC, getFocusE, 6);
		getFocusE[2] = 0xE3;
		WriteBytes(Memory::GetAddress() + 0x2FA09, getFocusE, 6);
	}
}

void KeyboardInput::ExecuteHotkey(WPARAM message)
{
	for(auto &hk : g_keyboard_hotkey_data)
	{
		if (*hk.message == message)
		{
			hk.callback();
		}
	}
	return;
}

void hotkeyFuncHelp() {
	addDebugText("------------------------------");
	addDebugText("Options:");
	addDebugText("%s - Print and show this help text.", GetVKeyCodeString(H2Config_hotkeyIdHelp).c_str());
	addDebugText("%s - Align/Correct window positioning (into Borderless).", GetVKeyCodeString(H2Config_hotkeyIdAlignWindow).c_str());
	addDebugText("%s - Toggle Windowed/Borderless mode.", GetVKeyCodeString(H2Config_hotkeyIdWindowMode).c_str());
	addDebugText("%s - Toggles hiding the in-game chat menu.", GetVKeyCodeString(H2Config_hotkeyIdToggleHideIngameChat).c_str());
	addDebugText("%s - Toggles hiding the Console Menu.", GetVKeyCodeString(H2Config_hotkeyIdConsole).c_str());
	addDebugText("------------------------------");

	if (!ImGuiHandler::IsWindowActive(k_cartographer_console_window_name))
	{
		ImGuiHandler::ToggleWindow(k_cartographer_console_window_name);
	}
	GetMainConsoleInstance()->SwitchToTab(_console_tab_logs);
}

//TODO:REFACTOR
void setBorderless(int originX, int originY, int width, int height) {
	SetWindowLong(H2hWnd, GWL_STYLE, GetWindowLong(H2hWnd, GWL_STYLE) & ~(WS_THICKFRAME | WS_BORDER | WS_DLGFRAME));// | WS_SIZEBOX
	//SetWindowLong(halo2hWnd, GWL_STYLE, GetWindowLong(halo2hWnd, GWL_EXSTYLE) & ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

	SetWindowPos(H2hWnd, NULL, originX, originY, width, height, 0);// SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

}
//TODO:REFACTOR
void setWindowed(int originX, int originY, int width, int height) {
	SetWindowLong(H2hWnd, GWL_STYLE, GetWindowLong(H2hWnd, GWL_STYLE) | WS_THICKFRAME | WS_BORDER | WS_DLGFRAME);
	SetWindowPos(H2hWnd, NULL, originX, originY, width, height, SWP_FRAMECHANGED);
}
//TODO: REFACTOR Content
void hotkeyFuncAlignWindow() {
	if (Memory::IsDedicatedServer()) {
		return;
	}

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();

	if (!device || !H2hWnd) {
		return;
	}
	HMONITOR monitor = MonitorFromWindow(H2hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info;
	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);
	int monitor_width = info.rcMonitor.right - info.rcMonitor.left;
	int monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;
	int interval_width = monitor_width / 2;
	int interval_height = monitor_height / 2;
	D3DVIEWPORT9 pViewport;
	device->GetViewport(&pViewport);
	int width = (int)(interval_width * round(pViewport.Width / (double)interval_width));
	int height = (int)(interval_height * round(pViewport.Height / (double)interval_height));
	RECT gameWindowRect;
	GetWindowRect(H2hWnd, &gameWindowRect);
	int monitorXOffset = gameWindowRect.left - info.rcMonitor.left;
	int monitorYOffset = gameWindowRect.top - info.rcMonitor.top;
	int padX = (int)(interval_width * round(monitorXOffset / (double)interval_width));
	int padY = (int)(interval_height * round(monitorYOffset / (double)interval_height));
	int posX = info.rcMonitor.left + padX;
	int posY = info.rcMonitor.top + padY;

	setBorderless(posX, posY, width, height);
}
//TODO: REFACTOR Content
void hotkeyFuncWindowMode() {
	if (Memory::IsDedicatedServer()) {
		return;
	}

	IDirect3DDevice9Ex* device = rasterizer_dx9_device_get_interface();
	if (!device || !H2hWnd) {
		return;
	}
	/*wchar_t title[255];
	wsprintf(title, L"Confirm Window Mode for Player %d", getPlayerNumber());
	int msgboxID = MessageBox(halo2hWnd,
	L"Go to Borderless Mode?\nNo = Windowed mode.\nWarning: Clicking the same option that is currently active can have weird side effects.",
	title,
	MB_ICONEXCLAMATION | MB_YESNOCANCEL
	);*/
	//if (msgboxID == IDYES) {}
	if (GetWindowLong(H2hWnd, GWL_STYLE) & (WS_THICKFRAME | WS_BORDER | WS_DLGFRAME)) {
		RECT rectScreen;
		GetWindowRect(H2hWnd, &rectScreen);
		D3DVIEWPORT9 pViewport;
		device->GetViewport(&pViewport);
		int width = pViewport.Width;
		int height = pViewport.Height;
		long borderPadX = 0;
		long borderPadY = 0;
		int excessY = GetSystemMetrics(SM_CYCAPTION);

		WINDOWPLACEMENT place3 = {};
		GetWindowPlacement(H2hWnd, &place3);
		if ((place3.flags & WPF_RESTORETOMAXIMIZED) == WPF_RESTORETOMAXIMIZED) {
			WINDOWPLACEMENT place2;
			GetWindowPlacement(H2hWnd, &place2);
			place2.showCmd = (place2.showCmd | SW_SHOWNOACTIVATE) & ~SW_MAXIMIZE;
			SetWindowPlacement(H2hWnd, &place2);
			borderPadX = GetSystemMetrics(SM_CXSIZEFRAME);
			borderPadY = GetSystemMetrics(SM_CYSIZEFRAME);
		}
		GetWindowRect(H2hWnd, &rectScreenOriginal);

		setBorderless(rectScreen.left + borderPadX, rectScreen.top + borderPadY, width, height + excessY);

	}
	else {
		//else if (msgboxID == IDNO) {
		long width = rectScreenOriginal.right - rectScreenOriginal.left;// -GetSystemMetrics(SM_CXSIZEFRAME) - GetSystemMetrics(SM_CXSIZEFRAME);
		long height = rectScreenOriginal.bottom - rectScreenOriginal.top;// -GetSystemMetrics(SM_CYSIZEFRAME) - GetSystemMetrics(SM_CYSIZEFRAME);
		setWindowed(rectScreenOriginal.left, rectScreenOriginal.top, width, height);
	}
}

void hotkeyFuncToggleHideIngameChat() {
	H2Config_hide_ingame_chat = !H2Config_hide_ingame_chat;
	if (H2Config_hide_ingame_chat) {
		addDebugText("Hiding in-game chat menu.");
	}
	else {
		addDebugText("Showing in-game chat menu.");
	}
}
void hotkeyFuncGuide() {
	ImGuiHandler::ImAdvancedSettings::set_controller_index(_controller_index_0);
	ImGuiHandler::ToggleWindow(k_advanced_settings_window_name);
}

void hotkeyFuncConsole() {
	ImGuiHandler::ToggleWindow(k_cartographer_console_window_name);
}

void KeyboardInput::Initialize()
{
	if (!enableKeyboard3[0]) {
		for (int i = 0; i < 6; i++) {
			enableKeyboard3[i] = *((BYTE*)Memory::GetAddress() + 0x2FA67 + i);
		}
	}
	ToggleKeyboardInput();

	addDebugText("Registering Hotkeys");
	g_keyboard_hotkey_data[0] = { &H2Config_hotkeyIdHelp, hotkeyFuncHelp };
	g_keyboard_hotkey_data[1] = { &H2Config_hotkeyIdAlignWindow, hotkeyFuncAlignWindow };
	g_keyboard_hotkey_data[2] = { &H2Config_hotkeyIdWindowMode, hotkeyFuncWindowMode };
	g_keyboard_hotkey_data[3] = { &H2Config_hotkeyIdGuide, hotkeyFuncGuide };
	g_keyboard_hotkey_data[4] = { &H2Config_hotkeyIdConsole, hotkeyFuncConsole };
	g_keyboard_hotkey_data[5] = { &H2Config_hotkeyIdToggleHideIngameChat, []() { H2Config_hide_ingame_chat = !H2Config_hide_ingame_chat; } };
	return;
}
