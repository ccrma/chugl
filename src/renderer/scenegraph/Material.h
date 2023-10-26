#pragma once

#include "chugl_pch.h"

#include "Locator.h"
#include "SceneGraphNode.h"
#include "SceneGraphObject.h"
#include "../Texture.h"
#include "../Shader.h"

// builtin material type struct
enum class MaterialType : unsigned int {
	Base = 0,
	Normal,
	Phong,
	CustomShader,
	Points,
	Mango,
	Line,
	Flat
};

enum class UniformType {
	Float = 0,
	Float2,
	Float3,
	Float4,
	Mat4,
	Int,
	Int2,
	Int3,
	Int4,
	Bool,
	Texture
};

struct MaterialUniform {
	UniformType type;
	std::string name;
	union {
		float f;
		float f2[2];
		float f3[3];
		float f4[4];
		int i;
		int i2[2];
		int i3[3];
		int i4[4];
		bool b;
		size_t texID;  // needs to be the ID so that when material is cloned into the renderer scenegraph it doesn't hold a chuck-side pointer reference
		
		// TODO: don't need to support Mat4 until matrices added to chuck
		// also at that point might want to stop using a union to be more space efficient
		// but that also would make memory access less efficient
	};

	// constructors  (because prior c++20 you can only initialize the first type in a union??)
	// also why doesn't cpp support C-style designated initializers?
	static MaterialUniform Create(const std::string s, float f0) {
		MaterialUniform m; m.type = UniformType::Float; m.name = s; m.f = f0; return m;
	}
	static MaterialUniform Create(std::string s, float f0, float f1) {
		MaterialUniform m; m.type = UniformType::Float2; m.name = s; m.f2[0] = f0; m.f2[1] = f1; return m;
	}
	static MaterialUniform Create(std::string s, float f0, float f1, float f2) {
		MaterialUniform m; m.type = UniformType::Float3; m.name = s; m.f3[0] = f0; m.f3[1] = f1; m.f3[2] = f2; return m;
	}
	static MaterialUniform Create(std::string s, glm::vec3 v) {
		MaterialUniform m; m.type = UniformType::Float3; m.name = s; m.f3[0] = v.x; m.f3[1] = v.y; m.f3[2] = v.z; return m;
	}
	static MaterialUniform Create(std::string s, float f0, float f1, float f2, float f3) {
		MaterialUniform m; m.type = UniformType::Float4; m.name = s; m.f4[0] = f0; m.f4[1] = f1; m.f4[2] = f2; m.f4[3] = f3; return m;
	}
	static MaterialUniform Create(std::string s, glm::vec4 v) {
		MaterialUniform m; m.type = UniformType::Float4; m.name = s; m.f4[0] = v.x; m.f4[1] = v.y; m.f4[2] = v.z; m.f4[3] = v.w; return m;
	}
	static MaterialUniform Create(std::string s, int i0) {
		MaterialUniform m; m.type = UniformType::Int; m.name = s; m.i = i0; return m;
	}
	static MaterialUniform Create(std::string s, int i0, int i1) {
		MaterialUniform m; m.type = UniformType::Int2; m.name = s; m.i2[0] = i0; m.i2[1] = i1; return m;
	}
	static MaterialUniform Create(std::string s, int i0, int i1, int i2) {
		MaterialUniform m; m.type = UniformType::Int3; m.name = s; m.i3[0] = i0; m.i3[1] = i1; m.i3[2] = i2; return m;
	}
	static MaterialUniform Create(std::string s, int i0, int i1, int i2, int i3) {
		MaterialUniform m; m.type = UniformType::Int4; m.name = s; m.i4[0] = i0; m.i4[1] = i1; m.i4[2] = i2; m.i4[3] = i3; return m;
	}
	static MaterialUniform Create(std::string s, bool b0) {
		MaterialUniform m; m.type = UniformType::Bool; m.name = s; m.b = b0; return m;
	}
	static MaterialUniform Create(std::string s, size_t texID) {
		MaterialUniform m; m.type = UniformType::Texture; m.name = s; m.texID = texID; return m;
	}
};

enum MaterialOptionParam : unsigned int {
	PolygonMode = 0,
	PrimitiveMode,
	Transparent
};

enum MaterialPolygonMode : unsigned int {
	Fill = 0,
	Line,
	Point
};

enum MaterialPrimitiveMode : unsigned int {
	Triangles = 0,
	TriangleStrip,
	Lines,
	LineStrip,
	LineLoop,
	Points,
};

enum MaterialOptionType : unsigned int {
	Float = 0,
	Int,
	UnsignedInt,
	Bool,
};

struct MaterialOption {
	MaterialOptionParam param; 	 // what option (e.g. wireframe, wireframe width, etc.)
	MaterialOptionType type; // what type of option (float, int, bool, etc.)
	union {   			     // option value
		float f;
		int i;
		unsigned int ui;
		bool b;
		MaterialPolygonMode polygonMode;
		MaterialPrimitiveMode primitiveMode;
	};

	// constructors  (because prior c++20 you can only initialize the first type in a union??)
	static MaterialOption Create(MaterialOptionParam param, float f) {
		MaterialOption m; m.type = MaterialOptionType::Float; m.param = param; m.f = f;  return m;
	}
	static MaterialOption Create(MaterialOptionParam param, int i) {
		MaterialOption m; m.type = MaterialOptionType::Int; m.param = param; m.i = i;  return m;
	}
	static MaterialOption Create(MaterialOptionParam param, unsigned int ui) {
		MaterialOption m; m.type = MaterialOptionType::UnsignedInt; m.param = param; m.ui = ui;  return m;
	}
	static MaterialOption Create(MaterialOptionParam param, bool b) {
		MaterialOption m; m.type = MaterialOptionType::Bool; m.param = param; m.b = b;  return m;
	}
	static MaterialOption Create(MaterialOptionParam param, MaterialPolygonMode polygonMode) {
		MaterialOption m; m.type = MaterialOptionType::UnsignedInt; m.param = param; m.polygonMode = polygonMode;  return m;
	}
	static MaterialOption Create(MaterialOptionParam param, MaterialPrimitiveMode primitiveMode) {
		MaterialOption m; m.type = MaterialOptionType::UnsignedInt; m.param = param; m.primitiveMode = primitiveMode;  return m;
	}
};


// Material abstract base class
class Material : public SceneGraphNode
{
public:
	Material() : m_VertIsPath(false), m_FragIsPath(false), m_VertShader(""), m_FragShader(""),
				 m_RecompileShader(false)
	{
		// set default material options
		SetOption(MaterialOption::Create(MaterialOptionParam::PolygonMode, MaterialPolygonMode::Fill));
		// default to triangle primitives
		SetOption(MaterialOption::Create(MaterialOptionParam::PrimitiveMode, MaterialPrimitiveMode::Triangles));
		// default to opaque
		SetOption(MaterialOption::Create(MaterialOptionParam::Transparent, false));


		// set default material uniforms
		SetUniform(MaterialUniform::Create(POINT_SIZE_UNAME, 25.0f));
		SetUniform(MaterialUniform::Create(LINE_WIDTH_UNAME, 1.0f));
		SetUniform(MaterialUniform::Create(COLOR_UNAME, 1.0f, 1.0f, 1.0f, 1.0f));

		// std::cerr << "Material constructor called, ID = " << this->GetID() << std::endl;
	};
	virtual ~Material() {
		// all state stored in stl containers, no need to free anything
	}

	virtual bool IsMaterial() override { return true; }

	virtual MaterialType GetMaterialType() { return MaterialType::Base; }

	virtual Material* Dup() {  // clone but get new ID 
		Material* mat = (Material*)Clone();
		mat->NewID();
		return mat;
	}

	// commands for telling a material how to update itself
	// TODO: inefficient, updates all uniforms each time. is there a clean way to only update the exact diff?
		// maybe parse entire command queue and collapse related commands... 
	virtual void * GenUpdate() { return new LocalUniformCache(m_Uniforms); }
	virtual void ApplyUpdate(void* uniform_data) { m_Uniforms = *(LocalUniformCache*)uniform_data; }
	virtual void FreeUpdate(void* uniform_data) { if (uniform_data) delete (LocalUniformCache*)uniform_data; }

	void SetOption(MaterialOption options) {
		m_Options[options.param] = options;
	}

	MaterialOption* GetOption(MaterialOptionParam p) {
		MaterialOption* ret = (m_Options.find(p) != m_Options.end()) ? &m_Options[p] : nullptr;
		return ret;
	}

	// Option getters
	MaterialPolygonMode GetPolygonMode() { return m_Options[MaterialOptionParam::PolygonMode].polygonMode; }
	MaterialPrimitiveMode GetPrimitiveMode() { return m_Options[MaterialOptionParam::PrimitiveMode].primitiveMode; }
	bool IsTransparent() { return m_Options[MaterialOptionParam::Transparent].b; }

	float GetPointSize() { 
		auto* uniform = GetUniform(POINT_SIZE_UNAME);
		if (uniform) return uniform->f;
		else return 0.0f;
	} 

	float GetLineWidth() { 
		auto* uniform = GetUniform(LINE_WIDTH_UNAME);
		if (uniform) return uniform->f;
		else return 0.0f;
	}
	glm::vec4 GetColor() { 
		auto* uniform = GetUniform(COLOR_UNAME);
		if (uniform) return glm::vec4(uniform->f4[0], uniform->f4[1], uniform->f4[2], uniform->f4[3]);
		else return glm::vec4(0.0f);
	}
	float GetAlpha() {
		auto* uniform = GetUniform(COLOR_UNAME);
		if (uniform) return uniform->f4[3];
		else return 0.0f;
	}

	// option setters
	void SetPolygonMode(MaterialPolygonMode mode) { m_Options[MaterialOptionParam::PolygonMode].polygonMode = mode; }
	void SetTransparent(bool b) { m_Options[MaterialOptionParam::Transparent].b = b; }

	// uniform setters
	virtual void SetPointSize(float size) { 
		SetUniform(MaterialUniform::Create(POINT_SIZE_UNAME, size));
	}

	void SetLineWidth(float width) { 
		SetUniform(MaterialUniform::Create(LINE_WIDTH_UNAME, width));
	}
	void SetColor(float r, float g, float b, float a) { 
		SetUniform(MaterialUniform::Create(COLOR_UNAME, r, g, b, a));
	}
	void SetAlpha(float a) { 
		auto& uniform = m_Uniforms[COLOR_UNAME];
		SetUniform(MaterialUniform::Create(COLOR_UNAME, uniform.f4[0], uniform.f4[1], uniform.f4[2], a));
	}

	void SetUniform(MaterialUniform uniform) {
		// get old uniform, if it exists
		auto it = m_Uniforms.find(uniform.name);

		// if it's a texture, refcount it
		if (uniform.type == UniformType::Texture) {
			CGL_Texture* texture = (CGL_Texture* )Locator::GetNode(uniform.texID, IsAudioThreadObject());
			CHUGL_ADD_REF(texture);
		}

		// if old uniform was a texture, unrefcount it
		if (it != m_Uniforms.end() && it->second.type == UniformType::Texture) {
			CGL_Texture* oldTexture = (CGL_Texture* )Locator::GetNode(it->second.texID, IsAudioThreadObject());
			CHUGL_RELEASE(oldTexture);
		}
		
		m_Uniforms[uniform.name] = uniform;
	}

	MaterialUniform* GetUniform(std::string s) {
		return (m_Uniforms.find(s) != m_Uniforms.end()) ? &m_Uniforms[s] : nullptr;
	}

public: // material options and uniforms settings
	typedef std::unordered_map<std::string, MaterialUniform> LocalUniformCache;
	// material options (affect rendering state, not directly passed to shader)
	// need to pass hash function, enum keys not supported until c++14 :(
	std::unordered_map<MaterialOptionParam, MaterialOption, std::hash<unsigned int>> m_Options;

	std::unordered_map<std::string, MaterialUniform>& GetLocalUniforms() { return m_Uniforms;  }

private:
	// uniform cache (copied to shader on render)
	LocalUniformCache m_Uniforms;

private: // custom shaders, only used by ShaderMaterial
	std::string m_VertShader, m_FragShader;	
	bool m_VertIsPath, m_FragIsPath;
	bool m_RecompileShader;


public:  // static consts
	// putting all unames here to facilitate being able to set all material options
	// and uniforms from base chuck Material class (so you don't have to cast e.g. mesh.mat() $ PhongMat)

	// supported polygon modes
	static const MaterialPolygonMode POLYGON_FILL;
	static const MaterialPolygonMode POLYGON_LINE;
	static const MaterialPolygonMode POLYGON_POINT;

	// line rendering modes
	static const unsigned int LINE_SEGMENTS_MODE;
	static const unsigned int LINE_STRIP_MODE;
	static const unsigned int LINE_LOOP_MODE;

	// uniform names (TODO ShaderCode should be reading from here)
	static const std::string POINT_SIZE_UNAME;
	static const std::string LINE_WIDTH_UNAME;
	static const std::string COLOR_UNAME;
	// static const std::string AFFECTED_BY_FOG_UNAME;  // TODO

	// normal material
	static const std::string USE_LOCAL_NORMALS_UNAME;
	
	// phong mats
	static const std::string DIFFUSE_MAP_UNAME;
	static const std::string SPECULAR_MAP_UNAME;
	static const std::string SPECULAR_COLOR_UNAME;
	static const std::string SHININESS_UNAME;

	// point mats
	static const std::string POINT_SIZE_ATTENUATION_UNAME;
	static const std::string POINT_SPRITE_TEXTURE_UNAME;


public:  // uniform getters and setters
	// uniform getters
	size_t GetDiffuseMapID() { 
		auto* uniform = GetUniform(Material::DIFFUSE_MAP_UNAME);
		if (uniform) return uniform->texID;
		else return 0;
	}
	size_t GetSpecularMapID() { 
		auto* uniform = GetUniform(Material::SPECULAR_MAP_UNAME);
		if (uniform) return uniform->texID;
		else return 0;
	}
	glm::vec3 GetSpecularColor() { 
		auto* uniform = GetUniform(Material::SPECULAR_COLOR_UNAME);
		if (uniform) return glm::vec3(uniform->f3[0], uniform->f3[1], uniform->f3[2]);
		else return glm::vec3(0.0f);
	}
	float GetLogShininess() { 
		auto* uniform = GetUniform(Material::SHININESS_UNAME);
		if (uniform) return std::log2(uniform->f);
		else return 0.0f;
	}
	bool GetUseLocalNormals() { 
		auto* uniform = GetUniform(USE_LOCAL_NORMALS_UNAME);
		if (uniform) return uniform->b;
		else return false;
	}
	// point uniform getters
	bool GetSizeAttenuation() { 
		auto* uniform = GetUniform(POINT_SIZE_ATTENUATION_UNAME);
		if (uniform) return uniform->b;
		else return false;
	}
	size_t GetSpriteID() { 
		auto* uniform = GetUniform(POINT_SPRITE_TEXTURE_UNAME);
		if (uniform) return uniform->texID;
		else return 0;
	}
	const std::string& GetVertShader() { return m_VertShader; }
	const std::string& GetFragShader() { return m_FragShader; }
	bool GetVertIsPath() { return m_VertIsPath; }
	bool GetFragIsPath() { return m_FragIsPath; }
	bool RecompileShader() { return m_RecompileShader; }

	// uniform setters
	void SetDiffuseMap(CGL_Texture* texture) { 
		SetUniform(MaterialUniform::Create(Material::DIFFUSE_MAP_UNAME, texture->GetID()));
	}

	void SetSpecularMap(CGL_Texture* texture) { 
		SetUniform(MaterialUniform::Create(Material::SPECULAR_MAP_UNAME, texture->GetID()));
	}


	void SetSpecularColor(float r, float g, float b) {
		SetUniform(MaterialUniform::Create(Material::SPECULAR_COLOR_UNAME, r, g, b));
		// auto& uniform = m_Uniforms[Material::SPECULAR_COLOR_UNAME];
		// uniform.f3[0] = r; uniform.f3[1] = g; uniform.f3[2] = b;
	}
	void SetLogShininess(float logShininess) { 
		float shininess = std::pow(2.0f, logShininess);
		SetUniform(MaterialUniform::Create(Material::SHININESS_UNAME, shininess));
	}
	void UseLocalNormals() { 
		SetUniform(MaterialUniform::Create(USE_LOCAL_NORMALS_UNAME, true));
	}
	void UseWorldNormals() { 
		SetUniform(MaterialUniform::Create(USE_LOCAL_NORMALS_UNAME, false));
	}
	virtual void SetFragShader(const std::string& fragmentShader, bool isPath = true) {
		m_FragShader = fragmentShader;
		m_FragIsPath = isPath;
		m_RecompileShader = true;
	}
	virtual void SetVertShader(const std::string& vertexShader, bool isPath = true) {
		m_VertShader = vertexShader;
		m_VertIsPath = isPath;
		m_RecompileShader = true;
	}
	void SetRecompileFlag(bool flag) {
		m_RecompileShader = flag;
	}
	// primitive mode setters
	void SetLineMode(MaterialPrimitiveMode mode) { SetOption(MaterialOption::Create(MaterialOptionParam::PrimitiveMode, mode)); }
	void SetLineStrip() { SetLineMode(MaterialPrimitiveMode::LineStrip); }
	void SetLineLoop() { SetLineMode(MaterialPrimitiveMode::LineLoop); }
	void SetLines() { SetLineMode(MaterialPrimitiveMode::Lines); }
	// point uniform setters
	void SetSizeAttenuation(bool b) { 
		SetUniform(MaterialUniform::Create(POINT_SIZE_ATTENUATION_UNAME, b));
	}
	void SetSprite(CGL_Texture* texture) { 
		SetUniform(MaterialUniform::Create(POINT_SPRITE_TEXTURE_UNAME, texture->GetID()));
	}

public:  // static material type --> chuck type name map
	typedef std::unordered_map<MaterialType, const std::string, EnumClassHash> CkTypeMap;
	static CkTypeMap s_CkTypeMap;
	static const char * CKName(MaterialType type) { return s_CkTypeMap[type].c_str(); }
	virtual const char * myCkName() { return CKName(GetMaterialType()); }
};

// material that colors using worldspace normals as rgb
class NormalMaterial : public Material
{
public:
	NormalMaterial() {
		SetUniform(MaterialUniform::Create(USE_LOCAL_NORMALS_UNAME, false));
	}

	virtual MaterialType GetMaterialType() override { return MaterialType::Normal; }
	virtual Material* Clone() override { 
		auto* mat = new NormalMaterial(*this);
		mat->SetID(GetID());
		return mat;
	}
};

// phong lighting (ambient + diffuse + specular)
class PhongMaterial : public Material
{
public:
	PhongMaterial(
		size_t diffuseMapID = 0,
		size_t specularMapID = 0,
		glm::vec4 specularColor = glm::vec4(1.0f),
		float logShininess = 5  // ==> 32 shininess
	) {
		SetUniform(MaterialUniform::Create(Material::DIFFUSE_MAP_UNAME, diffuseMapID));
		SetUniform(MaterialUniform::Create(Material::SPECULAR_MAP_UNAME, specularMapID));
		SetUniform(MaterialUniform::Create(Material::SPECULAR_COLOR_UNAME, specularColor));
		SetUniform(MaterialUniform::Create(Material::SHININESS_UNAME, logShininess));
	}

	virtual MaterialType GetMaterialType() override { return MaterialType::Phong; }
	virtual Material* Clone() override { 
		auto* mat = new PhongMaterial(*this);
		mat->SetID(GetID());
		return mat;
	}
};

class FlatMaterial : public Material
{
public:
	FlatMaterial(
		size_t diffuseMapID = 0
	) {
		SetUniform(MaterialUniform::Create(Material::DIFFUSE_MAP_UNAME, diffuseMapID));
	}

	virtual MaterialType GetMaterialType() override { return MaterialType::Flat; }
	virtual Material* Clone() override { 
		auto* mat = new FlatMaterial(*this);
		mat->SetID(GetID());
		return mat;
	}
};

// Custom shader material
class ShaderMaterial : public Material
{
public:
	ShaderMaterial (
		std::string vertexShader, std::string fragmentShader,
		bool vertIsPath = true, bool fragIsPath = true
	) {
		SetVertShader(vertexShader, vertIsPath);
		SetFragShader(fragmentShader, fragIsPath);
	}

	virtual MaterialType GetMaterialType() override { return MaterialType::CustomShader; }
	virtual Material* Clone() override {
		auto* mat = new ShaderMaterial(*this);
		mat->SetID(GetID());
		return mat;
	};
};

// Points material
class PointsMaterial : public Material
{
public:
	PointsMaterial() {
		SetOption(MaterialOption::Create(MaterialOptionParam::PolygonMode, MaterialPolygonMode::Point));
		SetOption(MaterialOption::Create(MaterialOptionParam::PrimitiveMode, MaterialPrimitiveMode::Points));

		// set point size attenuation option
		SetUniform(MaterialUniform::Create(POINT_SIZE_ATTENUATION_UNAME, true));
		// point sprite texture
		SetUniform(MaterialUniform::Create(POINT_SPRITE_TEXTURE_UNAME, (size_t)0));
	}

	virtual MaterialType GetMaterialType() override { return MaterialType::Points; }
	virtual Material* Clone() override {
		auto* mat = new PointsMaterial(*this);
		mat->SetID(GetID());
		return mat;
	};
};


// UV debug mat
class MangoMaterial : public Material
{
public:
	MangoMaterial() {
	}

	virtual MaterialType GetMaterialType() override { return MaterialType::Mango; }
	virtual Material* Clone() override {
		auto* mat = new MangoMaterial(*this);
		mat->SetID(GetID());
		return mat;
	}
};

// Line mat
class LineMaterial : public Material
{
public:
	LineMaterial() {
		SetOption(MaterialOption::Create(MaterialOptionParam::PolygonMode, MaterialPolygonMode::Line));
		SetOption(MaterialOption::Create(MaterialOptionParam::PrimitiveMode, MaterialPrimitiveMode::LineStrip));
	}

	virtual MaterialType GetMaterialType() override { return MaterialType::Line; }
	virtual Material* Clone() override {
		auto* mat = new LineMaterial(*this);
		mat->SetID(GetID());
		return mat;
	}
};
