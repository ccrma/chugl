#pragma once

#include "chugl_pch.h"
#include "scenegraph/CGL_Texture.h"

class Texture
{
protected:
	unsigned int m_RendererID;	   // openGL generated UID for this texture
	bool m_IsLoaded;			   // whether or not this texture has been generated
public:
	Texture() : m_RendererID(0), m_IsLoaded(false) {}
	virtual ~Texture();

	// Texture Factory
	static Texture* CreateTexture(CGL_Texture* cglTexture);

	// generate texture on GPU
	void Generate();
	bool IsLoaded() const { return m_IsLoaded; }

	// create texture from CGL_Texture
	virtual void Load(CGL_Texture* cglTexture) = 0;
	
	// update texture according to changes to CGL_Texture 
	virtual void Update() = 0;

	// bind/unbind texture to openGL context
	virtual void Bind(unsigned int slot = 0) const = 0;
	virtual void Unbind() const = 0;


public:
	static int GetInternalFormat(CGL_Texture* chugl_tex);
	static void GenTex2D(
		CGL_Texture* cglTexture,
		int width, int height,
		const unsigned char* data,
		int target,
		int mipLevel
	);
};

class Texture2D : public Texture
{
private:
	// params for if this texture is NOT tied to a CGL_texture
	std::string m_FilePath;		   // path to texture

	int m_Width, m_Height, m_BPP;  // texture params

	CGL_Texture* m_CGL_Texture;	   // CGL texture object
public:
	// Note: constructors do not make any openGL calls so that
	// textures can be allocated as member variables in classes
	// and be allocated BEFORE the openGL context is created
	Texture2D() : 
		Texture(),
		m_FilePath(""),
		m_Width(0), m_Height(0), m_BPP(0),
		m_CGL_Texture(nullptr)
	{}
	
	virtual void Update() override;
	virtual void Bind(unsigned int slot = 0) const override;
	virtual void Unbind() const override;
	virtual void Load(CGL_Texture* cglTexture) override;

	void LoadFile(const std::string& path);
	void LoadBuffer(  // create a texture from a buffer directly
		int texWidth, int texHeight, int bytesPerPixel,
		unsigned char* texBuffer
	);

public:  // static default pixel textures
	static Texture2D* DefaultWhiteTexture;
	static Texture2D* DefaultBlackTexture;
	static Texture2D* DefaultMagentaTexture;
	static Texture2D* GetDefaultWhiteTexture();
	static Texture2D* GetDefaultBlackTexture();
	static Texture2D* GetDefaultMagentaTexture();

private:
	// helper fns for creating textures. 
	// Assumes texture is already bound!
	// will not unbind, so they can be efficiently composed
	void GenTextureFromPath(const std::string& path);
	void GenTextureFromBuffer(
		int texWidth, int texHeight, const unsigned char* texBuffer
	);
	void SetSamplerParams(const CGL_TextureSamplerParams& params);
	void SetWrapMode(unsigned int axis, CGL_TextureWrapMode mode);
	void SetFilterMode(unsigned int op, CGL_TextureFilterMode mode, bool enableMipMaps);
};

class CubeMapTexture : public Texture
{
private:
	CGL_CubeMap* m_CGL_CubeMap;
public:
	// faces must be passed in the order:
	// Right, Left, Top, Bottom, Back, Front
	CubeMapTexture() : Texture(), m_CGL_CubeMap(nullptr) {}

	virtual void Update() override;
	virtual void Bind(unsigned int slot = 0) const override;
	virtual void Unbind() const override;
	virtual void Load(CGL_Texture* cglTexture) override;

	// load from files
	void LoadFiles( const std::vector<std::string>& faces );
	// load from a buffer
	void LoadBuffer( unsigned char* colorData);

public:  // static defaults
	static CubeMapTexture* DefaultBlackCubeMap;
	static CubeMapTexture* DefaultWhiteCubeMap;
	static CubeMapTexture* GetDefaultBlackCubeMap();
	static CubeMapTexture* GetDefaultWhiteCubeMap();

private: // helper fns
	// void GenTextureFromFiles(const std::vector<std::string>& faces);
	// void GenTextureFromBuffer(unsigned char* colorData);
	void SetSamplerParams();
};


