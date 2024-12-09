#include "stdafx.h"

#include "imgui.h"
#include "imgui_handler.h"

#include "game/game.h"


namespace ImGuiHandler
{
	namespace ImMessageBox
	{
		namespace
		{
			std::string message;
		}
		void Render(bool* p_open)
		{
			bool open = *p_open;
			ImGuiIO& io = ImGui::GetIO();
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGuiWindowFlags window_flags = 0;
			window_flags |= ImGuiWindowFlags_NoCollapse;
			window_flags |= ImGuiWindowFlags_NoResize;
			//window_flags |= ImGuiWindowFlags_MenuBar;
			ImGui::SetNextWindowPos(ImVec2(viewport->WorkSize.x * 0.5f, viewport->WorkSize.y * 0.5f), ImGuiCond_::ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 8));
			//ImGui::PushFont(font2);
			ImGui::SetNextWindowSize(ImVec2(650, 250), ImGuiCond_Appearing);
			ImGui::SetNextWindowSizeConstraints(ImVec2(610, 250), ImVec2(1920, 1080));
			if (game_is_ui_shell())
				ImGui::SetNextWindowBgAlpha(1);
			if (ImGui::Begin("Message", NULL, window_flags))
			{
				ImGui::TextWrapped(message.c_str());
				ImGui::SetCursorPosY(190);
				if (ImGui::Button("Ok", ImVec2(610, 50)))
				{
					ImGuiHandler::ToggleWindow(k_message_box_window_name);
				}
			}
			// Pop style var
			ImGui::PopStyleVar();
			ImGui::End();
			
		}
		void SetMessage(std::string Message)
		{
			message = Message;
		}

		void Open()
		{
		}
		void Close()
		{
		}
	}
}