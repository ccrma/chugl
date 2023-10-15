#include "Texture.h"
#include "Util.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb/stb_image.h"

#include "glad/glad.h"

#include <iostream>

/*
* Abtraction layer for 2D textures
* TODO: eventually support other texture types (1D, 3D, etc)
*/

// default textures ======================================================
unsigned char whitePixel[] = { 255, 255, 255, 255 };
unsigned char blackPixel[] = { 0, 0, 0, 255 };
Texture* Texture::DefaultWhiteTexture{ nullptr };
Texture* Texture::DefaultBlackTexture{ nullptr };
// Texture Texture::DefaultWhiteTexture(1, 1, 4, &whitePixel[0]);
// Texture Texture::DefaultBlackTexture(1, 1, 4, &blackPixel[0]);

// methods ===============================================================

// build texture from filepath
Texture::Texture(const std::string& path)
	: m_RendererID(0), m_FilePath(path), m_LocalBuffer(nullptr),
	m_Width(0), m_Height(0), m_BPP(0), m_CGL_Texture(nullptr)
{
	// generate the texture
	GLCall(glGenTextures(1, &m_RendererID));
	Bind();

	GenTextureFromPath(path);

	// generate mipmaps
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	// set texture filtering (REQUIRED by openGL)
	// note LINEAR = (BI)LINEAR filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// set texture clamping (REQUIRED)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // x axis
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  // y axis

	Unbind();
}

// build texture from raw data buffer
Texture::Texture(int texWidth, int texHeight, int bytesPerPixel, unsigned char * texBuffer)
	: m_RendererID(0), m_FilePath(""), m_LocalBuffer(texBuffer),
	m_Width(texWidth), m_Height(texHeight), m_BPP(bytesPerPixel),
	m_CGL_Texture(nullptr)
{
	// generate the texture
	GLCall(glGenTextures(1, &m_RendererID));
	Bind();

	// copy texture data to GPU
	glTexImage2D(
		GL_TEXTURE_2D,
		0, // mipmap LOD (for manually creating mipmaps)
		GL_RGBA8,  // format we want to store texture (TODO: should be based off bytes per pixel...right now hardcoded to 4)
		texWidth, texHeight,  // texture dims
		0,  // legacy border width, always set 0
		GL_RGBA,  // format of texture on CPU
		GL_UNSIGNED_BYTE,  // size of each channel on CPU
		texBuffer  // texture buffer
	);

	// generate mipmaps
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	// DON'T free m_LocalBuffer because it's a pointer to an external buffer?

	// Texture params
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	Unbind();
}

Texture::Texture(CGL_Texture *cglTexture)
	: m_RendererID(0), m_FilePath(cglTexture->m_FilePath), m_LocalBuffer(nullptr),
	m_Width(cglTexture->m_Width), m_Height(cglTexture->m_Height), m_BPP(0),
	m_CGL_Texture(cglTexture)
{
	// generate the texture
	GLCall(glGenTextures(1, &m_RendererID));
	Bind();

	// if (cglTexture->m_FilePath == "" && cglTexture->m_ImgBuffer == nullptr)
	// 	return;
	
	// no constructors in chuck for now, this is for when they are eventually added
	// switch on texture type
	switch (cglTexture->type)
	{
		case CGL_TextureType::Base:
			throw std::runtime_error("trying to init abstract base CGL_Texture");
		case CGL_TextureType::File2D:
			GenTextureFromPath(cglTexture->m_FilePath);
			break;
		case CGL_TextureType::RawData:
			GenTextureFromBuffer(
				cglTexture->m_Width, cglTexture->m_Height,
				cglTexture->m_DataBuffer.data()
			);
			break;
		default:
			throw std::runtime_error("Texture type undefined");
	}

	SetSamplerParams(cglTexture->m_SamplerParams);
	Unbind();
}

Texture::~Texture()
{
	// don't need to free m_localBuffer, already freed by stbi_image_free after loading
	// don't need to free m_CGL_Texture, already freed by deletion command
	GLCall(glDeleteTextures(1, &m_RendererID));
}

// update GPU texture data from changes to CGL_texture. Assumes already bound!
void Texture::Update()
{
	if (!m_CGL_Texture) return;

	if (!m_CGL_Texture->NeedsUpdate()) return;

	if (m_CGL_Texture->HasNewFilePath())
		GenTextureFromPath(m_CGL_Texture->m_FilePath);

	if (m_CGL_Texture->HasNewRawData()) {
		if (m_CGL_Texture->HasNewDimensions()) {
			// new dimensions, need to recreate from scratch
			GenTextureFromBuffer(
				m_CGL_Texture->m_Width, m_CGL_Texture->m_Height,
				m_CGL_Texture->m_DataBuffer.data()
			);
		} else {
			// same dimensions, just copy in place
			glTexSubImage2D(
				GL_TEXTURE_2D,
				0, // mipmap level
				0, 0,  // offset
				m_CGL_Texture->m_Width, m_CGL_Texture->m_Height,  // texture dims
				GL_RGBA,  // format we want to store texture (TODO: should be based off bytes per pixel...right now hardcoded to 4)
				GL_UNSIGNED_BYTE,  // size of each channel on CPU
				m_CGL_Texture->m_DataBuffer.data()// texture buffer
			);
		}
	}

	// always update sampler params last, because generating mipmaps must happen AFTER image data is loaded 
	if (m_CGL_Texture->HasNewSampler())
		SetSamplerParams(m_CGL_Texture->m_SamplerParams);

	m_CGL_Texture->ResetUpdateFlags();
}

void Texture::Bind(unsigned int slot) const
{
	// Util::println("activating texture slot " + std::to_string(slot));
	// activate texture unit first (GL_TEXTURE0 is default active unit)
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));

	// bind to the active texture unit
	GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
}

void Texture::Unbind()
{
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

Texture *Texture::GetDefaultWhiteTexture()
{
	if (!DefaultWhiteTexture) {
		DefaultWhiteTexture = new Texture(1, 1, 4, &whitePixel[0]);
	}
	return DefaultWhiteTexture;
}

Texture *Texture::GetDefaultBlackTexture()
{
	if (!DefaultBlackTexture) {
		DefaultBlackTexture = new Texture(1, 1, 4, &blackPixel[0]);
	}
	return DefaultBlackTexture;
}

void Texture::GenTextureFromPath(const std::string &path)
{
	// flip so first pixel in output array is on bottom left.
	// this is the format expected by openGL
	stbi_set_flip_vertically_on_load(1);

	// load texture to CPU
	m_LocalBuffer = stbi_load(
		path.c_str(),
		&m_Width,  // texture width
		&m_Height, // texture height
		&m_BPP,  // bits per pixel of channel data
		4		 // desired number of output channels. 4 for rgba
	);
	
	// make sure texture loaded
	if (!m_LocalBuffer) {
		std::cerr << "ERROR: failed to load texture at path " << path << std::endl;
		std::cerr << "STBI ERROR REASON:" << stbi_failure_reason() << std::endl;
		throw std::runtime_error("failed to load texture");
	}

	// copy texture data to GPU
	glTexImage2D(
		GL_TEXTURE_2D,// TODO: only supports 2D textures for now
		0, // mipmap LOD (for manually creating mipmaps)
		GL_RGBA8,  // format we want to store texture
		m_Width, m_Height,  // texture dims
		0,  // legacy border width, always set 0
		GL_RGBA,  // format of texture on CPU     -- TODO: accept RGB format no alpha?
		GL_UNSIGNED_BYTE,  // size of each channel on CPU
		m_LocalBuffer  // texture buffer
	);


	// free local image data
	stbi_image_free(m_LocalBuffer);
}

void Texture::GenTextureFromBuffer(int texWidth, int texHeight, unsigned char *texBuffer)
{
	glTexImage2D(
		GL_TEXTURE_2D,
		0, 
		GL_RGBA8,  // format we want to store texture (TODO: should be based off bytes per pixel...right now hardcoded to 4)
		texWidth, texHeight,  // texture dims
		0,  // legacy border width, always set 0
		GL_RGBA,  // format of texture on CPU
		GL_UNSIGNED_BYTE,  // size of each channel on CPU
		texBuffer
	);
}

void Texture::SetSamplerParams(const CGL_TextureSamplerParams &params)
{
	// generate mipmaps
	if (params.genMipMaps) GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	// wrap mode
	SetWrapMode(GL_TEXTURE_WRAP_S, params.wrapS);
	SetWrapMode(GL_TEXTURE_WRAP_T, params.wrapT);

	// filter mode
	SetFilterMode(GL_TEXTURE_MIN_FILTER, params.filterMin, params.genMipMaps);
	SetFilterMode(GL_TEXTURE_MAG_FILTER, params.filterMag, params.genMipMaps);
}

void Texture::SetWrapMode(unsigned int axis, CGL_TextureWrapMode mode)
{
	switch (mode) {
		case CGL_TextureWrapMode::Repeat:
			glTexParameteri(GL_TEXTURE_2D, axis, GL_REPEAT);
			break;
		case CGL_TextureWrapMode::MirroredRepeat:
			glTexParameteri(GL_TEXTURE_2D, axis, GL_MIRRORED_REPEAT);
			break;
		case CGL_TextureWrapMode::ClampToEdge:
			glTexParameteri(GL_TEXTURE_2D, axis, GL_CLAMP_TO_EDGE);
			break;
		default:
			throw std::runtime_error("invalid wrap mode");
	}
}

void Texture::SetFilterMode(unsigned int op, CGL_TextureFilterMode mode, bool enableMipMaps)
{
	if (op == GL_TEXTURE_MAG_FILTER) {
		switch (mode) {
			case CGL_TextureFilterMode::Linear:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
			case CGL_TextureFilterMode::Nearest:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;
			default:
				throw std::runtime_error("invalid mag filter mode");
		}
	} else if (op == GL_TEXTURE_MIN_FILTER) {
		if (enableMipMaps) {
			switch(mode) {
				case CGL_TextureFilterMode::Linear:
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
					break;
				case CGL_TextureFilterMode::Nearest:
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
					break;
				default:
					throw std::runtime_error("invalid mipmap min filter mode");
			}
		} else {
			switch(mode) {
				case CGL_TextureFilterMode::Nearest:
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
					break;
				case CGL_TextureFilterMode::Linear:
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					break;
				default:
					throw std::runtime_error("invalid min filter mode");
			}
		}
	} else {
		throw std::runtime_error("invalid filter mode");
	}
}
