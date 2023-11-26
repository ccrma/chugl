#include "ulib_texture.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "renderer/scenegraph/CGL_Texture.h"

//-----------------------------------------------------------------------------
// Texture API Declarations
//-----------------------------------------------------------------------------

CK_DLL_CTOR(cgl_texture_ctor);
CK_DLL_DTOR(cgl_texture_dtor);

// sampler wrap mode
CK_DLL_MFUN(cgl_texture_set_wrap);
CK_DLL_MFUN(cgl_texture_get_wrap_s);
CK_DLL_MFUN(cgl_texture_get_wrap_t);

// sampler filter mode
CK_DLL_MFUN(cgl_texture_set_filter);
CK_DLL_MFUN(cgl_texture_get_filter_min);
CK_DLL_MFUN(cgl_texture_get_filter_mag);

// color space
CK_DLL_MFUN(chugl_texture_set_colorspace);
CK_DLL_MFUN(chugl_texture_get_colorspace);

// Texture --> FileTexture (texture from filepath .png .jpg etc) ==============
CK_DLL_CTOR(cgl_texture_file_ctor);
CK_DLL_MFUN(cgl_texture_file_set_filepath);
CK_DLL_MFUN(cgl_texture_file_get_filepath);

// Texture --> DataTexture (texture from chuck array) ========================
CK_DLL_CTOR(cgl_texture_rawdata_ctor);
CK_DLL_MFUN(cgl_texture_rawdata_set_data);
// CK_DLL_MFUN(cgl_texture_rawdata_get_data);

// Texture --> CubeMapTexture (texture from 6 filepaths) =====================
CK_DLL_CTOR(cgl_texture_cubemap_ctor);
CK_DLL_MFUN(cgl_texture_cubemap_set_filepaths);
// CK_DLL_MFUN(cgl_texture_cubemap_get_filepaths);  // TODO: impl once we can create arrays from chugin
// TODO: add option to set each face individually, from file or from data



//-----------------------------------------------------------------------------
// Texture API Definition
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_texture(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, CGL_Texture::CKName(CGL_TextureType::Base), "Object");
    QUERY->doc_class(QUERY, "Base texture class, do not instantiate directly");

	QUERY->add_ctor(QUERY, cgl_texture_ctor);
	QUERY->add_dtor(QUERY, cgl_texture_dtor);
	CGL::SetTextureDataOffset(QUERY->add_mvar(QUERY, "int", "@texture_data", false));

	// texture options (static constants) ---------------------------------
	QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", TRUE, (void *)&CGL_Texture::Repeat);
    QUERY->doc_var(QUERY, "When passed into Texture.wrap(), sets the texture to repeat for UVs outside of [0,1]");
	QUERY->add_svar(QUERY, "int", "WRAP_MIRRORED", TRUE, (void *)&CGL_Texture::MirroredRepeat);
    QUERY->doc_var(QUERY, "When passed into Texture.wrap(), sets the texture to repeat and mirror for UVs outside of [0,1]");
	QUERY->add_svar(QUERY, "int", "WRAP_CLAMP", TRUE, (void *)&CGL_Texture::ClampToEdge);
    QUERY->doc_var(QUERY, "When passed into Texture.wrap(), sets the texture to clamp to the border pixel color for UVs outside of [0,1]");

	// not exposing mipmap filter options for simplicity
	QUERY->add_svar(QUERY, "int", "FILTER_NEAREST", TRUE, (void *)&CGL_Texture::Nearest);
    QUERY->doc_var(QUERY, "When passed into Texture.filter(), sets texture sampler to use nearest-neighbor filtering");
	QUERY->add_svar(QUERY, "int", "FILTER_LINEAR", TRUE, (void *)&CGL_Texture::Linear);
    QUERY->doc_var(QUERY, "When passed into Texture.filter(), sets texture sampler to use bilinear filtering");

	// colorspace options
	QUERY->add_svar(QUERY, "int", "COLOR_SPACE_LINEAR", TRUE, (void *)&CGL_Texture::ColorSpace_Linear);
	QUERY->doc_var(QUERY, 
		"Specifices the texture data to be in linear colorspace."
		"Textures used for lighting parameters (like specular or normal maps) are typically in this linear space"
		"Pass into Texture.colorSpace()"
	);
	QUERY->add_svar(QUERY, "int", "COLOR_SPACE_SRGB", TRUE, (void *)&CGL_Texture::ColorSpace_sRGB);
	QUERY->doc_var(QUERY, 
		"Specifices the colorspace of the texture data to be in gamma-corrected sRGB colorspace."
		"Textures used for color data (like diffuse maps) are typically in this colorspace"
		"Pass into Texture.colorSpace()"
	);

	// member fns -----------------------------------------------------------
	QUERY->add_mfun(QUERY, cgl_texture_set_wrap, "void", "wrap");
	QUERY->add_arg(QUERY, "int", "s");
	QUERY->add_arg(QUERY, "int", "t");
    QUERY->doc_func(QUERY, "Set texture wrap modes along s and t dimensions");

	QUERY->add_mfun(QUERY, cgl_texture_get_wrap_s, "int", "wrapS");
    QUERY->doc_func(QUERY, "Set texture wrap modes along s dimensions");
	QUERY->add_mfun(QUERY, cgl_texture_get_wrap_t, "int", "wrapT");
    QUERY->doc_func(QUERY, "Set texture wrap modes along t dimensions");

	QUERY->add_mfun(QUERY, cgl_texture_set_filter, "void", "filter");
	QUERY->add_arg(QUERY, "int", "min");
	QUERY->add_arg(QUERY, "int", "mag");
    QUERY->doc_func(QUERY, "Set texture sampler min and mag filter modes Texture.FILTER_NEAREST or Texture.FILTER_LINEAR");

	QUERY->add_mfun(QUERY, cgl_texture_get_filter_min, "int", "filterMin");
    QUERY->doc_func(QUERY, "Set texture sampler minification filter. Default FILTER_LINEAR");
	QUERY->add_mfun(QUERY, cgl_texture_get_filter_mag, "int", "filterMag");
    QUERY->doc_func(QUERY, "Set texture sampler magnification filter. Default FILTER_LINEAR");

	QUERY->add_mfun(QUERY, chugl_texture_set_colorspace, "int", "colorSpace");
	QUERY->add_arg(QUERY, "int", "colorSpace");
	QUERY->doc_func(QUERY, "Set the colorspace of the texture data. Default COLOR_SPACE_LINEAR");

	QUERY->add_mfun(QUERY, chugl_texture_get_colorspace, "int", "colorSpace");
	QUERY->doc_func(QUERY, "Get the colorspace of the texture data. Default COLOR_SPACE_LINEAR");

	QUERY->end_class(QUERY);

	// FileTexture -----------------------------------------------------------
	QUERY->begin_class(QUERY, CGL_Texture::CKName(CGL_TextureType::File2D), CGL_Texture::CKName(CGL_TextureType::Base));
    QUERY->doc_class(QUERY, "Class for loading textures from external files");
    QUERY->add_ex(QUERY, "textures/textures-1.ck");
    QUERY->add_ex(QUERY, "textures/snowstorm.ck");

	QUERY->add_ctor(QUERY, cgl_texture_file_ctor);

	QUERY->add_mfun(QUERY, cgl_texture_file_set_filepath, "string", "path");
	QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "loads texture data from path");

	QUERY->add_mfun(QUERY, cgl_texture_file_get_filepath, "string", "path");
    QUERY->doc_func(QUERY, "Get the filepath for the currently-loaded texture");

	QUERY->end_class(QUERY);

	// DataTexture -----------------------------------------------------------
	QUERY->begin_class(QUERY, CGL_Texture::CKName(CGL_TextureType::RawData2D), CGL_Texture::CKName(CGL_TextureType::Base));
    QUERY->doc_class(QUERY, "Class for dynamically creating textures from chuck arrays");
    QUERY->add_ex(QUERY, "audioshader/audio-texture.ck");

	QUERY->add_ctor(QUERY, cgl_texture_rawdata_ctor);

	QUERY->add_mfun(QUERY, cgl_texture_rawdata_set_data, "void", "data");
	QUERY->add_arg(QUERY, "float[]", "data");
	QUERY->add_arg(QUERY, "int", "width");
	QUERY->add_arg(QUERY, "int", "height");
    QUERY->doc_func(QUERY, 
		"Set the data for this texture. Data is expected to be a float array of length width*height*4, "
		"where each pixel is represented by 4 floats for r,g,b,a."
		"Currently only supports unsigned bytes, so each float must be in range [0,255]"
	);
	QUERY->end_class(QUERY);

	// CubeMapTexture -----------------------------------------------------------
	QUERY->begin_class(QUERY, CGL_Texture::CKName(CGL_TextureType::CubeMap), CGL_Texture::CKName(CGL_TextureType::Base));
	QUERY->doc_class(QUERY, 
		"Class for loading cubemap textures from external files or chuck arrays."
	);
	// QUERY->add_ex(QUERY, "textures/cubemap.ck");

	QUERY->add_ctor(QUERY, cgl_texture_cubemap_ctor);

	QUERY->add_mfun(QUERY, cgl_texture_cubemap_set_filepaths, "void", "paths");
	QUERY->add_arg(QUERY, "string", "right");
	QUERY->add_arg(QUERY, "string", "left");
	QUERY->add_arg(QUERY, "string", "top");
	QUERY->add_arg(QUERY, "string", "bottom");
	QUERY->add_arg(QUERY, "string", "front");
	QUERY->add_arg(QUERY, "string", "back");
	QUERY->doc_func(QUERY, 
		"Pass in paths to the 6 faces of the cubemap"
		" in the order: right, left, top, bottom, front, back"
	);

	QUERY->end_class(QUERY);

	return true;
}

// CGL_Texture API impl =====================================================
CK_DLL_CTOR(cgl_texture_ctor)
{
	// abstract base texture class, do nothing
	// chuck DLL will call all constructors in QUERY inheritance chain
}

CK_DLL_DTOR(cgl_texture_dtor)
{
	CGL::PushCommand(new DestroySceneGraphNodeCommand(SELF, CGL::GetTextureDataOffset(), &CGL::mainScene));
}

CK_DLL_MFUN(cgl_texture_set_wrap)
{
	CGL_Texture *texture = CGL::GetTexture(SELF);
	auto s = static_cast<CGL_TextureWrapMode>(GET_NEXT_INT(ARGS));
	auto t = static_cast<CGL_TextureWrapMode>(GET_NEXT_INT(ARGS));
	texture->SetWrapMode(s, t);

	CGL::PushCommand(new UpdateTextureSamplerCommand(texture));
}

CK_DLL_MFUN(cgl_texture_get_wrap_s)
{
	CGL_Texture *texture = CGL::GetTexture(SELF);
	RETURN->v_int = static_cast<t_CKINT>(texture->GetSamplerParams().wrapS);
}

CK_DLL_MFUN(cgl_texture_get_wrap_t)
{
	CGL_Texture *texture = CGL::GetTexture(SELF);
	RETURN->v_int = static_cast<t_CKINT>(texture->GetSamplerParams().wrapS);
}

CK_DLL_MFUN(cgl_texture_set_filter)
{
	CGL_Texture *texture = CGL::GetTexture(SELF);
	auto min = static_cast<CGL_TextureFilterMode>(GET_NEXT_INT(ARGS));
	auto mag = static_cast<CGL_TextureFilterMode>(GET_NEXT_INT(ARGS));
	texture->SetFilterMode(min, mag);

	CGL::PushCommand(new UpdateTextureSamplerCommand(texture));
}

CK_DLL_MFUN(cgl_texture_get_filter_min)
{
	CGL_Texture *texture = CGL::GetTexture(SELF);
	RETURN->v_int = static_cast<t_CKINT>(texture->GetSamplerParams().filterMin);
}

CK_DLL_MFUN(cgl_texture_get_filter_mag)
{
	CGL_Texture *texture = CGL::GetTexture(SELF);
	RETURN->v_int = static_cast<t_CKINT>(texture->GetSamplerParams().filterMag);
}

CK_DLL_MFUN(chugl_texture_set_colorspace)
{
	CGL_Texture *texture = CGL::GetTexture(SELF);
	t_CKINT colorspace = GET_NEXT_INT(ARGS);

	RETURN->v_int = colorspace;

	CGL::PushCommand(new UpdateTextureColorSpaceCommand(
		texture, 
		static_cast<CGL_TextureColorSpace>(colorspace)
	));
}

CK_DLL_MFUN(chugl_texture_get_colorspace)
{
	CGL_Texture *texture = CGL::GetTexture(SELF);
	RETURN->v_int = static_cast<t_CKINT>(texture->GetColorSpace());
}

// FileTexture API impl =====================================================
CK_DLL_CTOR(cgl_texture_file_ctor)
{
	CGL::PushCommand(
        new CreateSceneGraphNodeCommand(
            new FileTexture2D,
            &CGL::mainScene, SELF, CGL::GetTextureDataOffset()
        )
    );
}

CK_DLL_MFUN(cgl_texture_file_set_filepath)
{
	auto* texture = dynamic_cast<FileTexture2D*>(CGL::GetTexture(SELF));
	Chuck_String *path = GET_NEXT_STRING(ARGS);

	CGL::PushCommand(new UpdateTexturePathCommand(texture, path->str()));

	RETURN->v_string = path;
}

CK_DLL_MFUN(cgl_texture_file_get_filepath)
{
	auto* texture = dynamic_cast<FileTexture2D*>(CGL::GetTexture(SELF));
	RETURN->v_string = (Chuck_String *)API->object->create_string(VM, texture->GetFilePath().c_str(), false);
}

// DataTexture API impl =====================================================
CK_DLL_CTOR(cgl_texture_rawdata_ctor)
{
	CGL::PushCommand(
        new CreateSceneGraphNodeCommand(
            new DataTexture2D,
            &CGL::mainScene, SELF, CGL::GetTextureDataOffset()
        )
    );
}

CK_DLL_MFUN(cgl_texture_rawdata_set_data)
{
	auto* texture = dynamic_cast<DataTexture2D*>(CGL::GetTexture(SELF));
	Chuck_ArrayFloat *data = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);
	t_CKINT width = GET_NEXT_INT(ARGS);
	t_CKINT height = GET_NEXT_INT(ARGS);

	CGL::PushCommand(new UpdateTextureDataCommand(texture, data->m_vector, width, height));
}

// CubeMapTexture impl =====================================================
CK_DLL_CTOR(cgl_texture_cubemap_ctor)
{
	CGL::PushCommand(
		new CreateSceneGraphNodeCommand(
			new CGL_CubeMap,
			&CGL::mainScene, SELF, CGL::GetTextureDataOffset()
		)
	);
}

CK_DLL_MFUN(cgl_texture_cubemap_set_filepaths)
{
	CGL_CubeMap* cubeMap = dynamic_cast<CGL_CubeMap*>(CGL::GetTexture(SELF));
	Chuck_String* right = GET_NEXT_STRING(ARGS);
	Chuck_String* left = GET_NEXT_STRING(ARGS);
	Chuck_String* top = GET_NEXT_STRING(ARGS);
	Chuck_String* bottom = GET_NEXT_STRING(ARGS);
	Chuck_String* front = GET_NEXT_STRING(ARGS);
	Chuck_String* back = GET_NEXT_STRING(ARGS);

	CGL::PushCommand(new UpdateCubeMapPathsCommand(
		cubeMap, 
		{right->str(), left->str(), top->str(), bottom->str(), front->str(), back->str()}
	));
}