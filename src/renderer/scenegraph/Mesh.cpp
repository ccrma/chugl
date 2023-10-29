#include "chugl_pch.h"

#include "Mesh.h"
#include "Geometry.h"
#include "Material.h"
#include "SceneGraphNode.h"


Mesh::~Mesh()
{
    CHUGL_RELEASE(GetMaterial());
    CHUGL_RELEASE(GetGeometry());
}

Geometry *Mesh::GetGeometry()
{
    return (Geometry*) Locator::GetNode(m_GeoID, IsAudioThreadObject());
}

Material *Mesh::GetMaterial()
{
    auto* mat = (Material*) Locator::GetNode(m_MatID, IsAudioThreadObject());
    return mat;
}

void Mesh::SetGeometry(Geometry *geo)
{
    // mesh has a pointer to geo, so bump geo refcount
    CHUGL_ADD_REF(geo);

    // remove reference to previous geometry 
    // important: this should happen after add_ref in case
    // the incoming geometry is the same as the current one

    CHUGL_RELEASE(GetGeometry());

    m_GeoID = geo ? geo->GetID() : 0;
}

void Mesh::SetMaterial(Material *mat)
{
    // mesh has a pointer to material, so bump material refcount
    CHUGL_ADD_REF(mat);

    // remove reference to previous material
    // important: this should happen after add_ref in case
    // the incoming material is the same as the current one
    CHUGL_RELEASE(GetMaterial());

    m_MatID = mat ? mat->GetID() : 0;
}

size_t Mesh::GetMaterialID() { return m_MatID; }
size_t Mesh::GetGeometryID() { return m_GeoID; }