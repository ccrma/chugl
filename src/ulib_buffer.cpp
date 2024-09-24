#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "shaders.h"

#define GET_BUFFER(obj) (SG_GetBuffer(OBJ_MEMBER_UINT(obj, component_offset_id)))

CK_DLL_CTOR(storage_buffer_ctor);
CK_DLL_MFUN(storage_buffer_set_size);
CK_DLL_MFUN(storage_buffer_write);
CK_DLL_MFUN(storage_buffer_write_with_offset);
// CK_DLL_MFUN(storage_buffer_write_int); // TODO

// Invariant: buffer usage flags are immutable. Must be set at creation

void ulib_buffer_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS("StorageBuffer", SG_CKNames[SG_COMPONENT_BASE]);

    CTOR(storage_buffer_ctor);

    MFUN(storage_buffer_set_size, "void", "size");
    ARG("int", "size");
    DOC_FUNC(
      "Sets size in 4-byte units of the buffer. Does NOT copy data from old buffer to "
      "new one if buffer is resized. Buffer size in bytes will be 4 * size");

    MFUN(storage_buffer_write, "void", "write");
    ARG("float[]", "data");
    DOC_FUNC(
      "Writes data to the start of buffer. Floats from the chuck data array are "
      "converted into 4-byte f32s. Will increase buffer size if needed. Will "
      "not decrease buffer size if data is smaller than buffer size.");

    MFUN(storage_buffer_write_with_offset, "void", "write");
    ARG("float[]", "data");
    ARG("int", "offset_bytes");
    DOC_FUNC(
      "Writes data to the buffer at the specified offset in floatas. Floats from the "
      "chuck array "
      "are converted into 4-byte f32s. Fails if buffer size is too small.");

    END_CLASS();
}

CK_DLL_CTOR(storage_buffer_ctor)
{
    SG_Buffer* buff                            = SG_CreateBuffer(SELF);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = buff->id;

    // for now only support storage buffers
    // in future may add other buffer usages
    buff->desc.usage = WGPUBufferUsage_Storage;

    CQ_PushCommand_BufferUpdate(buff);
}

CK_DLL_MFUN(storage_buffer_set_size)
{
    SG_Buffer* buff = GET_BUFFER(SELF);
    buff->desc.size = GET_NEXT_INT(ARGS) * sizeof(float);
    CQ_PushCommand_BufferUpdate(buff);
}

CK_DLL_MFUN(storage_buffer_write)
{
    SG_Buffer* buff        = GET_BUFFER(SELF);
    Chuck_ArrayFloat* data = GET_NEXT_FLOAT_ARRAY(ARGS);

    // update buffer size if needed
    int data_len    = API->object->array_float_size(data);
    buff->desc.size = MAX(buff->desc.size, data_len * sizeof(float));

    // note: *not* saving data on audio-thread cpu side

    CQ_PushCommand_BufferWrite(buff, data, 0);
}

CK_DLL_MFUN(storage_buffer_write_with_offset)
{
    SG_Buffer* buff        = GET_BUFFER(SELF);
    Chuck_ArrayFloat* data = GET_NEXT_FLOAT_ARRAY(ARGS);
    t_CKUINT offset        = GET_NEXT_INT(ARGS);

    int data_len = API->object->array_float_size(data);
    if (data_len * sizeof(float) + offset > buff->desc.size) {
        CK_THROW("BufferWriteError",
                 "BufferWriteError: offset + data size exceeds buffer size.", SHRED);
    }

    // note: *not* saving data on audio-thread cpu side

    CQ_PushCommand_BufferWrite(buff, data, offset);
}