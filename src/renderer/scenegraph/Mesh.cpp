#include "Mesh.h"
#include "Geometry.h"
#include "Material.h"
#include "SceneGraphNode.h"

#include "chuck_dl.h"

Mesh::~Mesh()
{
    // TODO: only do refcounting if we're the chuck-side copy.
    // this is because release and add_ref are not thread-safe
    // decrement any resources we're holding onto

    CHUGL_RELEASE(m_Material);
    // if (m_Material) {
    //     CKAPI()->object->release( m_Material->m_ChuckObject );
    // }

    CHUGL_RELEASE(m_Geometry);
    // if (m_Geometry) {
    //     CKAPI()->object->release( m_Geometry->m_ChuckObject );
    // }
}

void Mesh::SetGeometry(Geometry *geo)
{
    // mesh has a pointer to geo, so bump geo refcount
    CHUGL_ADD_REF(geo);
    // if (geo) {
    //     CKAPI()->object->add_ref( geo->m_ChuckObject );
    // }

    // remove reference to previous geometry 
    // important: this should happen after add_ref in case
    // the incoming geometry is the same as the current one

    CHUGL_RELEASE(m_Geometry);
    // if (m_Geometry) {
    //     CKAPI()->object->release( m_Geometry->m_ChuckObject );
    // }

    m_Geometry = geo;
}

void Mesh::SetMaterial(Material *mat)
{
    // mesh has a pointer to material, so bump material refcount
    CHUGL_ADD_REF(mat);
    // if (mat) {
    //     CKAPI()->object->add_ref( mat->m_ChuckObject );
    // }

    // remove reference to previous material
    // important: this should happen after add_ref in case
    // the incoming material is the same as the current one
    CHUGL_RELEASE(m_Material);
    // if (m_Material) {
    //     CKAPI()->object->release( m_Material->m_ChuckObject );
    // }

    m_Material = mat;
}

size_t Mesh::GetMaterialID() { return m_Material ? m_Material->GetID() : 0; }
size_t Mesh::GetGeometryID() { return m_Geometry ? m_Geometry->GetID() : 0; }