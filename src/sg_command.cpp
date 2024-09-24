#include "sg_command.h"

#include "core/macros.h"
#include "core/spinlock.h"

// include for static assert
#include <type_traits>

// command queue
struct CQ {
    // command queue lock
    // only held when 1: adding new command and 2:
    // swapping the read/write queues
    spinlock write_q_lock;
    Arena* read_q;
    Arena* write_q;

    // TODO: test if toggle is faster
    // brings the command queue struct to below 64 bytes
    // b8 toggle;

    Arena cq_a = {};
    Arena cq_b = {};
};

// enforce it fits within a cache line
// static_assert(sizeof(CQ) <= 64);
// TODO: look into aligned_alloc for cache line alignment

static CQ cq = {};

// static Arena* _CQ_GetReadQ()
// {
//     return cq.toggle ? &cq.cq_a : &cq.cq_b;
// }

// static Arena* _CQ_GetWriteQ()
// {
//     return cq.toggle ? &cq.cq_b : &cq.cq_a;
// }

void CQ_Init()
{
    ASSERT(cq.cq_a.base == NULL);
    ASSERT(cq.cq_b.base == NULL);

    Arena::init(&cq.cq_a, MEGABYTE);
    Arena::init(&cq.cq_b, MEGABYTE);

    cq.read_q  = &cq.cq_a;
    cq.write_q = &cq.cq_b;
}

void CQ_Free()
{
    Arena::free(&cq.cq_a);
    Arena::free(&cq.cq_b);
}

// swap the command queue double buffer
void CQ_SwapQueues()
{
    // assert read queue has been flushed before swapping
    ASSERT(cq.read_q->curr == 0);

    Arena* temp = cq.read_q;
    cq.read_q   = cq.write_q;

    spinlock::lock(&cq.write_q_lock);
    cq.write_q = temp;
    spinlock::unlock(&cq.write_q_lock);
}

bool CQ_ReadCommandQueueIter(SG_Command** command)
{
    // command queue empty
    if (cq.read_q->curr == 0) {
        *command = NULL;
        return false;
    }

    // return the first command in queue
    if (*command == NULL) {
        *command = (SG_Command*)cq.read_q->base;
        return true;
    }

    // sanity bounds check
    ASSERT(*command >= (SG_Command*)cq.read_q->base
           && *command < (SG_Command*)Arena::top(cq.read_q));

    // at last element
    if ((*command)->nextCommandOffset == cq.read_q->curr) {
        *command = NULL;
        return false;
    }

    // else return the nextOffset
    *command = (SG_Command*)Arena::get(cq.read_q, (*command)->nextCommandOffset);
    return true;
}

void CQ_ReadCommandQueueClear()
{
    Arena::clear(cq.read_q);
}

void* CQ_ReadCommandGetOffset(u64 byte_offset)
{
    return Arena::get(cq.read_q, byte_offset);
}

// ============================================================================
// Command API
// ============================================================================

#define BEGIN_COMMAND(cmd_type, cmd_enum)                                              \
    spinlock::lock(&cq.write_q_lock);                                                  \
    cmd_type* command = ARENA_PUSH_TYPE(cq.write_q, cmd_type);                         \
    command->type     = cmd_enum;

#define BEGIN_COMMAND_ADDITIONAL_MEMORY(cmd_type, cmd_enum, additional_bytes)          \
    spinlock::lock(&cq.write_q_lock);                                                  \
    cmd_type* command                                                                  \
      = (cmd_type*)Arena::push(cq.write_q, sizeof(cmd_type) + (additional_bytes));     \
    void* memory  = (void*)(command + 1);                                              \
    command->type = cmd_enum;

#define BEGIN_COMMAND_ADDITIONAL_MEMORY_ZERO(cmd_type, cmd_enum, additional_bytes)     \
    spinlock::lock(&cq.write_q_lock);                                                  \
    cmd_type* command                                                                  \
      = (cmd_type*)Arena::pushZero(cq.write_q, sizeof(cmd_type) + (additional_bytes)); \
    void* memory  = (void*)(command + 1);                                              \
    command->type = cmd_enum;

#define END_COMMAND()                                                                  \
    command->nextCommandOffset = cq.write_q->curr;                                     \
    spinlock::unlock(&cq.write_q_lock);

void CQ_PushCommand_WindowClose()
{
    BEGIN_COMMAND(SG_Command_WindowClose, SG_COMMAND_WINDOW_CLOSE);
    END_COMMAND();
}

void CQ_PushCommand_WindowMode(SG_WindowMode mode, int width, int height)
{
    BEGIN_COMMAND(SG_Command_WindowMode, SG_COMMAND_WINDOW_MODE);
    command->mode   = mode;
    command->width  = width;
    command->height = height;
    END_COMMAND();
}

void CQ_PushCommand_WindowSizeLimits(int min_width, int min_height, int max_width,
                                     int max_height, int aspect_ratio_x,
                                     int aspect_ratio_y)
{
    BEGIN_COMMAND(SG_Command_WindowSizeLimits, SG_COMMAND_WINDOW_SIZE_LIMITS);
    command->min_width      = min_width;
    command->min_height     = min_height;
    command->max_width      = max_width;
    command->max_height     = max_height;
    command->aspect_ratio_x = aspect_ratio_x;
    command->aspect_ratio_y = aspect_ratio_y;
    END_COMMAND();
}

void CQ_PushCommand_WindowPosition(int x, int y)
{
    BEGIN_COMMAND(SG_Command_WindowPosition, SG_COMMAND_WINDOW_POSITION);
    command->x = x;
    command->y = y;
    END_COMMAND();
}

void CQ_PushCommand_WindowCenter()
{
    BEGIN_COMMAND(SG_Command_WindowPosition, SG_COMMAND_WINDOW_CENTER);
    END_COMMAND();
}

// copies title into command arena
void CQ_PushCommand_WindowTitle(const char* title)
{
    size_t title_len = strlen(title);
    BEGIN_COMMAND_ADDITIONAL_MEMORY_ZERO(SG_Command_WindowTitle,
                                         SG_COMMAND_WINDOW_TITLE, title_len + 1);
    // copy title
    strncpy((char*)memory, title, title_len);

    // store offset not pointer in case arena resizes
    command->title_offset = Arena::offsetOf(cq.write_q, memory);

    END_COMMAND();
}

void CQ_PushCommand_WindowIconify(bool iconify)
{
    BEGIN_COMMAND(SG_Command_WindowIconify, SG_COMMAND_WINDOW_ICONIFY);
    command->iconify = iconify;
    END_COMMAND();
}

void CQ_PushCommand_WindowAttribute(CHUGL_WindowAttrib attrib, bool value)
{
    BEGIN_COMMAND(SG_Command_WindowAttribute, SG_COMMAND_WINDOW_ATTRIBUTE);
    command->attrib = attrib;
    command->value  = value;
    END_COMMAND();
}

void CQ_PushCommand_WindowOpacity(float opacity)
{
    BEGIN_COMMAND(SG_Command_WindowOpacity, SG_COMMAND_WINDOW_OPACITY);
    command->opacity = opacity;
    END_COMMAND();
}

void CQ_PushCommand_MouseMode(int mode)
{
    BEGIN_COMMAND(SG_Command_MouseMode, SG_COMMAND_MOUSE_MODE);
    command->mode = mode;
    END_COMMAND();
}

void CQ_PushCommand_MouseCursorNormal()
{
    BEGIN_COMMAND(SG_Command_MouseCursorNormal, SG_COMMAND_MOUSE_CURSOR_NORMAL);
    END_COMMAND();
}

void CQ_PushCommand_MouseCursor(CK_DL_API API, Chuck_ArrayInt* image_data, u32 width,
                                u32 height, u32 xhot, u32 yhot)
{
    u32 data_size = API->object->array_int_size(image_data);
    ASSERT(data_size == width * height * 4);

    BEGIN_COMMAND_ADDITIONAL_MEMORY(SG_Command_MouseCursor, SG_COMMAND_MOUSE_CURSOR,
                                    data_size);
    // push bytes for pixel data
    char* image_data_bytes = (char*)memory;
    // copy
    for (u32 i = 0; i < data_size; i++) {
        image_data_bytes[i]
          = (unsigned char)CLAMP(API->object->array_int_get_idx(image_data, i), 0, 255);
    }
    // store offset not pointer in case arena resizes
    command->mouse_cursor_image_offset = Arena::offsetOf(cq.write_q, image_data_bytes);
    command->width                     = width;
    command->height                    = height;
    command->xhot                      = xhot;
    command->yhot                      = yhot;
    command->type                      = SG_COMMAND_MOUSE_CURSOR;
    END_COMMAND();
}

void CQ_PushCommand_UI_Disabled(bool disabled)
{
    BEGIN_COMMAND(SG_Command_UI_Disabled, SG_COMMAND_UI_DISABLED);
    command->disabled = disabled;
    END_COMMAND();
}

void CQ_PushCommand_ComponentUpdateName(SG_Component* component)
{
    BEGIN_COMMAND_ADDITIONAL_MEMORY_ZERO(SG_Command_ComponentUpdateName,
                                         SG_COMMAND_COMPONENT_UPDATE_NAME,
                                         sizeof(component->name));

    // copy string
    strncpy((char*)memory, component->name, sizeof(component->name));
    command->sg_id       = component->id;
    command->name_offset = Arena::offsetOf(cq.write_q, memory);
    END_COMMAND();
}

void CQ_PushCommand_GG_Scene(SG_Scene* scene)
{
    BEGIN_COMMAND(SG_Command_GG_Scene, SG_COMMAND_GG_SCENE);
    command->sg_id = scene ? scene->id : 0;
    END_COMMAND();
}

void CQ_PushCommand_CreateTransform(SG_Transform* xform)
{
    BEGIN_COMMAND(SG_Command_CreateXform, SG_COMMAND_CREATE_XFORM);
    command->sg_id = xform->id;
    command->pos   = xform->pos;
    command->rot   = xform->rot;
    command->sca   = xform->sca;
    END_COMMAND();
}

void CQ_PushCommand_AddChild(SG_Transform* parent, SG_Transform* child)
{
    // execute change on audio thread side
    SG_Transform::addChild(parent, child);

    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_AddChild* command = ARENA_PUSH_TYPE(cq.write_q, SG_Command_AddChild);

        // initialize memory
        command->type              = SG_COMMAND_ADD_CHILD;
        command->nextCommandOffset = cq.write_q->curr;
        command->parent_id         = parent->id;
        command->child_id          = child->id;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_RemoveChild(SG_Transform* parent, SG_Transform* child)
{
    if (parent == NULL || child == NULL) return;

    // execute change on audio thread side
    SG_Transform::removeChild(parent, child);

    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_RemoveChild* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_RemoveChild);

        // initialize memory
        command->type              = SG_COMMAND_REMOVE_CHILD;
        command->nextCommandOffset = cq.write_q->curr;
        command->parent            = parent->id;
        command->child             = child->id;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_RemoveAllChildren(SG_Transform* parent)
{
    BEGIN_COMMAND(SG_Command_RemoveAllChildren, SG_COMMAND_REMOVE_ALL_CHILDREN);
    command->parent = parent->id;
    END_COMMAND();
}

void CQ_PushCommand_SetPosition(SG_Transform* xform)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_SetPosition* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_SetPosition);

        // initialize memory
        command->type              = SG_COMMAND_SET_POSITION;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = xform->id;
        command->pos               = xform->pos;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_SetRotation(SG_Transform* xform)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_SetRotation* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_SetRotation);

        // initialize memory
        command->type              = SG_COMMAND_SET_ROTATATION;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = xform->id;
        command->rot               = xform->rot;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_SetScale(SG_Transform* xform)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_SetScale* command = ARENA_PUSH_TYPE(cq.write_q, SG_Command_SetScale);

        // initialize memory
        command->type              = SG_COMMAND_SET_SCALE;
        command->nextCommandOffset = cq.write_q->curr;
        command->sg_id             = xform->id;
        command->sca               = xform->sca;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_SceneUpdate(SG_Scene* scene)
{
    BEGIN_COMMAND(SG_Command_SceneUpdate, SG_COMMAND_SCENE_UPDATE);
    command->sg_id = scene->id;
    command->desc  = scene->desc;
    END_COMMAND();
}

void CQ_PushCommand_GeometryCreate(SG_Geometry* geo)
{
    BEGIN_COMMAND(SG_Command_GeoCreate, SG_COMMAND_GEO_CREATE);
    command->sg_id = geo->id;
    END_COMMAND();
}

// copies data pointer into command arena. does NOT take ownership
// assumes data is 4bytes (i32 or f32)
// data_len is the number of elements in data array, NOT size in bytes
void CQ_PushCommand_GeometrySetVertexAttribute(SG_Geometry* geo, int location,
                                               int num_components, void* data,
                                               int data_size_bytes)
{
    if (data == NULL || data_size_bytes == 0 || num_components == 0) return;

    BEGIN_COMMAND_ADDITIONAL_MEMORY(SG_Command_GeoSetVertexAttribute,
                                    SG_COMMAND_GEO_SET_VERTEX_ATTRIBUTE,
                                    data_size_bytes);

    // get cq memory for vertex data
    f32* attribute_data = (f32*)memory;
    memcpy(attribute_data, data, data_size_bytes);

    command->sg_id           = geo->id;
    command->num_components  = num_components;
    command->location        = location;
    command->data_size_bytes = data_size_bytes;
    command->data_offset     = Arena::offsetOf(cq.write_q, attribute_data);

    ASSERT((data_size_bytes % 4) == 0);
    ASSERT((data_size_bytes / 4) % num_components == 0);

    END_COMMAND();
}

void CQ_PushCommand_GeometrySetIndices(SG_Geometry* geo, u32* indices, int index_count)
{
    BEGIN_COMMAND_ADDITIONAL_MEMORY(SG_Command_GeoSetIndices,
                                    SG_COMMAND_GEO_SET_INDICES,
                                    index_count * sizeof(*indices));

    u32* index_data = (u32*)memory;
    memcpy(index_data, indices, index_count * sizeof(*indices));

    command->sg_id          = geo->id;
    command->index_count    = index_count;
    command->indices_offset = Arena::offsetOf(cq.write_q, index_data);

    END_COMMAND();
}

void CQ_PushCommand_GeometrySetPulledVertexAttribute(SG_Geometry* geo, int location,
                                                     void* data, size_t bytes)
{
    BEGIN_COMMAND_ADDITIONAL_MEMORY(SG_Command_GeometrySetPulledVertexAttribute,
                                    SG_COMMAND_GEO_SET_PULLED_VERTEX_ATTRIBUTE, bytes);

    u8* attribute_data = (u8*)memory;
    if (bytes && data) {
        memcpy(attribute_data, data, bytes);
    }

    command->sg_id       = geo->id;
    command->location    = location;
    command->data_bytes  = bytes;
    command->data_offset = Arena::offsetOf(cq.write_q, attribute_data);
    END_COMMAND();
}

void CQ_PushCommand_GeometrySetVertexCount(SG_Geometry* geo, int count)
{
    BEGIN_COMMAND(SG_Command_GeometrySetVertexCount, SG_COMMAND_GEO_SET_VERTEX_COUNT);
    command->sg_id = geo->id;
    command->count = count;
    END_COMMAND();
}

void CQ_PushCommand_GeometrySetIndicesCount(SG_Geometry* geo, int count)
{
    BEGIN_COMMAND(SG_Command_GeometrySetIndicesCount, SG_COMMAND_GEO_SET_INDICES_COUNT);
    command->sg_id = geo->id;
    command->count = count;
    END_COMMAND();
}

// Textures ====================================================================

// maybe change to TextureUpdate + lazy creation to support mutable texture
// formats/usage/dimension
void CQ_PushCommand_TextureCreate(SG_Texture* texture)
{
    BEGIN_COMMAND(SG_Command_TextureCreate, SG_COMMAND_TEXTURE_CREATE);
    command->sg_id = texture->id;
    command->desc  = texture->desc;
    END_COMMAND();
}

void CQ_PushCommand_TextureWrite(SG_Texture* texture, SG_TextureWriteDesc* desc,
                                 Chuck_ArrayFloat* ck_array, CK_DL_API API)
{
    // calculate the necessary size in bytes from data (assume data length has already
    // been validated)
    int write_region_num_texels = desc->width * desc->height * desc->depth;
    int bytes_per_texel         = SG_Texture_byteSizePerTexel(texture->desc.format);
    int write_region_num_components
      = write_region_num_texels
        * SG_Texture_numComponentsPerTexel(texture->desc.format);
    int write_size_bytes = write_region_num_texels * bytes_per_texel;

    BEGIN_COMMAND_ADDITIONAL_MEMORY(SG_Command_TextureWrite, SG_COMMAND_TEXTURE_WRITE,
                                    write_size_bytes);

    command->sg_id           = texture->id;
    command->write_desc      = *desc;
    command->data_size_bytes = write_size_bytes;
    command->data_offset     = Arena::offsetOf(cq.write_q, memory);

    // copy texture data to write_q
#define CQ_TEXTURE_WRITE(type, type_max)                                               \
    ASSERT(write_region_num_components <= API->object->array_float_size(ck_array));    \
    type* pixel_data = (type*)memory;                                                  \
    for (int i = 0; i < write_region_num_components; i++) {                            \
        pixel_data[i]                                                                  \
          = (type)(type_max * API->object->array_float_get_idx(ck_array, i));          \
    }

    switch (texture->desc.format) {
        case WGPUTextureFormat_RGBA8Unorm: {
            CQ_TEXTURE_WRITE(u8, UINT8_MAX);
        } break;
        case WGPUTextureFormat_RGBA16Float: {
            ASSERT(false); // not impl
        } break;
        case WGPUTextureFormat_R32Float:
        case WGPUTextureFormat_RGBA32Float: {
            CQ_TEXTURE_WRITE(f32, 1.0f);
        } break;
        default: ASSERT(false);
    }

    END_COMMAND();
#undef CQ_TEXTURE_WRITE
}

void CQ_PushCommand_TextureFromFile(SG_Texture* texture, const char* filepath,
                                    SG_TextureLoadDesc* desc)
{
    size_t filepath_len = strlen(filepath);
    BEGIN_COMMAND_ADDITIONAL_MEMORY_ZERO(
      SG_Command_TextureFromFile, SG_COMMAND_TEXTURE_FROM_FILE, filepath_len + 1);
    command->sg_id      = texture->id;
    char* filepath_copy = (char*)memory;
    strncpy(filepath_copy, filepath, strlen(filepath));
    command->filepath_offset = Arena::offsetOf(cq.write_q, filepath_copy);
    command->flip_vertically = desc->flip_y;
    command->gen_mips        = desc->gen_mips;
    END_COMMAND();
}

// Shader ======================================================================

void CQ_PushCommand_ShaderCreate(SG_Shader* shader)
{
    size_t vertex_filepath_len
      = shader->vertex_filepath_owned ? strlen(shader->vertex_filepath_owned) + 1 : 1;
    size_t fragment_filepath_len = shader->fragment_filepath_owned ?
                                     strlen(shader->fragment_filepath_owned) + 1 :
                                     1;
    size_t vertex_string_len
      = shader->vertex_string_owned ? strlen(shader->vertex_string_owned) + 1 : 1;
    size_t fragment_string_len
      = shader->fragment_string_owned ? strlen(shader->fragment_string_owned) + 1 : 1;

    size_t compute_string_len
      = shader->compute_string_owned ? strlen(shader->compute_string_owned) + 1 : 1;
    size_t compute_filepath_len
      = shader->compute_filepath_owned ? strlen(shader->compute_filepath_owned) + 1 : 1;

    size_t additional_memory = vertex_filepath_len + fragment_filepath_len
                               + vertex_string_len + fragment_string_len
                               + compute_string_len + compute_filepath_len;

    BEGIN_COMMAND_ADDITIONAL_MEMORY_ZERO(SG_Command_ShaderCreate,
                                         SG_COMMAND_SHADER_CREATE, additional_memory);

    command->sg_id = shader->id;

    char* vertex_filepath   = (char*)memory;
    char* fragment_filepath = vertex_filepath + vertex_filepath_len;
    char* vertex_string     = fragment_filepath + fragment_filepath_len;
    char* fragment_string   = vertex_string + vertex_string_len;
    char* compute_string    = fragment_string + fragment_string_len;
    char* compute_filepath  = compute_string + compute_string_len;

    // copy strings (leaving space for null terminators)
    strncpy(vertex_filepath, shader->vertex_filepath_owned, vertex_filepath_len - 1);
    strncpy(fragment_filepath, shader->fragment_filepath_owned,
            fragment_filepath_len - 1);
    strncpy(vertex_string, shader->vertex_string_owned, vertex_string_len - 1);
    strncpy(fragment_string, shader->fragment_string_owned, fragment_string_len - 1);
    strncpy(compute_string, shader->compute_string_owned, compute_string_len - 1);
    strncpy(compute_filepath, shader->compute_filepath_owned, compute_filepath_len - 1);

    // set offsets
    command->vertex_filepath_offset   = Arena::offsetOf(cq.write_q, vertex_filepath);
    command->fragment_filepath_offset = Arena::offsetOf(cq.write_q, fragment_filepath);
    command->vertex_string_offset     = Arena::offsetOf(cq.write_q, vertex_string);
    command->fragment_string_offset   = Arena::offsetOf(cq.write_q, fragment_string);
    command->compute_string_offset    = Arena::offsetOf(cq.write_q, compute_string);
    command->compute_filepath_offset  = Arena::offsetOf(cq.write_q, compute_filepath);

    ASSERT(sizeof(shader->vertex_layout) == sizeof(command->vertex_layout));
    memcpy(command->vertex_layout, shader->vertex_layout,
           sizeof(shader->vertex_layout));

    command->lit = shader->lit;

    END_COMMAND();
}

void CQ_PushCommand_MaterialCreate(SG_Material* material)
{
    BEGIN_COMMAND(SG_Command_MaterialCreate, SG_COMMAND_MATERIAL_CREATE);
    command->material_type = material->material_type; // TODO remove?
    command->sg_id         = material->id;
    command->pso           = material->pso;
    // command->params        = material->params;
    END_COMMAND();
}

void CQ_PushCommand_MaterialUpdatePSO(SG_Material* material)
{
    BEGIN_COMMAND(SG_Command_MaterialUpdatePSO, SG_COMMAND_MATERIAL_UPDATE_PSO);
    command->sg_id = material->id;
    command->pso   = material->pso;
    END_COMMAND();
}

void CQ_PushCommand_MaterialSetUniform(SG_Material* material, int location)
{
    BEGIN_COMMAND(SG_Command_MaterialSetUniform, SG_COMMAND_MATERIAL_SET_UNIFORM);
    command->sg_id    = material->id;
    command->uniform  = material->uniforms[location];
    command->location = location;
    END_COMMAND();
}

void CQ_PushCommand_MaterialSetStorageBuffer(SG_Material* material, int location,
                                             Chuck_ArrayFloat* ck_arr)
{
    int data_count = g_chuglAPI->object->array_float_size(ck_arr);
    BEGIN_COMMAND_ADDITIONAL_MEMORY(SG_Command_MaterialSetStorageBuffer,
                                    SG_COMMAND_MATERIAL_SET_STORAGE_BUFFER,
                                    data_count * sizeof(f32));
    command->sg_id           = material->id;
    command->location        = location;
    command->data_size_bytes = data_count * sizeof(f32);
    f32* data                = (f32*)memory;
    chugin_copyCkFloatArray(ck_arr, data, data_count);
    command->data_offset = Arena::offsetOf(cq.write_q, data);
    END_COMMAND();
}

void CQ_PushCommand_MeshUpdate(SG_Mesh* mesh)
{
    BEGIN_COMMAND(SG_Command_MeshUpdate, SG_COMMAND_MESH_UPDATE);
    command->mesh_id = mesh->id;
    command->geo_id  = mesh->_geo_id;
    command->mat_id  = mesh->_mat_id;
    END_COMMAND();
}

void CQ_PushCommand_CameraCreate(SG_Camera* camera)
{
    BEGIN_COMMAND(SG_Command_CameraCreate, SG_COMMAND_CAMERA_CREATE);
    command->camera = *camera;
    END_COMMAND();
}

void CQ_PushCommand_CameraSetParams(SG_Camera* cam)
{
    BEGIN_COMMAND(SG_Command_CameraSetParams, SG_COMMAND_CAMERA_SET_PARAMS);
    command->camera_id = cam->id;
    command->params    = cam->params;
    END_COMMAND();
}

void CQ_PushCommand_TextRebuild(SG_Text* text)
{
    size_t additional_bytes = text->text.length() + 1 + text->font_path.length() + 1;
    ASSERT(text->_mat_id != 0);

    BEGIN_COMMAND_ADDITIONAL_MEMORY_ZERO(SG_Command_TextRebuild,
                                         SG_COMMAND_TEXT_REBUILD, additional_bytes);
    command->text_id          = text->id;
    command->material_id      = text->_mat_id;
    command->control_point    = { text->control_points.x, text->control_points.y };
    command->vertical_spacing = text->vertical_spacing;

    char* text_copy = (char*)memory;
    char* font_path = text_copy + text->text.length() + 1;

    // copy strings
    strncpy(text_copy, text->text.c_str(), text->text.length());
    strncpy(font_path, text->font_path.c_str(), text->font_path.length());

    command->text_str_offset      = Arena::offsetOf(cq.write_q, text_copy);
    command->font_path_str_offset = Arena::offsetOf(cq.write_q, font_path);
    END_COMMAND();
}

void CQ_PushCommand_TextDefaultFont(const char* font_path)
{
    size_t additional_bytes = font_path ? strlen(font_path) : 1;
    BEGIN_COMMAND_ADDITIONAL_MEMORY_ZERO(
      SG_Command_TextDefaultFont, SG_COMMAND_TEXT_DEFAULT_FONT, additional_bytes);
    if (font_path) strncpy((char*)memory, font_path, additional_bytes - 1);
    command->font_path_str_offset = Arena::offsetOf(cq.write_q, memory);
    END_COMMAND();
}

void CQ_PushCommand_PassCreate(SG_Pass* pass)
{
    BEGIN_COMMAND(SG_Command_PassCreate, SG_COMMAND_PASS_CREATE);
    command->pass_id   = pass->id;
    command->pass_type = pass->pass_type;
    END_COMMAND();
}

void CQ_PushCommand_PassUpdate(SG_Pass* pass)
{
    BEGIN_COMMAND(SG_Command_PassUpdate, SG_COMMAND_PASS_UPDATE);
    memcpy(&command->pass, pass, sizeof(*pass));
    END_COMMAND();
}

void CQ_PushCommand_PassConnect(SG_Pass* pass, SG_Pass* next_pass)
{
    BEGIN_COMMAND(SG_Command_PassConnect, SG_COMMAND_PASS_CONNECT);
    command->pass_id      = pass ? pass->id : 0;
    command->next_pass_id = next_pass ? next_pass->id : 0;
    END_COMMAND();
}

void CQ_PushCommand_PassDisconnect(SG_Pass* pass, SG_Pass* next_pass)
{
    BEGIN_COMMAND(SG_Command_PassConnect, SG_COMMAND_PASS_DISCONNECT);
    command->pass_id      = pass ? pass->id : 0;
    command->next_pass_id = next_pass ? next_pass->id : 0;
    END_COMMAND();
}

void CQ_PushCommand_b2World_Set(u32 world_id)
{
    spinlock::lock(&cq.write_q_lock);
    {
        // allocate memory
        SG_Command_b2World_Set* command
          = ARENA_PUSH_TYPE(cq.write_q, SG_Command_b2World_Set);

        // initialize memory
        command->type              = SG_COMMAND_b2_WORLD_SET;
        command->nextCommandOffset = cq.write_q->curr;
        command->b2_world_id       = world_id;
    }
    spinlock::unlock(&cq.write_q_lock);
}

void CQ_PushCommand_b2SubstepCount(u32 substep_count)
{
    BEGIN_COMMAND(SG_Command_b2_SubstepCount, SG_COMMAND_b2_SUBSTEP_COUNT);
    command->substep_count = substep_count;
    END_COMMAND();
}

void CQ_PushCommand_BufferUpdate(SG_Buffer* buffer)
{
    BEGIN_COMMAND(SG_Command_BufferUpdate, SG_COMMAND_BUFFER_UPDATE);
    command->buffer_id = buffer->id;
    command->desc      = buffer->desc;
    END_COMMAND();
}

void CQ_PushCommand_BufferWrite(SG_Buffer* buffer, Chuck_ArrayFloat* data,
                                u64 offset_bytes)
{
    int data_count       = g_chuglAPI->object->array_float_size(data);
    int additional_bytes = data_count * sizeof(f32);
    BEGIN_COMMAND_ADDITIONAL_MEMORY(SG_Command_BufferWrite, SG_COMMAND_BUFFER_WRITE,
                                    additional_bytes);
    command->buffer_id       = buffer->id;
    command->offset_bytes    = offset_bytes;
    command->data_size_bytes = additional_bytes;
    f32* data_ptr            = (f32*)memory;
    chugin_copyCkFloatArray(data, data_ptr, data_count);
    command->data_offset = Arena::offsetOf(cq.write_q, data_ptr);
    END_COMMAND();
}

void CQ_PushCommand_LightUpdate(SG_Light* light)
{
    BEGIN_COMMAND(SG_Command_LightUpdate, SG_COMMAND_LIGHT_UPDATE);
    command->light_id = light->id;
    command->desc     = light->desc;
    END_COMMAND();
}
