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
    // if (m_Material) {
    //     CKAPI()->object->release( m_Material->m_ChuckObject );
    // }

    // if (m_Geometry) {
    //     CKAPI()->object->release( m_Geometry->m_ChuckObject );
    // }
}

void Mesh::SetGeometry(Geometry *geo)
{
    assert(geo);

    // mesh has a pointer to geo, so bump geo refcount
    // CKAPI()->object->add_ref( geo->m_ChuckObject );

    // remove reference to previous geometry 
    // important: this should happen after add_ref in case
    // the incoming geometry is the same as the current one
    // if (m_Geometry) {
    //     CKAPI()->object->release( m_Geometry->m_ChuckObject );
    // }

    m_Geometry = geo;
}

void Mesh::SetMaterial(Material *mat)
{
    assert(mat);

    // mesh has a pointer to material, so bump material refcount
    // CKAPI()->object->add_ref( mat->m_ChuckObject );

    // remove reference to previous material
    // important: this should happen after add_ref in case
    // the incoming material is the same as the current one
    // if (m_Material) {
    //     CKAPI()->object->release( m_Material->m_ChuckObject );
    // }

    m_Material = mat;
}

size_t Mesh::GetMaterialID() { return m_Material ? m_Material->GetID() : 0; }
size_t Mesh::GetGeometryID() { return m_Geometry ? m_Geometry->GetID() : 0; }