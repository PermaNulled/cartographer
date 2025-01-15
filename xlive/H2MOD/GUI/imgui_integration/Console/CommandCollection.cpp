#include "stdafx.h"

#include "CommandCollection.h"
#include "CommandsUtil.h"
#include "ComVar.h"

#include "main/main.h"
#include "game/game.h"
#include "main/main_game.h"
#include "main/main_game_time.h"
#include "main/main_render.h"
#include "main/main_screenshot.h"
#include "networking/logic/life_cycle_manager.h"
#include "networking/NetworkMessageTypeCollection.h"
#include "objects/objects.h"
#include "simulation/game_interface/simulation_game_action.h"
#include "text/unicode.h"

#include "H2MOD.h"
#include "H2MOD/GUI/imgui_integration/imgui_handler.h"
#include "H2MOD/Modules/MapManager/MapManager.h"

// for XNet connection logging
#include "tag_files/tag_loader/tag_injection.h"
#include "XLive/xnet/IpManagement/XnIp.h"

std::mutex commandInsertMtx;

bool readObjectIds = true;
std::map<std::string, unsigned int> objectIds;

const char command_error_bad_arg[] = "# exception catch (bad arg): ";

ComVarFromPtr(network_stats_overlay_var_cmd, int, &ImGuiHandler::g_network_stats_overlay,
	"var_net_metrics", "displays various network parameters, 1 parameter(s): <int>(1 - basic debug info, 2 - complete debug)", 0, 1, CommandCollection::NetworkMetricsCmd);

ComVarFromPtr(og_frame_limiter_var_cmd, bool, &g_main_game_time_frame_limiter_enabled,
	"var_og_frame_limiter", "enabled/disable original h2 frame limiter", 0, 1, CommandCollection::BoolVarHandlerCmd);

extern real32 g_rumble_factor;
ComVarFromPtr(rumble_var_cmd, real32, &g_rumble_factor,
	"var_rumble_scale", "change controller vibration strength (0.0 to 1.0), 1 parameter(s): <float>", 1, 1, CommandCollection::RumbleScaleCmd);

ComVarFromPtr(debug_render_horizontal_splitscreen, bool, &g_debug_render_horizontal_splitscreen,
	"var_debug_render_horizontal_splitscreen", "force horizontal spliscreen split", 0, 1, CommandCollection::BoolVarHandlerCmd);

// don't forget to add '_cmd' after the name, 
// if you add a variable command created using `DECL_ComVarCommandPtr` macro
std::vector<ConsoleCommand*> CommandCollection::commandTable;

void CommandCollection::InitializeCommands()
{
	static bool InitializeCommandsMap_initialized = false;
	if (InitializeCommandsMap_initialized) return;
	InitializeCommandsMap_initialized = true;

	InsertCommand(new ConsoleCommand(network_stats_overlay_var_cmd));
	InsertCommand(new ConsoleCommand(og_frame_limiter_var_cmd));
	InsertCommand(new ConsoleCommand(rumble_var_cmd));
	InsertCommand(new ConsoleCommand(debug_render_horizontal_splitscreen));
	InsertCommand(new ConsoleCommand("help", "outputs all commands, 0 - 1 parameter(s): <string>(optional): command name", 0, 1, CommandCollection::HelpCmd));
	InsertCommand(new ConsoleCommand("log_peers", "logs all peers to console, 0 parameter(s)", 0, 0, CommandCollection::LogPeersCmd));
	InsertCommand(new ConsoleCommand("log_players", "logs all players to console, 0 parameter(s)", 0, 0, CommandCollection::LogPlayersCmd));
	InsertCommand(new ConsoleCommand("kick_peer", "kicks peer from network session, 1 parameter(s): <int>: peer index", 1, 1, CommandCollection::KickPeerCmd));
	InsertCommand(new ConsoleCommand("leave_session", "leave current session, 0 parameter(s)", 0, 0, CommandCollection::LeaveNetworkSessionCmd));
	InsertCommand(new ConsoleCommand("is_host", "logs if you are session host or not, 0 parameter(s)", 0, 0, CommandCollection::IsSessionHostCmd));
	InsertCommand(new ConsoleCommand("map_download", "download specified map, 1 parameter(s): <string>", 1, 1, CommandCollection::DownloadMapCmd));
	InsertCommand(new ConsoleCommand("reload_maps", "re-load custom map data cache into memory, 0 parameter(s)", 0, 0, CommandCollection::ReloadMapsCmd));
	InsertCommand(new ConsoleCommand("log_map_file_name", "logs selected map filename, 0 parameter(s)", 0, 0, CommandCollection::LogSelectedMapFilenameCmd));
	InsertCommand(new ConsoleCommand("request_map_file", "requests map file name from host, 0 parameter(s)", 0, 0, CommandCollection::RequestFileNameCmd));
	InsertCommand(new ConsoleCommand("max_players", "set maximum players that can join, 1 parameter(s): <int>", 1, 1, CommandCollection::SetMaxPlayersCmd));
	InsertCommand(new ConsoleCommand("delete_object", "deletes an object, 1 parameter(s): <int>: object datum index", 1, 1, CommandCollection::DestroyObjectCmd));
	InsertCommand(new ConsoleCommand("warp_fix", "(EXPERIMENTAL) increases client position update control threshold", 1, 1, CommandCollection::WarpFixCmd, CommandFlags_::CommandFlag_Hidden));
	InsertCommand(new ConsoleCommand("log_xnet_connections", "logs the xnet connections for debugging purposes, 0 parameter(s)", 0, 0, CommandCollection::LogXNetConnectionsCmd, CommandFlags_::CommandFlag_Hidden));
	InsertCommand(new ConsoleCommand("spawn", "spawn an object from the list, 4 - 10 parameter(s): "
		"<string>: object name <int>: count <bool>: same team, near player <float3>: (only if near player false) position xyz, rotation (optional) ijk", 4, 10, CommandCollection::SpawnCmd));
	InsertCommand(new ConsoleCommand("spawn_reload_command_list", "reload object ids for spawn command from file, 0 parameter(s)", 0, 0, CommandCollection::ReloadSpawnCommandListCmd));
	InsertCommand(new ConsoleCommand("tag_inject", "injects tag into memory, 3 parameter(s): <string>: tag_name, tag_type, map_name", 3, 3, CommandCollection::InjectTagCmd, CommandFlags_::CommandFlag_Hidden));
	InsertCommand(new ConsoleCommand("crash", "crashes the game", 0, 0, CommandCollection::Crash));
	InsertCommand(new ConsoleCommand("map_name", "load a map with the following name, 1 parameter(s): <string>", 1, 1, CommandCollection::map_name));
	InsertCommand(new ConsoleCommand("game_difficulty", "set the difficulty when using the map_load command, 1 parameter(s): <int>", 1, 1, CommandCollection::game_difficulty));
	InsertCommand(new ConsoleCommand("game_coop_players", "set the coop player count when using the map_load command, 1 parameter(s): <int>", 1, 1, CommandCollection::game_coop_players));
	InsertCommand(new ConsoleCommand("game_multiplayer", "sets the multiplayer variant for the next map, 1 parameter(s): <string>", 1, 1, CommandCollection::game_multiplayer));
	InsertCommand(new ConsoleCommand("game_splitscreen", "sets the number of multiplayer splitscreen players for the next map, 1 parameter(s): <int>", 1, 1, CommandCollection::game_splitscreen));
	InsertCommand(new ConsoleCommand("game_mode", "sets the game mode for the next map, 1 parameter(s): <int>", 1, 1, CommandCollection::game_mode));
	InsertCommand(new ConsoleCommand("invite", "creates a invite code that you can send to people for direct connecting", 0, 0, CommandCollection::invite));
	InsertCommand(new ConsoleCommand("connect", "lets you directly connect to a session with an invite code", 1, 1, CommandCollection::connect));
	InsertCommand(new ConsoleCommand("sv_change_player_team", "changes the player team to the specivied team", 2, 2, CommandCollection::change_player_team));
	InsertCommand(new ConsoleCommand("quit", "quits the game to desktop", 0, 0, CommandCollection::quit));
	InsertCommand(new ConsoleCommand("screenshot_cubemap", "takes a cubemap screenshot and saves as <name>.tif", 1, 1, CommandCollection::_screenshot_cubemap));

	atexit([]() -> void {
		for (auto command : commandTable)
		{
			delete command;
		}

		commandTable.clear();
		}
	);
}

void CommandCollection::InsertCommand(ConsoleCommand* newCommand)
{
	std::scoped_lock lock(commandInsertMtx);

	for (auto command : commandTable)
	{
		if (!strcmp(newCommand->GetName(), command->GetName()))
		{
			LOG_ERROR_GAME("{} - command {} already present!", __FUNCTION__, newCommand->GetName());
			return;
		}
	}

	commandTable.emplace_back(newCommand);
}

ConsoleCommand* CommandCollection::GetCommandByName(const std::string& name)
{
	std::scoped_lock lock(commandInsertMtx);

	for (auto command : commandTable)
	{
		if (!strcmp(name.c_str(), command->GetName()))
		{
			return command;
		}
	}

	return nullptr;
}

// in case your variable needs to be set/updated
void CommandCollection::SetVarCommandPtr(const std::string& name, ComVarBase* varPtr)
{
	ConsoleCommand* commandPtr = GetCommandByName(name);
	if (commandPtr != nullptr)
	{
		commandPtr->SetCommandVarPtr(varPtr);
	}
}

//////////////////////////////////////////////////////////////////////////
//	commands
//////////////////////////////////////////////////////////////////////////

int CommandCollection::BoolVarHandlerCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	auto booleanCmdVar = ctx.consoleCommand->GetVar<ComVar<bool>>();

	if (!ctx.consoleCommand->SetsVariable())
	{
		outputCb(StringFlag_None, "# command does not set a variable");
		return 0;
	}

	if (tokens.size() - 1 < 1)
	{
		booleanCmdVar->SetVal(!booleanCmdVar->GetVal());
		outputCb(StringFlag_None, "%s", booleanCmdVar->AsString().c_str());
		return 0;
	}

	std::string exception;
	if (!booleanCmdVar->SetFromStr(tokens[1], exception))
	{
		outputCb(StringFlag_None, command_error_bad_arg);
		outputCb(StringFlag_None, "	%s", exception.c_str());
	}
	return 0;
}

int CommandCollection::RumbleScaleCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	real32 rumbleScale;
	std::string exception;

	if (!ComVar(&rumbleScale).SetFromStr(tokens[1], exception))
	{
		outputCb(StringFlag_None, command_error_bad_arg);
		outputCb(StringFlag_None, "	%s", exception.c_str());
	}
	else
	{
		g_rumble_factor = PIN(rumbleScale, 0.f, 1.f);
	}

	return 0;
}

int CommandCollection::NetworkMetricsCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	int32 network_debug_display_type;
	std::string exception;

	if (Memory::IsDedicatedServer()) {
		outputCb(StringFlag_None, "# command unavailable on dedicated servers");
		return 0;
	}

	if (tokens.size() - 1 < 1)
	{
		ImGuiHandler::g_network_stats_overlay = !ImGuiHandler::g_network_stats_overlay;
		return 0;
	}
	else if (!ComVar(&network_debug_display_type).SetFromStr(tokens[1], 0, exception))
	{
		outputCb(StringFlag_None, command_error_bad_arg);
		outputCb(StringFlag_None, "	%s", exception.c_str());
	}
	else
	{
		switch (network_debug_display_type)
		{
		case _network_stats_display_basic:
		case _network_stats_display_complete:
			ImGuiHandler::g_network_stats_overlay = network_debug_display_type;
			break;
		case _network_stats_display_none:
		default:
			ImGuiHandler::g_network_stats_overlay = _network_stats_display_none;
			break;
		}
	}

	return 0;
}

int CommandCollection::SetD3D9ExStateCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	if (Memory::IsDedicatedServer()) {
		outputCb(StringFlag_None, "# command unavailable on dedicated servers");
		return 0;
	}

	return BoolVarHandlerCmd(tokens, ctx);
}

int CommandCollection::LogXNetConnectionsCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	gXnIpMgr.LogConnectionsToConsole(outputCb);
	return 0;
}

int CommandCollection::LogSelectedMapFilenameCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	if (!network_life_cycle_in_squad_session(NULL))
	{
		outputCb(StringFlag_None, "# not in a network session");
		return 0;
	}

	std::wstring mapFilenameWide;
	MapManager::GetMapFilename(mapFilenameWide);

	utf8 map_name[256];
	wchar_string_to_utf8_string(mapFilenameWide.c_str(), map_name, NUMBEROF(map_name));
	outputCb(StringFlag_None, "# map file name: %s", map_name);
	return 0;
}

int CommandCollection::RequestFileNameCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	if (!network_life_cycle_in_squad_session(NULL))
	{
		outputCb(StringFlag_None, "# not in a network session");
		return 0;
	}
	else if (NetworkSession::LocalPeerIsSessionHost())
	{
		outputCb(StringFlag_None, "# only clients can request map file name");
		return 0;
	}

	NetworkMessage::SendRequestMapFilename(NONE);
	return 0;
}

int CommandCollection::LeaveNetworkSessionCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	if (!network_life_cycle_in_squad_session(NULL))
	{
		outputCb(StringFlag_None, "# not in a network session");
		return 0;
	}
	else if (Memory::IsDedicatedServer()) {
		outputCb(StringFlag_None, "# command unavailable on dedicated servers");
		return 0;
	}

	NetworkSession::LeaveSession();
	return 0;
}

int CommandCollection::IsSessionHostCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	c_network_session* session = NULL;
	if (!network_life_cycle_in_squad_session(&session))
	{
		outputCb(StringFlag_None, "# not in a network session");
		return 0;
	}

	std::string isHostStr;
	isHostStr += "# Session host: ";
	isHostStr += (session->is_host() ? "yes" : "no");
	isHostStr += ", value=" + std::to_string(session->m_local_state);
	outputCb(StringFlag_None, isHostStr.c_str());
	return 0;
}

int CommandCollection::KickPeerCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	int32 peer_index;
	std::string exception;

	do
	{
		if (!ComVar(&peer_index).SetFromStr(tokens[1], 0, exception))
		{
			outputCb(StringFlag_None, command_error_bad_arg);
			outputCb(StringFlag_None, "	%s", exception.c_str());
			break;
		}
		else if (Memory::IsDedicatedServer()) {
			outputCb(StringFlag_None, "# command unavailable on dedicated servers");
			break;
		}
		else if (!NetworkSession::LocalPeerIsSessionHost()) {
			outputCb(StringFlag_None, "# only the host can kick players");
			break;
		}
		else if (NetworkSession::GetLocalPeerIndex() == peer_index) {
			outputCb(StringFlag_None, "# don't kick yourself");
			break;
		}
		else if (peer_index >= NetworkSession::GetPeerCount()) {
			outputCb(StringFlag_None, "# peer at the specified index doesn't exist");
			break;
		}

		NetworkSession::KickPeer(peer_index);
	} while (0);

	return 0;
}

int CommandCollection::DownloadMapCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	if (!NetworkSession::LocalPeerIsSessionHost())
	{
		outputCb(StringFlag_None, "# cannot download map using command while not being the session host!");
		return 0;
	}

	std::wstring fileNameToDownload(std::wstring(tokens[1].begin(), tokens[1].end()));
	auto downloadQuery = mapManager->AddDownloadQuery(fileNameToDownload);
	downloadQuery->StartMapDownload(); // since we have the map name, start the download
	return 0;
}

int CommandCollection::ReloadMapsCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	mapManager->ReloadAllMaps();
	return 0;
}

int CommandCollection::HelpCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	const ConsoleCommand* command_data = ctx.consoleCommand;

	const std::string* commandToHelp = nullptr;

	int parameterCount = tokens.size() - 1; // only parameters
	bool singleCommandHelp = parameterCount >= 1;
	bool singleCommandHelpFound = false;

	if (singleCommandHelp)
		commandToHelp = &tokens[1];

	if (!singleCommandHelp)
		outputCb(StringFlag_None, "# available commands: ");

	for (auto command_entry : CommandCollection::commandTable)
	{
		if (singleCommandHelp && _strnicmp(command_entry->GetName(), commandToHelp->c_str(), commandToHelp->length()) != 0)
			continue;

		if (!command_entry->Hidden())
		{
			outputCb(StringFlag_None, "# %s ", command_entry->GetName());
			if (command_entry->GetDescription() != NULL)
			{
				outputCb(StringFlag_None, "    # command description: %s", command_entry->GetDescription());
			}
			else
			{
				outputCb(StringFlag_None, "	# command has no description");
			}

			if (singleCommandHelp)
			{
				singleCommandHelpFound = true;
				break;
			}
		}
	}

	if (singleCommandHelp && !singleCommandHelpFound)
		outputCb(StringFlag_None, "	# unknown command: %s", commandToHelp->c_str());

	return 0;
}

int CommandCollection::LogPlayersCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	const ConsoleCommand* command_data = ctx.consoleCommand;

	c_network_session* session = NULL;
	if (!network_life_cycle_in_squad_session(&session))
	{
		outputCb(StringFlag_None, "# not in a network session");
		return 0;
	}
	else if (!session->is_host())
	{
		outputCb(StringFlag_None, "# must be network session host");
		return 0;
	}

	outputCb(StringFlag_None, "# %i players: ", session->get_player_count());

	for (int32 player_index = 0; player_index < k_maximum_players; player_index++)
	{
		if (session->is_session_player_active(player_index))
		{
			std::wstring playerNameWide(session->get_player_name(player_index));
			std::string playerName(playerNameWide.begin(), playerNameWide.end());
			std::string outStr = "# Player index=" + std::to_string(player_index);
			outStr += ", Peer index=" + std::to_string(NetworkSession::GetPeerIndex(player_index));
			outStr += ", PlayerName=" + playerName;

			playerNameWide = s_player::get_name(player_index);
			playerName = std::string(playerNameWide.begin(), playerNameWide.end());

			outStr += ", Name from game player state=" + playerName;
			outStr += ", Team=" + std::to_string(NetworkSession::GetPlayerTeam(player_index));
			outStr += ", Identifier=" + std::to_string(NetworkSession::GetPlayerId(player_index));

			outputCb(StringFlag_None, outStr.c_str());
		}
	}

	return 0;
}

int CommandCollection::LogPeersCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	const ConsoleCommand* command_data = ctx.consoleCommand;

	c_network_session* session = NULL;
	if (!network_life_cycle_in_squad_session(&session))
	{
		outputCb(StringFlag_None, "# not in a network session");
		return 0;
	}
	else if (!session->is_host())
	{
		outputCb(StringFlag_None, "# must be network session host");
		return 0;
	}

	c_network_observer* observer = session->m_network_observer;

	outputCb(StringFlag_None, "# %i peers: ", session->get_peer_count());

	for (int32 peer_index = 0; peer_index < session->get_peer_count(); peer_index++)
	{
		auto peer_observer_channel = &observer->m_observer_channels[session->m_session_peers[peer_index].observer_channel_index];

		std::wstring peerNameWide(session->m_session_membership.membership_peers[peer_index].name);
		std::string peerName(peerNameWide.begin(), peerNameWide.end());

		std::string outStr = "# Peer index=" + std::to_string(peer_index);
		outStr += ", Peer Name=" + peerName;
		outStr += ", Connection Status=" + std::to_string(peer_observer_channel->state);
		outStr += ", Peer map state: " + std::to_string(session->m_session_membership.membership_peers[peer_index].map_status);
		datum player_index = session->m_session_membership.membership_peers[peer_index].local_players_indexes[0];
		if (player_index != NONE)
		{
			std::wstring playerNameWide(session->get_player_membership(player_index)->properties[0].player_name);
			std::string playerName(playerNameWide.begin(), playerNameWide.end());
			outStr += ", Player index=" + std::to_string(player_index);
			outStr += ", Player name=" + playerName;

			playerNameWide = s_player::get_name(player_index);
			playerName = std::string(playerNameWide.begin(), playerNameWide.end());
			outStr += ", Name from game player state=" + playerName;
		}
		outputCb(StringFlag_None, outStr.c_str());
	}

	return 0;
}

int CommandCollection::SetMaxPlayersCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	int max_players;
	std::string exception;

	do
	{
		c_network_session* session = NULL;
		if (!network_life_cycle_in_squad_session(&session))
		{
			outputCb(StringFlag_None, "# not in a network session");
			break;
		}
		else if (!session->is_host()) {
			outputCb(StringFlag_None, "# can be only used by host");
			break;
		}
		else if (!ComVar(&max_players).SetFromStr(tokens[1], 0, exception))
		{
			outputCb(StringFlag_None, command_error_bad_arg);
			outputCb(StringFlag_None, "	%s", exception.c_str());
			break;
		}
		else if (max_players < 1 || max_players > 16) {
			outputCb(StringFlag_None, "# the value needs to be between 1 and 16");
			break;
		}
		else if (max_players < session->get_player_count()) {
			outputCb(StringFlag_None, "# you can't set a value of max players smaller than the actual number of players on the server");
			break;
		}

		session->m_session_parameters.max_party_players = max_players;
		outputCb(StringFlag_None, "# maximum players set: %i", max_players);
	} while (0);

	return 0;
}

int CommandCollection::WarpFixCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	bool warpFixVar;

	if (Memory::IsDedicatedServer()) {
		outputCb(StringFlag_None, "# command unavailable on dedicated servers");
		return 0;
	}

	std::string exception;
	if (!ComVar(&warpFixVar).SetFromStr(tokens[1], exception))
	{
		outputCb(StringFlag_None, command_error_bad_arg);
		outputCb(StringFlag_None, "	%s", exception.c_str());
		return 0;
	}

	H2MOD::player_position_increase_client_position_margin_of_error(warpFixVar);
	return 0;
}

int CommandCollection::DestroyObjectCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	datum datumIdx;

	std::string exception;
	if (!ComVar(&datumIdx).SetFromStr(tokens[1], 0, exception))
	{
		outputCb(StringFlag_None, command_error_bad_arg);
		outputCb(StringFlag_None, "	%s", exception.c_str());
		return 0;
	}

	void* object_data = (void*)object_try_and_get_and_verify_type(datumIdx, -1);
	if (object_data != NULL)
	{
		DeleteObject(datumIdx);
		outputCb(StringFlag_None, "# deleted object idx: 0x%X", datumIdx);
	}
	else
	{
		outputCb(StringFlag_None, "# failed to delete object idx: 0x%X", datumIdx);
	}

	return 0;
}

int CommandCollection::ReloadSpawnCommandListCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	outputCb(StringFlag_None, "# object ids reset next time spawn is used");
	readObjectIds = false;
	return 0;
}

int CommandCollection::SpawnCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	int tokenArgPos = 1;

	// spawn object_name count same_team near_player x y z i j k

	datum objectDatum = NONE;
	int count;
	real_point3d position;
	real_vector3d rotation;
	bool sameTeam, nearPlayerSpawn;
	int parameterCount = tokens.size() - 1; // only parameters

	datum localPlayerIdx = player_index_from_user_index(0);

	std::string objectName = tokens[tokenArgPos++];

	if (game_is_ui_shell()) {
		outputCb(StringFlag_None, "# can only be used ingame");
		return 0;
	}
	else if (!NetworkSession::LocalPeerIsSessionHost()) {
		outputCb(StringFlag_None, "# can only be used by the session host");
		return 0;
	}
	else if (!ComVar(&count).SetFromStr(tokens[tokenArgPos++])
		|| !ComVar(&sameTeam).SetFromStr(tokens[tokenArgPos++])
		|| !ComVar(&nearPlayerSpawn).SetFromStr(tokens[tokenArgPos++])
		)
	{
		outputCb(StringFlag_None, "# one or more invalid spawn arguments");
		return 0;
	}

	if (nearPlayerSpawn)
	{
		real_point3d* localPlayerPos = &object_get_fast_unsafe(s_player::get(localPlayerIdx)->unit_index)->position;
		position.x = localPlayerPos->x + 0.5f;
		position.y = localPlayerPos->y + 0.5f;
		position.z = localPlayerPos->z + 0.5f;
	}
	else
	{
		if (parameterCount < 7
			|| !ComVar(&position.x).SetFromStr(tokens[tokenArgPos++])
			|| !ComVar(&position.y).SetFromStr(tokens[tokenArgPos++])
			|| !ComVar(&position.z).SetFromStr(tokens[tokenArgPos++]))
		{
			outputCb(StringFlag_None, "# insufficient/invalid xyz position spawn arguments");
			return 0;
		}
	}

	// check if rotation parameters were passed
	// even if invalid, error handled after
	bool withRotation = (parameterCount >= 5 && nearPlayerSpawn)
		|| (parameterCount >= 8 && !nearPlayerSpawn);

	if (withRotation)
	{
		if ((parameterCount < 7 && nearPlayerSpawn)
			|| (parameterCount < 10 && !nearPlayerSpawn)
			|| !ComVar(&rotation.i).SetFromStr(tokens[tokenArgPos++])
			|| !ComVar(&rotation.j).SetFromStr(tokens[tokenArgPos++])
			|| !ComVar(&rotation.k).SetFromStr(tokens[tokenArgPos++]))
		{
			outputCb(StringFlag_None, "# insufficient/invalid ijk rotation spawn arguments");
			return 0;
		}
	}

	//lookup a commands.txt file that contain string->object_datums
	if (readObjectIds)
		ReadObjectDatumIdx();

	if (objectIds.find(objectName) == objectIds.end()) {
		objectDatum = strtoul(objectName.c_str(), NULL, 0);
	}
	else {
		objectDatum = objectIds[objectName];
	}

	real_vector3d* pRotation = nullptr;
	real_point3d* pPosition = nullptr;

	if (!nearPlayerSpawn)
	{
		pPosition = &position;
	}

	if (withRotation)
	{
		pRotation = &rotation;
	}

	ObjectSpawn(objectDatum, count, pPosition, pRotation, 1.0f, sameTeam);

	// outputCbFmt(StringFlag_None, "# spawned: %s, near player: %s with rotation: %i", objectName.c_str(), nearPlayerSpawn.GetValStr().c_str(), withRotation);

	return 0;
}

int CommandCollection::InjectTagCmd(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	if (!NetworkSession::LocalPeerIsSessionHost()
		&& !game_is_campaign())
	{
		outputCb(StringFlag_None, "# can only be used by the session host");
		return 0;
	}

	std::string tagName = tokens[1];
	std::wstring mapName(tokens[3].begin(), tokens[3].end());

	const char* p_string = tokens[2].c_str();
	tag_group tag_type;
	tag_type.string[3] = p_string[0];
	tag_type.string[2] = p_string[1];
	tag_type.string[1] = p_string[2];
	tag_type.string[0] = p_string[3];

	tag_injection_set_active_map(mapName.c_str());
	auto tag_datum = tag_injection_load(tag_type.group, tagName.c_str(), true);
	tag_injection_inject();

	outputCb(StringFlag_None, "# loaded tag datum: %#X", tag_datum);

	LOG_INFO_GAME("{} - {} {}", tagName, tag_type.string);
	return 0;
}

int CommandCollection::Crash(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	int* test = nullptr;
#pragma warning( push )
#pragma warning( disable : 6011 )
	return *test++;
#pragma warning( pop )
}

//////////////////////////////////////////////////////////////////////////
//	commands end
//////////////////////////////////////////////////////////////////////////

void CommandCollection::ObjectSpawn(datum object_idx, int count, const real_point3d* position, const real_vector3d* rotation, float randomMultiplier, bool sameTeam)
{
	typedef void(__cdecl* set_orientation_t)(real_vector3d* forward, real_vector3d* up, const real_vector3d* orient);
	auto p_vector3d_from_euler_angles3d = Memory::GetAddress<set_orientation_t>(0x3347B);

	for (int i = 0; i < count; i++)
	{
		try
		{
			object_placement_data new_object_placement;
			datum localPlayerIdx = player_index_from_user_index(0);
			real_point3d* localPlayerPos = &object_get_fast_unsafe(s_player::get(localPlayerIdx)->unit_index)->position;

			if (object_idx != NONE)
			{
				object_placement_data_new(&new_object_placement, object_idx, -1, 0);

				if (position)
				{
					new_object_placement.position = *position;
				}
				else if (localPlayerPos)
				{
					new_object_placement.position.x = localPlayerPos->x * static_cast <float> (rand()) / static_cast<float>(RAND_MAX);
					new_object_placement.position.y = localPlayerPos->y * static_cast <float> (rand()) / static_cast<float>(RAND_MAX);
					new_object_placement.position.z = (localPlayerPos->z + 5.0f) * static_cast <float> (rand()) / static_cast<float>(RAND_MAX);
				}

				if (rotation)
				{
					p_vector3d_from_euler_angles3d(&new_object_placement.forward, &new_object_placement.up, rotation);
				}

				if (!sameTeam)
					new_object_placement.team_index = NONE;

				LOG_TRACE_GAME("object_datum = {0:#x}, x={1:f}, y={2:f}, z={3:f}", object_idx, new_object_placement.position.x, new_object_placement.position.y, new_object_placement.position.z);
				datum new_object_index = object_new(&new_object_placement);
				simulation_action_object_create(new_object_index);
			}
		}
		catch (...) {
			LOG_TRACE_GAME("{} - error running spawn command", __FUNCTION__);
		}
	}
}

void CommandCollection::ReadObjectDatumIdx() {
	std::ifstream infile("commands.txt");
	if (infile.good()) {
		std::string line;
		while (std::getline(infile, line)) {
			std::vector<std::string> command;
			if (tokenize(line.c_str(), line.length(), ",", command))
			{
				if (command.size() != 2) {
					LOG_TRACE_GAME("Found line not comma separated right, {}", line);
				}
				else {
					objectIds[command[0]] = strtoul(command[1].c_str(), NULL, 0);
				}
			}
		}
	}
}

void CommandCollection::DeleteObject(datum objectDatumIdx)
{
	if (objectDatumIdx != NONE)
	{
		object_delete(objectDatumIdx);
	}
}

int CommandCollection::map_name(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	main_game_launch(tokens[1].c_str());
	return 0;
}

int CommandCollection::game_difficulty(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	int difficulty;
	std::string exception;
	ComVar(&difficulty).SetFromStr(tokens[1], 0, exception);

	main_game_launch_set_difficulty(difficulty);
	return 0;
}

int CommandCollection::game_coop_players(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	int player_count;
	std::string exception;
	ComVar(&player_count).SetFromStr(tokens[1], 0, exception);

	main_game_launch_set_coop_player_count(player_count);
	return 0;
}

int CommandCollection::game_multiplayer(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	main_game_launch_set_multiplayer_variant(tokens[1].c_str());
	return 0;
}

int CommandCollection::game_splitscreen(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	int player_count;
	std::string exception;
	ComVar(&player_count).SetFromStr(tokens[1], 0, exception);

	main_game_launch_set_multiplayer_splitscreen_count(player_count);
	return 0;
}

int CommandCollection::game_mode(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	int game_mode;
	std::string exception;
	ComVar(&game_mode).SetFromStr(tokens[1], 0, exception);

	main_game_launch_set_game_mode(game_mode);
	return 0;
}

int CommandCollection::invite(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;
	c_network_session* session = NULL;
	network_life_cycle_in_squad_session(&session);
	bool session_host = session->is_host();

	XSESSION_INFO x_session_info;
	x_session_info.sessionID = session->session_id;
	x_session_info.keyExchangeKey = session->xnkey;
	x_session_info.hostAddress = (session_host ? session->m_session_virtual_couch.xsession_info.hostAddress :
		session->m_network_observer->m_observer_channels[session->get_session_peer(session->m_session_host_peer_index)->observer_channel_index].xnaddr);

	uint8* session_bytes = (uint8*)&x_session_info;
	char connect_string[sizeof(XSESSION_INFO) * 2 + 1];

	// Encode the data into hex string
	for (uint32 i = 0; i < sizeof(XSESSION_INFO); i++)
	{
		sprintf(&connect_string[2 * i], "%02hhX", session_bytes[i]);
	}

	outputCb(StringFlag_None, "Invite code generated:");
	outputCb(StringFlag_CopyToClipboard, connect_string);
	outputCb(StringFlag_None, "Invite code has been copied to your clipboard.");
	return 0;
}

int CommandCollection::connect(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	XSESSION_INFO session;
	uint8* session_bytes = (uint8*)&session;

	// Decode the data from hex string
	for (uint32 i = 0; i < sizeof(XSESSION_INFO); i++)
	{
		(void)sscanf(&tokens[1].c_str()[2 * i], "%02hhX", &session_bytes[i]);
	}

	game_direct_connect_to_session(session.sessionID, session.keyExchangeKey, &session.hostAddress, EXECUTABLE_TYPE, EXECUTABLE_VERSION, COMPATIBLE_VERSION);

	return 0;
}

int CommandCollection::change_player_team(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	TextOutputCb* outputCb = ctx.outputCb;

	int32 player_index = NONE;
	int32 new_team_index = NONE;

	if (ComVar(&player_index).SetFromStr(tokens[1])
		&& ComVar(&new_team_index).SetFromStr(tokens[2]))
	{
		c_network_session* session = NULL;
		if (!network_life_cycle_in_squad_session(&session))
		{
			outputCb(StringFlag_None, "# not in a network session");
			return 0;
		}

		session->switch_player_team(player_index, (e_game_team)new_team_index);
	}

	return 0;
}


int CommandCollection::quit(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	main_quit();
	return 0;
}

int CommandCollection::_screenshot_cubemap(const std::vector<std::string>& tokens, ConsoleCommandCtxData ctx)
{
	ImGuiHandler::ToggleWindow("console");	// Close the console window so it doesn't appear in the cubemap screenshot
	screenshot_cubemap(tokens[1].c_str());
	return 0;
}
