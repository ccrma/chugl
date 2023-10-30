#pragma once
#include "SceneGraphObject.h"

class Geometry;
class Material;

// container for a geometry + material
class Mesh : public SceneGraphObject
{
public:
	Mesh(Geometry *geo, Material* mat) : m_GeoID(0), m_MatID(0) {
		SetGeometry(geo);
		SetMaterial(mat);
	}

	Mesh() : m_GeoID(0), m_MatID(0) {
		// fprintf(stderr, "Mesh(%zu) default ctor \n", m_ID);
	}

	virtual ~Mesh();

	virtual SceneGraphNode* Clone() override { 
		assert(false);
		return nullptr;
		// TODO implement this after refactoring material and geo to be IDs
	}
	
	virtual bool IsMesh() override { return true; }

	Geometry* GetGeometry();
	Material* GetMaterial();

	void SetGeometry(Geometry* geo);
	void SetMaterial(Material* mat);

	size_t GetMaterialID();
	size_t GetGeometryID();

	// to get around lack of chuck constructors for now
	

private:
	// Geometry* m_Geometry;
	// Material* m_Material;
	size_t m_GeoID, m_MatID;
};
