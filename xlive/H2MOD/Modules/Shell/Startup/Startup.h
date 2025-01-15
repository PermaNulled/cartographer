#pragma once
#include "cseries/cseries_strings.h"

void InitH2Startup();
void H2DedicatedServerStartup();
void DeinitH2Startup();

// use only after initLocalAppData has been called
// by default useAppDataLocalPath is set to true, if not specified
inline void prepareLogFileName(const wchar_t* logFileName, c_static_wchar_string<MAX_PATH>* path, bool useAppDataLocalPath = true);

extern wchar_t* H2ProcessFilePath;
extern wchar_t* H2AppDataLocal;
extern wchar_t* FlagFilePathConfig;
