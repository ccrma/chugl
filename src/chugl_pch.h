#pragma once

// system includes =======================================
#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>

// data structures =======================================
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ChucK includes ========================================
#include "chugin.h"

// glm includes ==========================================
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

// other vendors =========================================

// tracy profiler ========================================
#if defined(__clang__) || defined(__GNUC__)
#define TracyFunction __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define TracyFunction __FUNCSIG__
#endif
#include <tracy/Tracy.hpp>

// assimp asset import library ===========================
// #include <assimp/Importer.hpp>
// #include <assimp/scene.h>
// #include <assimp/postprocess.h>

// freetype font library =================================
#include <ft2build.h>
#include FT_FREETYPE_H

// ChuGL includes ========================================
// #include "util/Timer.h"
