#pragma once
#include "SceneGraphNode.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/epsilon.hpp"
#include <vector>
#include <unordered_map>

enum class GeometryType {
	Base = 0,
	Box,
	Sphere,
	Circle,
	Cylinder,
	Cone,
	Plane,
	Quad,
	Custom
};

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;

	static float& VecIndex(glm::vec3& vec, char c) {
		if (c == 'x' || c == 'r')
			return vec.x;
		if (c == 'y' || c == 'g')
			return vec.y;
		if (c == 'z' || c == 'b')
			return vec.z;
		assert(false);
	}

	float& Pos(char c) { return VecIndex(Position, c); }
	float& Norm(char c) { return VecIndex(Normal, c); }
};

// attribute struct for CGL Geometry
struct CGL_GeoAttribute {
	std::string name;  // attribute name
	unsigned int location;  // attribute array ptr location
	unsigned int numComponents;  // number of components per vertex attribute accepted values are (1,2,3,4)
	bool normalize;
	std::vector<float> data;  // data for this attribute

	CGL_GeoAttribute() : name("") {}

	CGL_GeoAttribute(
		const std::string& n, unsigned int loc, unsigned int numComp
	) : name(n), location(loc), numComponents(numComp), normalize(false) {}

	size_t SizeInBytes() const {
		return data.size() * sizeof(float);
	}

	size_t NumVertices() const {
		return data.size() / numComponents;
	}

};

// returns reference to float at index 'c'

class Geometry : public SceneGraphNode // abstract base class for buffered geometry data
{
public:
	Geometry() : m_Dirty(true) {
		fprintf(stderr, "Geometry constructor (%zu)\n", m_ID);
	}
	virtual ~Geometry() {}
	virtual void BuildGeometry() = 0;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() = 0;
	virtual Geometry* Clone() = 0;  // deepcopy the geometry data
	virtual void * GenUpdate() = 0;
	virtual void FreeUpdate(void* data) = 0;
	virtual void ApplyUpdate(void * data) = 0;

	void ResetVertexData() {
		m_Attributes.clear();
		m_Indices.clear();

		// reset attribute map
		m_Attributes[POSITION_ATTRIB_IDX] = CGL_GeoAttribute("position", POSITION_ATTRIB_IDX, 3);
		m_Attributes[NORMAL_ATTRIB_IDX] = CGL_GeoAttribute("normal", NORMAL_ATTRIB_IDX, 3);
		m_Attributes[COLOR_ATTRIB_IDX] = CGL_GeoAttribute("color", COLOR_ATTRIB_IDX, 3);
		m_Attributes[UV0_ATTRIB_IDX] = CGL_GeoAttribute("uv", UV0_ATTRIB_IDX, 2);
	}
	void AddVertex(Vertex v);
	void AddTriangleIndices(unsigned int i1, unsigned int i2, unsigned int i3);

	// moves attribute into attributeMap. param attrib is unusable afterwards
	// uses move semantics to prevent copying
	void AddAttribute(CGL_GeoAttribute& attrib) {
		m_Attributes[attrib.location] = std::move(attrib);
		m_Dirty = true;
	}
	void SetIndices(std::vector<unsigned int>& indices) {
		m_Indices = std::move(indices);
		m_Dirty = true;
	}

	// attribute getters
	CGL_GeoAttribute& GetAttribute(unsigned int idx) { return m_Attributes[idx]; }
	CGL_GeoAttribute& GetPositions() { return m_Attributes[POSITION_ATTRIB_IDX]; }

	inline bool IsDirty() { return m_Dirty; }

	// geometry data, only computed/stored in the renderer-side copy!
	// chuck-side will only store high-level params like width, heigh, #segments etc
	std::vector<unsigned int> m_Indices;

	typedef std::unordered_map<unsigned int, CGL_GeoAttribute> AttributeMap;
	AttributeMap m_Attributes;
	// std::vector<Vertex> m_Vertices;

	bool m_Dirty;  // if true, rebuild buffers

public:  // constants
	static const unsigned int POSITION_ATTRIB_IDX;
	static const unsigned int NORMAL_ATTRIB_IDX;
	static const unsigned int COLOR_ATTRIB_IDX;
	static const unsigned int UV0_ATTRIB_IDX;
};

class BoxGeometry : public Geometry
{
public:
	struct Params {
		float width, height, depth;
		int widthSeg, heightSeg, depthSeg;
	} m_Params;
public:
	BoxGeometry(
		float width = 1, float height = 1, float depth = 1,
		int widthSeg = 1, int heightSeg = 1, int depthSeg = 1
	) : m_Params {width, height, depth, widthSeg, heightSeg, depthSeg}
	{}

	void UpdateParams(
		float width, float height, float depth,
		int widthSeg, int heightSeg, int depthSeg
	) {
		m_Params = {width, height, depth, widthSeg, heightSeg, depthSeg};
		m_Dirty = true;
	}

	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() override { return GeometryType::Box; }
	virtual Geometry* Clone() override { 
		BoxGeometry* geo = new BoxGeometry(*this);
		geo->SetID(GetID());
		return geo;
	}
	virtual void * GenUpdate() override {
		return new BoxGeometry::Params(m_Params);
	}
	virtual void FreeUpdate(void * data) override {
		delete (BoxGeometry::Params *)data;
	}
	virtual void ApplyUpdate(void * data) override {
		m_Params = *(BoxGeometry::Params *)data;
	}

private:
	void buildPlane(
		char u, char v, char w, 
		int udir, int vdir,  // uv dirs
		float width, float height, float depth, 
		int gridX, int gridY, // how much we subdivide
		int materialIndex  // for allowing groups within a single geometry. ignore for now
	);
};

class SphereGeometry : public Geometry
{
public: 
	struct Params 
	{
		float radius;
		int widthSeg, heightSeg;
		float phiStart, phiLength;
		float thetaStart, thetaLength;
	} m_Params;
public:
	SphereGeometry(
		float radius = 0.5, int widthSegments = 32, int heightSegments = 16, 
		float phiStart = 0.0, float phiLength = glm::pi<float>() * 2.0, // how much along circumference
		float thetaStart = 0.0, float thetaLength = glm::pi<float>()  // how much along central diameter
	) : m_Params {
		radius, widthSegments, heightSegments, phiStart, phiLength, thetaStart, thetaLength
	} 
	{}

	void UpdateParams(
		float radius, int widthSeg, int heightSeg, 
		float phiStart, float phiLength, float thetaStart, float thetaLength
	) {
		m_Params = {
			radius, widthSeg, heightSeg, phiStart, phiLength, thetaStart, thetaLength
		};
		m_Dirty = true;
	}

	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() override { return GeometryType::Sphere; }
	virtual Geometry* Clone() override { 
		SphereGeometry* geo = new SphereGeometry(*this); 
		geo->SetID(GetID());
		return geo;
	}

	// update command methods
	virtual void * GenUpdate() override {
		return new SphereGeometry::Params(m_Params);
	};
	virtual void FreeUpdate(void* data) override {
		delete (SphereGeometry::Params*)data;
	}
	virtual void ApplyUpdate(void * data) override {
		m_Params = *(SphereGeometry::Params*)data;
	}
};

class CustomGeometry : public Geometry
{
public:
	virtual void BuildGeometry() override {
		// nothing to do, geometry buffers are already passed from user
		m_Dirty = false;
	}
	virtual GeometryType GetGeoType() override { return GeometryType::Custom; }

	virtual Geometry* Clone() override {
		auto* geo = new CustomGeometry(*this);
		geo->SetID(GetID());
		return geo;
	};

	// currently unused for custom geo
	virtual void* GenUpdate() override { return nullptr; }
	virtual void FreeUpdate(void* data) override {}
	virtual void ApplyUpdate(void * data) override {}
};

class CircleGeometry : public Geometry
{
public:
	struct Params {
		float radius;
		int segments;
		float thetaStart;
		float thetaLength;
	} m_Params;
public:
	CircleGeometry(
		float radius = 1, int segments = 32, 
		float thetaStart = 0, float thetaLength = glm::pi<float>() * 2.0
	) : m_Params{radius, segments, thetaStart, thetaLength}
	{ 
		m_Params.segments = std::max(3, m_Params.segments);
		// for now don't build geometry
		// at least until chuck API implements geometry buffer getters 
	}

	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() override { return GeometryType::Circle; }
	virtual Geometry* Clone() override { 
		CircleGeometry* geo = new CircleGeometry(*this);
		geo->SetID(GetID());
		return geo;
	}

	// update command methods
	virtual void * GenUpdate() override {
		return new CircleGeometry::Params(m_Params);
	};
	virtual void FreeUpdate(void* data) override {
		delete (CircleGeometry::Params*) data;
	}
	virtual void ApplyUpdate(void * data) override {
		m_Params = *(CircleGeometry::Params*)data;
	}
};



