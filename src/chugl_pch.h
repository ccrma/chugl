#pragma once

// system includes =======================================
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <condition_variable>

// Data Structures =======================================
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Chuck includes ========================================
#include "chuck_def.h"
#include "chuck_dl.h"
#include "chuck_vm.h"

// tracy profiler ========================================
#if defined( __clang__ ) || defined(__GNUC__)
    # define TracyFunction __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
    # define TracyFunction __FUNCSIG__
#endif
#include <tracy/Tracy.hpp>
