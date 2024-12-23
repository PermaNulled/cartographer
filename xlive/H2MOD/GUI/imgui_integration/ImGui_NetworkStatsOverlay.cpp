#include "stdafx.h"

#include "imgui.h"
#include "ImGui_NetworkStatsOverlay.h"

#include "game/game_time.h"
#include "networking/memory/networking_memory.h"
#include "networking/logic/life_cycle_manager.h"
#include "simulation/simulation.h"

#include "XLive/xnet/IpManagement/XnIp.h"

// converts the ammount of bytes to the appropriate measurement unit and gets the units description string
net_bandwidth_display_data get_net_bandwidth_display_data(unsigned int bytes_count)
{
	static const char* network_bandwidth_measurement_unit[] =
	{
		"byte(s)",
		"kbyte(s)",
		"mbytes(s)",
		"gbytes(s)",
		"<unknown>",
	};

	net_bandwidth_display_data result = { (float)bytes_count, network_bandwidth_measurement_unit[0] };

	int bandwidth_measurement_unit_idx = 0;
	while (result.val > 1000)
	{
		if (bandwidth_measurement_unit_idx >= 5)
			break;

		result.val = (float)result.val / 1000.f;
		result.unit_str = network_bandwidth_measurement_unit[++bandwidth_measurement_unit_idx];
	}

	return result;
}

void ShowNetworkStatsOverlay(bool* p_open)
{
	if (p_open == nullptr
		|| !*p_open)
		return;

	static int corner = 0;
	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = 0
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav
		| ImGuiWindowFlags_NoMouseInputs
		;

	if (corner != -1)
	{
		const float PAD = 10.0f;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
		ImVec2 work_size = viewport->WorkSize;
		ImVec2 window_pos, window_pos_pivot;
		window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
		window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
		window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
		window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		window_flags |= 0
			| ImGuiWindowFlags_NoMove
			;
	}

	ImGui::SetNextWindowBgAlpha(0.40f); // Transparent background
	if (ImGui::Begin("##Network Status", p_open, window_flags))
	{
		ImGui::Text("Network stats overlay\n");
		ImGui::Separator();

		c_network_session* session = NULL;
		if (network_life_cycle_in_squad_session(&session))
		{
			ImGui::Text("Network protocol: %s", session->describe_network_protocol_type());
		}
		else
		{
			ImGui::Text("Network protocol: <disconnected>");
		}

		unsigned long long network_time = *Memory::GetAddressRelative<unsigned long long*>(0x879E98);

		ImGui::Text("Network time: %llu msec", network_time);

		XnIp* localIp = gXnIpMgr.GetLocalUserXn();

		if (localIp->m_valid)
		{
			const XnIpPckTransportStats* local_user_net_metrics;
			localIp->PckGetStats(&local_user_net_metrics);

			localIp->m_pckStats.PckDataSampleUpdate();

			int currentSentPerSecIdx = local_user_net_metrics->pckCurrentSendPerSecIdx;
			int currentRecvdPerSecIdx = local_user_net_metrics->pckCurrentRecvdPerSecIdx;

			if (currentSentPerSecIdx != -1)
			{
				net_bandwidth_display_data bandwidth_usage = get_net_bandwidth_display_data(local_user_net_metrics->pckBytesSentPerSec[currentSentPerSecIdx]);

				ImGui::Text("Network pck xmit:  %d pck/s bandwidth:  %.3g %s/s",
					local_user_net_metrics->pckSentPerSec[currentSentPerSecIdx],
					bandwidth_usage.val,
					bandwidth_usage.unit_str);
			}

			if (currentRecvdPerSecIdx != -1)
			{
				net_bandwidth_display_data bandwidth_usage = get_net_bandwidth_display_data(local_user_net_metrics->pckBytesRecvdPerSec[currentRecvdPerSecIdx]);

				ImGui::Text("Network pck recvd: %d pck/s bandwidth: %.3g %s/s",
					local_user_net_metrics->pckRecvdPerSec[currentRecvdPerSecIdx],
					bandwidth_usage.val,
					bandwidth_usage.unit_str);
			}
		}

		// ImGui::Text("Object count: %d", object_get_count());
		// ImGui::Text("Object count from iter: %d", object_count_from_iter());
		// ImGui::Text("Object to interpolate count: %d", ObjectInterpolate::GetObjectInterpolateCount());

		ImGui::Text("Game tick fraction leftover: %.5f", time_globals::get_ticks_fraction_leftover());

		// s_player_effect_user_globals* user_effect = player_effects_get_user_globals(0);

		// ImGui::Text("Game impulse transition scale: %.5f, camera shake transition scale: %.5f", 
		// 	user_effect->camera_impulse_transition_scale,
		// 	user_effect->camera_shake_transition_scale
		// );

		s_network_heap_stats* net_heap_stats = network_heap_get_description();
		ImGui::Text("Network heap allocated count: [%d], size in bytes: [%d]", net_heap_stats->allocations, net_heap_stats->allocations_in_bytes);

		c_simulation_world* sim_world = simulation_get_world();
		const s_simulation_queue_stats* sim_queue_stats;
		bool sim_queue_stats_available = sim_world->queue_describe(_simulation_queue, &sim_queue_stats);

		if (sim_queue_stats_available)
		{
			ImGui::Text("Simulation queue statistics:");
			ImGui::Text("allocated: [%d], queued: [%d], in bytes: [%d]",
				sim_queue_stats->allocated,
				sim_queue_stats->queued,
				sim_queue_stats->allocated_in_bytes);

			static bool restricted_allocations_once = false;
			static real32 allocated_percentage_max;
			static real32 allocated_in_bytes_percentage_max;
			bool restricting_allocations = false;

			allocated_percentage_max = MAX(allocated_percentage_max, sim_queue_stats->allocated_percentage);
			allocated_in_bytes_percentage_max = MAX(allocated_in_bytes_percentage_max, sim_queue_stats->allocated_bytes_percentage);

			// if we allocated more than 90% of the buffer
			// skip some updates to aleviate some of the stress on the queue
			// especially if the game froze for multiple seconds
			// and allow the allocation for important updates only
			// entity deletion, entity promotion, and global game events
			if (sim_queue_stats->allocated_percentage > 90.f / 100.f
				|| sim_queue_stats->allocated_bytes_percentage > 90.f / 100.f)
			{
				restricting_allocations = true;
				restricted_allocations_once = true;
			}

			ImGui::Text("allocated %%: [%.3f] [%.3f], restricting?: %s",
				sim_queue_stats->allocated_percentage * 100.f,
				sim_queue_stats->allocated_bytes_percentage * 100.f,
				restricting_allocations ? "YES!" : "no");

			ImGui::Text("max allocated %%: [%.3f] [%.3f], restricted alloc: %s",
				allocated_percentage_max * 100.f,
				allocated_in_bytes_percentage_max * 100.f,
				restricted_allocations_once ? "YES!" : "no");
		}

		if (!(window_flags & ImGuiWindowFlags_NoMouseInputs))
		{
			if (ImGui::BeginPopupContextWindow())
			{
				if (ImGui::MenuItem("Custom", NULL, corner == -1)) corner = -1;
				if (ImGui::MenuItem("Top-left", NULL, corner == 0)) corner = 0;
				if (ImGui::MenuItem("Top-right", NULL, corner == 1)) corner = 1;
				if (ImGui::MenuItem("Bottom-left", NULL, corner == 2)) corner = 2;
				if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
				if (p_open && ImGui::MenuItem("Close")) *p_open = false;
				ImGui::EndPopup();
			}
		}
	}

	ImGui::End();
}
