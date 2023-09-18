#include "Texture.h"
#include "Util.h"
#define STB_IMAGE_IMPLEMENTATION
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

Texture::Texture(const std::string& path)
	: m_RendererID(0), m_FilePath(path), m_LocalBuffer(nullptr),
	m_Width(0), m_Height(0), m_BPP(0)
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
	ASSERT(m_LocalBuffer);

	// generate the texture
	GLCall(glGenTextures(1, &m_RendererID));
	Bind();

	// copy texture data to GPU
	glTexImage2D(
		GL_TEXTURE_2D,
		0, // mipmap LOD (for manually creating mipmaps)
		GL_RGBA8,  // format we want to store texture
		m_Width, m_Height,  // texture dims
		0,  // legacy border width, always set 0
		GL_RGBA,  // format of texture on CPU
		GL_UNSIGNED_BYTE,  // size of each channel on CPU
		m_LocalBuffer  // texture buffer
	);

	// generate mipmaps
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	// free local image data
	stbi_image_free(m_LocalBuffer);

	// set texture filtering (REQUIRED by openGL)
	// note LINEAR = (BI)LINEAR filtering
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// set texture clamping (REQUIRED)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);  // x axis
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);  // y axis
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);


	Unbind();
}

Texture::Texture(int texWidth, int texHeight, int bytesPerPixel, unsigned char * texBuffer)
	: m_RendererID(0), m_FilePath(""), m_LocalBuffer(texBuffer),
	m_Width(texWidth), m_Height(texHeight), m_BPP(bytesPerPixel)
{
	// generate the texture
	GLCall(glGenTextures(1, &m_RendererID));
	Bind();

	// copy texture data to GPU
	glTexImage2D(
		GL_TEXTURE_2D,
		0, // mipmap LOD (for manually creating mipmaps)
		GL_RGBA8,  // format we want to store texture
		texWidth, texHeight,  // texture dims
		0,  // legacy border width, always set 0
		GL_RGBA,  // format of texture on CPU
		GL_UNSIGNED_BYTE,  // size of each channel on CPU
		texBuffer  // texture buffer
	);

	// generate mipmaps
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	// Texture params
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	Unbind();
}

Texture::~Texture()
{
	GLCall(glDeleteTextures(1, &m_RendererID));
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
