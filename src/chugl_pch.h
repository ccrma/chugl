#pragma once

// system includes =======================================
#include <iostream>
#include <sstream>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <condition_variable>
#include <mutex>
#include <functional>

// data structures =======================================
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// ChucK includes ========================================
#include "chugin.h"

// glm includes ==========================================
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/epsilon.hpp>

// other vendors =========================================

// tracy profiler ========================================
#if defined( __clang__ ) || defined(__GNUC__)
    # define TracyFunction __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
    # define TracyFunction __FUNCSIG__
#endif
#include <tracy/Tracy.hpp>

// assimp asset import library ===========================
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// ChuGL includes ========================================
#include "util/Timer.h"
