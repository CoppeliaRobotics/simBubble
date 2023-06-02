#pragma once

#include <simLib/simExp.h>

// The 3 required entry points of the plugin:
SIM_DLLEXPORT int simInit(const char* pluginName);
SIM_DLLEXPORT void simCleanup();
SIM_DLLEXPORT void simMsg(int message,int* auxData,void* pointerData);