#include "ulib_mesh.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "renderer/scenegraph/SceneGraphObject.h"

//-----------------------------------------------------------------------------
// Object -> Mesh
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_mesh_ctor);
CK_DLL_MFUN(cgl_mesh_set);
CK_DLL_MFUN(cgl_mesh_get_mat);
CK_DLL_MFUN(cgl_mesh_get_geo);
CK_DLL_MFUN(cgl_mesh_set_mat);
CK_DLL_MFUN(cgl_mesh_set_geo);

// duplicators
CK_DLL_MFUN(cgl_mesh_dup_mat);
CK_DLL_MFUN(cgl_mesh_dup_geo);
CK_DLL_MFUN(cgl_mesh_dup_all);

//-----------------------------------------------------------------------------
// Object -> Mesh -> GTypes
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_gcube_ctor);
CK_DLL_CTOR(cgl_gsphere_ctor);
CK_DLL_CTOR(cgl_gcircle_ctor);
CK_DLL_CTOR(cgl_gplane_ctor);
CK_DLL_CTOR(cgl_gtorus_ctor);
CK_DLL_CTOR(cgl_glines_ctor);
CK_DLL_CTOR(cgl_gpoints_ctor);

//-----------------------------------------------------------------------------
// init_chugl_mesh()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_mesh(Chuck_DL_Query *QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL scene");

	QUERY->begin_class(QUERY, "GMesh", "GGen");
	QUERY->doc_class(QUERY, "Mesh class. A mesh is a geometry and a material. It can be added to a scene to be rendered. Parent class of GCube, GSphere, GCircle, etc");

	QUERY->add_ctor(QUERY, cgl_mesh_ctor);

	QUERY->add_mfun(QUERY, cgl_mesh_set, "void", "set");
	QUERY->add_arg(QUERY, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->add_arg(QUERY, Material::CKName(MaterialType::Base), "mat");
	QUERY->doc_func(QUERY, "Set the geometry and material of the mesh");

	QUERY->add_mfun(QUERY, cgl_mesh_set_geo, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->add_arg(QUERY, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->doc_func(QUERY, "Set the mesh geometry");


	QUERY->add_mfun(QUERY, cgl_mesh_set_mat, Material::CKName(MaterialType::Base), "mat");
	QUERY->add_arg(QUERY, Material::CKName(MaterialType::Base), "mat");
	QUERY->doc_func(QUERY, "Set the mesh material");

	QUERY->add_mfun(QUERY, cgl_mesh_get_geo, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->doc_func(QUERY, "Get the mesh geometry");

	QUERY->add_mfun(QUERY, cgl_mesh_get_mat, Material::CKName(MaterialType::Base), "mat");
	QUERY->doc_func(QUERY, "Get the mesh material");

	QUERY->add_mfun(QUERY, cgl_mesh_dup_mat, Material::CKName(MaterialType::Base), "dupMat");
	QUERY->doc_func(QUERY, "Clone the mesh material and set it to this mesh");

	QUERY->add_mfun(QUERY, cgl_mesh_dup_geo, Geometry::CKName(GeometryType::Base) , "dupGeo");
	QUERY->doc_func(QUERY, "Clone the mesh geometry and set it to this mesh");

	QUERY->add_mfun(QUERY, cgl_mesh_dup_all, "GMesh", "dup");
	QUERY->doc_func(QUERY, "Clone both the mesh geometry and material and set it to this mesh");

	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GCube", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses BoxGeometry and PhongMaterial");

	QUERY->add_ctor(QUERY, cgl_gcube_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GSphere", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses SphereGeometry and PhongMaterial");
	QUERY->add_ctor(QUERY, cgl_gsphere_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GCircle", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses CircleGeometry and PhongMaterial");
	QUERY->add_ctor(QUERY, cgl_gcircle_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GPlane", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses PlaneGeometry and PhongMaterial");
	QUERY->add_ctor(QUERY, cgl_gplane_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GTorus", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses TorusGeometry and PhongMaterial");

	QUERY->add_ctor(QUERY, cgl_gtorus_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GLines", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses CustomGeometry and LineMaterial");
	QUERY->add_ex(QUERY, "basic/circles.ck");
    QUERY->add_ex(QUERY, "sndpeek/sndpeek-minimal.ck");
    QUERY->add_ex(QUERY, "sndpeek/sndpeek.ck");

	QUERY->add_ctor(QUERY, cgl_glines_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GPoints", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses CustomGeometry and PointMaterial");
	QUERY->add_ctor(QUERY, cgl_gpoints_ctor);
	QUERY->end_class(QUERY);

	return true;
}
// CGL Scene ==============================================
CK_DLL_CTOR(cgl_mesh_ctor)
{
	// Chuck_DL_Api::Type type = API->type->lookup(VM, "GMesh");
	// if (API->type->is_equal(type, API->object->get_type(SELF))) {
	// 	std::cerr << "GMesh instantiated" << std::endl;
	// }

	Mesh *mesh = new Mesh;
	CGL::PushCommand(new CreateMeshCommand(mesh, &CGL::mainScene, SELF, CGL::GetGGenDataOffset()));
}

static void cglMeshSet(Mesh *mesh, Geometry *geo, Material *mat)
{
	// set on CGL side
	mesh->SetGeometry(geo);
	mesh->SetMaterial(mat);
	// command queue to update renderer side
	CGL::PushCommand(new SetMeshCommand(mesh));
}

CK_DLL_MFUN(cgl_mesh_set)
{
	Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);
	// Geometry * geo = (Geometry *)GET_NEXT_OBJECT(ARGS);
	// Material * mat = (Material *)GET_NEXT_OBJECT(ARGS);
	Chuck_Object *geo_obj = GET_NEXT_OBJECT(ARGS);
	Chuck_Object *mat_obj = GET_NEXT_OBJECT(ARGS);

	Geometry *geo = geo_obj == nullptr ? nullptr : CGL::GetGeometry(geo_obj);
	Material *mat = mat_obj == nullptr ? nullptr : CGL::GetMaterial(mat_obj);

	cglMeshSet(mesh, geo, mat);
}

CK_DLL_MFUN(cgl_mesh_set_geo)
{
	Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);
	Chuck_Object *geo_obj = GET_NEXT_OBJECT(ARGS);
	Geometry *geo = geo_obj == nullptr ? nullptr : CGL::GetGeometry(geo_obj);

	mesh->SetGeometry(geo);

	RETURN->v_object = geo_obj;


	CGL::PushCommand(new SetMeshCommand(mesh));
}

CK_DLL_MFUN(cgl_mesh_set_mat)
{
	Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);
	Chuck_Object *mat_obj = GET_NEXT_OBJECT(ARGS);
	Material *mat = mat_obj == nullptr ? nullptr : CGL::GetMaterial(mat_obj);

	mesh->SetMaterial(mat);

	RETURN->v_object = mat_obj;


	CGL::PushCommand(new SetMeshCommand(mesh));
}

CK_DLL_MFUN(cgl_mesh_get_mat)
{
	Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);
	RETURN->v_object = mesh->GetMaterial()->m_ChuckObject;
}

CK_DLL_MFUN(cgl_mesh_get_geo)
{
	Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);
	RETURN->v_object = mesh->GetGeometry()->m_ChuckObject;
}



// duplicators
CK_DLL_MFUN(cgl_mesh_dup_mat)
{
	Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);
	RETURN->v_object = CGL::DupMeshMat(API, VM, mesh, SHRED)->m_ChuckObject;
}

CK_DLL_MFUN(cgl_mesh_dup_geo)
{
	Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);
	RETURN->v_object = CGL::DupMeshGeo(API, VM, mesh, SHRED)->m_ChuckObject;
}

CK_DLL_MFUN(cgl_mesh_dup_all)
{
	Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);
	CGL::DupMeshGeo(API, VM, mesh, SHRED);
	CGL::DupMeshMat(API, VM, mesh, SHRED);
	RETURN->v_object = SELF;
}

CK_DLL_CTOR(cgl_gcube_ctor)
{
    Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);

	Material* mat = new PhongMaterial;
	// don't refcount here because will be refcounted when we assign to mat
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, false);  
	Geometry* geo = new BoxGeometry;
	// don't refcount here because will be refcounted when we assign to mat
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, false);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gsphere_ctor)
{
    Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);

	Material* mat = new PhongMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new SphereGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gcircle_ctor)
{
    Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);

	Material* mat = new PhongMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new CircleGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gplane_ctor)
{
    Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);

	Material* mat = new PhongMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new PlaneGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gtorus_ctor)
{
    Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);

	Material* mat = new PhongMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new TorusGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_glines_ctor)
{
    Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);

	Material* mat = new LineMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new CustomGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

	std::vector<double> firstLine = {0, 0, 0, 0, 0, 0};

	// initialize with single line
	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, firstLine, false
		)
	);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gpoints_ctor)
{
    Mesh *mesh = (Mesh *) CGL::GetSGO(SELF);

	Material* mat = new PointsMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new CustomGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

	std::vector<double> firstPoint = {0, 0, 0};

	// initialize with single line
	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, firstPoint, false
		)
	);

    cglMeshSet(mesh, geo, mat);
}