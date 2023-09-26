#pragma once

#include "SceneGraphNode.h"
// TODO: move these rendererAPI abstractions into their own directory
#include "../VertexBufferLayout.h"
#include "../VertexArray.h"
#include "glm/glm.hpp"
#include "glm/gtc/constants.hpp"
#include "glm/gtc/epsilon.hpp"
#include <vector>

enum class GeometryType {
	Base = 0,
	Box,
	Sphere,
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

struct Index {
	unsigned int a, b, c;
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

	inline bool IsDirty() { return m_Dirty; }

	// geometry data, only computed/stored in the renderer-side copy!
	// chuck-side will only store high-level params like width, heigh, #segments etc
	std::vector<Index> m_Indices;
	std::vector<Vertex> m_Vertices;

	bool m_Dirty;  // if true, rebuild buffers
};

struct BoxGeoUpdateData
{
	float width, height, depth;
	int widthSeg, heightSeg, depthSeg;
};

class BoxGeometry : public Geometry
{
public:
	BoxGeometry(
		float width = 1, float height = 1, float depth = 1,
		int widthSeg = 1, int heightSeg = 1, int depthSeg = 1
	);
	void UpdateParams(
			float width, float height, float depth, int widthSeg, int heightSeg, int depthSeg) 
	{
		m_Dirty = true;
		m_Width = width; m_Height = height; m_Depth = depth;
		m_WidthSeg = widthSeg; m_HeightSeg = heightSeg; m_DepthSeg = depthSeg;	
	}
	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() override { return GeometryType::Box; }
	virtual Geometry* Clone() override { 
		return new BoxGeometry(m_Width, m_Height, m_Depth, m_WidthSeg, m_HeightSeg, m_DepthSeg);
	}
	virtual void * GenUpdate() override {
		return new BoxGeoUpdateData{m_Width, m_Height, m_Depth, m_WidthSeg, m_HeightSeg, m_DepthSeg};
	};
	virtual void FreeUpdate(void * data) override {
		delete (BoxGeoUpdateData *)data;
	}
	virtual void ApplyUpdate(void * data) override {
		BoxGeoUpdateData * updateData = (BoxGeoUpdateData *)data;
		UpdateParams(updateData->width, updateData->height, updateData->depth, 
			updateData->widthSeg, updateData->heightSeg, updateData->depthSeg);
	};

	float m_Width, m_Height, m_Depth;
	int m_WidthSeg, m_HeightSeg, m_DepthSeg;
private:
	void buildPlane(
		char u, char v, char w, 
		int udir, int vdir,  // uv dirs
		float width, float height, float depth, 
		int gridX, int gridY, // how much we subdivide
		int materialIndex  // for allowing groups within a single geometry. ignore for now
	);
};

struct SphereGeoUpdateData
{
	float radius;
	int widthSeg, heightSeg;
	float phiStart, phiLength;
	float thetaStart, thetaLength;
};

class SphereGeometry : public Geometry
{
public:
	SphereGeometry(
		float radius = 0.5, int widthSegments = 32, int heightSegments = 16, 
		float phiStart = 0.0, float phiLength = glm::pi<float>() * 2.0, // how much along circumference
		float thetaStart = 0.0, float thetaLength = glm::pi<float>()  // how much along central diameter
	);
	void UpdateParams(
		float radius, int widthSeg, int heightSeg, 
		float phiStart, float phiLength, float thetaStart, float thetaLength)
	{
		m_Dirty = true;
		m_Radius = radius; m_WidthSeg = widthSeg; m_HeightSeg = heightSeg;
		m_PhiStart = phiStart; m_PhiLength = phiLength;
		m_ThetaStart = thetaStart; m_ThetaLength = thetaLength;
	}
	virtual void BuildGeometry() override;  // given data, builds cpu-side index and vertex buffs
	virtual GeometryType GetGeoType() override { return GeometryType::Sphere; }
	virtual Geometry* Clone() override { 
		return new SphereGeometry(
			m_Radius, m_WidthSeg, m_HeightSeg, m_PhiStart, m_PhiLength, m_ThetaStart, m_ThetaLength
		); 
	}

	// update command methods
	virtual void * GenUpdate() override {
		return new SphereGeoUpdateData{
			m_Radius, m_WidthSeg, m_HeightSeg, m_PhiStart, m_PhiLength, m_ThetaStart, m_ThetaLength
		};
	};
	virtual void FreeUpdate(void* data) override {
		delete (SphereGeoUpdateData*)data;
	}
	virtual void ApplyUpdate(void * data) override {
		SphereGeoUpdateData * updateData = (SphereGeoUpdateData *)data;
		UpdateParams(
			updateData->radius, updateData->widthSeg, updateData->heightSeg, 
			updateData->phiStart, updateData->phiLength, updateData->thetaStart, updateData->thetaLength
		);
	};

public:
	// TODO: refactor this to just use struct type
	float m_Radius;
	int m_WidthSeg, m_HeightSeg;
	float m_PhiStart, m_PhiLength;
	float m_ThetaStart, m_ThetaLength;
};

