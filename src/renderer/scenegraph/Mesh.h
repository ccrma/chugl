#pragma once
#include "SceneGraphObject.h"

class Geometry;
class Material;

// container for a geometry + material
class Mesh : public SceneGraphObject
{
public:
	Mesh(Geometry *geo, Material* mat) : m_Geometry(geo), m_Material(mat) {
		fprintf(stderr, "Mesh(%d) with geo mat\n", m_ID);
	}
	Mesh() : m_Geometry(nullptr), m_Material(nullptr) {
		fprintf(stderr, "Mesh(%d) default ctor \n", m_ID);
	}
	
	virtual bool IsMesh() override { return true; }

	inline Geometry* GetGeometry() { return m_Geometry; }
	inline Material* GetMaterial() { return m_Material; }

	inline void SetGeometry(Geometry* geo) { m_Geometry = geo; }
	inline void SetMaterial(Material* mat) { m_Material = mat; }
	
	// to get around lack of chuck constructors for now
	

private:
	Geometry* m_Geometry;
	Material* m_Material;
};
