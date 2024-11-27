// what does a draw call need?

struct WGPURenderPassColorAttachment {
    WGPU_NULLABLE WGPUTextureView view;
    uint32_t depthSlice;
    WGPU_NULLABLE WGPUTextureView resolveTarget; // for MSAA resolve
    WGPULoadOp loadOp;                           // undefined, clear, load
    WGPUStoreOp storeOp;                         // undefined, store, discard
    WGPUColor clearValue;
};

struct WGPURenderPassDescriptor {
    size_t colorAttachmentCount;
    WGPURenderPassColorAttachment const* colorAttachments;
    WGPU_NULLABLE WGPURenderPassDepthStencilAttachment const* depthStencilAttachment;
};

WGPURenderPassEncoder*
wgpuCommandEncoderBeginRenderPass(WGPUCommandEncoder commandEncoder,
                                  WGPURenderPassDescriptor const* descriptor);

wgpuRenderPassEncoderSetPipeline(WGPURenderPassEncoder renderPassEncoder,
                                 WGPURenderPipeline pipeline);

// set per-material and per-frame bind group
wgpuRenderPassEncoderSetBindGroup(WGPURenderPassEncoder, uint32_t groupIndex,
                                  WGPU_NULLABLE WGPUBindGroup);

// set vertex/index buffers (per geometry)
wgpuRenderPassEncoderSetVertexBuffer(WGPURenderPassEncoder, uint32_t slot,
                                     WGPU_NULLABLE WGPUBuffer buffer, uint64_t offset,
                                     uint64_t size);
wgpuRenderPassEncoderSetIndexBuffer(WGPURenderPassEncoder WGPUBuffer,
                                    WGPUIndexFormat uint64_t offset, uint64_t size);

// set per-draw bind group (xform data)
wgpuRenderPassEncoderSetBindGroup(WGPURenderPassEncoder, uint32_t groupIndex,
                                  WGPU_NULLABLE WGPUBindGroup);

// draw (non-indexed or indexed)
{
    wgpuRenderPassEncoderDraw(WGPURenderPassEncoder renderPassEncoder,
                              uint32_t vertexCount, uint32_t instanceCount,
                              // uint32_t firstVertex,
                              // uint32_t firstInstance
    );

    wgpuRenderPassEncoderDrawIndexed(WGPURenderPassEncoder renderPassEncoder,
                                     uint32_t indexCount, uint32_t instanceCount,
                                     // uint32_t firstIndex,
                                     // int32_t baseVertex,
                                     // uint32_t firstInstance
    );
}

// =====================================================================================

struct DrawCall {
    // render pass state ---------------------------------------------
    struct WGPURenderPassDescriptor {
        WGPURenderPassColorAttachment colorAttachments[] WGPU_NULLABLE WGPUTextureView
          view;
        uint32_t depthSlice;
        WGPU_NULLABLE WGPUTextureView resolveTarget; // for MSAA resolve
        WGPULoadOp loadOp;                           // undefined, clear, load
        WGPUStoreOp storeOp;                         // undefined, store, discard
        WGPUColor clearValue;                        // clear color

        WGPU_NULLABLE WGPURenderPassDepthStencilAttachment const*
          depthStencilAttachment;
        WGPUTextureView view;
        WGPULoadOp depthLoadOp;
        WGPUStoreOp depthStoreOp;
        float depthClearValue; // 1.0f for "far"
        WGPUBool depthReadOnly;
        // ignoring stencil
    };

    // pipeline state ---------------------------------------------
    WGPURenderPipeline struct WGPURenderPipelineDescriptor {
        WGPUVertexState vertex;
        - WGPUVertexBufferLayout buffers[];
        WGPUPrimitiveState primitive;
        - WGPUPrimitiveTopology topology; // (e.g. points, lines, tris)
        - WGPUCullMode cullMode;          // (none, front, back)
        WGPU_NULLABLE WGPUDepthStencilState const*
          depthStencil;           // (NO STENCIL, simplify)
        WGPUTextureFormat format; // probably lock this down, 32 bit no stencil
        WGPUBool depthWriteEnabled;
        WGPUCompareFunction
          // Undefined Never Less LessEqual Greater GreaterEqual Equal NotEqual Always
          int32_t depthBias;
        float depthBiasSlopeScale;
        float depthBiasClamp;
        WGPUMultisampleState
          multisample; // needs to match render pass color attachment sample count
        int sample_count;
        // ms.mask                   = 0xFFFFFFFF; // ignore for now
        // ms.alphaToCoverageEnabled = false;      // ignore for now
        WGPU_NULLABLE WGPUFragmentState const* fragment;
        WGPUColorTargetState
          targets[]; // guess this needs to match renderpass colorAttachments
        WGPUTextureFormat format;
        WGPU_NULLABLE WGPUBlendState const* blend;
        WGPUBlendComponent color;          // operation, srcFactor, dstFactor
        WGPUBlendComponent alpha;          // operation, srcFactor, dstFactor
        WGPUColorWriteMaskFlags writeMask; // None Red Green Blue Alpha All
    };

    // bind groups (currently spread across 0: frame uniforms, 1: material, 2: per_draw
    // (instancing)  3: vertexPullGroups)
    // ---------------------------------------------
    uint32_t groupIndex[];
    WGPU_NULLABLE WGPUBindGroup[];

    // Vertex/Index buffers ---------------------------------------------
    WGPUBuffer vertex_buffers[];
    uint64_t vertex_buffer_sizes_bytes[];
    // uint64_t vertex_buffer_offsets[]
    WGPUBuffer index_buffers;
    uint64_t index_buffer_size_bytes;
    // uint64_t index_buffer_offset

    // actual draw call ---------------------------------------------
    bool draw_indexed; // else draw normal
    int index_or_vertex_count;
    int instance_count;
    // int first_index_or_vertex,
    // int baseVertex,  // only for indexed draw
    // int firstInstance
};

// ============================================================================
// New IMPL
// ============================================================================

/*
High level:
- dynamically create bindgroups right before the draw call, and release
immediately after
  - reason: BindGroups are specific to a BindGroupLayout, and thus a single
  RenderPipeline (when using implicit layout: auto). Adding dynamic MSAA means
  that a single material can be associated with multiple RenderPipelines
  (e.g. no MSAA, 2xMSAA, 4xMSAA) and therefore it doesn't make sense to cache
  the bindgroup any more. Same goes for all the XForm matrices in a GScene

*/

typedef int SG_ID;

struct DrawCall {
    SG_ID shader_id;
    SG_ID material_id;
    SG_ID geo_id;
};

struct R_Scene {

    // add
    DrawCall drawcalls[];
    bool stale; // optional. optimization
    // in case we render the same GScene in multiple render passes, flag so we don't
    // need to rebuild all the drawcall list state every time?

    // keep
    Hashmap<(material, geo), xform_ids : int[]>
    // - keep the lazy deletion, and marking an entry as "stale" when xforms change
    // this preserves our instanced-draw-by-default and keeps buffer uploads of
    // transform data to a minimum

    // Remove
    // all the other complicated hashmap/tree state
};

void onAddSubgraph(...)
{
    // Convert mesh to drawcall struct and add to scene->drawcalls
}

void onRemoveSubgraph(...)
{
    // don't need to remove drawcall from scene->drawcalls, the lazy deletion
    // will happen in prepareScene
}

// convert the mesh_id list into a list of sorted draw call structs
void prepareScene(R_Scene* scene)
{
    Arena draw_call_arena;

    foreach (DrawCall drawcall : scene->mesh_ids) {
        ASSERT(every id is valid);

        // rebuild material GPU buffer if stale
        // NOTE: this no longer creates a bindgroup

        // lookup in scene G2X map if (material, geo) is empty
        // if so, remove from both G2X and scene->drawcalls

        // push for sorting
        // this would be the place to add transparency
        ARENA_PUSH(draw_call_arena, drawcall);
    }

    qsort();

    // render all the drawcalls in (now sorted) draw_call_arena in order
    // except now we dynamically create and release bindgroups within the nested loops
    R_RenderPipeline pipeline = GetOrCreateRenderPipeline(R_PSO);
}

// new lookup key for all standard material render pipelines
struct R_PSO {
    SG_ID shader_id;
    Material PSO stuff;
    MSAA Count;
    // whatever else, add as needed
};

// new Shader GC

struct R_Shader {
    // add
    WGPUPipeline pipelines[];
};

RenderPipeline::init()
{
    // add pipeline to R_Shader pipelines[]
    // needed for GC
}