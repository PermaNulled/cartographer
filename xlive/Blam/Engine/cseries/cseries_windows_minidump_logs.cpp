#include "stdafx.h"
#include "cseries_windows_minidump_logs.h"

#include "cseries_strings.h"
#include "cseries_windows_modules.h"
#include "cseries_windows_debug_pc.h"

#include "game/game.h"
#include "game/game_options.h"
#include "rasterizer/dx9/rasterizer_dx9_main.h"

/* constants */

#define k_crash_message_header_break L"=============================================\n"
const wchar_t *const k_report_text_file_names[k_report_text_file_type_count] = { L"exception_info.txt", L"cpu_info.txt", L"game_options.txt", L"game_globals.txt", L"rasterizer.txt"};

// globals

static c_static_wchar_string<MAX_PATH>* g_report_text_file_paths = { NULL };

/* prototypes */

// Create and populate new cpu_info.txt file in the report path
static void setup_cpu_info_text(const wchar_t* reports_path, const MINIDUMP_EXCEPTION_INFORMATION* minidump_info);

// Create and populate new exception_info.txt file in the report path
static void setup_exception_text(const wchar_t* reports_path, const MINIDUMP_EXCEPTION_INFORMATION* minidump_info);

// Create and populate new game_options.txt file in the report path
static void setup_game_options_text(const wchar_t* reports_path);

// Create and populate new game_globals.txt file in the report path
static void setup_game_global_text(const wchar_t* reports_path);

// Create and populate new rasterizer.txt file in the report path
static void setup_rasterizer_text(const wchar_t* reports_path);

// Inserts a value into a file as a string represented in hex
static void print_hex_value_to_file(FILE* file, uint32 value);

// Inserts a bool into a file as a string that's either "true" or "false"
static void print_bool_to_file(FILE* file, bool value);

// Verify the pointers in a minidump file
static bool minidump_info_verify_pointers(FILE* file, const MINIDUMP_EXCEPTION_INFORMATION* minidump_info);

/* public code */

const wchar_t* get_crash_info_text_file_path(e_report_file_type type)
{
    return g_report_text_file_paths[type].get_string();
}

void crash_info_text_files_create(const wchar_t* reports_path, const MINIDUMP_EXCEPTION_INFORMATION* minidump_info)
{
    g_report_text_file_paths = new c_static_wchar_string<MAX_PATH>[k_report_text_file_type_count]();
    
    setup_cpu_info_text(reports_path, minidump_info);
    setup_exception_text(reports_path, minidump_info);
    setup_game_options_text(reports_path);
    setup_game_global_text(reports_path);
    setup_rasterizer_text(reports_path);

    delete[] g_report_text_file_paths;
    return;
}

/* private code */

static void setup_cpu_info_text(const wchar_t* reports_path, const MINIDUMP_EXCEPTION_INFORMATION* minidump_info)
{
    c_static_wchar_string<MAX_PATH>* report_info_path_cpu = &g_report_text_file_paths[_report_file_type_cpu];
    report_info_path_cpu->set(reports_path);
    report_info_path_cpu->append(k_reports_path);
    report_info_path_cpu->append(k_report_text_file_names[_report_file_type_cpu]);

    FILE* file;
    errno_t error = _wfopen_s(&file, report_info_path_cpu->get_string(), L"w+");
    if (!error && file != NULL)
    {
        fwprintf(file, L"CPU INFORMATION\n");
        fwprintf(file, L"%ls", k_crash_message_header_break);

        if (minidump_info_verify_pointers(file, minidump_info))
        {
            fwprintf(file, L"EDI: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ContextRecord->Edi);

            fwprintf(file, L"ESI: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ContextRecord->Esi);

            fwprintf(file, L"EBX: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ContextRecord->Ebx);

            fwprintf(file, L"EDX: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ContextRecord->Edx);

            fwprintf(file, L"ECX: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ContextRecord->Ecx);

            fwprintf(file, L"EAX: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ContextRecord->Eax);

            fwprintf(file, L"EBP: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ContextRecord->Ebp);

            fwprintf(file, L"EIP: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ContextRecord->Eip);

            fwprintf(file, L"ESP: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ContextRecord->Esp);

            fwprintf(file, L"Thread ID: ");
            fwprintf(file, L"%u\n", minidump_info->ThreadId);
        }

        fclose(file);
    }

    return;
}

static void setup_module_text(FILE* file, const MINIDUMP_EXCEPTION_INFORMATION* minidump_info)
{
    // Get module info
    DWORD process_id = GetCurrentProcessId();
    s_loaded_module_info* module_info = get_loaded_module_info();
    const size_t module_count = module_info_populate(process_id, module_info);

    // Sort module by address
    qsort(module_info, module_count, sizeof(s_loaded_module_info), compare_loaded_module_info);     
    DWORD crash_address = minidump_info->ExceptionPointers->ContextRecord->Eip;

    size_t crashed_module_index = get_crashed_module_index(module_info, module_count, crash_address);

    // Get module name
    int index = module_info[crashed_module_index].module_path.last_index_of(L"\\");
    wchar_t* module_name = &module_info[crashed_module_index].module_path.get_buffer()[index + 1];
    uintptr_t base_address = get_module_base_address(process_id, module_name);                          // Get base address of the module we crashed in
    void* module_data = module_data_load(module_info[crashed_module_index].module_path.get_string());   // Read module data
    uintptr_t crash_file_offset = devirtualize_address(module_data, crash_address, base_address);       // Get file offset for where we crashed in the module
    module_data_free(module_data);                                                                      // Cleanup module data

    fwprintf(file, L"Crash File Offset: %ls+0x", module_name);
    print_hex_value_to_file(file, crash_file_offset);
    fwprintf(file, L"\n");

    // Print every module that's currently loaded
    fwprintf(file, L"MODULES:\n");
    fwprintf(file, L"%ls", k_crash_message_header_break);

    // Print out every module
    for (size_t i = 0; i < module_count; i++)
    {
        // Store the index of the module info array where we crashed
        if (crash_address > (DWORD)module_info[i].module_handle)
        {
            crashed_module_index = i;
        }

        fwprintf(file, L"%ls", module_info[i].module_path.get_string());
        fwprintf(file, L": ");
        print_hex_value_to_file(file, (DWORD)module_info[i].module_handle);
    }

    return;
}

void setup_exception_text(const wchar_t* reports_path, const MINIDUMP_EXCEPTION_INFORMATION* minidump_info)
{
    c_static_wchar_string<MAX_PATH>* report_info_path_exception = &g_report_text_file_paths[_report_file_type_exception];
    report_info_path_exception->set(reports_path);
    report_info_path_exception->append(k_reports_path);
    report_info_path_exception->append(k_report_text_file_names[_report_file_type_exception]);

    FILE* file;
    errno_t error = _wfopen_s(&file, report_info_path_exception->get_string(), L"w+");
    if (!error && file != NULL)
    {
        fwprintf(file, L"EXCEPTION INFORMATION\n");
        fwprintf(file, L"%ls", k_crash_message_header_break);
        
        if (minidump_info_verify_pointers(file, minidump_info))
        {
            fwprintf(file, L"Exception Code: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ExceptionRecord->ExceptionCode);

            fwprintf(file, L"Exception Flags: ");
            print_hex_value_to_file(file, minidump_info->ExceptionPointers->ExceptionRecord->ExceptionFlags);

            fwprintf(file, L"Exception Address: ");
            print_hex_value_to_file(file, (DWORD)&minidump_info->ExceptionPointers->ExceptionRecord->ExceptionAddress);

            fwprintf(file, L"Number Parameters: ");
            fwprintf(file, L"%u\n", minidump_info->ExceptionPointers->ExceptionRecord->NumberParameters);

            fwprintf(file, L"Exception Information: ");
            print_array_to_file(file, minidump_info->ExceptionPointers->ExceptionRecord->ExceptionInformation, EXCEPTION_MAXIMUM_PARAMETERS);

            setup_module_text(file, minidump_info);
        }

        fclose(file);
    }

    return;
}

static void setup_game_options_text(const wchar_t* reports_path)
{
    c_static_wchar_string<MAX_PATH>* report_info_path_game_options = &g_report_text_file_paths[_report_file_type_game_options];
    report_info_path_game_options->set(reports_path);
    report_info_path_game_options->append(k_reports_path);
    report_info_path_game_options->append(k_report_text_file_names[_report_file_type_game_options]);

    FILE* file;
    errno_t error = _wfopen_s(&file, report_info_path_game_options->get_string(), L"w+");
    const s_game_options* game_options = game_options_get();
    if (!error && file != NULL && game_options != NULL)
    {
        fwprintf(file, L"GAME OPTIONS\n");
        fwprintf(file, L"%ls", k_crash_message_header_break);

        fwprintf(file, L"Engine Type: ");
        switch (game_options->game_mode)
        {
        case _game_mode_campaign:
            fwprintf(file, L"Singleplayer");
            break;
        case _game_mode_multiplayer:
            fwprintf(file, L"Multiplayer");
            break;
        case _game_mode_ui_shell:
            fwprintf(file, L"Main Menu");
            break;
        case _game_mode_mutiplayer_shared:
            fwprintf(file, L"Shared");
            break;
        case _game_mode_single_player_shared:
            fwprintf(file, L"Singleplayer Shared");
            break;
        default:
            fwprintf(file, L"None");
            break;
        }
        fwprintf(file, L"\n");

        fwprintf(file, L"Simulation Type: ");
        switch (game_options->simulation_type)
        {
        case _game_simulation_none:
            fwprintf(file, L"None");
            break;
        case _game_simulation_local:
            fwprintf(file, L"Local");
            break;
        case _game_simulation_synchronous_client:
            fwprintf(file, L"Synchronous Client");
            break;
        case _game_simulation_synchronous_server:
            fwprintf(file, L"Synchronous Server");
            break;
        case _game_simulation_distributed_client:
            fwprintf(file, L"Distributed Client");
            break;
        case _game_simulation_distributed_server:
            fwprintf(file, L"Distributed Server");
            break;
        default:
            fwprintf(file, L"Not defined");
            break;
        }
        fwprintf(file, L"\n");

        fwprintf(file, L"Scenario Custom: ");
        print_bool_to_file(file, game_options->scenario_custom);

        fwprintf(file, L"Game Tick Rate: ");
        fwprintf(file, L"%d\n", game_options->game_tick_rate);

        fwprintf(file, L"Random Data: ");
        print_array_to_file(file, game_options->random_data, 8);

        fwprintf(file, L"Random Seed: ");
        fwprintf(file, L"%d\n", game_options->random_seed);

        fwprintf(file, L"Is Custom Map: ");
        print_bool_to_file(file, game_options->is_custom_map);

        fwprintf(file, L"Custom Map Name: %ls\n", game_options->custom_map_name);

        fwprintf(file, L"Campaign ID: ");
        fwprintf(file, L"%d\n", game_options->campaign_id);

        fwprintf(file, L"Map ID: ");
        fwprintf(file, L"%d\n", game_options->map_id);

        fwprintf(file, L"Scenario Path: %ls\n", game_options->scenario_path);

        fwprintf(file, L"Map ID: ");
        fwprintf(file, L"%d\n", game_options->map_id);

        fwprintf(file, L"Initial BSP Index: ");
        fwprintf(file, L"%hi\n", game_options->initial_bsp_index);

        fwprintf(file, L"Load Level Only: ");
        print_bool_to_file(file, game_options->load_level_only);

        fwprintf(file, L"Local Peer Index: ");
        fwprintf(file, L"%hhu\n", game_options->local_peer_index);

        fwprintf(file, L"Dump Object Log: ");
        print_bool_to_file(file, game_options->dump_object_log);

        fwprintf(file, L"Dump Random Seeds: ");
        print_bool_to_file(file, game_options->dump_random_seeds);

        fwprintf(file, L"Playtest Mode: ");
        print_bool_to_file(file, game_options->playtest_mode);

        fwprintf(file, L"Difficulty: ");
        fwprintf(file, L"%hi\n", game_options->difficulty);

        fwprintf(file, L"Coop: ");
        print_bool_to_file(file, game_options->coop);

        fwprintf(file, L"Player Count: ");
        fwprintf(file, L"%hhu\n", game_options->player_count);

        fwprintf(file, L"Menu Context: ");
        fwprintf(file, L"%u\n", game_options->menu_context);

        fwprintf(file, L"Valid Machine Mask: ");
        fwprintf(file, L"%u\n", game_options->valid_machine_mask);

        fwprintf(file, L"Machine Identifiers: ");
        for (size_t i = 0; i < 17; i++)
        {
            print_array_to_file(file, game_options->machines[i].machine_identifier, 6);
        }

        fwprintf(file, L"Local Machine Exists: ");
        print_bool_to_file(file, game_options->coop);

        fwprintf(file, L"Local Machine Identifier: ");
        print_array_to_file(file, game_options->local_machine_identifier.machine_identifier, 6);

        fclose(file);
    }

    return;
}

static void setup_game_global_text(const wchar_t* reports_path)
{
    c_static_wchar_string<MAX_PATH>* report_info_path_game_globals = &g_report_text_file_paths[_report_file_type_game_globals];
    report_info_path_game_globals->set(reports_path);
    report_info_path_game_globals->append(k_reports_path);
    report_info_path_game_globals->append(k_report_text_file_names[_report_file_type_game_globals]);

    FILE* file;
    errno_t error = _wfopen_s(&file, report_info_path_game_globals->get_string(), L"w+");
    const s_main_game_globals* game_globals = get_main_game_globals();
    if (!error && file != NULL && game_globals != NULL)
    {

        fwprintf(file, L"GAME GLOBALS\n");
        fwprintf(file, L"%ls", k_crash_message_header_break);

        fwprintf(file, L"Initializing: ");
        print_bool_to_file(file, game_globals->initializing);

        fwprintf(file, L"Map Active: ");
        print_bool_to_file(file, game_globals->map_active);

        fwprintf(file, L"Active Structure BSP Index: ");
        print_bool_to_file(file, game_globals->active_structure_bsp_index);

        fwprintf(file, L"Game In Progress: ");
        print_bool_to_file(file, game_globals->game_in_progress);

        fwprintf(file, L"Game Is Lost: ");
        print_bool_to_file(file, game_globals->game_is_lost);

        fwprintf(file, L"Ticks To Reset Game: %d\n", game_globals->ticks_to_reset_game);

        fwprintf(file, L"Game Is Finished: ");
        print_bool_to_file(file, game_globals->game_is_finished);

        fwprintf(file, L"Game Sounds Disabled: ");
        print_bool_to_file(file, game_globals->game_sounds_disabled);

        fwprintf(file, L"Ticks Till End: %u\n", game_globals->ticks_till_end);

        fwprintf(file, L"Game Ragdoll Count: %d\n", game_globals->game_ragdoll_count);

        fwprintf(file, L"Cluster PVS: ");
        print_array_to_file(file, game_globals->cluster_pvs.cluster_bitvector, NUMBEROF(game_globals->cluster_pvs.cluster_bitvector));

        fwprintf(file, L"Cluster PVS Local: ");
        print_array_to_file(file, game_globals->cluster_pvs_local.cluster_bitvector, NUMBEROF(game_globals->cluster_pvs_local.cluster_bitvector));

        fwprintf(file, L"Cluster Activation: ");
        print_array_to_file(file, game_globals->cluster_activation.cluster_bitvector, NUMBEROF(game_globals->cluster_activation.cluster_bitvector));

        fwprintf(file, L"Enable Scripted Camera PVS: ");
        print_bool_to_file(file, game_globals->enable_scripted_camera_pvs);

        fwprintf(file, L"PVS Object Is Set: %hu\n", game_globals->pvs_object_is_set);

        fwprintf(file, L"PVS Object Datum: %d\n", game_globals->pvs_object_datum);

        fclose(file);
    }

    return;
}

static void setup_rasterizer_text(const wchar_t* reports_path)
{
    c_static_wchar_string<MAX_PATH>* report_info_path_game_globals = &g_report_text_file_paths[_report_file_type_rasterizer];
    report_info_path_game_globals->set(reports_path);
    report_info_path_game_globals->append(k_reports_path);
    report_info_path_game_globals->append(k_report_text_file_names[_report_file_type_rasterizer]);

    FILE* file;
    errno_t error = _wfopen_s(&file, report_info_path_game_globals->get_string(), L"w+");
    const s_main_game_globals* game_globals = get_main_game_globals();
    if (!error && file != NULL && game_globals != NULL)
    {
        fwprintf(file, L"RASTERIZER\n");
        fwprintf(file, L"%ls", k_crash_message_header_break);

        fwprintf(file, L"Last Bitmap Tag Datum: ");
        print_hex_value_to_file(file, (uint32)last_bitmap_tag_index_get());

        fclose(file);
    }

    return;
}

// Print value parameter to file in hexadecimal format
static void print_hex_value_to_file(FILE* file, uint32 value)
{
    const size_t length = sizeof(value) * 2;
    wchar_t hex_string[length + 1];

    // Set string to value
    wsprintf(hex_string, L"%X", value);
    hex_string[length] = L'\0';

    fwprintf(file, L"%ls", hex_string);
    fwprintf(file, L"\n");

    return;
}

// Print value parameter to file as True or False
static void print_bool_to_file(FILE* file, bool value)
{
    fwprintf(file, L"%ls\n", (value ? L"True" : L"False"));
    return;
}

// Verify pointers are not null in minidump_info struct
static bool minidump_info_verify_pointers(FILE* file, const MINIDUMP_EXCEPTION_INFORMATION* minidump_info)
{
    bool print_data = true;
    if (minidump_info->ExceptionPointers == nullptr)
    {
        fwprintf(file, L"Exception Record: NULL\n");
        print_data = false;
    }
    else
    {
        if (minidump_info->ExceptionPointers->ContextRecord == nullptr)
        {
            fwprintf(file, L"Context Record: NULL\n");
            print_data = false;
        }
    }

    return print_data;
}
