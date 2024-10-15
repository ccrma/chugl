#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "core/log.h"

#include <pl/pl_mpeg.h>

#define GET_VIDEO(ckobj) SG_GetVideo(OBJ_MEMBER_UINT(ckobj, component_offset_id))

CK_DLL_CTOR(video_ctor_with_path);
CK_DLL_TICKF(video_tick_multichannel);

// video metadata
CK_DLL_MFUN(video_get_framerate);
CK_DLL_MFUN(video_get_samplerate);
CK_DLL_MFUN(video_get_duration_secs);

CK_DLL_MFUN(video_get_texture_rgba);
CK_DLL_MFUN(video_get_width_texels);
CK_DLL_MFUN(video_get_height_texels);

// video playback
CK_DLL_MFUN(video_get_time);

// manipulation
CK_DLL_MFUN(video_seek);
CK_DLL_MFUN(video_set_rate);

void ulib_video_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_VIDEO], "UGen_Multi");

    QUERY->add_ugen_funcf(QUERY, video_tick_multichannel, NULL,
                          0, // 0 channels in
                          2  // stereo out
    );

    CTOR(video_ctor_with_path);
    ARG("string", "path");

    MFUN(video_get_framerate, "float", "framerate");
    DOC_FUNC("Get the framerate of the video.");

    MFUN(video_get_samplerate, "int", "samplerate");
    DOC_FUNC("Get the samplerate of the video's audio stream");

    MFUN(video_get_duration_secs, "float", "duration");
    DOC_FUNC("Get the duration of the video in seconds.");

    MFUN(video_get_texture_rgba, SG_CKNames[SG_COMPONENT_TEXTURE], "texture");
    DOC_FUNC("Get the RGBA texture of the video.");

    MFUN(video_get_width_texels, "int", "width");
    DOC_FUNC("Get the width of the video in texels.");

    MFUN(video_get_height_texels, "int", "height");
    DOC_FUNC("Get the height of the video in texels.");

    MFUN(video_get_time, "time", "timestamp");
    DOC_FUNC("Get the current time in the video in seconds");

    MFUN(video_set_rate, "void", "rate");
    ARG("float", "rate");
    DOC_FUNC("Set the playback rate of the video. 1.0 is normal speed.");

    MFUN(video_seek, "void", "seek");
    ARG("time", "time");
    DOC_FUNC("Seek to a specific time in the video.");

    END_CLASS();
}

// (Chuck_Object* SELF, SAMPLE* in, SAMPLE* out, t_CKUINT nframes,CK_DL_API API)
CK_DLL_TICKF(video_tick_multichannel)
{
    static u64 audio_frame_count{ 0 };
    ASSERT(nframes == 1); // TODO ask Ge/Nick if this is ever not 1?

    SG_Video* video = GET_VIDEO(SELF);

    if (video->samples == NULL || video->audio_playhead >= video->samples->count) {
        video->samples        = plm_decode_audio(video->plm);
        video->audio_playhead = 0;
        log_info("decoded 1 audio frame with time %f, count %d", video->samples->time,
                 video->samples->count);
    }

    ASSERT(video->audio_playhead < video->samples->count);

    out[0] = video->samples->interleaved[video->audio_playhead * 2 + 0];
    out[1] = video->samples->interleaved[video->audio_playhead * 2 + 1];

    video->audio_playhead++;

    audio_frame_count++;

    return TRUE;
}

static void ulib_video_on_audio(plm_t* mpeg, plm_samples_t* samples, void* user)
{
    // plm_samples_t should be valid until the next audio callback

    SG_Video* video = SG_GetVideo((intptr_t)user);

    log_error("decoding audio samples %d", samples->count);

    { // update the audio samples
        video->samples        = samples;
        video->audio_playhead = 0;
    }
}

CK_DLL_CTOR(video_ctor_with_path)
{
    const char* path = chugin_copyCkString(GET_NEXT_STRING(ARGS));

    plm_t* plm = NULL;

    // TODO destroy plm in destructor

    SG_Texture* video_texture_rgba = SG_GetTexture(g_builtin_textures.magenta_pixel_id);
    SG_Video* video                = SG_CreateVideo(SELF);

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
    }

    if (plm) {
        // save video metadata
        video->framerate     = (float)plm_get_framerate(plm);
        video->samplerate    = plm_get_samplerate(plm);
        video->duration_secs = (float)plm_get_duration(plm);
        log_info("Opened %s - framerate: %f, samplerate: %d, duration: %f", path,
                 video->framerate, video->samplerate, video->duration_secs);
    }

    // initialize audio
    if (plm && plm_get_num_audio_streams(plm) > 0) {
        plm_set_audio_decode_callback(plm, ulib_video_on_audio,
                                      (void*)(intptr_t)video->id);

        // TODO make configurable
        plm_set_loop(plm, TRUE);
        plm_set_audio_enabled(plm, TRUE);
        plm_set_video_enabled(plm, TRUE);
        plm_set_audio_stream(plm, 0);

        // Adjust the audio lead time according to the audio_spec buffer size
        // TODO hardcoded to 512 samples, change to bufsize when chugin headers adds
        // it
        plm_set_audio_lead_time(plm, (double)512 / (double)API->vm->srate(VM));

        // warn if sample rate doesn't match
        if (plm_get_samplerate(plm) != API->vm->srate(VM)) {
            log_warn(
              "Video %s: Audio stream samplerate for (%d) does not match VM samplerate "
              "(%d)",
              path, plm_get_samplerate(plm), API->vm->srate(VM));
        }
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

    video->plm                   = plm;
    video->path_OWNED            = path;
    video->video_texture_rgba_id = video_texture_rgba->id;

    CQ_PushCommand_VideoUpdate(video);
}

CK_DLL_MFUN(video_get_framerate)
{
    RETURN->v_float = GET_VIDEO(SELF)->framerate;
}

CK_DLL_MFUN(video_get_samplerate)
{
    RETURN->v_int = GET_VIDEO(SELF)->samplerate;
}

CK_DLL_MFUN(video_get_duration_secs)
{
    RETURN->v_float = GET_VIDEO(SELF)->duration_secs;
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

CK_DLL_MFUN(video_get_time)
{
    plm_t* plm     = GET_VIDEO(SELF)->plm;
    RETURN->v_time = plm_get_time(plm) * API->vm->srate(VM);
}

CK_DLL_MFUN(video_set_rate)
{
    SG_Video* video = GET_VIDEO(SELF);
    video->rate     = GET_NEXT_FLOAT(ARGS);

    CQ_PushCommand_VideoRate(video->id, video->rate);
}

CK_DLL_MFUN(video_seek)
{
    SG_Video* video = GET_VIDEO(SELF);

    // get time and wrap around video length (allows negative indexing)
    int time_samples = (int)GET_NEXT_TIME(ARGS);
    while (time_samples < 0) {
        time_samples += SG_Video::audioFrames(video);
    }
    time_samples = time_samples % SG_Video::audioFrames(video);

    double time_seconds = (float)time_samples / API->vm->srate(VM);

    log_error("seeking to %f seconds from %f seconds", time_seconds,
              plm_get_time(video->plm));

    // int ret = plm_seek(video->plm, time_seconds, false);
    double out_seek_time = 0;
    int ret              = plm_seek_audio(video->plm, time_seconds, &out_seek_time);
    ASSERT(ret);

    CQ_PushCommand_VideoSeek(video->id, out_seek_time);
}