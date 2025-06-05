#pragma once

#define CHUGL_RENDERGRAPH_MAX_PASSES 32
#define CHUGL_MATERIAL_MAX_BINDINGS 32 // @group(1) @binding(0 - 31)
#define CHUGL_MAX_BINDGROUPS 4

// how many frames does a bindgroup need to be unused before we evict it from our
// rendergraph cache
#define CHUGL_CACHE_BINDGROUP_FRAMES_TILL_EXPIRED 4