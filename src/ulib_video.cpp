#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "core/log.h"

#include <pl/pl_mpeg.h>

#define GET_VIDEO(ckobj) SG_GetVideo(OBJ_MEMBER_UINT(ckobj, component_offset_id))

CK_DLL_CTOR(video_ctor_with_path);

CK_DLL_MFUN(video_get_texture_rgba);
CK_DLL_MFUN(video_get_width_texels);
CK_DLL_MFUN(video_get_height_texels);

void ulib_video_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_VIDEO], "Object");

    CTOR(video_ctor_with_path);
    ARG("string", "path");

    MFUN(video_get_texture_rgba, SG_CKNames[SG_COMPONENT_TEXTURE], "texture");
    DOC_FUNC("Get the RGBA texture of the video.");

    MFUN(video_get_width_texels, "int", "width");
    DOC_FUNC("Get the width of the video in texels.");

    MFUN(video_get_height_texels, "int", "height");
    DOC_FUNC("Get the height of the video in texels.");

    END_CLASS();
}

CK_DLL_CTOR(video_ctor_with_path)
{
    const char* path = chugin_copyCkString(GET_NEXT_STRING(ARGS));

    plm_t* plm = NULL;
    defer(if (plm) plm_destroy(plm));
    SG_Texture* video_texture_rgba = SG_GetTexture(g_builtin_textures.magenta_pixel_id);

    // Initialize plmpeg, load the video file, install decode callbacks
    {
        plm = plm_create_with_filename(path);
        if (!plm) {
            log_warn("Could not open MPG video '%s'", path);
            log_warn(" |- Defaulting to magenta texture");
        }

        // probe first 5MB of file for video or audio streams
        if (!plm_probe(plm, 5000 * 1024)) {
            plm_destroy(plm);
            log_warn("No MPEG video or audio streams found in %s", path);
            plm = NULL;
        }

        // log video metadata (TODO store on SG_Video)
        log_info("Opened %s - framerate: %f, samplerate: %d, duration: %f", path,
                 plm_get_framerate(plm), plm_get_samplerate(plm),
                 plm_get_duration(plm));
    }

    // create the rgb video texture (TODO support YcbCr later)
    if (plm) {
        SG_TextureDesc desc = {};
        desc.width          = plm_get_width(plm);
        desc.height         = plm_get_height(plm);
        desc.dimension      = WGPUTextureDimension_2D;
        desc.format         = WGPUTextureFormat_RGBA8Unorm;
        desc.usage          = WGPUTextureUsage_All; // TODO: restrict usage?
        desc.mips           = 1;                    // no mipmaps for video

        video_texture_rgba = SG_CreateTexture(&desc, NULL, SHRED, true);
    }

    SG_Video* video              = SG_CreateVideo(SELF);
    video->path_OWNED            = path;
    video->video_texture_rgba_id = video_texture_rgba->id;

    CQ_PushCommand_VideoUpdate(video);
}

CK_DLL_MFUN(video_get_texture_rgba)
{
    SG_Video* video  = GET_VIDEO(SELF);
    RETURN->v_object = SG_GetTexture(video->video_texture_rgba_id)->ckobj;
}

CK_DLL_MFUN(video_get_width_texels)
{
    SG_Video* video = GET_VIDEO(SELF);
    SG_Texture* tex = SG_GetTexture(video->video_texture_rgba_id);
    RETURN->v_uint  = tex->desc.width;
}

CK_DLL_MFUN(video_get_height_texels)
{
    SG_Video* video = GET_VIDEO(SELF);
    SG_Texture* tex = SG_GetTexture(video->video_texture_rgba_id);
    RETURN->v_uint  = tex->desc.height;
}