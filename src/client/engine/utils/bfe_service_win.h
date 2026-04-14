#pragma once

#include "engine/helper/helper.h"

// Manage BFE (Base filtering engine) service on Windows
namespace BFEService
{
    bool isRunning(Helper *helper);
    bool start(Helper *helper);
}
