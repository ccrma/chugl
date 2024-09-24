#include <chuck/chugin.h>

#include "sg_command.h"
#include "sg_component.h"

#include "ulib_helper.h"

#include "core/log.h"

#include <stb/stb_image.h>

#if 0
enum WGPUTextureUsage {
    WGPUTextureUsage_CopySrc = 0x00000001,
    WGPUTextureUsage_CopyDst = 0x00000002,
    WGPUTextureUsage_TextureBinding = 0x00000004,
    WGPUTextureUsage_StorageBinding = 0x00000008,
    WGPUTextureUsage_RenderAttachment = 0x00000010,
}

enum WGPUTextureDimension {
    WGPUTextureDimension_1D = 0x00000000,
    WGPUTextureDimension_2D = 0x00000001,
    WGPUTextureDimension_3D = 0x00000002,
}

struct WGPUExtent3D {
    uint32_t width;
    uint32_t height;
    uint32_t depthOrArrayLayers;
}

struct WGPUTextureFormat {
    WGPUTextureFormat_RGBA8Unorm = 0x00000012,
    WGPUTextureFormat_RGBA16Float = 0x00000021,
    WGPUTextureFormat_Depth24PlusStencil8 = 0x00000028,
}

struct WGPUTextureDescriptor {
    WGPUTextureUsageFlags usage;    // 
    WGPUTextureDimension dimension; //  
    WGPUExtent3D size;
    WGPUTextureFormat format;
    // uint32_t mipLevelCount; // always gen mips
    // uint32_t sampleCount;   // don't expose for now
}

typedef struct WGPUOrigin3D {
    uint32_t x;
    uint32_t y;
    uint32_t z;
}

enum WGPUTextureAspect {
    WGPUTextureAspect_All = 0x00000000,
    WGPUTextureAspect_StencilOnly = 0x00000001,
    WGPUTextureAspect_DepthOnly = 0x00000002,
    WGPUTextureAspect_Force32 = 0x7FFFFFFF
}

struct WGPUImageCopyTexture {
    WGPUTexture texture;
    uint32_t mipLevel;
    WGPUOrigin3D origin;
    WGPUTextureAspect aspect; // default to Aspect_All for non-depth textures
}

struct WGPUTextureDataLayout {
    uint64_t offset; // offset into CPU-pointer void* data
    uint32_t bytesPerRow; // minimum value for bytesPerRow that is set to 256 by the API.
    uint32_t rowsPerImage; // required if there are multiple images (3D or 2D array textures)
}

void wgpuQueueWriteTexture(
    WGPUQueue queue, 
    WGPUImageCopyTexture* destination, 
    void* data, 
    size_t dataSize, 
    WGPUTextureDataLayout* dataLayout,  // layout of cpu-side void* data
    WGPUExtent3D* writeSize             // size of destination region in texture
);
#endif

#define GET_TEXTURE(ckobj) SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id))
void ulib_texture_createDefaults(CK_DL_API API);

// TextureSampler ---------------------------------------------------------------------
CK_DLL_CTOR(sampler_ctor);

// TextureDesc -----------------------------------------------------------------

static t_CKUINT texture_desc_format_offset    = 0;
static t_CKUINT texture_desc_dimension_offset = 0;
static t_CKUINT texture_desc_width_offset     = 0;
static t_CKUINT texture_desc_height_offset    = 0;
static t_CKUINT texture_desc_depth_offset     = 0;
static t_CKUINT texture_desc_usage_offset     = 0;
// static t_CKUINT texture_desc_samples_offset   = 0; // not exposing for now
static t_CKUINT texture_desc_mips_offset = 0; // not exposing for now
CK_DLL_CTOR(texture_desc_ctor);

// TextureWriteDesc -----------------------------------------------------------------
CK_DLL_CTOR(texture_write_desc_ctor);
// dst image location
static t_CKUINT texture_write_desc_mip_offset      = 0;
static t_CKUINT texture_write_desc_offset_x_offset = 0;
static t_CKUINT texture_write_desc_offset_y_offset = 0;
static t_CKUINT texture_write_desc_offset_z_offset = 0;
// dst region size
static t_CKUINT texture_write_desc_width_offset  = 0;
static t_CKUINT texture_write_desc_height_offset = 0;
static t_CKUINT texture_write_desc_depth_offset  = 0;

// TextureWriteDesc -----------------------------------------------------------------
CK_DLL_CTOR(texture_load_desc_ctor);

static t_CKUINT texture_load_desc_flip_y_offset   = 0;
static t_CKUINT texture_load_desc_gen_mips_offset = 0;

// Texture ---------------------------------------------------------------------
CK_DLL_CTOR(texture_ctor);
CK_DLL_CTOR(texture_ctor_with_desc);

CK_DLL_MFUN(texture_get_format);
CK_DLL_MFUN(texture_get_dimension);
CK_DLL_MFUN(texture_get_width);
CK_DLL_MFUN(texture_get_height);
CK_DLL_MFUN(texture_get_depth);
CK_DLL_MFUN(texture_get_usage);
CK_DLL_MFUN(texture_get_mips);

CK_DLL_MFUN(texture_write);
CK_DLL_MFUN(texture_write_with_desc);

CK_DLL_SFUN(texture_load_2d_file);
CK_DLL_SFUN(texture_load_2d_file_with_params); // not exposed yet (figure out hdr
// first)

static void ulib_texture_query(Chuck_DL_Query* QUERY)
{
    { // Sampler (only passed by value)
        QUERY->begin_class(QUERY, "TextureSampler", "Object");

        // static vars
        static t_CKINT WRAP_REPEAT    = SG_SAMPLER_WRAP_REPEAT;
        static t_CKINT WRAP_MIRROR    = SG_SAMPLER_WRAP_MIRROR_REPEAT;
        static t_CKINT WRAP_CLAMP     = SG_SAMPLER_WRAP_CLAMP_TO_EDGE;
        static t_CKINT FILTER_NEAREST = SG_SAMPLER_FILTER_NEAREST;
        static t_CKINT FILTER_LINEAR  = SG_SAMPLER_FILTER_LINEAR;
        QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", true, &WRAP_REPEAT);
        QUERY->add_svar(QUERY, "int", "WRAP_MIRROR", true, &WRAP_MIRROR);
        QUERY->add_svar(QUERY, "int", "WRAP_CLAMP", true, &WRAP_CLAMP);
        QUERY->add_svar(QUERY, "int", "FILTER_NEAREST", true, &FILTER_NEAREST);
        QUERY->add_svar(QUERY, "int", "FILTER_LINEAR", true, &FILTER_LINEAR);

        // member vars
        sampler_offset_wrapU     = QUERY->add_mvar(QUERY, "int", "wrapU", false);
        sampler_offset_wrapV     = QUERY->add_mvar(QUERY, "int", "wrapV", false);
        sampler_offset_wrapW     = QUERY->add_mvar(QUERY, "int", "wrapW", false);
        sampler_offset_filterMin = QUERY->add_mvar(QUERY, "int", "filterMin", false);
        sampler_offset_filterMag = QUERY->add_mvar(QUERY, "int", "filterMag", false);
        sampler_offset_filterMip = QUERY->add_mvar(QUERY, "int", "filterMip", false);

        // constructor
        QUERY->add_ctor(QUERY, sampler_ctor); // default constructor

        QUERY->end_class(QUERY); // Sampler
    }

    { // TextureDesc
        BEGIN_CLASS("TextureDesc", "Object");
        DOC_CLASS("Texture Descriptor -- options for creating a texture");

        CTOR(texture_desc_ctor);

        // member vars
        texture_desc_format_offset    = MVAR("int", "format", false);
        texture_desc_dimension_offset = MVAR("int", "dimension", false);
        texture_desc_width_offset     = MVAR("int", "width", false);
        texture_desc_height_offset    = MVAR("int", "height", false);
        texture_desc_depth_offset     = MVAR("int", "depth", false);
        texture_desc_usage_offset     = MVAR("int", "usage", false);
        // texture_desc_samples_offset   = MVAR("int", "samples");
        texture_desc_mips_offset = MVAR("int", "mips", false);

        END_CLASS();
    } // end TextureDesc

    { // TextureWriteDesc
        BEGIN_CLASS("TextureWriteDesc", "Object");
        DOC_CLASS("Options for writing to a texture");

        CTOR(texture_write_desc_ctor);

        texture_write_desc_mip_offset = MVAR("int", "mip", false);
        DOC_VAR("Which mip level to write to. Default is 0 (base level)");

        texture_write_desc_offset_x_offset = MVAR("int", "x", false);
        DOC_VAR("X offset of write region. Default 0");

        texture_write_desc_offset_y_offset = MVAR("int", "y", false);
        DOC_VAR("Y offset of write region. Default 0");

        texture_write_desc_offset_z_offset = MVAR("int", "z", false);
        DOC_VAR("Z offset of write region. Default 0");

        texture_write_desc_width_offset = MVAR("int", "width", false);
        DOC_VAR("Width of write region. Default 0");

        texture_write_desc_height_offset = MVAR("int", "height", false);
        DOC_VAR("Height of write region. Default 0");

        texture_write_desc_depth_offset = MVAR("int", "depth", false);
        DOC_VAR("Depth of write region. Default 0");

        END_CLASS();
    };

    { // TextureLoadDesc
        BEGIN_CLASS("TextureLoadDesc", "Object");
        DOC_CLASS("Options for loading a texture from a file");

        CTOR(texture_load_desc_ctor);

        texture_load_desc_flip_y_offset = MVAR("int", "flip_y", false);
        DOC_VAR("Flip the image vertically before loading. Default false");

        texture_load_desc_gen_mips_offset = MVAR("int", "gen_mips", false);
        DOC_VAR("Generate mipmaps for the texture. Default true");

        END_CLASS();
    }

    // Texture
    {
        BEGIN_CLASS(SG_CKNames[SG_COMPONENT_TEXTURE], SG_CKNames[SG_COMPONENT_BASE]);

        // svars ---------------
        static t_CKINT texture_usage_copy_src        = WGPUTextureUsage_CopySrc;
        static t_CKINT texture_usage_copy_dst        = WGPUTextureUsage_CopyDst;
        static t_CKINT texture_usage_texture_binding = WGPUTextureUsage_TextureBinding;
        static t_CKINT texture_usage_storage_binding = WGPUTextureUsage_StorageBinding;
        static t_CKINT texture_usage_render_attachment
          = WGPUTextureUsage_RenderAttachment;
        static t_CKINT texture_usage_all = WGPUTextureUsage_All;
        SVAR("int", "Usage_CopySrc", &texture_usage_copy_src);
        SVAR("int", "Usage_CopyDst", &texture_usage_copy_dst);
        SVAR("int", "Usage_TextureBinding", &texture_usage_texture_binding);
        SVAR("int", "Usage_StorageBinding", &texture_usage_storage_binding);
        SVAR("int", "Usage_RenderAttachment", &texture_usage_render_attachment);
        SVAR("int", "Usage_All", &texture_usage_all);

        // 1D textures currently unsupported
        // static t_CKINT texture_dimension_1d = WGPUTextureDimension_1D;
        static t_CKINT texture_dimension_2d = WGPUTextureDimension_2D;
        // 3D textures currently unsupported
        // static t_CKINT texture_dimension_3d = WGPUTextureDimension_3D;
        // SVAR("int", "Dimension_1D", &texture_dimension_1d);
        SVAR("int", "Dimension_2D", &texture_dimension_2d);
        // SVAR("int", "Dimension_3D", &texture_dimension_3d);

        static t_CKINT texture_format_rgba8unorm  = WGPUTextureFormat_RGBA8Unorm;
        static t_CKINT texture_format_rgba16float = WGPUTextureFormat_RGBA16Float;
        static t_CKINT texture_format_rgba32float = WGPUTextureFormat_RGBA32Float;
        static t_CKINT texture_format_r32float    = WGPUTextureFormat_R32Float;
        // static t_CKINT texture_format_depth24plusstencil8
        //   = WGPUTextureFormat_Depth24PlusStencil8;
        SVAR("int", "Format_RGBA8Unorm", &texture_format_rgba8unorm);
        SVAR("int", "Format_RGBA16Float", &texture_format_rgba16float); // not
        // supported currently
        SVAR("int", "Format_RGBA32Float", &texture_format_rgba32float);
        SVAR("int", "Format_R32Float", &texture_format_r32float);
        // SVAR("int", "Format_Depth24PlusStencil8",
        // &texture_format_depth24plusstencil8);

        // sfun ------------------------------------------------------------------

        SFUN(texture_load_2d_file, SG_CKNames[SG_COMPONENT_TEXTURE], "load");
        ARG("string", "filepath");
        DOC_FUNC("Load a 2D texture from a file");

        SFUN(texture_load_2d_file_with_params, SG_CKNames[SG_COMPONENT_TEXTURE],
             "load");
        ARG("string", "filepath");
        ARG("TextureLoadDesc", "load_desc");
        DOC_FUNC("Load a 2D texture from a file with additional parameters");

        // mfun ------------------------------------------------------------------

        CTOR(texture_ctor);

        CTOR(texture_ctor_with_desc);
        ARG("TextureDesc", "texture_desc");

        MFUN(texture_write, "void", "write");
        ARG("float[]", "pixel_data");
        DOC_FUNC(
          "Convenience function for writing into a texture. Assumes pixel_data is "
          "being written into the texture origin (0,0,0) with a region equal to the "
          "full texture dimensions (width, height, depth) at mip level 0");

        MFUN(texture_write_with_desc, "void", "write");
        ARG("float[]", "pixel_data");
        ARG("TextureWriteDesc", "write_desc");
        DOC_FUNC(
          "Write pixel data to an arbitrary texture region. The input float data is "
          "automatically converted based on the texture format");

        MFUN(texture_get_format, "int", "format");
        DOC_FUNC(
          "Get the texture format (immutable). Returns a value from the "
          "Texture.Format_XXXXX enum, e.g. Texture.Format_RGBA8Unorm");

        MFUN(texture_get_dimension, "int", "dimension");
        DOC_FUNC(
          "Get the texture dimension (immutable). Returns a value from the "
          "Texture.Dimension_XXXXX enum, e.g. Texture.Dimension_2D");

        MFUN(texture_get_width, "int", "width");
        DOC_FUNC("Get the texture width (immutable)");

        MFUN(texture_get_height, "int", "height");
        DOC_FUNC("Get the texture height (immutable)");

        MFUN(texture_get_depth, "int", "depth");
        DOC_FUNC(
          "Get the texture depth (immutable). For a 2D texture, depth corresponds to "
          "the number of array layers (e.g. depth=6 for a cubemap)");

        MFUN(texture_get_usage, "int", "usage");
        DOC_FUNC(
          "Get the texture usage flags (immutable). Returns a bitmask of usage flgas "
          "from the Texture.Usage_XXXXX enum e.g. Texture.Usage_TextureBinding | "
          "Texture.Usage_RenderAttachment. By default, textures are created with ALL "
          "usages enabled");

        MFUN(texture_get_mips, "int", "mips");
        DOC_FUNC(
          "Get the number of mip levels (immutable). Returns the number of mip levels "
          "in the texture.");

        // TODO: specify in WGPUImageCopyTexture where in texture to write to ?
        // e.g. texture.subData()

        END_CLASS();
    }

    ulib_texture_createDefaults(QUERY->ck_api(QUERY));
}

// TextureSampler ------------------------------------------------------------------

Chuck_Object* ulib_texture_ckobj_from_sampler(SG_Sampler sampler, bool add_ref,
                                              Chuck_VM_Shred* shred)
{
    CK_DL_API API = g_chuglAPI;

    Chuck_Object* ckobj = chugin_createCkObj("TextureSampler", add_ref, shred);

    OBJ_MEMBER_INT(ckobj, sampler_offset_wrapU)     = sampler.wrapU;
    OBJ_MEMBER_INT(ckobj, sampler_offset_wrapV)     = sampler.wrapV;
    OBJ_MEMBER_INT(ckobj, sampler_offset_wrapW)     = sampler.wrapW;
    OBJ_MEMBER_INT(ckobj, sampler_offset_filterMin) = sampler.filterMin;
    OBJ_MEMBER_INT(ckobj, sampler_offset_filterMag) = sampler.filterMag;
    OBJ_MEMBER_INT(ckobj, sampler_offset_filterMip) = sampler.filterMip;

    return ckobj;
}

CK_DLL_CTOR(sampler_ctor)
{
    // default to repeat wrapping and linear filtering
    OBJ_MEMBER_INT(SELF, sampler_offset_wrapU)     = SG_SAMPLER_WRAP_REPEAT;
    OBJ_MEMBER_INT(SELF, sampler_offset_wrapV)     = SG_SAMPLER_WRAP_REPEAT;
    OBJ_MEMBER_INT(SELF, sampler_offset_wrapW)     = SG_SAMPLER_WRAP_REPEAT;
    OBJ_MEMBER_INT(SELF, sampler_offset_filterMin) = SG_SAMPLER_FILTER_LINEAR;
    OBJ_MEMBER_INT(SELF, sampler_offset_filterMag) = SG_SAMPLER_FILTER_LINEAR;
    OBJ_MEMBER_INT(SELF, sampler_offset_filterMip) = SG_SAMPLER_FILTER_LINEAR;
}

// TextureDesc ---------------------------------------------------------------------

CK_DLL_CTOR(texture_desc_ctor)
{
    OBJ_MEMBER_INT(SELF, texture_desc_format_offset)    = WGPUTextureFormat_RGBA8Unorm;
    OBJ_MEMBER_INT(SELF, texture_desc_dimension_offset) = WGPUTextureDimension_2D;
    OBJ_MEMBER_INT(SELF, texture_desc_width_offset)     = 1;
    OBJ_MEMBER_INT(SELF, texture_desc_height_offset)    = 1;
    OBJ_MEMBER_INT(SELF, texture_desc_depth_offset)     = 1;
    OBJ_MEMBER_INT(SELF, texture_desc_usage_offset)     = WGPUTextureUsage_All;
    // OBJ_MEMBER_INT(SELF, texture_desc_samples_offset) = 1;
    OBJ_MEMBER_INT(SELF, texture_desc_mips_offset) = 0;
}

static SG_TextureDesc ulib_texture_textureDescFromCkobj(Chuck_Object* ckobj)
{
    CK_DL_API API = g_chuglAPI;

    SG_TextureDesc desc = {};
    desc.format = (WGPUTextureFormat)OBJ_MEMBER_INT(ckobj, texture_desc_format_offset);
    desc.dimension
      = (WGPUTextureDimension)OBJ_MEMBER_INT(ckobj, texture_desc_dimension_offset);
    desc.width  = OBJ_MEMBER_INT(ckobj, texture_desc_width_offset);
    desc.height = OBJ_MEMBER_INT(ckobj, texture_desc_height_offset);
    desc.depth  = OBJ_MEMBER_INT(ckobj, texture_desc_depth_offset);
    desc.usage  = OBJ_MEMBER_INT(ckobj, texture_desc_usage_offset);
    // desc.samples        = OBJ_MEMBER_INT(ckobj, texture_desc_samples_offset);
    desc.mips = OBJ_MEMBER_INT(ckobj, texture_desc_mips_offset);

    // validation happens at final layer SG_CreateTexture
    return desc;
}

// TextureWriteDesc -----------------------------------------------------------------

CK_DLL_CTOR(texture_write_desc_ctor)
{
    OBJ_MEMBER_INT(SELF, texture_write_desc_mip_offset)      = 0;
    OBJ_MEMBER_INT(SELF, texture_write_desc_offset_x_offset) = 0;
    OBJ_MEMBER_INT(SELF, texture_write_desc_offset_y_offset) = 0;
    OBJ_MEMBER_INT(SELF, texture_write_desc_offset_z_offset) = 0;
    OBJ_MEMBER_INT(SELF, texture_write_desc_width_offset)    = 1;
    OBJ_MEMBER_INT(SELF, texture_write_desc_height_offset)   = 1;
    OBJ_MEMBER_INT(SELF, texture_write_desc_depth_offset)    = 1;
}

static SG_TextureWriteDesc ulib_texture_textureWriteDescFromCkobj(Chuck_Object* ckobj)
{
    CK_DL_API API = g_chuglAPI;

    SG_TextureWriteDesc desc = {};
    desc.mip                 = OBJ_MEMBER_INT(ckobj, texture_write_desc_mip_offset);
    desc.offset_x = OBJ_MEMBER_INT(ckobj, texture_write_desc_offset_x_offset);
    desc.offset_y = OBJ_MEMBER_INT(ckobj, texture_write_desc_offset_y_offset);
    desc.offset_z = OBJ_MEMBER_INT(ckobj, texture_write_desc_offset_z_offset);
    desc.width    = OBJ_MEMBER_INT(ckobj, texture_write_desc_width_offset);
    desc.height   = OBJ_MEMBER_INT(ckobj, texture_write_desc_height_offset);
    desc.depth    = OBJ_MEMBER_INT(ckobj, texture_write_desc_depth_offset);

    // validation happens at final write

    return desc;
}

// TextureLoadDesc -----------------------------------------------------------------

CK_DLL_CTOR(texture_load_desc_ctor)
{
    OBJ_MEMBER_INT(SELF, texture_load_desc_flip_y_offset)   = false;
    OBJ_MEMBER_INT(SELF, texture_load_desc_gen_mips_offset) = true;
}

static SG_TextureLoadDesc ulib_texture_textureLoadDescFromCkobj(Chuck_Object* ckobj)
{
    CK_DL_API API = g_chuglAPI;

    SG_TextureLoadDesc desc = {};
    desc.flip_y             = OBJ_MEMBER_INT(ckobj, texture_load_desc_flip_y_offset);
    desc.gen_mips           = OBJ_MEMBER_INT(ckobj, texture_load_desc_gen_mips_offset);

    return desc;
}

// Texture -----------------------------------------------------------------

// create default pixel textures and samplers
void ulib_texture_createDefaults(CK_DL_API API)
{
    SG_TextureDesc texture_binding_desc = {};
    texture_binding_desc.usage
      = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst;

    SG_TextureWriteDesc texture_write_desc = {};

    // white pixel
    {
        SG_Texture* tex = SG_CreateTexture(&texture_binding_desc, NULL, NULL, true);
        // upload pixel data
        CQ_PushCommand_TextureWrite(tex, &texture_write_desc,
                                    g_builtin_ckobjs.white_pixel_data, API);
        // set global
        g_builtin_textures.white_pixel_id = tex->id;
    }
    //  default render texture (hdr)
    {
        SG_TextureDesc render_texture_desc = {};
        render_texture_desc.usage          = WGPUTextureUsage_RenderAttachment
                                    | WGPUTextureUsage_TextureBinding
                                    | WGPUTextureUsage_StorageBinding;
        render_texture_desc.format = WGPUTextureFormat_RGBA16Float;
        // set global
        g_builtin_textures.default_render_texture_id
          = SG_CreateTexture(&render_texture_desc, NULL, NULL, true)->id;
    }

    { // black pixel
        SG_Texture* tex = SG_CreateTexture(&texture_binding_desc, NULL, NULL, true);
        // upload pixel data
        CQ_PushCommand_TextureWrite(tex, &texture_write_desc,
                                    g_builtin_ckobjs.black_pixel_data, API);
        // set global
        g_builtin_textures.black_pixel_id = tex->id;
    }

    { // default normal map
        SG_Texture* tex = SG_CreateTexture(&texture_binding_desc, NULL, NULL, true);
        // upload pixel data
        CQ_PushCommand_TextureWrite(tex, &texture_write_desc,
                                    g_builtin_ckobjs.normal_pixel_data, API);
        // set global
        g_builtin_textures.normal_pixel_id = tex->id;
    }
}

CK_DLL_CTOR(texture_ctor)
{
    SG_TextureDesc desc = {};
    SG_CreateTexture(&desc, SELF, SHRED, false);
}

CK_DLL_CTOR(texture_ctor_with_desc)
{
    SG_TextureDesc desc = ulib_texture_textureDescFromCkobj(GET_NEXT_OBJECT(ARGS));
    SG_CreateTexture(&desc, SELF, SHRED, false);
}

CK_DLL_MFUN(texture_get_format)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.format;
}

CK_DLL_MFUN(texture_get_dimension)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.dimension;
}

CK_DLL_MFUN(texture_get_width)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.width;
}

CK_DLL_MFUN(texture_get_height)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.height;
}

CK_DLL_MFUN(texture_get_depth)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.depth;
}

CK_DLL_MFUN(texture_get_usage)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.usage;
}

CK_DLL_MFUN(texture_get_mips)
{
    RETURN->v_int = GET_TEXTURE(SELF)->desc.mips;
}

static void ulib_texture_write(SG_Texture* tex, Chuck_ArrayFloat* ck_arr,
                               SG_TextureWriteDesc* desc, Chuck_VM_Shred* SHRED)
{
    CK_DL_API API = g_chuglAPI;

    int num_texels   = tex->desc.width * tex->desc.height * tex->desc.depth;
    int expected_len = num_texels * SG_Texture_numComponentsPerTexel(tex->desc.format);

    { // validation
        char err_msg[256] = {};
        // check offset within image bounds
        if (desc->offset_x + desc->width > tex->desc.width
            || desc->offset_y + desc->height > tex->desc.height
            || desc->offset_z + desc->depth > tex->desc.depth) {
            snprintf(err_msg, sizeof(err_msg),
                     "Texture write region out of bounds. Texture dimensions [%d, %d, "
                     "%d]. Write offsets [%d, %d, %d]. Write region size [%d, %d, %d]",
                     tex->desc.width, tex->desc.height, tex->desc.depth, desc->offset_x,
                     desc->offset_y, desc->offset_z, desc->width, desc->height,
                     desc->depth);
            CK_THROW("TextureWriteOutOfBounds", err_msg, SHRED);
        }

        // check mip level valid
        if (desc->mip >= tex->desc.mips) {
            snprintf(err_msg, sizeof(err_msg),
                     "Invalid mip level. Texture has %d mips, but tried to "
                     "write to mip level %d",
                     tex->desc.mips, desc->mip);
            CK_THROW("TextureWriteInvalidMip", err_msg, SHRED);
        }

        // check ck_array
        int ck_arr_len = API->object->array_float_size(ck_arr);
        if (ck_arr_len < expected_len) {
            snprintf(
              err_msg, sizeof(err_msg),
              "Incorrect number of components in pixel data. Expected %d, got %d",
              expected_len, ck_arr_len);
            CK_THROW("TextureWriteInvalidPixelData", err_msg, SHRED);
        }
    }

    // convert ck array into byte buffer based on texture format
    CQ_PushCommand_TextureWrite(tex, desc, ck_arr, API);
}

CK_DLL_MFUN(texture_write)
{
    SG_Texture* tex          = GET_TEXTURE(SELF);
    SG_TextureWriteDesc desc = {};
    desc.width               = tex->desc.width;
    desc.height              = tex->desc.height;
    desc.depth               = tex->desc.depth;

    ulib_texture_write(tex, GET_NEXT_FLOAT_ARRAY(ARGS), &desc, SHRED);
}

CK_DLL_MFUN(texture_write_with_desc)
{
    SG_Texture* tex          = GET_TEXTURE(SELF);
    Chuck_ArrayFloat* ck_arr = GET_NEXT_FLOAT_ARRAY(ARGS);
    SG_TextureWriteDesc desc
      = ulib_texture_textureWriteDescFromCkobj(GET_NEXT_OBJECT(ARGS));

    ulib_texture_write(tex, ck_arr, &desc, SHRED);
}

SG_Texture* ulib_texture_load(const char* filepath, SG_TextureLoadDesc* load_desc,
                              Chuck_VM_Shred* shred)
{
    int width, height, num_components;
    if (!stbi_info(filepath, &width, &height, &num_components)) {
        log_error("Couldn't load texture file '%s'. Reason: %s", filepath,
                  stbi_failure_reason());
    }

    SG_TextureDesc desc = {};
    desc.width          = width;
    desc.height         = height;
    desc.dimension      = WGPUTextureDimension_2D;
    desc.format         = WGPUTextureFormat_RGBA8Unorm;
    desc.usage          = WGPUTextureUsage_All;

    SG_Texture* tex = SG_CreateTexture(&desc, NULL, shred, false);

    CQ_PushCommand_TextureFromFile(tex, filepath, load_desc);

    return tex;
}

CK_DLL_SFUN(texture_load_2d_file)
{
    SG_TextureLoadDesc load_desc = {};
    SG_Texture* tex
      = ulib_texture_load(API->object->str(GET_NEXT_STRING(ARGS)), &load_desc, SHRED);
    RETURN->v_object = tex ? tex->ckobj : NULL;
}

CK_DLL_SFUN(texture_load_2d_file_with_params)
{
    const char* filepath = API->object->str(GET_NEXT_STRING(ARGS));
    SG_TextureLoadDesc load_desc
      = ulib_texture_textureLoadDescFromCkobj(GET_NEXT_OBJECT(ARGS));

    SG_Texture* tex = ulib_texture_load(filepath, &load_desc, SHRED);

    RETURN->v_object = tex ? tex->ckobj : NULL;
}
