#include "Texture.h"
#include "Util.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb/stb_image.h>


/*
* Abtraction layer for 2D textures
* TODO: eventually support other texture types (1D, 3D, etc)
*/

// default textures ======================================================
unsigned char whitePixel[] = { 255, 255, 255, 255 };
unsigned char blackPixel[] = { 0, 0, 0, 255 };
unsigned char magentaPixel[] = { 255, 0, 255, 255 };
Texture2D* Texture2D::DefaultWhiteTexture { nullptr };
Texture2D* Texture2D::DefaultBlackTexture { nullptr };
Texture2D* Texture2D::DefaultMagentaTexture { nullptr };

// Texture Texture::DefaultWhiteTexture(1, 1, 4, &whitePixel[0]);
// Texture Texture::DefaultBlackTexture(1, 1, 4, &blackPixel[0]);

// ===============================================================
// Texture base class
// ===============================================================

Texture::~Texture()
{
	// don't need to free m_CGL_Texture, already freed by deletion command
	GLCall(glDeleteTextures(1, &m_RendererID));
}

Texture* Texture::CreateTexture(CGL_Texture* cglTexture)
{
	Texture* texture = nullptr;
	switch (cglTexture->GetTextureType())
	{
	case CGL_TextureType::File2D:
		texture = new Texture2D;
		break;
	case CGL_TextureType::RawData2D:
		texture = new Texture2D;
		break;
	case CGL_TextureType::CubeMap:
		texture = new CubeMapTexture;
		break;
	default:
		throw std::runtime_error("Invalid CGL_TextureType");
	}
	texture->Load(cglTexture);
	return texture;
}

void Texture::Generate()
{
	if (!m_RendererID) glGenTextures(1, &m_RendererID);
}

// ===============================================================
// Texture2D
// ===============================================================

// build texture from filepath
void Texture2D::LoadFile(const std::string& path)
{
	// set member vars
	m_FilePath = path;

	// generate the texture
	Generate();
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
void Texture2D::LoadBuffer(
	int texWidth, int texHeight, int bytesPerPixel, 
	unsigned char * texBuffer
)
{
	// update member vars
	m_Width = texWidth;
	m_Height = texHeight;
	m_BPP = bytesPerPixel;

	// generate the texture
	Generate();
	Bind();

	// copy texture data to GPU
	GenTextureFromBuffer(texWidth, texHeight, texBuffer);

	// generate mipmaps
	GLCall(glGenerateMipmap(GL_TEXTURE_2D));

	// Texture params
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	Unbind();
}

// build texture from CGL_Texture
void Texture2D::Load(CGL_Texture *cglTexture)
{
	// don't allow overwriting
	assert(!m_CGL_Texture);

	// must be a 2D texture type
	assert(
		cglTexture->GetTextureType() == CGL_TextureType::File2D ||
		cglTexture->GetTextureType() == CGL_TextureType::RawData2D
	);

	// update member vars
	m_CGL_Texture = cglTexture;

	// generate the texture
	Generate();
	Bind();
	Update();
}


// update GPU texture data from changes to CGL_texture. Assumes already bound!
void Texture2D::Update()
{
	if (!m_CGL_Texture) return;

	if (!m_CGL_Texture->NeedsUpdate()) return;


	
	// load texture data
	auto type = m_CGL_Texture->GetTextureType();
	if (type == CGL_TextureType::File2D) {
		FileTexture2D* fileTex = dynamic_cast<FileTexture2D*>(m_CGL_Texture);
		assert(fileTex);
		if (m_CGL_Texture->HasNewFilePath())
			GenTextureFromPath(fileTex->GetFilePath());
	}
	else if (type == CGL_TextureType::RawData2D) {
		DataTexture2D* dataTex = dynamic_cast<DataTexture2D*>(m_CGL_Texture);
		assert(dataTex);
		unsigned int width = dataTex->GetWidth();
		unsigned int height = dataTex->GetHeight();
		const unsigned char* dataBuffer = dataTex->GetDataBuffer().data();
		if (dataTex->HasNewRawData()) {

			if (m_CGL_Texture->HasNewDimensions()) {
				// new dimensions, need to recreate from scratch
				GenTextureFromBuffer(width, height, dataBuffer);
			}
			else {
				// same dimensions, just copy in place
				glTexSubImage2D(
					GL_TEXTURE_2D,
					0, // mipmap level
					0, 0,  // offset
					width, height,  // texture dims
					GL_RGBA,  // format we want to store texture (TODO: should be based off bytes per pixel...right now hardcoded to 4)
					GL_UNSIGNED_BYTE,  // size of each channel on CPU
					dataBuffer // texture buffer
				);
			}
		}
	} else { throw std::runtime_error("Texture type unsupported"); }

	// always update sampler params last, because generating mipmaps must happen AFTER image data is loaded 
	if (m_CGL_Texture->HasNewSampler())
		SetSamplerParams(m_CGL_Texture->GetSamplerParams());

	m_CGL_Texture->ResetUpdateFlags();
}

void Texture2D::Bind(unsigned int slot) const
{
	// activate texture unit first (GL_TEXTURE0 is default active unit)
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));

	// bind to the active texture unit
	GLCall(glBindTexture(GL_TEXTURE_2D, m_RendererID));
}

void Texture2D::Unbind() const
{
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

Texture2D* Texture2D::GetDefaultWhiteTexture()
{
	if (!DefaultWhiteTexture) {
		DefaultWhiteTexture = new Texture2D;
		DefaultWhiteTexture->LoadBuffer(1, 1, 4, &whitePixel[0]);
	}
	return DefaultWhiteTexture;
}

Texture2D* Texture2D::GetDefaultBlackTexture()
{
	if (!DefaultBlackTexture) {
		DefaultBlackTexture = new Texture2D;
		DefaultBlackTexture->LoadBuffer(1, 1, 4, &blackPixel[0]);
	}
	return DefaultBlackTexture;
}

Texture2D* Texture2D::GetDefaultMagentaTexture()
{
	if (!DefaultMagentaTexture) {
		DefaultMagentaTexture = new Texture2D;
		DefaultMagentaTexture->LoadBuffer(1, 1, 4, &magentaPixel[0]);
	}
	return DefaultMagentaTexture;
}

void Texture2D::GenTextureFromPath(const std::string &path)
{
	// flip so first pixel in output array is on bottom left.
	// this is the format expected by openGL
	stbi_set_flip_vertically_on_load(1);

	// load texture to CPU
	unsigned char* imageBuffer = stbi_load(
		path.c_str(),
		&m_Width,  // texture width
		&m_Height, // texture height
		&m_BPP,  // bits per pixel of channel data
		4		 // desired number of output channels. 4 for rgba
	);
	
	// make sure texture loaded
	if (!imageBuffer) {
		std::cerr << "ERROR: failed to load texture at path " << path << std::endl;
		std::cerr << "REASON:" << stbi_failure_reason() << std::endl;
		std::cerr << "Defaulting to 1x1 magenta pixel texture instead" << std::endl;
	}

	// copy texture data to GPU
	glTexImage2D(
		GL_TEXTURE_2D,// TODO: only supports 2D textures for now
		0, // mipmap LOD (for manually creating mipmaps)
		GL_RGBA8,  // format we want to store texture
		imageBuffer ? m_Width : 1, 
		imageBuffer ? m_Height : 1,  // texture dims
		0,  // legacy border width, always set 0
		GL_RGBA,  // format of texture on CPU     -- TODO: accept RGB format no alpha?
		GL_UNSIGNED_BYTE,  // size of each channel on CPU
		imageBuffer ? imageBuffer : &magentaPixel[0]  // texture buffer
	);


	// free local image data
	if (imageBuffer) stbi_image_free(imageBuffer);

	m_IsLoaded = true;
}

void Texture2D::GenTextureFromBuffer(
	int texWidth, int texHeight, const unsigned char *texBuffer
)
{
	GLCall(glTexImage2D(
		GL_TEXTURE_2D,
		0, 
		GL_RGBA8,  // format we want to store texture (TODO: should be based off bytes per pixel...right now hardcoded to 4)
		texWidth, texHeight,  // texture dims
		0,  // legacy border width, always set 0
		GL_RGBA,  // format of texture on CPU
		GL_UNSIGNED_BYTE,  // size of each channel on CPU
		texBuffer
	));

	m_IsLoaded = true;
}

void Texture2D::SetSamplerParams(const CGL_TextureSamplerParams &params)
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

void Texture2D::SetWrapMode(unsigned int axis, CGL_TextureWrapMode mode)
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

void Texture2D::SetFilterMode(unsigned int op, CGL_TextureFilterMode mode, bool enableMipMaps)
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


// =======================================================================
// CubeMapTexture
// =======================================================================

CubeMapTexture* CubeMapTexture::DefaultBlackCubeMap {nullptr};
CubeMapTexture* CubeMapTexture::DefaultWhiteCubeMap {nullptr};
CubeMapTexture* CubeMapTexture::GetDefaultBlackCubeMap() {
	unsigned char blackPixel[3] = { 0, 0, 0 };
	if (DefaultBlackCubeMap) return DefaultBlackCubeMap;
	DefaultBlackCubeMap = new CubeMapTexture;
	DefaultBlackCubeMap->LoadBuffer(blackPixel);
	return DefaultBlackCubeMap;
}
CubeMapTexture* CubeMapTexture::GetDefaultWhiteCubeMap() {
	unsigned char whitePixel[3] = { 255, 255, 255 };
	if (DefaultWhiteCubeMap) return DefaultWhiteCubeMap;
	DefaultWhiteCubeMap = new CubeMapTexture;
	DefaultWhiteCubeMap->LoadBuffer(whitePixel);
	return DefaultWhiteCubeMap;
}

void CubeMapTexture::SetSamplerParams()
{
	// For now, cubemaps are only for skyboxes and therefore have 
	// a fixed sampler as defined below 

	// set texture params
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	// clamp to edge to avoid seams
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
}

void CubeMapTexture::LoadBuffer(unsigned char* colorData)
{
	Generate();
	Bind();

	for (unsigned int i = 0; i < 6; i++) {
		GLCall(
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0,
				GL_RGB, GL_UNSIGNED_BYTE, colorData
			)
		);
	}

	SetSamplerParams();
	m_IsLoaded = true;
}

void CubeMapTexture::Load(CGL_Texture* cglTexture)
{
	assert(!m_CGL_CubeMap);  // don't allow overwriting
	assert(cglTexture->GetTextureType() == CGL_TextureType::CubeMap);

	// update member vars
	m_CGL_CubeMap = dynamic_cast<CGL_CubeMap*>(cglTexture);
	assert(m_CGL_CubeMap);

	Generate();
	Bind();
	Update();
}

void CubeMapTexture::LoadFiles(const std::vector<std::string> &faces)
{
	Generate();
	Bind();

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++) {
		unsigned char* data = stbi_load(
			faces[i].c_str(), &width, &height, &nrChannels, 0
		);
		if (data) {
			std::cout << "Cubemap texture loaded at path: " << faces[i] << std::endl;
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0,
				GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else {
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			// default to magenta pixel
			unsigned char magentaPixel[3] = { 255, 0, 255 };
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 1, 1, 0,
				GL_RGB, GL_UNSIGNED_BYTE, magentaPixel
			);
			// don't need to free data, was never allocated
		}
	}

	SetSamplerParams();

	m_IsLoaded = true;
}

void CubeMapTexture::Update()
{
	if (!m_CGL_CubeMap) return;

	if (!m_CGL_CubeMap->NeedsUpdate()) return;

	// Update texture data if new paths
	if (m_CGL_CubeMap->HasNewFilePath()) {
		LoadFiles(m_CGL_CubeMap->GetFilePaths());
		SetSamplerParams();
	}

	m_CGL_CubeMap->ResetUpdateFlags();
}

void CubeMapTexture::Bind(unsigned int slot) const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	// bind to the active texture unit
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, m_RendererID));
}

void CubeMapTexture::Unbind() const
{
	GLCall(glBindTexture(GL_TEXTURE_CUBE_MAP, 0));
}

