#pragma once
#include "SceneGraphObject.h"

class Geometry;
class Material;

// container for a geometry + material
class Mesh : public SceneGraphObject
{
public:
	Mesh(Geometry *geo, Material* mat) {
		// fprintf(stderr, "Mesh(%zu) with geo mat\n", m_ID);
		SetGeometry(geo);
		SetMaterial(mat);
	}

	Mesh() : m_Geometry(nullptr), m_Material(nullptr) {
		// fprintf(stderr, "Mesh(%zu) default ctor \n", m_ID);
	}

	virtual ~Mesh();
	
	virtual bool IsMesh() override { return true; }

	inline Geometry* GetGeometry() { return m_Geometry; }
	inline Material* GetMaterial() { return m_Material; }

	void SetGeometry(Geometry* geo);
	void SetMaterial(Material* mat);

	size_t GetMaterialID();
	size_t GetGeometryID();

	// to get around lack of chuck constructors for now
	

private:
// TODO: should these be IDs instead?
	Geometry* m_Geometry;
	Material* m_Material;
};
