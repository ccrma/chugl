#include "SceneGraphNode.h"

// static initialization ==============================
size_t SceneGraphNode::idCounter = 1;  // start at 1. reserve 0 for null or default types

// static instantiation
CK_DL_API SceneGraphNode::s_CKAPI = NULL;
