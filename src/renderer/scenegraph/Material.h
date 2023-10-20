#pragma once

#include "SceneGraphNode.h"
#include "SceneGraphObject.h"
#include "../Texture.h"
#include "../Shader.h"
#include <string>
#include <unordered_map>
// #include <cmath>

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
	PrimitiveMode
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
	Material() {
		// set default material options
		SetOption(MaterialOption::Create(MaterialOptionParam::PolygonMode, MaterialPolygonMode::Fill));
		// default to triangle primitives
		SetOption(MaterialOption::Create(MaterialOptionParam::PrimitiveMode, MaterialPrimitiveMode::Triangles));


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
	virtual std::unordered_map<std::string, MaterialUniform>& GetLocalUniforms() { return m_Uniforms;  }  // for setting properties specific to the material, e.g. color

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

	inline void SetOption(MaterialOption options) {
		m_Options[options.param] = options;
	}


	inline MaterialOption* GetOption(MaterialOptionParam p) {
		return (m_Options.find(p) != m_Options.end()) ? &m_Options[p] : nullptr;
	}

	// Option getters
	MaterialPolygonMode GetPolygonMode() { return m_Options[MaterialOptionParam::PolygonMode].polygonMode; }
	MaterialPrimitiveMode GetPrimitiveMode() { return m_Options[MaterialOptionParam::PrimitiveMode].primitiveMode;}
	// float GetLineWidth() { return m_Options[MaterialOptionParam::LineWidth].f; }
	float GetPointSize() { return m_Uniforms[POINT_SIZE_UNAME].f; } 
	float GetLineWidth() { return m_Uniforms[LINE_WIDTH_UNAME].f; }
	glm::vec4 GetColor() { 
		auto& matUniform = m_Uniforms[COLOR_UNAME];
		return glm::vec4(matUniform.f4[0], matUniform.f4[1], matUniform.f4[2], matUniform.f4[3]);
	}
	float GetAlpha() {
		auto& matUniform = m_Uniforms[COLOR_UNAME];
		return matUniform.f4[3];
	}

	// option setters
	void SetPolygonMode(MaterialPolygonMode mode) { m_Options[MaterialOptionParam::PolygonMode].polygonMode = mode; }
	// virtual void SetLineWidth(float width) { m_Options[MaterialOptionParam::LineWidth].f = width; }
	virtual void SetPointSize(float size) { m_Uniforms[POINT_SIZE_UNAME].f = size; }
	void SetLineWidth(float width) { m_Uniforms[LINE_WIDTH_UNAME].f = width; }
	void SetColor(float r, float g, float b, float a) { 
		auto& uniform = m_Uniforms[COLOR_UNAME];
		uniform.f4[0] = r; uniform.f4[1] = g; uniform.f4[2] = b; uniform.f4[3] = a;
	}
	void SetAlpha(float a) { 
		auto& uniform = m_Uniforms[COLOR_UNAME];
		uniform.f4[3] = a;
	}

	inline void SetUniform(MaterialUniform uniform) {
		// can't do this here because texture refcounting won't happen
		// assert(uniform.type != UniformType::Texture);
		
		m_Uniforms[uniform.name] = uniform;
	}

	inline MaterialUniform* GetUniform(std::string s) {
		return (m_Uniforms.find(s) != m_Uniforms.end()) ? &m_Uniforms[s] : nullptr;
	}

public: // material options and uniforms settings
	typedef std::unordered_map<std::string, MaterialUniform> LocalUniformCache;
	// material options (affect rendering state, not directly passed to shader)
	// need to pass hash function, enum keys not supported until c++14 :(
	std::unordered_map<MaterialOptionParam, MaterialOption, std::hash<unsigned int>> m_Options;

	// uniform cache (copied to shader on render)
	LocalUniformCache m_Uniforms;

public: // custom shaders, only used by ShaderMaterial
	std::string m_VertShaderPath, m_FragShaderPath;	


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
	size_t GetDiffuseMapID() { return m_Uniforms[Material::DIFFUSE_MAP_UNAME].texID; }
	size_t GetSpecularMapID() { return m_Uniforms[Material::SPECULAR_MAP_UNAME].texID; }
	glm::vec3 GetSpecularColor() { 
		auto& matUniform = m_Uniforms[Material::SPECULAR_COLOR_UNAME];
		return glm::vec3(matUniform.f3[0], matUniform.f3[1], matUniform.f3[2]); 
	}
	float GetLogShininess() { return m_Uniforms[Material::SHININESS_UNAME].f; }
	bool GetUseLocalNormals() { return m_Uniforms[USE_LOCAL_NORMALS_UNAME].b; }
	// point uniform getters
	bool GetSizeAttenuation() { return m_Uniforms[POINT_SIZE_ATTENUATION_UNAME].b; }
	size_t GetSpriteID() { return m_Uniforms[POINT_SPRITE_TEXTURE_UNAME].texID; }

	// uniform setters
	// void SetTexture(CGL_Texture* )
	
	void SetDiffuseMap(CGL_Texture* texture) { 
		m_Uniforms[Material::DIFFUSE_MAP_UNAME].texID = texture->GetID(); 
	}

	void SetSpecularMap(CGL_Texture* texture) { 
		m_Uniforms[Material::SPECULAR_COLOR_UNAME].texID = texture->GetID(); 
	}

	void SetSpecularColor(float r, float g, float b) {
		auto& uniform = m_Uniforms[Material::SPECULAR_COLOR_UNAME];
		uniform.f3[0] = r; uniform.f3[1] = g; uniform.f3[2] = b;
	}
	void SetLogShininess(float logShininess) { 
		m_Uniforms[Material::SHININESS_UNAME].f = std::pow(2.0f, logShininess);
	}
	void UseLocalNormals() { m_Uniforms[USE_LOCAL_NORMALS_UNAME].b = true; }
	void UseWorldNormals() { m_Uniforms[USE_LOCAL_NORMALS_UNAME].b = false; }
	virtual void SetShaderPaths(std::string vertexShaderPath, std::string fragmentShaderPath) {
		m_VertShaderPath = vertexShaderPath;
		m_FragShaderPath = fragmentShaderPath;
	}
	// primitive mode setters
	void SetLineMode(MaterialPrimitiveMode mode) { SetOption(MaterialOption::Create(MaterialOptionParam::PrimitiveMode, mode)); }
	void SetLineStrip() { SetLineMode(MaterialPrimitiveMode::LineStrip); }
	void SetLineLoop() { SetLineMode(MaterialPrimitiveMode::LineLoop); }
	void SetLines() { SetLineMode(MaterialPrimitiveMode::Lines); }
	// point uniform setters
	void SetSizeAttenuation(bool b) { m_Uniforms[POINT_SIZE_ATTENUATION_UNAME].b = b; }
	void SetSprite(CGL_Texture* texture) { m_Uniforms[POINT_SPRITE_TEXTURE_UNAME].texID = texture->GetID(); }

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
		std::string vertexShaderPath, std::string fragmentShaderPath
	) {
		m_FragShaderPath = fragmentShaderPath;
		m_VertShaderPath = vertexShaderPath;
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
