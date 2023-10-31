#pragma once

#include "chugl_pch.h"
#include "SceneGraphNode.h"

enum class GeometryType {
	Base = 0,
	Box,
	Sphere,
	Circle,
	Cylinder,
	Capsule,
	Lathe,
	Cone,
	Plane,
	Quad,
	Torus,
	Custom
};

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec4 Color;
    glm::vec2 TexCoords;

    // constructor
    Vertex() : Color(1.0f) { }

	static float & VecIndex(glm::vec3& vec, char c) {
		if (c == 'x' || c == 'r')
			return vec.x;
		if (c == 'y' || c == 'g')
			return vec.y;
		if (c == 'z' || c == 'b')
			return vec.z;
        assert(false);
        // return anything to get around compiler warning/error
        return vec.x;
	}

    static float & VecIndex(glm::vec4& vec, char c) {
        if (c == 'x' || c == 'r')
            return vec.x;
        if (c == 'y' || c == 'g')
            return vec.y;
        if (c == 'z' || c == 'b')
            return vec.z;
        if (c == 'w' || c == 'a')
            return vec.w;
        assert(false);
        // return anything to get around compiler warning/error
        return vec.x;
    }

    static float & VecIndex(glm::vec2& vec, char c) {
        if (c == 'u' || c == 's' || c == 'x')
            return vec.x;
        if (c == 'v' || c == 't' || c == 'y')
            return vec.y;
        assert(false);
        // return anything to get around compiler warning/error
        return vec.x;
    }

	float& Pos(char c) { return VecIndex(Position, c); }
	float& Norm(char c) { return VecIndex(Normal, c); }
    float& Col(char c) { return VecIndex(Color, c); }
    float& Tex(char c) { return VecIndex(TexCoords, c); }
};

// attribute struct for CGL Geometry
struct CGL_GeoAttribute {
	std::string name;  // attribute name
	unsigned int location;  // attribute array ptr location
	unsigned int numComponents;  // number of components per vertex attribute accepted values are (1,2,3,4)
	bool normalize;
	std::vector<float> data;  // data for this attribute

	CGL_GeoAttribute() : name(""), location(0), numComponents(0), normalize(false) {}

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
		// fprintf(stderr, "Geometry constructor (%zu)\n", m_ID);
	}
	virtual ~Geometry() {
		// fprintf(stderr, "Geometry destructor (%zu)\n", m_ID);
		// all the data is in stl containers, so no need to do anything here nice
	}

	virtual bool IsGeometry() override { return true; }

	virtual void BuildGeometry() = 0;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() = 0;
	virtual Geometry* Dup() {
		Geometry* geo = (Geometry*) Clone();
		geo->NewID();
		return geo;
	}
	virtual void * GenUpdate() = 0;
	virtual void FreeUpdate(void* data) = 0;
	virtual void ApplyUpdate(void * data) {
		m_Dirty = true;
	};

	void ResetVertexData() {
		m_Attributes.clear();
		m_Indices.clear();

		// reset attribute map
		m_Attributes[POSITION_ATTRIB_IDX] = CGL_GeoAttribute("position", POSITION_ATTRIB_IDX, 3);
		m_Attributes[NORMAL_ATTRIB_IDX] = CGL_GeoAttribute("normal", NORMAL_ATTRIB_IDX, 3);
		m_Attributes[COLOR_ATTRIB_IDX] = CGL_GeoAttribute("color", COLOR_ATTRIB_IDX, 4);
		m_Attributes[UV0_ATTRIB_IDX] = CGL_GeoAttribute("uv", UV0_ATTRIB_IDX, 2);
	}
	void AddVertex(Vertex v);
	void AddTriangleIndices(size_t i1, size_t i2, size_t i3);

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
	static const t_CKUINT POSITION_ATTRIB_IDX;
	static const t_CKUINT NORMAL_ATTRIB_IDX;
	static const t_CKUINT COLOR_ATTRIB_IDX;
	static const t_CKUINT UV0_ATTRIB_IDX;

public: // chuck type names
	// TODO can probably template this and genarlize across all scenegraph classes?
	typedef std::unordered_map<GeometryType, const std::string, EnumClassHash> CkTypeMap;
	static CkTypeMap s_CkTypeMap;
	static const char * CKName(GeometryType type);
	virtual const char * myCkName() { return CKName(GetGeoType()); }
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
		m_Dirty = true;
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
		m_Dirty = true;
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

	void UpdateParams(
		float radius, int segments, float thetaStart, float thetaLength
	) {
		m_Params = {radius, std::max(3, segments), thetaStart, thetaLength};
		m_Dirty = true;
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
		m_Dirty = true;
	}
};

class PlaneGeometry: public Geometry
{
public:
	struct Params {
		float width;
		float height;
		int widthSegments;
		int heightSegments;
	} m_Params;
public:
	PlaneGeometry(
		float width = 1, float height = 1, int widthSegments = 1, int heightSegments = 1
	) : m_Params {width, height, widthSegments, heightSegments}
	{}

	void UpdateParams(
		float width, float height, int widthSegments, int heightSegments	
	) {
		m_Params = {width, height, widthSegments, heightSegments};
		m_Dirty = true;
	}

	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() override { return GeometryType::Plane; }
	virtual Geometry* Clone() override { 
		PlaneGeometry* geo = new PlaneGeometry(*this);
		geo->SetID(GetID());
		return geo;
	}

	// update command methods
	virtual void * GenUpdate() override {
		return new PlaneGeometry::Params(m_Params);
	};
	virtual void FreeUpdate(void* data) override {
		delete (PlaneGeometry::Params*) data;
	}
	virtual void ApplyUpdate(void * data) override {
		m_Params = *(PlaneGeometry::Params*)data;
		m_Dirty = true;
	}
};

class TorusGeometry: public Geometry
{
public:
	struct Params {
		float radius;
		float tubeRadius;
		int radialSegments;
		int tubularSegments;
		float arcLength;
	} m_Params;
public:
	TorusGeometry(
		float radius = 1.0, 
		float tubeRadius = 0.4, 
		int radialSegments = 12, 
		int tubularSegments = 48,
		float arcLength = glm::pi<float>() * 2.0
	) : m_Params {radius, tubeRadius, radialSegments, tubularSegments, arcLength}
	{}

	void UpdateParams(
		float radius, float tubeRadius, int radialSegments, int tubularSegments, float arcLength
	) {
		m_Params = {radius, tubeRadius, radialSegments, tubularSegments, arcLength};
		m_Dirty = true;
	}

	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() override { return GeometryType::Torus; }
	virtual Geometry* Clone() override { 
		TorusGeometry* geo = new TorusGeometry(*this);
		geo->SetID(GetID());
		return geo;
	}

	// update command methods
	virtual void * GenUpdate() override {
		return new TorusGeometry::Params(m_Params);
	};
	virtual void FreeUpdate(void* data) override {
		delete (TorusGeometry::Params*) data;
	}
	virtual void ApplyUpdate(void * data) override {
		m_Params = *(TorusGeometry::Params*)data;
		m_Dirty = true;
	}
};


class LatheGeometry: public Geometry
{
public:
	struct Params {
		std::vector<glm::vec2> points;
		int segments;
		float phiStart;
		float phiLength;
	} m_Params;
public:
	LatheGeometry(
		std::vector<glm::vec2> points = {glm::vec2(0, -0.5), glm::vec2(0.5, 0), glm::vec2(0, 0.5)}, 
		int segments = 12, 
		float phiStart = 0, 
		float phiLength = glm::pi<float>() * 2.0
	) : m_Params {points, segments, phiStart, phiLength}
	{}

	void UpdateParams(
		std::vector<glm::vec2> points, int segments, float phiStart, float phiLength
	) {
		m_Params = {points, segments, phiStart, phiLength};
		m_Dirty = true;
	}
	void UpdateParams(
		int segments, float phiStart, float phiLength
	);
	void UpdateParams(
		std::vector<double> points, int segments, float phiStart, float phiLength
	);

	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() override { return GeometryType::Lathe; }
	virtual Geometry* Clone() override { 
		LatheGeometry* geo = new LatheGeometry(*this);
		geo->SetID(GetID());
		return geo;
	}

	// update command methods
	virtual void * GenUpdate() override {
		return new LatheGeometry::Params(m_Params);
	};
	virtual void FreeUpdate(void* data) override {
		delete (LatheGeometry::Params*) data;
	}
	virtual void ApplyUpdate(void * data) override {
		m_Params = *(LatheGeometry::Params*)data;
		m_Dirty = true;
	}
};

class CylinderGeometry : public Geometry
{
public:
	struct Params {
		float radiusTop;
		float radiusBottom;
		float height;
		unsigned int radialSegments;
		unsigned int heightSegments;
		bool openEnded;
		float thetaStart;
		float thetaLength;
	} m_Params;

public:
	CylinderGeometry(
		float radiusTop = .2f, float radiusBottom = .2f, float height = 1.0f, 
		unsigned int radialSegments = 32, unsigned int heightSegments = 1, bool openEnded = false, 
		float thetaStart = 0.0f, float thetaLength = glm::pi<float>() * 2.0f
	) : m_Params {
		radiusTop, radiusBottom, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength
	} {}

	void UpdateParams(
		float radiusTop, float radiusBottom, float height, 
		unsigned int radialSegments, unsigned int heightSegments, bool openEnded, 
		float thetaStart, float thetaLength
	) {
		m_Params = {
			radiusTop, radiusBottom, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength
		};
		m_Dirty = true;
	}

	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() override { return GeometryType::Cylinder; }
	virtual Geometry* Clone() override { 
		CylinderGeometry* geo = new CylinderGeometry(*this);
		geo->SetID(GetID());
		return geo;
	}

	// update command methods
	virtual void * GenUpdate() override {
		return new CylinderGeometry::Params(m_Params);
	};
	virtual void FreeUpdate(void* data) override {
		delete (CylinderGeometry::Params*) data;
	}
	virtual void ApplyUpdate(void * data) override {
		m_Params = *(CylinderGeometry::Params*)data;
		m_Dirty = true;
	}
private:  // construction helpers
	void GenerateTorso(unsigned int& index);
	void GenerateCap(bool top, unsigned int& index);
};



// class CapsuleGeometry: public Geometry
// {
// public:
// 	struct Params {
// 		float radius;
// 		float length;
// 		int capSegments;
// 		int radialSegments;
// 	} m_Params;
// public:
// 	CapsuleGeometry(
// 		float radius = 0.5, float length = 1.0, int capSegments = 4, int radialSegments = 8 
// 	) : m_Params {radius, length, capSegments, radialSegments}
// 	{}

// 	void UpdateParams(
// 		float radius, float length, int capSegments, int radialSegments
// 	) {
// 		m_Params = {radius, length, capSegments, radialSegments};
// 		m_Dirty = true;
// 	}

// 	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
// 	virtual GeometryType GetGeoType() override { return GeometryType::Capsule; }
// 	virtual Geometry* Clone() override { 
// 		CapsuleGeometry* geo = new CapsuleGeometry(*this);
// 		geo->SetID(GetID());
// 		return geo;
// 	}

// 	// update command methods
// 	virtual void * GenUpdate() override {
// 		return new CapsuleGeometry::Params(m_Params);
// 	};
// 	virtual void FreeUpdate(void* data) override {
// 		delete (CapsuleGeometry::Params*) data;
// 	}
// 	virtual void ApplyUpdate(void * data) override {
// 		m_Params = *(CapsuleGeometry::Params*)data;
// 		m_Dirty = true;
// 	}
// };
