#include "Mesh.h"
#include "Geometry.h"
#include "Material.h"


size_t Mesh::GetMaterialID() { return m_Material ? m_Material->GetID() : 0; }
size_t Mesh::GetGeometryID() { return m_Geometry ? m_Geometry->GetID() : 0; }