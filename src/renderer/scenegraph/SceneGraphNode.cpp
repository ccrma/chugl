#include "SceneGraphNode.h"

// static initialization ==============================
size_t SceneGraphNode::idCounter = 1;  // start at 1. reserve 0 for null or default types

// static instantiation
const Chuck_DL_Api* SceneGraphNode::s_CKAPI = NULL;
