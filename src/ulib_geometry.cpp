#include "ulib_geometry.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "renderer/scenegraph/Geometry.h"

//-----------------------------------------------------------------------------
// Geometry API declarations
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_geo_ctor);
CK_DLL_DTOR(cgl_geo_dtor);
CK_DLL_MFUN(cgl_geo_clone);

// box
CK_DLL_CTOR(cgl_geo_box_ctor);
CK_DLL_MFUN(cgl_geo_box_set);

// sphere
CK_DLL_CTOR(cgl_geo_sphere_ctor);
CK_DLL_MFUN(cgl_geo_sphere_set);
// TODO: sphere parameter setter

// circle
CK_DLL_CTOR(cgl_geo_circle_ctor);
CK_DLL_MFUN(cgl_geo_circle_set);

// plane
CK_DLL_CTOR(cgl_geo_plane_ctor);
CK_DLL_MFUN(cgl_geo_plane_set);

// torus
CK_DLL_CTOR(cgl_geo_torus_ctor);
CK_DLL_MFUN(cgl_geo_torus_set);

// lathe
CK_DLL_CTOR(cgl_geo_lathe_ctor);
CK_DLL_MFUN(cgl_geo_lathe_set);
CK_DLL_MFUN(cgl_geo_lathe_set_no_points);

// cylinder
CK_DLL_CTOR(cgl_geo_cylinder_ctor);
CK_DLL_MFUN(cgl_geo_cylinder_set);

// triangle
CK_DLL_CTOR(cgl_geo_triangle_ctor);
CK_DLL_MFUN(cgl_geo_triangle_set);



// custom
CK_DLL_CTOR(cgl_geo_custom_ctor);
CK_DLL_MFUN(cgl_geo_set_attribute); // general case for any kind of vertex data
CK_DLL_MFUN(cgl_geo_set_positions);
CK_DLL_MFUN(cgl_geo_set_positions_vec3);
CK_DLL_MFUN(cgl_geo_set_colors);
CK_DLL_MFUN(cgl_geo_set_colors_vec3);
CK_DLL_MFUN(cgl_geo_set_colors_vec4);
CK_DLL_MFUN(cgl_geo_set_normals);
CK_DLL_MFUN(cgl_geo_set_normals_vec3);
CK_DLL_MFUN(cgl_geo_set_uvs);
CK_DLL_MFUN(cgl_geo_set_uvs_vec2);
CK_DLL_MFUN(cgl_geo_set_indices);

//-----------------------------------------------------------------------------
// Geometry API Impl
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_geometry(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Base) , "Object");
    QUERY->doc_class(QUERY, "Base geometry class, do not instantiate directly");
    QUERY->add_ex(QUERY, "basic/polygon-modes.ck");

	QUERY->add_ctor(QUERY, cgl_geo_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);
    CGL::SetGeometryDataOffset(QUERY->add_mvar(QUERY, "int", "@geometry_data", false));

	// attribute locations
	QUERY->add_svar(QUERY, "int", "POS_ATTRIB_LOC", TRUE, (void *)&Geometry::POSITION_ATTRIB_IDX);
	QUERY->add_svar(QUERY, "int", "NORM_ATTRIB_LOC", TRUE, (void *)&Geometry::NORMAL_ATTRIB_IDX);
	QUERY->add_svar(QUERY, "int", "COL_ATTRIB_LOC", TRUE, (void *)&Geometry::COLOR_ATTRIB_IDX);
	QUERY->add_svar(QUERY, "int", "UV0_ATTRIB_LOC", TRUE, (void *)&Geometry::UV0_ATTRIB_IDX);

	// clone
	QUERY->add_mfun(QUERY, cgl_geo_clone, Geometry::CKName(GeometryType::Base), "clone");
    QUERY->doc_func(QUERY, "clone the geometry, including all attributes");

	// attribute setters
	QUERY->add_mfun(QUERY, cgl_geo_set_attribute, "void", "setAttribute");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "location");
	QUERY->add_arg(QUERY, "int", "numComponents");
	// QUERY->add_arg(QUERY, "int", "normalize");
	QUERY->add_arg(QUERY, "float[]", "data");
    QUERY->doc_func(QUERY, "Set the attribute data for the given attribute location, to be passed into the vertex shader. Builtin attribute locations are POS_ATTRIB_LOC, NORM_ATTRIB_LOC, COL_ATTRIB_LOC, UV0_ATTRIB_LOC");

	QUERY->add_mfun(QUERY, cgl_geo_set_positions, "void", "positions");
	QUERY->add_arg(QUERY, "float[]", "positions");
    QUERY->doc_func(QUERY, "Set position attribute data from an array of floats; every 3 floats correspond to (x, y, z) values of a vertex position");

	QUERY->add_mfun(QUERY, cgl_geo_set_positions_vec3, "void", "positions");
	QUERY->add_arg(QUERY, "vec3[]", "positions");
    QUERY->doc_func(QUERY, "Set position attribute data from an array of vec3 (x, y, z)");

	QUERY->add_mfun(QUERY, cgl_geo_set_colors, "void", "colors");
	QUERY->add_arg(QUERY, "float[]", "colors");
    QUERY->doc_func(QUERY, "Set color attribute data from an array of floats; every 4 floats corresdpond to (r, g, b, a) values of a vertex color);" );

    QUERY->add_mfun(QUERY, cgl_geo_set_colors_vec3, "void", "colors");
    QUERY->add_arg(QUERY, "vec3[]", "colors");
    QUERY->doc_func(QUERY, "Set color attribute data from an array of vec3 (r, g, b); alpha is assumed to be 1.0" );

    QUERY->add_mfun(QUERY, cgl_geo_set_colors_vec4, "void", "colors");
    QUERY->add_arg(QUERY, "vec4[]", "colors");
    QUERY->doc_func(QUERY, "Set color attribute data from an array of vec4 (r, g, b, a)" );

	QUERY->add_mfun(QUERY, cgl_geo_set_normals, "void", "normals");
	QUERY->add_arg(QUERY, "float[]", "normals");
    QUERY->doc_func(QUERY, "Set normal attribute data from an array of floats; every 3 floats corresdpond to (x, y, z) values of a vertex normal");

    QUERY->add_mfun(QUERY, cgl_geo_set_normals_vec3, "void", "normals");
    QUERY->add_arg(QUERY, "vec3[]", "normals");
    QUERY->doc_func(QUERY, "Set normal attribute data from an array of vec3 (x, y, z)");

	QUERY->add_mfun(QUERY, cgl_geo_set_uvs, "void", "uvs");
	QUERY->add_arg(QUERY, "float[]", "uvs");
    QUERY->doc_func(QUERY, "Set UV attribute data from an array of floats; every pair of floats corresponds to (u, v) values (used for texture mapping)");

    QUERY->add_mfun(QUERY, cgl_geo_set_uvs_vec2, "void", "uvs");
    QUERY->add_arg(QUERY, "vec2[]", "uvs");
    QUERY->doc_func(QUERY, "Set UV attribute data from an array of vec2 (u,v) or (s,t)");

	QUERY->add_mfun(QUERY, cgl_geo_set_indices, "void", "indices");
	QUERY->add_arg(QUERY, "int[]", "indices");
    QUERY->doc_func(QUERY, "sets vertex indices for indexed drawing. If not set, renderer will default to non-indexed drawing");

	// TODO: add svars for attribute locations
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Box), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for boxes aka cubes");
	QUERY->add_ctor(QUERY, cgl_geo_box_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_box_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "width");
	QUERY->add_arg(QUERY, "float", "height");
	QUERY->add_arg(QUERY, "float", "depth");
	QUERY->add_arg(QUERY, "int", "widthSeg");
	QUERY->add_arg(QUERY, "int", "heightSeg");
	QUERY->add_arg(QUERY, "int", "depthSeg");
    QUERY->doc_func(QUERY, "Set box dimensions and subdivisions");

	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Sphere), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for spheres");
	QUERY->add_ctor(QUERY, cgl_geo_sphere_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_sphere_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radius");
	QUERY->add_arg(QUERY, "int", "widthSeg");
	QUERY->add_arg(QUERY, "int", "heightSeg");
	QUERY->add_arg(QUERY, "float", "phiStart");
	QUERY->add_arg(QUERY, "float", "phiLength");
	QUERY->add_arg(QUERY, "float", "thetaStart");
	QUERY->add_arg(QUERY, "float", "thetaLength");
    QUERY->doc_func(QUERY, "Set sphere dimensions and subdivisions");

	QUERY->end_class(QUERY);

	// circle geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Circle), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for circles");
	QUERY->add_ctor(QUERY, cgl_geo_circle_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_circle_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radius");
	QUERY->add_arg(QUERY, "int", "segments");
	QUERY->add_arg(QUERY, "float", "thetaStart");
	QUERY->add_arg(QUERY, "float", "thetaLength");
    QUERY->doc_func(QUERY, "Set circle dimensions and subdivisions");

	QUERY->end_class(QUERY);

	// plane geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Plane), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for planes");
	QUERY->add_ctor(QUERY, cgl_geo_plane_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_plane_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "width");
	QUERY->add_arg(QUERY, "float", "height");
	QUERY->add_arg(QUERY, "int", "widthSegments");
	QUERY->add_arg(QUERY, "int", "heightSegments");
    QUERY->doc_func(QUERY, "Set plane dimensions and subdivisions");

	QUERY->end_class(QUERY);

	// Torus geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Torus), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for toruses");
	QUERY->add_ctor(QUERY, cgl_geo_torus_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_torus_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radius");
	QUERY->add_arg(QUERY, "float", "tubeRadius");
	QUERY->add_arg(QUERY, "int", "radialSegments");
	QUERY->add_arg(QUERY, "int", "tubularSegments");
	QUERY->add_arg(QUERY, "float", "arcLength");
    QUERY->doc_func(QUERY, "Set torus dimensions and subdivisions");

	QUERY->end_class(QUERY);

	// lathe geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Lathe), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for lathes (i.e. rotated curves)");
	QUERY->add_ex(QUERY, "basic/polygon-modes.ck");

	QUERY->add_ctor(QUERY, cgl_geo_lathe_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_lathe_set, "void", "set");
	QUERY->add_arg(QUERY, "float[]", "path"); // these are converted to vec2s
	QUERY->add_arg(QUERY, "int", "segments");
	QUERY->add_arg(QUERY, "float", "phiStart");
	QUERY->add_arg(QUERY, "float", "phiLength");
    QUERY->doc_func(QUERY, 
		"Set lathe curve, dimensions and subdivisions. Path is rotated phiLength to form a curved surface"
		"NOTE: path takes a float[] of alternating x,y values, describing a 2D curve in the x,y plane"
		"These values are rotated around the y-axis to form a 3D surface"
	);

	QUERY->add_mfun(QUERY, cgl_geo_lathe_set_no_points, "void", "set");
	QUERY->add_arg(QUERY, "int", "segments");
	QUERY->add_arg(QUERY, "float", "phiStart");
	QUERY->add_arg(QUERY, "float", "phiLength");
    QUERY->doc_func(QUERY, "Set lathe dimensions and subdivisions while maintaining the previously set curve");

	QUERY->end_class(QUERY);

	// cylinder geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Cylinder), Geometry::CKName(GeometryType::Base) );
	QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for cylinders");
	QUERY->add_ex(QUERY, "geometry/cylinder.ck");

	QUERY->add_ctor(QUERY, cgl_geo_cylinder_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_cylinder_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radiusTop");
	QUERY->add_arg(QUERY, "float", "radiusBottom");
	QUERY->add_arg(QUERY, "float", "height");
	QUERY->add_arg(QUERY, "int", "radialSegments");
	QUERY->add_arg(QUERY, "int", "heightSegments");
	QUERY->add_arg(QUERY, "int", "openEnded");
	QUERY->add_arg(QUERY, "float", "thetaStart");
	QUERY->add_arg(QUERY, "float", "thetaLength");
	QUERY->doc_func(QUERY, "Set cylinder dimensions and subdivisions");
	
	QUERY->end_class(QUERY);

	// triangle geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Triangle), Geometry::CKName(GeometryType::Base) );
	QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for triangles");
	QUERY->add_ex(QUERY, "geometry/triangle.ck");

	QUERY->add_ctor(QUERY, cgl_geo_triangle_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_triangle_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "theta");
	QUERY->add_arg(QUERY, "float", "width");
	QUERY->add_arg(QUERY, "float", "height");
	QUERY->doc_func(QUERY, 
		"Set triangle construction params."
		"Theta is the angle in radians betweeen the base and the hypotenuse"
		"Width is the length of the base"
		"Height is distance from base to the opposite vertex"
	);

	QUERY->end_class(QUERY);

	// custom geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Custom), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for providing your own vertex data. Used implicitly by GLines and GPoints");
    QUERY->add_ex(QUERY, "basic/custom-geo.ck");
    QUERY->add_ex(QUERY, "basic/obj-loader.ck");

	QUERY->add_ctor(QUERY, cgl_geo_custom_ctor);


	QUERY->end_class(QUERY);

	return true;
}

// CGL Geometry =======================
CK_DLL_CTOR(cgl_geo_ctor) {}
CK_DLL_DTOR(cgl_geo_dtor) // all geos can share this base destructor
{
	CGL::PushCommand(
        new DestroySceneGraphNodeCommand(
            SELF, CGL::GetGeometryDataOffset(), API, &CGL::mainScene
        )
    );
}

CK_DLL_MFUN(cgl_geo_clone)
{
	Geometry *geo = CGL::GetGeometry(SELF);
	// Note: we are NOT refcounting here because we're returning a reference to the new cloned object
	// If this returned reference is assigned to a chuck variable, chuck should handle the refcounting
	// bumping the refcount here would cause a memory leak, as the refcount would never be decremented
	RETURN->v_object = CGL::CreateChuckObjFromGeo(API, VM, (Geometry *)geo->Clone(), SHRED, false)->m_ChuckObject;
}


// box geo
CK_DLL_CTOR(cgl_geo_box_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new BoxGeometry, &CGL::mainScene, SELF,  CGL::GetGeometryDataOffset(), API));
}

CK_DLL_MFUN(cgl_geo_box_set)
{
	BoxGeometry *geo = (BoxGeometry*) CGL::GetGeometry(SELF);
	t_CKFLOAT width = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT height = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT depth = GET_NEXT_FLOAT(ARGS);
	t_CKINT widthSeg = GET_NEXT_INT(ARGS);
	t_CKINT heightSeg = GET_NEXT_INT(ARGS);
	t_CKINT depthSeg = GET_NEXT_INT(ARGS);
	geo->UpdateParams(width, height, depth, widthSeg, heightSeg, depthSeg);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// sphere geo
CK_DLL_CTOR(cgl_geo_sphere_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new SphereGeometry, &CGL::mainScene, SELF, CGL::GetGeometryDataOffset(), API));
}

CK_DLL_MFUN(cgl_geo_sphere_set)
{
	SphereGeometry *geo = (SphereGeometry*) CGL::GetGeometry(SELF);
	t_CKFLOAT radius = GET_NEXT_FLOAT(ARGS);
	t_CKINT widthSeg = GET_NEXT_INT(ARGS);
	t_CKINT heightSeg = GET_NEXT_INT(ARGS);
	t_CKFLOAT phiStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT phiLength = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT thetaStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT thetaLength = GET_NEXT_FLOAT(ARGS);
	geo->UpdateParams(radius, widthSeg, heightSeg, phiStart, phiLength, thetaStart, thetaLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// Circle geo ---------
CK_DLL_CTOR(cgl_geo_circle_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new CircleGeometry, &CGL::mainScene, SELF, CGL::GetGeometryDataOffset(), API));
}

CK_DLL_MFUN(cgl_geo_circle_set)
{
	CircleGeometry *geo = (CircleGeometry *) CGL::GetGeometry(SELF);
	t_CKFLOAT radius = GET_NEXT_FLOAT(ARGS);
	t_CKINT segments = GET_NEXT_INT(ARGS);
	t_CKFLOAT thetaStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT thetaLength = GET_NEXT_FLOAT(ARGS);
	geo->UpdateParams(radius, segments, thetaStart, thetaLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// plane geo ----------
CK_DLL_CTOR(cgl_geo_plane_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new PlaneGeometry, &CGL::mainScene, SELF, CGL::GetGeometryDataOffset(), API));
}

CK_DLL_MFUN(cgl_geo_plane_set)
{
	PlaneGeometry *geo = (PlaneGeometry *) CGL::GetGeometry(SELF);
	t_CKFLOAT width = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT height = GET_NEXT_FLOAT(ARGS);
	t_CKINT widthSegments = GET_NEXT_INT(ARGS);
	t_CKINT heightSegments = GET_NEXT_INT(ARGS);
	geo->UpdateParams(width, height, widthSegments, heightSegments);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// torus geo  ----------
CK_DLL_CTOR(cgl_geo_torus_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new TorusGeometry, &CGL::mainScene, SELF, CGL::GetGeometryDataOffset(), API));
}

CK_DLL_MFUN(cgl_geo_torus_set)
{
	TorusGeometry *geo = (TorusGeometry *) CGL::GetGeometry(SELF);
	t_CKFLOAT radius = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT tubeRadius = GET_NEXT_FLOAT(ARGS);
	t_CKINT radialSegments = GET_NEXT_INT(ARGS);
	t_CKINT tubularSegments = GET_NEXT_INT(ARGS);
	t_CKFLOAT arcLength = GET_NEXT_FLOAT(ARGS);
	geo->UpdateParams(radius, tubeRadius, radialSegments, tubularSegments, arcLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// Lathe geo ----------
CK_DLL_CTOR(cgl_geo_lathe_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new LatheGeometry, &CGL::mainScene, SELF, CGL::GetGeometryDataOffset(), API));
}

CK_DLL_MFUN(cgl_geo_lathe_set)
{
	LatheGeometry * geo = (LatheGeometry *) CGL::GetGeometry(SELF);
	Chuck_ArrayFloat * points = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);
	t_CKINT segments = GET_NEXT_INT(ARGS);
	t_CKFLOAT phiStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT phiLength = GET_NEXT_FLOAT(ARGS);

    // TODO: extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_float_size(points);
    std::vector<t_CKFLOAT> thePoints; thePoints.reserve(vsize);
    for( t_CKINT idx = 0; idx < vsize; idx++ )
    { thePoints .push_back( API->object->array_float_get_idx(points,idx) ); }
    
    // update geo
	geo->UpdateParams(thePoints, segments, phiStart, phiLength);
    // push command
	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

CK_DLL_MFUN(cgl_geo_lathe_set_no_points)
{
	LatheGeometry *geo = (LatheGeometry *) CGL::GetGeometry(SELF);
	t_CKINT segments = GET_NEXT_INT(ARGS);
	t_CKFLOAT phiStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT phiLength = GET_NEXT_FLOAT(ARGS);

	geo->UpdateParams(segments, phiStart, phiLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// Cylinder geo ----------

CK_DLL_CTOR(cgl_geo_cylinder_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new CylinderGeometry, &CGL::mainScene, SELF, CGL::GetGeometryDataOffset(), API));
}

CK_DLL_MFUN(cgl_geo_cylinder_set)
{
	CylinderGeometry *geo = (CylinderGeometry *) CGL::GetGeometry(SELF);
	t_CKFLOAT radiusTop = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT radiusBottom = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT height = GET_NEXT_FLOAT(ARGS);
	t_CKINT radialSegments = GET_NEXT_INT(ARGS);
	t_CKINT heightSegments = GET_NEXT_INT(ARGS);
	t_CKBOOL openEnded = GET_NEXT_INT(ARGS);
	t_CKFLOAT thetaStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT thetaLength = GET_NEXT_FLOAT(ARGS);
	geo->UpdateParams(radiusTop, radiusBottom, height, radialSegments, heightSegments, openEnded, thetaStart, thetaLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// Triangle geo -------------------------------------------------
CK_DLL_CTOR(cgl_geo_triangle_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new TriangleGeometry, &CGL::mainScene, SELF, CGL::GetGeometryDataOffset(), API));
}

CK_DLL_MFUN(cgl_geo_triangle_set)
{
	TriangleGeometry *geo = (TriangleGeometry *) CGL::GetGeometry(SELF);
	t_CKFLOAT theta = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT width = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT height = GET_NEXT_FLOAT(ARGS);
	geo->UpdateParams(theta, width, height);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// Custom geo ---------
CK_DLL_CTOR(cgl_geo_custom_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new CustomGeometry, &CGL::mainScene, SELF, CGL::GetGeometryDataOffset(), API));
}

CK_DLL_MFUN(cgl_geo_set_attribute)
{
	CustomGeometry *geo = (CustomGeometry *) CGL::GetGeometry(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	t_CKINT location = GET_NEXT_INT(ARGS);
	t_CKINT numComponents = GET_NEXT_INT(ARGS);
	bool normalize = GET_NEXT_INT(ARGS);
	Chuck_ArrayFloat * ckarray = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

	// not stored in chuck-side copy to save time
	// geo->SetAttribute(name, location, numComponents, normalize, data);

    // TODO: extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_float_size(ckarray);
    std::vector<t_CKFLOAT> theVec; theVec.reserve(vsize);
    for( t_CKINT idx = 0; idx < vsize; idx++ )
    { theVec.push_back( API->object->array_float_get_idx(ckarray,idx) ); }
    
	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, API->object->str(name), location, numComponents, theVec, normalize));
}

CK_DLL_MFUN(cgl_geo_set_positions)
{
	CustomGeometry * geo = (CustomGeometry *) CGL::GetGeometry(SELF);
	Chuck_ArrayFloat * ckarray = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

    // TODO: extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_float_size(ckarray);
    std::vector<t_CKFLOAT> theVec; theVec.reserve(vsize);
    for( t_CKINT idx = 0; idx < vsize; idx++ )
    { theVec.push_back( API->object->array_float_get_idx(ckarray,idx) ); }
    
	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, theVec, false));
}

CK_DLL_MFUN(cgl_geo_set_positions_vec3)
{
	CustomGeometry * geo = (CustomGeometry *) CGL::GetGeometry(SELF);
	auto * ckarray = (Chuck_ArrayVec3 *)GET_NEXT_OBJECT(ARGS);

    // TODO extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_vec3_size(ckarray);
    std::vector<t_CKFLOAT> vec3s;
    vec3s.reserve(3 * vsize);
    t_CKVEC3 val;
    for( t_CKINT idx = 0; idx < vsize; idx++ ) {
        val = API->object->array_vec3_get_idx(ckarray,idx);
        vec3s.emplace_back(val.x);
        vec3s.emplace_back(val.y);
        vec3s.emplace_back(val.z);
    }

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, vec3s, false
		)
	);
}

// set colors
CK_DLL_MFUN(cgl_geo_set_colors)
{
	CustomGeometry * geo = (CustomGeometry *) CGL::GetGeometry(SELF);
	Chuck_ArrayFloat * ckarray = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

    // TODO: extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_float_size(ckarray);
    std::vector<t_CKFLOAT> theVec; theVec.reserve(vsize);
    for( t_CKINT idx = 0; idx < vsize; idx++ )
    { theVec.push_back( API->object->array_float_get_idx(ckarray,idx) ); }

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "color", Geometry::COLOR_ATTRIB_IDX, 4, theVec, false)
    );
}

CK_DLL_MFUN(cgl_geo_set_colors_vec3)
{
    CustomGeometry * geo = (CustomGeometry *) CGL::GetGeometry(SELF);
    auto * ckarray = (Chuck_ArrayVec3*)GET_NEXT_OBJECT(ARGS);

    // TODO extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_vec3_size(ckarray);
    std::vector<t_CKFLOAT> vec4s;
    vec4s.reserve(4 * vsize);
    t_CKVEC3 val;
    for( t_CKINT idx = 0; idx < vsize; idx++ ) {
        val = API->object->array_vec3_get_idx(ckarray,idx);
        vec4s.emplace_back(val.x);
        vec4s.emplace_back(val.y);
        vec4s.emplace_back(val.z);
        vec4s.emplace_back(1.0);
    }

    CGL::PushCommand(
        new UpdateGeometryAttributeCommand(
            geo, "color", Geometry::COLOR_ATTRIB_IDX, 4, vec4s, false
        )
    );
}

CK_DLL_MFUN(cgl_geo_set_colors_vec4)
{
    CustomGeometry * geo = (CustomGeometry *) CGL::GetGeometry(SELF);
    auto * ckarray = (Chuck_ArrayVec4 *)GET_NEXT_OBJECT(ARGS);

    // TODO extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_vec4_size(ckarray);
    std::vector<t_CKFLOAT> vec4s;
    vec4s.reserve(4 * vsize);
    t_CKVEC4 val;
    for( t_CKINT idx = 0; idx < vsize; idx++ ) {
        val = API->object->array_vec4_get_idx(ckarray,idx);
        vec4s.emplace_back(val.x);
        vec4s.emplace_back(val.y);
        vec4s.emplace_back(val.z);
        vec4s.emplace_back(val.w);
    }

    CGL::PushCommand(
        new UpdateGeometryAttributeCommand(
            geo, "color", Geometry::COLOR_ATTRIB_IDX, 4, vec4s, false
        )
    );
}

// set normals
CK_DLL_MFUN(cgl_geo_set_normals)
{
	CustomGeometry * geo = (CustomGeometry *) CGL::GetGeometry(SELF);
	Chuck_ArrayFloat * ckarray = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

    // TODO: extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_float_size(ckarray);
    std::vector<t_CKFLOAT> theVec; theVec.reserve(vsize);
    for( t_CKINT idx = 0; idx < vsize; idx++ )
    { theVec.push_back(API->object->array_float_get_idx(ckarray,idx)); }

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "normal", Geometry::NORMAL_ATTRIB_IDX, 3, theVec, false)
    );
}

// set normals
CK_DLL_MFUN(cgl_geo_set_normals_vec3)
{
    CustomGeometry *geo = (CustomGeometry *) CGL::GetGeometry(SELF);
    auto * ckarray = (Chuck_ArrayVec3*)GET_NEXT_OBJECT(ARGS);

    // TODO extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_vec3_size(ckarray);
    std::vector<t_CKFLOAT> vec3s;
    vec3s.reserve(3 * vsize);
    t_CKVEC3 val;
    for( t_CKINT idx = 0; idx < vsize; idx++ ) {
        val = API->object->array_vec3_get_idx(ckarray,idx);
        vec3s.emplace_back(val.x);
        vec3s.emplace_back(val.y);
        vec3s.emplace_back(val.z);
    }

    CGL::PushCommand(
        new UpdateGeometryAttributeCommand(
            geo, "normal", Geometry::NORMAL_ATTRIB_IDX, 3, vec3s, false)
    );
}

// set uvs
CK_DLL_MFUN(cgl_geo_set_uvs)
{
	CustomGeometry * geo = (CustomGeometry *) CGL::GetGeometry(SELF);
	Chuck_ArrayFloat * ckarray = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

    // TODO: extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_float_size(ckarray);
    std::vector<t_CKFLOAT> theVec; theVec.reserve(vsize);
    for( t_CKINT idx = 0; idx < vsize; idx++ )
    { theVec.push_back(API->object->array_float_get_idx(ckarray,idx)); }
    
	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "uv", Geometry::UV0_ATTRIB_IDX, 2, theVec, false));
}

// set uvs
CK_DLL_MFUN(cgl_geo_set_uvs_vec2)
{
    CustomGeometry * geo = (CustomGeometry *)CGL::GetGeometry(SELF);
    auto * ckarray = (Chuck_ArrayVec2 *)GET_NEXT_OBJECT(ARGS);

    // TODO extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_vec2_size(ckarray);
    std::vector<t_CKFLOAT> vec2s; vec2s.reserve(2 * vsize);
    t_CKVEC2 val;
    for( t_CKINT idx = 0; idx < vsize; idx++ ) {
        val = API->object->array_vec2_get_idx(ckarray,idx);
        vec2s.emplace_back(val.x);
        vec2s.emplace_back(val.y);
    }

    CGL::PushCommand(
        new UpdateGeometryAttributeCommand(
            geo, "uv", Geometry::UV0_ATTRIB_IDX, 2, vec2s, false) );
}

// set indices
CK_DLL_MFUN(cgl_geo_set_indices)
{
    CustomGeometry * geo = (CustomGeometry *) CGL::GetGeometry(SELF);
    Chuck_ArrayInt * ckarray = (Chuck_ArrayInt *)GET_NEXT_OBJECT(ARGS);

    // TODO: extra round of copying here, can avoid if it matters
    t_CKINT vsize = API->object->array_int_size(ckarray);
    std::vector<t_CKUINT> theVec; theVec.reserve(vsize);
    for( t_CKINT idx = 0; idx < vsize; idx++ )
    { theVec.push_back( (t_CKUINT)API->object->array_int_get_idx(ckarray,idx)); }

    CGL::PushCommand(
        new UpdateGeometryIndicesCommand(geo, theVec) );
}
