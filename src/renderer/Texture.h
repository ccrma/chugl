#pragma once
#include <string>
#include "scenegraph/CGL_Texture.h"

class Texture
{
private:
	unsigned int m_RendererID;	   // openGL generated UID for this texture
	std::string m_FilePath;		   // path to texture
	unsigned char* m_LocalBuffer;  // CPU-side texture data 
	int m_Width, m_Height, m_BPP;  // texture params
	CGL_Texture* m_CGL_Texture;	   // CGL texture object

public:
	Texture(const std::string& path);
	Texture(  // create a texture from a buffer directly
		int texWidth, int texHeight, int bytesPerPixel,
		unsigned char* texBuffer
	);
	Texture(CGL_Texture* cglTexture);
	~Texture();

	void Update();
	void Bind(unsigned int slot = 0) const;
	void Unbind();

	// default pixel textures
	static Texture* DefaultWhiteTexture;
	static Texture* DefaultBlackTexture;
	static Texture* GetDefaultWhiteTexture();
	static Texture* GetDefaultBlackTexture();

private:
	// helper fns for creating textures. 
	// Assumes texture is already bound!
	// will not unbind, so they can be efficiently composed
	void GenTextureFromPath(const std::string& path);
	void SetSamplerParams(const CGL_TextureSamplerParams& params);
	void SetWrapMode(unsigned int axis, CGL_TextureWrapMode mode);
	void SetFilterMode(unsigned int op, CGL_TextureFilterMode mode, bool enableMipMaps);

};


