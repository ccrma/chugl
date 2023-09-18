#pragma once
#include <string>

class Texture
{
private:
	unsigned int m_RendererID;	   // openGL generated UID for this texture
	std::string m_FilePath;		   // path to texture
	unsigned char* m_LocalBuffer;  // CPU-side texture data 
	int m_Width, m_Height, m_BPP;  // texture params

public:
	Texture(const std::string& path);
	Texture(  // create a texture from a buffer directly
		int texWidth, int texHeight, int bytesPerPixel,
		unsigned char* texBuffer
	);
	~Texture();

	void Bind(unsigned int slot = 0) const;
	void Unbind();

	static Texture* GetDefaultWhiteTexture();
	static Texture* GetDefaultBlackTexture();


	// default pixel textures
	static Texture* DefaultWhiteTexture;
	static Texture* DefaultBlackTexture;
};


