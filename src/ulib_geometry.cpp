
#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "geometry.h"

#define GET_GEOMETRY(ckobj) SG_GetGeometry(OBJ_MEMBER_UINT(ckobj, component_offset_id))

// ===============================================================
// Geometry  (for now, immutable)
// ===============================================================

CK_DLL_CTOR(geo_ctor);

CK_DLL_MFUN(geo_set_vertex_attribute);
CK_DLL_MFUN(geo_set_vertex_attribute_int);

CK_DLL_MFUN(geo_set_positions);
CK_DLL_MFUN(geo_set_normals);
CK_DLL_MFUN(geo_set_uvs);
CK_DLL_MFUN(geo_set_tangents);
CK_DLL_MFUN(geo_get_positions);
CK_DLL_MFUN(geo_get_normals);
CK_DLL_MFUN(geo_get_uvs);
CK_DLL_MFUN(geo_get_tangents);

CK_DLL_MFUN(geo_get_vertex_attribute_num_components);
CK_DLL_MFUN(geo_get_vertex_attribute_num_components_all);

CK_DLL_MFUN(geo_get_vertex_attribute_data);
CK_DLL_MFUN(geo_get_vertex_attribute_data_int);

CK_DLL_MFUN(geo_set_indices);
CK_DLL_MFUN(geo_get_indices);

CK_DLL_MFUN(geo_set_vertex_count);

CK_DLL_MFUN(geo_set_pulled_vertex_attribute);
CK_DLL_MFUN(geo_set_pulled_vertex_attribute_vec2);
CK_DLL_MFUN(geo_set_pulled_vertex_attribute_vec3);
CK_DLL_MFUN(geo_set_pulled_vertex_attribute_vec4);
CK_DLL_MFUN(geo_set_pulled_vertex_attribute_int);
CK_DLL_MFUN(geo_get_pulled_vertex_attribute);
CK_DLL_MFUN(geo_get_pulled_vertex_attribute_int);

CK_DLL_MFUN(geo_generate_tangents);

// end Geometry -----------------------------------------------------

CK_DLL_CTOR(plane_geo_ctor);
CK_DLL_CTOR(plane_geo_ctor_params);
CK_DLL_MFUN(plane_geo_build);
CK_DLL_MFUN(plane_geo_get_width);
CK_DLL_MFUN(plane_geo_get_height);
CK_DLL_MFUN(plane_geo_get_widthSegments);
CK_DLL_MFUN(plane_geo_get_heightSegments);

CK_DLL_CTOR(sphere_geo_ctor);
CK_DLL_CTOR(sphere_geo_ctor_params);
CK_DLL_MFUN(sphere_geo_build);
CK_DLL_MFUN(sphere_geo_get_radius);
CK_DLL_MFUN(sphere_geo_get_widthSeg);
CK_DLL_MFUN(sphere_geo_get_heightSeg);
CK_DLL_MFUN(sphere_geo_get_phiStart);
CK_DLL_MFUN(sphere_geo_get_phiLength);
CK_DLL_MFUN(sphere_geo_get_thetaStart);
CK_DLL_MFUN(sphere_geo_get_thetaLength);

CK_DLL_CTOR(suzanne_geo_ctor);

CK_DLL_CTOR(lines2d_geo_ctor);
CK_DLL_CTOR(lines2d_geo_ctor_params);
CK_DLL_MFUN(lines2d_geo_set_line_points);
CK_DLL_MFUN(lines2d_geo_set_line_colors);

CK_DLL_CTOR(box_geo_ctor);
CK_DLL_CTOR(box_geo_ctor_params);
CK_DLL_MFUN(box_geo_build);
CK_DLL_MFUN(box_geo_get_width);
CK_DLL_MFUN(box_geo_get_height);
CK_DLL_MFUN(box_geo_get_depth);
CK_DLL_MFUN(box_geo_get_width_segments);
CK_DLL_MFUN(box_geo_get_height_segments);
CK_DLL_MFUN(box_geo_get_depth_segments);

CK_DLL_CTOR(circle_geo_ctor);
CK_DLL_CTOR(circle_geo_ctor_params);
CK_DLL_MFUN(circle_geo_build);
CK_DLL_MFUN(circle_geo_get_radius);
CK_DLL_MFUN(circle_geo_get_segments);
CK_DLL_MFUN(circle_geo_get_thetaStart);
CK_DLL_MFUN(circle_geo_get_thetaLength);

CK_DLL_CTOR(torus_geo_ctor);
CK_DLL_CTOR(torus_geo_ctor_params);
CK_DLL_MFUN(torus_geo_build);
CK_DLL_MFUN(torus_geo_get_radius);
CK_DLL_MFUN(torus_geo_get_tubeRadius);
CK_DLL_MFUN(torus_geo_get_radialSegments);
CK_DLL_MFUN(torus_geo_get_tubularSegments);
CK_DLL_MFUN(torus_geo_get_arcLength);

CK_DLL_CTOR(cylinder_geo_ctor);
CK_DLL_CTOR(cylinder_geo_ctor_params);
CK_DLL_MFUN(cylinder_geo_build);
CK_DLL_MFUN(cylinder_geo_get_radiusTop);
CK_DLL_MFUN(cylinder_geo_get_radiusBottom);
CK_DLL_MFUN(cylinder_geo_get_height);
CK_DLL_MFUN(cylinder_geo_get_radialSegments);
CK_DLL_MFUN(cylinder_geo_get_heightSegments);
CK_DLL_MFUN(cylinder_geo_get_openEnded);
CK_DLL_MFUN(cylinder_geo_get_thetaStart);
CK_DLL_MFUN(cylinder_geo_get_thetaLength);

CK_DLL_CTOR(knot_geo_ctor);
CK_DLL_CTOR(knot_geo_ctor_params);
CK_DLL_MFUN(knot_geo_build);
CK_DLL_MFUN(knot_geo_get_radius);
CK_DLL_MFUN(knot_geo_get_tube);
CK_DLL_MFUN(knot_geo_get_tubularSegments);
CK_DLL_MFUN(knot_geo_get_radialSegments);
CK_DLL_MFUN(knot_geo_get_p);
CK_DLL_MFUN(knot_geo_get_q);

static void ulib_geometry_query(Chuck_DL_Query* QUERY)
{
    // Geometry -----------------------------------------------------
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_GEOMETRY], SG_CKNames[SG_COMPONENT_BASE]);

    // contants
    static t_CKINT sg_geometry_max_attributes{ SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES };
    static t_CKINT pos_attr_loc{ SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION };
    static t_CKINT norm_attr_loc{ SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION };
    static t_CKINT uv_attr_loc{ SG_GEOMETRY_UV_ATTRIBUTE_LOCATION };
    static t_CKINT tangent_attr_loc{ SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION };

    SVAR("int", "AttributeLocation_Count", &sg_geometry_max_attributes);
    DOC_VAR("Maximum number of vertex attributes.");

    SVAR("int", "AttributeLocation_Position", &pos_attr_loc);
    DOC_VAR("Position attribute location used by builtin renderer");

    SVAR("int", "AttributeLocation_Normal", &norm_attr_loc);
    DOC_VAR("Normal attribute location used by builtin renderer");

    SVAR("int", "AttributeLocation_UV", &uv_attr_loc);
    DOC_VAR("UV attribute location used by builtin renderer");

    SVAR("int", "AttributeLocation_Tangent", &tangent_attr_loc);
    DOC_VAR("Tangent attribute location used by builtin renderer");

    // ctor
    CTOR(geo_ctor);

    // mfuns
    MFUN(geo_set_vertex_attribute, "void", "vertexAttribute");
    ARG("int", "location");
    ARG("int", "numComponents");
    ARG("float[]", "attributeData");
    DOC_FUNC(
      "Set the vertex attribute data for a geometry. location must be between "
      "0 and AttributeLocation_Max.");

    MFUN(geo_set_vertex_attribute_int, "void", "vertexAttribute");
    ARG("int", "location");
    ARG("int", "numComponents");
    ARG("int[]", "attributeData");
    DOC_FUNC(
      "Set the vertex attribute data for a geometry. location must be between "
      "0 and Geometry.AttributeLocation_Count");

    MFUN(geo_set_positions, "void", "positions");
    ARG("vec3[]", "positions");
    DOC_FUNC(
      "Set the positions for a geometry. Equivalent to vertexAttribute(0, 3, "
      "positions)");

    MFUN(geo_set_normals, "void", "normals");
    ARG("vec3[]", "normals");
    DOC_FUNC(
      "Set the normals for a geometry. Equivalent to vertexAttribute(1, 3, "
      "normals)");

    MFUN(geo_set_uvs, "void", "uvs");
    ARG("vec2[]", "uvs");
    DOC_FUNC("Set the UVs for a geometry. Equivalent to vertexAttribute(2, 2, uvs)");

    MFUN(geo_set_tangents, "void", "tangents");
    ARG("vec4[]", "tangents");
    DOC_FUNC(
      "Set the tangents for a geometry. Equivalent to vertexAttribute(3, 4, "
      "tangents)");

    MFUN(geo_get_positions, "vec3[]", "positions");
    DOC_FUNC(
      "Get the positions for a geometry. Equivalent to getVertexAttributeData(0)");

    MFUN(geo_get_normals, "vec3[]", "normals");
    DOC_FUNC("Get the normals for a geometry. Equivalent to getVertexAttributeData(1)");

    MFUN(geo_get_uvs, "vec2[]", "uvs");
    DOC_FUNC("Get the UVs for a geometry. Equivalent to getVertexAttributeData(2)");

    MFUN(geo_get_tangents, "vec4[]", "tangents");
    DOC_FUNC(
      "Get the tangents for a geometry. Equivalent to getVertexAttributeData(3)");

    MFUN(geo_get_vertex_attribute_num_components, "int",
         "vertexAttributeNumComponents");
    ARG("int", "location");
    DOC_FUNC("Get the number of components for a vertex attribute. Default 0.");

    MFUN(geo_get_vertex_attribute_num_components_all, "int[]",
         "vertexAttributeNumComponents");
    DOC_FUNC("Get the number of components for all vertex attributes.");

    MFUN(geo_get_vertex_attribute_data, "float[]", "vertexAttributeData");
    ARG("int", "location");
    DOC_FUNC("Get the vertex data for a vertex attribute. Default empty array.");

    MFUN(geo_get_vertex_attribute_data_int, "int[]", "vertexAttributeDataInt");
    ARG("int", "location");
    DOC_FUNC("Get the vertex data for a vertex attribute. Default empty array.");

    MFUN(geo_set_indices, "void", "indices");
    ARG("int[]", "indices");
    DOC_FUNC("Set the indices for a geometry.");

    MFUN(geo_get_indices, "int[]", "indices");
    DOC_FUNC("Get the indices for a geometry.");

    MFUN(geo_set_pulled_vertex_attribute, "void", "pulledVertexAttribute");
    ARG("int", "location");
    ARG("float[]", "data");
    DOC_FUNC(
      "Set storage buffer for programmable vertex pulling.  pulled attributes will be "
      "set as storage buffers bound to @group(3) in the shader. Location must be "
      "between 0-3");

    MFUN(geo_set_pulled_vertex_attribute_vec2, "void", "pulledVertexAttribute");
    ARG("int", "location");
    ARG("vec2[]", "data");
    DOC_FUNC(
      "Set storage buffer for programmable vertex pulling.  pulled attributes will be "
      "set as storage buffers bound to @group(3) in the shader. Location must be "
      "between 0-3");

    MFUN(geo_set_pulled_vertex_attribute_vec3, "void", "pulledVertexAttribute");
    ARG("int", "location");
    ARG("vec3[]", "data");
    DOC_FUNC(
      "Set storage buffer for programmable vertex pulling.  pulled attributes will be "
      "set as storage buffers bound to @group(3) in the shader. Location must be "
      "between 0-3");

    MFUN(geo_set_pulled_vertex_attribute_vec4, "void", "pulledVertexAttribute");
    ARG("int", "location");
    ARG("vec4[]", "data");
    DOC_FUNC(
      "Set storage buffer for programmable vertex pulling.  pulled attributes will be "
      "set as storage buffers bound to @group(3) in the shader. Location must be "
      "between 0-3");

    MFUN(geo_set_pulled_vertex_attribute_int, "void", "pulledVertexAttribute");
    ARG("int", "location");
    ARG("int[]", "data");
    DOC_FUNC(
      "Set storage buffer for programmable vertex pulling.  pulled attributes will be "
      "set as storage buffers bound to @group(3) in the shader. Location must be "
      "between 0-3");

    MFUN(geo_get_pulled_vertex_attribute, "float[]", "pulledVertexAttribute");
    ARG("int", "location");
    DOC_FUNC(
      "Get the pulled vertex attribute data at this location. location must be "
      "between 0-3");

    MFUN(geo_get_pulled_vertex_attribute_int, "int[]", "pulledVertexAttributeInt");
    ARG("int", "location");
    DOC_FUNC(
      "Get the pulled vertex attribute data at this location. location must be "
      "between 0-3");

    MFUN(geo_set_vertex_count, "void", "vertexCount");
    ARG("int", "count");
    DOC_FUNC(
      "If this geometry uses vertex pulling or non-indexed drawing, "
      "Set the number of vertices to be drawn of this geometry by the renderer"
      "Default is -1, which means all vertices are drawn. Values will be clamped to "
      "the actual number of vertices in the geometry.");

    MFUN(geo_generate_tangents, "void", "generateTangents");
    DOC_FUNC(
      "Generate tangents assuming default vertex attribute locations: "
      "position=0, normal=1, uv=2, tangent=3. Tangents are written to location = 3."
      "Indices are supported but optional."
      " Call *after* all other attribute data and indices have been supplied.");

    END_CLASS();

    // Plane -----------------------------------------------------
    QUERY->begin_class(QUERY, SG_GeometryTypeNames[SG_GEOMETRY_PLANE],
                       SG_CKNames[SG_COMPONENT_GEOMETRY]);

    QUERY->add_ctor(QUERY, plane_geo_ctor);
    QUERY->add_ctor(QUERY, plane_geo_ctor_params);
    QUERY->add_arg(QUERY, "float", "width");
    QUERY->add_arg(QUERY, "float", "height");
    QUERY->add_arg(QUERY, "int", "widthSegments");
    QUERY->add_arg(QUERY, "int", "heightSegments");
    QUERY->doc_func(QUERY, "Set plane dimensions and subdivisions");

    MFUN(plane_geo_build, "void", "build");
    ARG("float", "width");
    ARG("float", "height");
    ARG("int", "widthSegments");
    ARG("int", "heightSegments");
    DOC_FUNC("Set plane dimensions and subdivisions");

    MFUN(plane_geo_get_width, "float", "width");
    DOC_FUNC("Get the plane width.");

    MFUN(plane_geo_get_height, "float", "height");
    DOC_FUNC("Get the plane height.");

    MFUN(plane_geo_get_widthSegments, "int", "widthSegments");
    DOC_FUNC("Get the number of segments along the width.");

    MFUN(plane_geo_get_heightSegments, "int", "heightSegments");
    DOC_FUNC("Get the number of segments along the height.");

    QUERY->end_class(QUERY);

    // Sphere -----------------------------------------------------
    QUERY->begin_class(QUERY, SG_GeometryTypeNames[SG_GEOMETRY_SPHERE],
                       SG_CKNames[SG_COMPONENT_GEOMETRY]);

    QUERY->add_ctor(QUERY, sphere_geo_ctor);
    QUERY->add_ctor(QUERY, sphere_geo_ctor_params);
    QUERY->add_arg(QUERY, "float", "radius");
    QUERY->add_arg(QUERY, "int", "widthSeg");
    QUERY->add_arg(QUERY, "int", "heightSeg");
    QUERY->add_arg(QUERY, "float", "phiStart");
    QUERY->add_arg(QUERY, "float", "phiLength");
    QUERY->add_arg(QUERY, "float", "thetaStart");
    QUERY->add_arg(QUERY, "float", "thetaLength");
    QUERY->doc_func(QUERY, "Set sphere dimensions and subdivisions");

    QUERY->add_mfun(QUERY, sphere_geo_build, "void", "build");
    QUERY->add_arg(QUERY, "float", "radius");
    QUERY->add_arg(QUERY, "int", "widthSeg");
    QUERY->add_arg(QUERY, "int", "heightSeg");
    QUERY->add_arg(QUERY, "float", "phiStart");
    QUERY->add_arg(QUERY, "float", "phiLength");
    QUERY->add_arg(QUERY, "float", "thetaStart");
    QUERY->add_arg(QUERY, "float", "thetaLength");
    QUERY->doc_func(QUERY, "Set sphere dimensions and subdivisions");

    MFUN(sphere_geo_get_radius, "float", "radius");
    DOC_FUNC("Get the sphere radius.");

    MFUN(sphere_geo_get_widthSeg, "int", "widthSegments");
    DOC_FUNC("Get the number of segments along the width.");

    MFUN(sphere_geo_get_heightSeg, "int", "heightSegments");
    DOC_FUNC("Get the number of segments along the height.");

    MFUN(sphere_geo_get_phiStart, "float", "phiStart");
    DOC_FUNC("Get the starting angle of the sphere along the circumference.");

    MFUN(sphere_geo_get_phiLength, "float", "phiLength");
    DOC_FUNC("Get the length of the sphere along the circumference.");

    MFUN(sphere_geo_get_thetaStart, "float", "thetaStart");
    DOC_FUNC("Get the starting angle of the sphere along the central diameter.");

    MFUN(sphere_geo_get_thetaLength, "float", "thetaLength");
    DOC_FUNC("Get the length of the sphere along the central diameter.");

    QUERY->end_class(QUERY);

    // Suzanne ----------
    BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_SUZANNE],
                SG_CKNames[SG_COMPONENT_GEOMETRY]);

    CTOR(suzanne_geo_ctor);
    END_CLASS();

    // lines2d
    BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_LINES2D],
                SG_CKNames[SG_COMPONENT_GEOMETRY]);

    CTOR(lines2d_geo_ctor);

    // CTOR(lines2d_geo_ctor_params);
    // ARG("vec2[]", "points");
    // DOC_FUNC("construct with line points");

    MFUN(lines2d_geo_set_line_points, "void", "linePositions");
    ARG("vec2[]", "points");
    DOC_FUNC("Set the line positions. Z values are fixed to 0.0");

    MFUN(lines2d_geo_set_line_colors, "void", "lineColors");
    ARG("vec3[]", "colors");
    DOC_FUNC(
      "Set the line colors. Colors between line points are interpolated. If the number "
      "of colors is less than the number of points, the vertices will cycle through "
      "the colors.");

    END_CLASS();

    // box -----------------------------------------------------
    {
        BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_CUBE],
                    SG_CKNames[SG_COMPONENT_GEOMETRY]);

        CTOR(box_geo_ctor);

        CTOR(box_geo_ctor_params);
        ARG("float", "width");
        ARG("float", "height");
        ARG("float", "depth");
        ARG("int", "widthSegments");
        ARG("int", "heightSegments");
        ARG("int", "depthSegments");
        DOC_FUNC("Create box with given dimensions and subdivisions");

        MFUN(box_geo_build, "void", "build");
        ARG("float", "width");
        ARG("float", "height");
        ARG("float", "depth");
        ARG("int", "widthSegments");
        ARG("int", "heightSegments");
        ARG("int", "depthSegments");
        DOC_FUNC("Rebuild box dimensions and subdivisions");

        MFUN(box_geo_get_width, "float", "width");
        DOC_FUNC("Get the box width.");

        MFUN(box_geo_get_height, "float", "height");
        DOC_FUNC("Get the box height.");

        MFUN(box_geo_get_depth, "float", "depth");
        DOC_FUNC("Get the box depth.");

        MFUN(box_geo_get_width_segments, "int", "widthSegments");
        DOC_FUNC("Get the number of segments along the width.");

        MFUN(box_geo_get_height_segments, "int", "heightSegments");
        DOC_FUNC("Get the number of segments along the height.");

        MFUN(box_geo_get_depth_segments, "int", "depthSegments");
        DOC_FUNC("Get the number of segments along the depth.");

        END_CLASS();
    }

    // circle --------------------------------------------------------
    {

        BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_CIRCLE],
                    SG_CKNames[SG_COMPONENT_GEOMETRY]);

        CTOR(circle_geo_ctor);

        CTOR(circle_geo_ctor_params);
        ARG("float", "radius");
        ARG("int", "segments");
        ARG("float", "thetaStart");
        ARG("float", "thetaLength");

        MFUN(circle_geo_build, "void", "build");
        ARG("float", "radius");
        ARG("int", "segments");
        ARG("float", "thetaStart");
        ARG("float", "thetaLength");

        MFUN(circle_geo_get_radius, "float", "radius");
        DOC_FUNC("Get the circle radius.");

        MFUN(circle_geo_get_segments, "int", "segments");
        DOC_FUNC("Get the number of segments.");

        MFUN(circle_geo_get_thetaStart, "float", "thetaStart");
        DOC_FUNC("Get the starting angle of the circle.");

        MFUN(circle_geo_get_thetaLength, "float", "thetaLength");
        DOC_FUNC("Get the length of the circle.");

        END_CLASS();
    }

    // torus --------------------------------------------------------
    {
        BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_TORUS],
                    SG_CKNames[SG_COMPONENT_GEOMETRY]);

        CTOR(torus_geo_ctor);

        CTOR(torus_geo_ctor_params);
        ARG("float", "radius");
        ARG("float", "tubeRadius");
        ARG("int", "radialSegments");
        ARG("int", "tubularSegments");
        ARG("float", "arcLength");

        MFUN(torus_geo_build, "void", "build");
        ARG("float", "radius");
        ARG("float", "tubeRadius");
        ARG("int", "radialSegments");
        ARG("int", "tubularSegments");
        ARG("float", "arcLength");

        MFUN(torus_geo_get_radius, "float", "radius");
        DOC_FUNC("Get the torus radius.");

        MFUN(torus_geo_get_tubeRadius, "float", "tubeRadius");
        DOC_FUNC("Get the torus tube radius.");

        MFUN(torus_geo_get_radialSegments, "int", "radialSegments");
        DOC_FUNC("Get the number of radial segments.");

        MFUN(torus_geo_get_tubularSegments, "int", "tubularSegments");
        DOC_FUNC("Get the number of tubular segments.");

        MFUN(torus_geo_get_arcLength, "float", "arcLength");
        DOC_FUNC("Get the arc length.");

        END_CLASS();
    }

    // cylinder --------------------------------------------------------
    {

        BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_CYLINDER],
                    SG_CKNames[SG_COMPONENT_GEOMETRY]);

        CTOR(cylinder_geo_ctor);

        CTOR(cylinder_geo_ctor_params);
        ARG("float", "radiusTop");
        ARG("float", "radiusBottom");
        ARG("float", "height");
        ARG("int", "radialSegments");
        ARG("int", "heightSegments");
        ARG("int", "openEnded");
        ARG("float", "thetaStart");
        ARG("float", "thetaLength");

        MFUN(cylinder_geo_build, "void", "build");
        ARG("float", "radiusTop");
        ARG("float", "radiusBottom");
        ARG("float", "height");
        ARG("int", "radialSegments");
        ARG("int", "heightSegments");
        ARG("int", "openEnded");
        ARG("float", "thetaStart");
        ARG("float", "thetaLength");

        MFUN(cylinder_geo_get_radiusTop, "float", "radiusTop");
        DOC_FUNC("Get the cylinder top radius.");

        MFUN(cylinder_geo_get_radiusBottom, "float", "radiusBottom");
        DOC_FUNC("Get the cylinder bottom radius.");

        MFUN(cylinder_geo_get_height, "float", "height");
        DOC_FUNC("Get the cylinder height.");

        MFUN(cylinder_geo_get_radialSegments, "int", "radialSegments");
        DOC_FUNC("Get the number of radial segments.");

        MFUN(cylinder_geo_get_heightSegments, "int", "heightSegments");
        DOC_FUNC("Get the number of height segments.");

        MFUN(cylinder_geo_get_openEnded, "int", "openEnded");
        DOC_FUNC("Get the cylinder open ended flag.");

        MFUN(cylinder_geo_get_thetaStart, "float", "thetaStart");
        DOC_FUNC("Get the starting angle of the cylinder.");

        MFUN(cylinder_geo_get_thetaLength, "float", "thetaLength");
        DOC_FUNC("Get the length of the cylinder.");

        END_CLASS();
    }

    // knot --------------------------------------------------------
    {
        BEGIN_CLASS(SG_GeometryTypeNames[SG_GEOMETRY_KNOT],
                    SG_CKNames[SG_COMPONENT_GEOMETRY]);

        CTOR(knot_geo_ctor);

        CTOR(knot_geo_ctor_params);
        ARG("float", "radius");
        ARG("float", "tube");
        ARG("int", "tubularSegments");
        ARG("int", "radialSegments");
        ARG("int", "p");
        ARG("int", "q");

        MFUN(knot_geo_build, "void", "build");
        ARG("float", "radius");
        ARG("float", "tube");
        ARG("int", "tubularSegments");
        ARG("int", "radialSegments");
        ARG("int", "p");
        ARG("int", "q");

        MFUN(knot_geo_get_radius, "float", "radius");
        DOC_FUNC("Get the knot radius.");

        MFUN(knot_geo_get_tube, "float", "tube");
        DOC_FUNC("Get the knot tube radius.");

        MFUN(knot_geo_get_tubularSegments, "int", "tubularSegments");
        DOC_FUNC("Get the number of tubular segments.");

        MFUN(knot_geo_get_radialSegments, "int", "radialSegments");
        DOC_FUNC("Get the number of radial segments.");

        MFUN(knot_geo_get_p, "int", "p");
        DOC_FUNC("Get the knot p value.");

        MFUN(knot_geo_get_q, "int", "q");
        DOC_FUNC("Get the knot q value.");

        END_CLASS();
    }
}

// Geometry -----------------------------------------------------

// if params is NULL, uses default values
static void ulib_geometry_build(SG_Geometry* geo, SG_GeometryType geo_type,
                                void* params)
{
    geo->geo_type = geo_type;

    switch (geo->geo_type) {
        case SG_GEOMETRY: {
            // custom geometry has no setup
        }; break;
        case SG_GEOMETRY_PLANE: {
            PlaneParams p = {};
            if (params) p = *(PlaneParams*)params;
            SG_Geometry::buildPlane(geo, &p);
        } break;
        case SG_GEOMETRY_CUBE: {
            BoxParams p = {};
            if (params) p = *(BoxParams*)params;
            SG_Geometry::buildBox(geo, &p);
        } break;
        case SG_GEOMETRY_CIRCLE: {
            CircleParams p = {};
            if (params) p = *(CircleParams*)params;
            SG_Geometry::buildCircle(geo, &p);
        } break;
        case SG_GEOMETRY_SPHERE: {
            SphereParams p = {};
            if (params) p = *(SphereParams*)params;
            SG_Geometry::buildSphere(geo, &p);
        } break;
        case SG_GEOMETRY_CYLINDER: {
            CylinderParams p = {};
            if (params) p = *(CylinderParams*)params;
            SG_Geometry::buildCylinder(geo, &p);
        } break;
        case SG_GEOMETRY_TORUS: {
            TorusParams p = {};
            if (params) p = *(TorusParams*)params;
            SG_Geometry::buildTorus(geo, &p);
        } break;
        case SG_GEOMETRY_SUZANNE: {
            SG_Geometry::buildSuzanne(geo);
        } break;
        case SG_GEOMETRY_KNOT: {
            KnotParams p = {};
            if (params) p = *(KnotParams*)params;
            SG_Geometry::buildKnot(geo, &p);
        } break;
        case SG_GEOMETRY_LINES2D: {
            // set default vertex positions and colors
            ulib_geo_lines2d_set_lines_points(
              geo, (Chuck_Object*)g_builtin_ckobjs.init_2d_pos);
            ulib_geo_lines2d_set_line_colors(
              geo, (Chuck_Object*)g_builtin_ckobjs.init_white_color);
        } break;
        default: ASSERT(false);
    }

    CQ_UpdateAllVertexAttributes(geo);
}

SG_Geometry* ulib_geometry_create(SG_GeometryType type, Chuck_VM_Shred* shred)
{
    CK_DL_API API = g_chuglAPI;

    Chuck_Object* obj = chugin_createCkObj(SG_GeometryTypeNames[type], false, shred);

    SG_Geometry* geo                          = SG_CreateGeometry(obj);
    OBJ_MEMBER_UINT(obj, component_offset_id) = geo->id;

    CQ_PushCommand_GeometryCreate(geo);

    ulib_geometry_build(geo, type, NULL);

    return geo;
}

CK_DLL_CTOR(geo_ctor)
{
    SG_Geometry* geo                           = SG_CreateGeometry(SELF);
    geo->geo_type                              = SG_GEOMETRY;
    OBJ_MEMBER_UINT(SELF, component_offset_id) = geo->id;
    CQ_PushCommand_GeometryCreate(geo);
}

CK_DLL_MFUN(geo_set_vertex_attribute)
{
    t_CKINT location         = GET_NEXT_INT(ARGS);
    t_CKINT num_components   = GET_NEXT_INT(ARGS);
    Chuck_ArrayFloat* ck_arr = GET_NEXT_FLOAT_ARRAY(ARGS);

    /*
    idea: pass ck array of doubles into SG_Geometry::setAttribute directly.
    have SG_Geometry copy and convert f64 -> f32

    then push to command queue, passing a pointer to this f32 array

    the command queue copies the array into the command queue arena

    renderer writes data from command queue arena to GPU buffer and DOES NOT
    need to free because arena memory is cleared on each frame
    */

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    // set attribute locally
    Arena* vertex_attrib_data = SG_Geometry::setAttribute(
      geo, location, num_components, API, (Chuck_Object*)ck_arr, 1, false);

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(geo, location, num_components,
                                              vertex_attrib_data->base,
                                              vertex_attrib_data->curr);
}

CK_DLL_MFUN(geo_set_vertex_attribute_int)
{
    t_CKINT location       = GET_NEXT_INT(ARGS);
    t_CKINT num_components = GET_NEXT_INT(ARGS);
    Chuck_ArrayInt* ck_arr = GET_NEXT_INT_ARRAY(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    // set attribute locally
    Arena* vertex_attrib_data = SG_Geometry::setAttribute(
      geo, location, num_components, API, (Chuck_Object*)ck_arr, 1, true);

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(geo, location, num_components,
                                              vertex_attrib_data->base,
                                              vertex_attrib_data->curr);
}

CK_DLL_MFUN(geo_set_positions)
{
    Chuck_ArrayVec3* ck_arr = GET_NEXT_VEC3_ARRAY(ARGS);
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    Arena* attrib_arena
      = SG_Geometry::setAttribute(geo, SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION, 3, API,
                                  (Chuck_Object*)ck_arr, 3, false);

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(
      geo, SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION, 3, attrib_arena->base,
      attrib_arena->curr);
}

CK_DLL_MFUN(geo_set_normals)
{
    Chuck_ArrayVec3* ck_arr = GET_NEXT_VEC3_ARRAY(ARGS);
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    Arena* attrib_arena
      = SG_Geometry::setAttribute(geo, SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION, 3, API,
                                  (Chuck_Object*)ck_arr, 3, false);

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(geo,
                                              SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION, 3,
                                              attrib_arena->base, attrib_arena->curr);
}

CK_DLL_MFUN(geo_set_uvs)
{
    Chuck_ArrayVec2* ck_arr = GET_NEXT_VEC2_ARRAY(ARGS);
    SG_Geometry* geo    = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    Arena* attrib_arena = SG_Geometry::setAttribute(
      geo, SG_GEOMETRY_UV_ATTRIBUTE_LOCATION, 2, API, (Chuck_Object*)ck_arr, 2, false);

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(geo, SG_GEOMETRY_UV_ATTRIBUTE_LOCATION, 2,
                                              attrib_arena->base, attrib_arena->curr);
}

CK_DLL_MFUN(geo_set_tangents)
{
    Chuck_ArrayVec4* ck_arr = GET_NEXT_VEC4_ARRAY(ARGS);
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    Arena* attrib_arena
      = SG_Geometry::setAttribute(geo, SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION, 4, API,
                                  (Chuck_Object*)ck_arr, 4, false);

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(geo,
                                              SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION, 4,
                                              attrib_arena->base, attrib_arena->curr);
}

CK_DLL_MFUN(geo_get_positions)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    glm::vec3* data
      = (glm::vec3*)geo->vertex_attribute_data[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION]
          .base;
    int data_count = ARENA_LENGTH(
      &geo->vertex_attribute_data[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION], glm::vec3);

    RETURN->v_object = (Chuck_Object*)chugin_createCkFloat3Array(data, data_count);
}

CK_DLL_MFUN(geo_get_normals)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    glm::vec3* data
      = (glm::vec3*)geo->vertex_attribute_data[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION]
          .base;
    int data_count = ARENA_LENGTH(
      &geo->vertex_attribute_data[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION], glm::vec3);

    RETURN->v_object = (Chuck_Object*)chugin_createCkFloat3Array(data, data_count);
}

CK_DLL_MFUN(geo_get_uvs)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    glm::vec2* data
      = (glm::vec2*)geo->vertex_attribute_data[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION].base;
    int data_count = ARENA_LENGTH(
      &geo->vertex_attribute_data[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION], glm::vec2);

    RETURN->v_object = (Chuck_Object*)chugin_createCkFloat2Array(data, data_count);
}

CK_DLL_MFUN(geo_get_tangents)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    glm::vec4* data
      = (glm::vec4*)geo->vertex_attribute_data[SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION]
          .base;
    int data_count = ARENA_LENGTH(
      &geo->vertex_attribute_data[SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION], glm::vec4);

    RETURN->v_object = (Chuck_Object*)chugin_createCkFloat4Array(data, data_count);
}

CK_DLL_MFUN(geo_get_vertex_attribute_num_components)
{
    t_CKINT location = GET_NEXT_INT(ARGS);
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    RETURN->v_int = geo->vertex_attribute_num_components[location];
}

CK_DLL_MFUN(geo_get_vertex_attribute_num_components_all)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    Chuck_ArrayInt* ck_arr
      = chugin_createCkIntArray(geo->vertex_attribute_num_components,
                                ARRAY_LENGTH(geo->vertex_attribute_num_components));

    RETURN->v_object = (Chuck_Object*)ck_arr;
}

CK_DLL_MFUN(geo_get_vertex_attribute_data)
{
    t_CKINT location = GET_NEXT_INT(ARGS);
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    f32* data      = (f32*)geo->vertex_attribute_data[location].base;
    int data_count = ARENA_LENGTH(&geo->vertex_attribute_data[location], f32);

    RETURN->v_object = (Chuck_Object*)chugin_createCkFloatArray(data, data_count);
}

CK_DLL_MFUN(geo_get_vertex_attribute_data_int)
{
    t_CKINT location = GET_NEXT_INT(ARGS);
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    i32* data      = (i32*)geo->vertex_attribute_data[location].base;
    int data_count = ARENA_LENGTH(&geo->vertex_attribute_data[location], i32);

    RETURN->v_object = (Chuck_Object*)chugin_createCkIntArray(data, data_count);
}

CK_DLL_MFUN(geo_set_indices)
{
    Chuck_ArrayInt* ck_arr = GET_NEXT_INT_ARRAY(ARGS);
    t_CKINT ck_arr_len     = API->object->array_int_size(ck_arr);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    u32* indices = SG_Geometry::setIndices(geo, API, ck_arr, ck_arr_len);

    CQ_PushCommand_GeometrySetIndices(geo, indices, ck_arr_len);
}

CK_DLL_MFUN(geo_get_indices)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    u32* indices    = SG_Geometry::getIndices(geo);
    int index_count = SG_Geometry::indexCount(geo);

    Chuck_ArrayInt* ck_arr = (Chuck_ArrayInt*)chugin_createCkObj("int[]", false, SHRED);
    for (int i = 0; i < index_count; i++)
        API->object->array_int_push_back(ck_arr, indices[i]);

    RETURN->v_object = (Chuck_Object*)ck_arr;
}

void ulib_geo_set_pulled_vertex_attribute_data(SG_Geometry* geo, t_CKINT location,
                                               f32* data, int data_len)
{
    // store locally on SG_Geometry
    Arena* pull_buffer = &geo->vertex_pull_buffers[location];
    Arena::clear(pull_buffer);

    f32* local_data = ARENA_PUSH_COUNT(pull_buffer, f32, data_len);
    memcpy(local_data, data, data_len * sizeof(*data));

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetPulledVertexAttribute(geo, location, local_data,
                                                    pull_buffer->curr);
}

void geoSetPulledVertexAttribute(SG_Geometry* geo, t_CKINT location,
                                 Chuck_Object* ck_arr, int num_components, bool is_int)
{
    int ck_arr_len = 0;

    // store locally on SG_Geometry
    Arena* pull_buffer = &geo->vertex_pull_buffers[location];
    Arena::clear(pull_buffer);

    if (is_int) {
        ASSERT(num_components == 1);
        ck_arr_len
          = ck_arr ? g_chuglAPI->object->array_int_size((Chuck_ArrayInt*)ck_arr) : 0;
        i32* data = ARENA_PUSH_COUNT(pull_buffer, i32, ck_arr_len * num_components);
        chugin_copyCkIntArray((Chuck_ArrayInt*)ck_arr, data, ck_arr_len);
    } else {
        f32* local_data = NULL;
        switch (num_components) {
            case 1: {
                ck_arr_len
                  = ck_arr ?
                      g_chuglAPI->object->array_float_size((Chuck_ArrayFloat*)ck_arr) :
                      0;
                local_data
                  = ARENA_PUSH_COUNT(pull_buffer, f32, ck_arr_len * num_components);
                chugin_copyCkFloatArray((Chuck_ArrayFloat*)ck_arr, local_data,
                                        ck_arr_len);
            } break;
            case 2: {
                ck_arr_len
                  = ck_arr ?
                      g_chuglAPI->object->array_vec2_size((Chuck_ArrayVec2*)ck_arr) :
                      0;
                local_data
                  = ARENA_PUSH_COUNT(pull_buffer, f32, ck_arr_len * num_components);
                chugin_copyCkVec2Array((Chuck_ArrayVec2*)ck_arr, local_data);
            } break;
            case 3: {
                ck_arr_len
                  = ck_arr ?
                      g_chuglAPI->object->array_vec3_size((Chuck_ArrayVec3*)ck_arr) :
                      0;
                local_data
                  = ARENA_PUSH_COUNT(pull_buffer, f32, ck_arr_len * num_components);
                chugin_copyCkVec3Array((Chuck_ArrayVec3*)ck_arr, local_data);
            } break;
            case 4: {
                ck_arr_len
                  = ck_arr ?
                      g_chuglAPI->object->array_vec4_size((Chuck_ArrayVec4*)ck_arr) :
                      0;
                local_data
                  = ARENA_PUSH_COUNT(pull_buffer, f32, ck_arr_len * num_components);
                chugin_copyCkVec4Array((Chuck_ArrayVec4*)ck_arr, local_data);
            } break;
            default: ASSERT(false);
        }
    }

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetPulledVertexAttribute(geo, location, pull_buffer->base,
                                                    pull_buffer->curr);
}

CK_DLL_MFUN(geo_set_pulled_vertex_attribute)
{
    t_CKINT location     = GET_NEXT_INT(ARGS);
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    geoSetPulledVertexAttribute(geo, location, ck_arr, 1, false);
}

CK_DLL_MFUN(geo_set_pulled_vertex_attribute_vec2)
{
    t_CKINT location     = GET_NEXT_INT(ARGS);
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    geoSetPulledVertexAttribute(geo, location, ck_arr, 2, false);
}

CK_DLL_MFUN(geo_set_pulled_vertex_attribute_vec3)
{
    t_CKINT location     = GET_NEXT_INT(ARGS);
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    geoSetPulledVertexAttribute(geo, location, ck_arr, 3, false);
}

CK_DLL_MFUN(geo_set_pulled_vertex_attribute_vec4)
{
    t_CKINT location     = GET_NEXT_INT(ARGS);
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    geoSetPulledVertexAttribute(geo, location, ck_arr, 4, false);
}

CK_DLL_MFUN(geo_set_pulled_vertex_attribute_int)
{
    t_CKINT location     = GET_NEXT_INT(ARGS);
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    geoSetPulledVertexAttribute(geo, location, ck_arr, 1, true);
}

CK_DLL_MFUN(geo_get_pulled_vertex_attribute)
{
    t_CKINT location = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    Arena* pull_buffer = &geo->vertex_pull_buffers[location];

    RETURN->v_object = (Chuck_Object*)chugin_createCkFloatArray(
      (f32*)pull_buffer->base, ARENA_LENGTH(pull_buffer, f32));
}

CK_DLL_MFUN(geo_get_pulled_vertex_attribute_int)
{
    t_CKINT location = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    Arena* pull_buffer = &geo->vertex_pull_buffers[location];

    RETURN->v_object = (Chuck_Object*)chugin_createCkIntArray(
      (i32*)pull_buffer->base, ARENA_LENGTH(pull_buffer, i32));
}

CK_DLL_MFUN(geo_set_vertex_count)
{
    t_CKINT count = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    CQ_PushCommand_GeometrySetVertexCount(geo, count);
}

CK_DLL_MFUN(geo_generate_tangents)
{
    SG_Geometry* g = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    // check num components to make sure they match expected values
    if (g->vertex_attribute_num_components[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION] != 3
        || g->vertex_attribute_num_components[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION]
             != 3
        || g->vertex_attribute_num_components[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION] != 2) {
        CK_THROW("tangentGenerationException",
                 "Tangent generation requires position, normal, and uv "
                 "attributes to have 3, 3, and 2 components respectively, and be set "
                 "at locations 0, 1, and 2 respectively.",
                 SHRED);
        return;
    }

    GeometryArenaBuilder b = {};
    // set arena pointers
    b.pos_arena  = &g->vertex_attribute_data[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION];
    b.norm_arena = &g->vertex_attribute_data[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION];
    b.uv_arena   = &g->vertex_attribute_data[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION];
    b.tangent_arena = &g->vertex_attribute_data[SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION];
    b.indices_arena = &g->indices;

    // clear arenas and resize
    Arena::clear(b.tangent_arena);
    ARENA_PUSH_COUNT(b.tangent_arena, f32, SG_Geometry::vertexCount(g) * 4);

    // set num components
    g->vertex_attribute_num_components[SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION] = 4;

    // generate tangents
    Geometry_computeTangents(&b);

    // push to command queue
    CQ_PushCommand_GeometrySetVertexAttribute(g, SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION,
                                              4, b.tangent_arena->base,
                                              b.tangent_arena->curr);
}

// Plane Geometry -----------------------------------------------------

void CQ_UpdateAllVertexAttributes(SG_Geometry* geo)
{ // push attribute changes to command queue
    ASSERT(geo);
    Arena* positions_arena
      = &geo->vertex_attribute_data[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION];
    CQ_PushCommand_GeometrySetVertexAttribute(
      geo, SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION, 3, positions_arena->base,
      positions_arena->curr);

    Arena* normals_arena
      = &geo->vertex_attribute_data[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION];
    CQ_PushCommand_GeometrySetVertexAttribute(geo,
                                              SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION, 3,
                                              normals_arena->base, normals_arena->curr);

    Arena* uvs_arena = &geo->vertex_attribute_data[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION];
    CQ_PushCommand_GeometrySetVertexAttribute(geo, SG_GEOMETRY_UV_ATTRIBUTE_LOCATION, 2,
                                              uvs_arena->base, uvs_arena->curr);

    Arena* tangents_arena
      = &geo->vertex_attribute_data[SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION];
    CQ_PushCommand_GeometrySetVertexAttribute(
      geo, SG_GEOMETRY_TANGENT_ATTRIBUTE_LOCATION, 4, tangents_arena->base,
      tangents_arena->curr);

    Arena* indices_arena = &geo->indices;
    if (indices_arena->curr > 0) {
        CQ_PushCommand_GeometrySetIndices(geo, (u32*)indices_arena->base,
                                          ARENA_LENGTH(indices_arena, u32));
    }
}

CK_DLL_CTOR(plane_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    ASSERT(geo);

    ulib_geometry_build(geo, SG_GEOMETRY_PLANE, NULL);
}

CK_DLL_CTOR(plane_geo_ctor_params)
{
    PlaneParams params    = {};
    params.width          = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.widthSegments  = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_PLANE, &params);
}

CK_DLL_MFUN(plane_geo_build)
{
    SG_Geometry* geo      = GET_GEOMETRY(SELF);
    PlaneParams params    = {};
    params.width          = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.widthSegments  = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);

    ulib_geometry_build(geo, SG_GEOMETRY_PLANE, &params);
}

CK_DLL_MFUN(plane_geo_get_width)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.plane.width;
}

CK_DLL_MFUN(plane_geo_get_height)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.plane.height;
}

CK_DLL_MFUN(plane_geo_get_widthSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.plane.widthSegments;
}

CK_DLL_MFUN(plane_geo_get_heightSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.plane.heightSegments;
}

CK_DLL_CTOR(sphere_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_SPHERE, NULL);
}

CK_DLL_CTOR(sphere_geo_ctor_params)
{
    SphereParams params = {};
    params.radius       = GET_NEXT_FLOAT(ARGS);
    params.widthSeg     = GET_NEXT_INT(ARGS);
    params.heightSeg    = GET_NEXT_INT(ARGS);
    params.phiStart     = GET_NEXT_FLOAT(ARGS);
    params.phiLength    = GET_NEXT_FLOAT(ARGS);
    params.thetaStart   = GET_NEXT_FLOAT(ARGS);
    params.thetaLength  = GET_NEXT_FLOAT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_SPHERE, &params);
}

CK_DLL_MFUN(sphere_geo_build)
{
    SG_Geometry* geo    = GET_GEOMETRY(SELF);
    SphereParams params = {};
    params.radius       = GET_NEXT_FLOAT(ARGS);
    params.widthSeg     = GET_NEXT_INT(ARGS);
    params.heightSeg    = GET_NEXT_INT(ARGS);
    params.phiStart     = GET_NEXT_FLOAT(ARGS);
    params.phiLength    = GET_NEXT_FLOAT(ARGS);
    params.thetaStart   = GET_NEXT_FLOAT(ARGS);
    params.thetaLength  = GET_NEXT_FLOAT(ARGS);

    ulib_geometry_build(geo, SG_GEOMETRY_SPHERE, &params);
}

CK_DLL_MFUN(sphere_geo_get_radius)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.radius;
}

CK_DLL_MFUN(sphere_geo_get_widthSeg)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.sphere.widthSeg;
}

CK_DLL_MFUN(sphere_geo_get_heightSeg)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.sphere.heightSeg;
}

CK_DLL_MFUN(sphere_geo_get_phiStart)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.phiStart;
}

CK_DLL_MFUN(sphere_geo_get_phiLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.phiLength;
}

CK_DLL_MFUN(sphere_geo_get_thetaStart)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.thetaStart;
}

CK_DLL_MFUN(sphere_geo_get_thetaLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.sphere.thetaLength;
}

// Suzanne Geometry -----------------------------------------------------

CK_DLL_CTOR(suzanne_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_SUZANNE, NULL);
}

// Lines2D Geometry -----------------------------------------------------

void ulib_geo_lines2d_set_line_colors(SG_Geometry* geo, Chuck_Object* ck_arr)
{
    geoSetPulledVertexAttribute(geo, 1, ck_arr, 3, false);
}

void ulib_geo_lines2d_set_lines_points(SG_Geometry* geo, Chuck_Object* ck_arr)
{
    // need to add sentinel points to beginning and end of array
    int ck_arr_len
      = ck_arr ? g_chuglAPI->object->array_vec2_size((Chuck_ArrayVec2*)ck_arr) : 0;
    if (ck_arr_len < 2) {
        // nothing to draw for a line with < 2 points
        CQ_PushCommand_GeometrySetVertexCount(geo, 0);
        return;
    }

    Arena* pull_buffer = &geo->vertex_pull_buffers[0];
    Arena::clear(pull_buffer);
    f32* local_data = ARENA_PUSH_COUNT(pull_buffer, f32, (ck_arr_len + 2) * 2);

    // copy ck array positions
    chugin_copyCkVec2Array((Chuck_ArrayVec2*)ck_arr, &local_data[2]);

    glm::vec2 start      = glm::vec2(local_data[2], local_data[3]);
    glm::vec2 start_next = glm::vec2(local_data[4], local_data[5]);
    glm::vec2 end
      = glm::vec2(local_data[ck_arr_len * 2], local_data[ck_arr_len * 2 + 1]);
    glm::vec2 end_prev
      = glm::vec2(local_data[ck_arr_len * 2 - 2], local_data[ck_arr_len * 2 - 1]);

    // write the start sentinel point
    // TODO: if looping, set to the last point
    // if not looping, set along the line from the first to second point
    glm::vec2 start_sentinel = start - (start_next - start);
    local_data[0]            = start_sentinel.x;
    local_data[1]            = start_sentinel.y;

    // end sentinel point
    glm::vec2 end_sentinel               = end + (end - end_prev);
    local_data[(ck_arr_len + 1) * 2]     = end_sentinel.x;
    local_data[(ck_arr_len + 1) * 2 + 1] = end_sentinel.y;

    // push attribute change to command queue
    CQ_PushCommand_GeometrySetPulledVertexAttribute(geo, 0, local_data,
                                                    pull_buffer->curr);
    ASSERT(ARENA_LENGTH(pull_buffer, glm::vec2) == (ck_arr_len + 2));

    // always draw ck_arr_len+1 points to handle line loop
    CQ_PushCommand_GeometrySetVertexCount(geo,
                                          ck_arr_len * 4); // 3 vertices per segment
}

CK_DLL_CTOR(lines2d_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_LINES2D, NULL);
}

CK_DLL_CTOR(lines2d_geo_ctor_params)
{
    ASSERT(false); // not implemented
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));
    geo->geo_type    = SG_GEOMETRY_LINES2D;
    // ==nice-to-have==
}

CK_DLL_MFUN(lines2d_geo_set_line_points)
{
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);
    SG_Geometry* geo     = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geo_lines2d_set_lines_points(geo, ck_arr);
}

CK_DLL_MFUN(lines2d_geo_set_line_colors)
{
    Chuck_Object* ck_arr = GET_NEXT_OBJECT(ARGS);
    if (!ck_arr) return;
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    int length = API->object->array_vec3_size((Chuck_ArrayVec3*)ck_arr);
    if (length == 0) return;

    geoSetPulledVertexAttribute(geo, 1, ck_arr, 3, false);
}

// Box Geometry -----------------------------------------------------

CK_DLL_CTOR(box_geo_ctor)
{
    SG_Geometry* geo = GET_GEOMETRY(SELF);

    ulib_geometry_build(geo, SG_GEOMETRY_CUBE, NULL);
}

CK_DLL_CTOR(box_geo_ctor_params)
{
    BoxParams params = {};
    params.width     = GET_NEXT_FLOAT(ARGS);
    params.height    = GET_NEXT_FLOAT(ARGS);
    params.depth     = GET_NEXT_FLOAT(ARGS);
    params.widthSeg  = GET_NEXT_INT(ARGS);
    params.heightSeg = GET_NEXT_INT(ARGS);
    params.depthSeg  = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = GET_GEOMETRY(SELF);

    ulib_geometry_build(geo, SG_GEOMETRY_CUBE, &params);
}

CK_DLL_MFUN(box_geo_build)
{
    SG_Geometry* geo = GET_GEOMETRY(SELF);
    BoxParams params = {};
    params.width     = GET_NEXT_FLOAT(ARGS);
    params.height    = GET_NEXT_FLOAT(ARGS);
    params.depth     = GET_NEXT_FLOAT(ARGS);
    params.widthSeg  = GET_NEXT_INT(ARGS);
    params.heightSeg = GET_NEXT_INT(ARGS);
    params.depthSeg  = GET_NEXT_INT(ARGS);

    ulib_geometry_build(geo, SG_GEOMETRY_CUBE, &params);
}

CK_DLL_MFUN(box_geo_get_width)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.box.width;
}

CK_DLL_MFUN(box_geo_get_height)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.box.height;
}

CK_DLL_MFUN(box_geo_get_depth)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.box.depth;
}

CK_DLL_MFUN(box_geo_get_width_segments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.box.widthSeg;
}

CK_DLL_MFUN(box_geo_get_height_segments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.box.heightSeg;
}

CK_DLL_MFUN(box_geo_get_depth_segments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.box.depthSeg;
}

// Circle Geometry -----------------------------------------------------

CK_DLL_CTOR(circle_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_CIRCLE, NULL);
}

CK_DLL_CTOR(circle_geo_ctor_params)
{
    CircleParams params = {};
    params.radius       = GET_NEXT_FLOAT(ARGS);
    params.segments     = GET_NEXT_INT(ARGS);
    params.thetaStart   = GET_NEXT_FLOAT(ARGS);
    params.thetaLength  = GET_NEXT_FLOAT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_CIRCLE, &params);
}

CK_DLL_MFUN(circle_geo_build)
{
    SG_Geometry* geo    = GET_GEOMETRY(SELF);
    CircleParams params = {};
    params.radius       = GET_NEXT_FLOAT(ARGS);
    params.segments     = GET_NEXT_INT(ARGS);
    params.thetaStart   = GET_NEXT_FLOAT(ARGS);
    params.thetaLength  = GET_NEXT_FLOAT(ARGS);

    ulib_geometry_build(geo, SG_GEOMETRY_CIRCLE, &params);
}

CK_DLL_MFUN(circle_geo_get_radius)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.circle.radius;
}

CK_DLL_MFUN(circle_geo_get_segments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.circle.segments;
}

CK_DLL_MFUN(circle_geo_get_thetaStart)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.circle.thetaStart;
}

CK_DLL_MFUN(circle_geo_get_thetaLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.circle.thetaLength;
}

// Torus Geometry -----------------------------------------------------

CK_DLL_CTOR(torus_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_TORUS, NULL);
}

CK_DLL_CTOR(torus_geo_ctor_params)
{
    TorusParams params     = {};
    params.radius          = GET_NEXT_FLOAT(ARGS);
    params.tubeRadius      = GET_NEXT_FLOAT(ARGS);
    params.radialSegments  = GET_NEXT_INT(ARGS);
    params.tubularSegments = GET_NEXT_INT(ARGS);
    params.arcLength       = GET_NEXT_FLOAT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_TORUS, &params);
}

CK_DLL_MFUN(torus_geo_build)
{
    SG_Geometry* geo       = GET_GEOMETRY(SELF);
    TorusParams params     = {};
    params.radius          = GET_NEXT_FLOAT(ARGS);
    params.tubeRadius      = GET_NEXT_FLOAT(ARGS);
    params.radialSegments  = GET_NEXT_INT(ARGS);
    params.tubularSegments = GET_NEXT_INT(ARGS);
    params.arcLength       = GET_NEXT_FLOAT(ARGS);

    ulib_geometry_build(geo, SG_GEOMETRY_TORUS, &params);
}

CK_DLL_MFUN(torus_geo_get_radius)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.torus.radius;
}

CK_DLL_MFUN(torus_geo_get_tubeRadius)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.torus.tubeRadius;
}

CK_DLL_MFUN(torus_geo_get_radialSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.torus.radialSegments;
}

CK_DLL_MFUN(torus_geo_get_tubularSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.torus.tubularSegments;
}

CK_DLL_MFUN(torus_geo_get_arcLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.torus.arcLength;
}

// Cylinder Geometry -----------------------------------------------------

CK_DLL_CTOR(cylinder_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_CYLINDER, NULL);
}

CK_DLL_CTOR(cylinder_geo_ctor_params)
{
    CylinderParams params = {};
    params.radiusTop      = GET_NEXT_FLOAT(ARGS);
    params.radiusBottom   = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.radialSegments = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);
    params.openEnded      = GET_NEXT_INT(ARGS);
    params.thetaStart     = GET_NEXT_FLOAT(ARGS);
    params.thetaLength    = GET_NEXT_FLOAT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_CYLINDER, &params);
}

CK_DLL_MFUN(cylinder_geo_build)
{
    SG_Geometry* geo      = GET_GEOMETRY(SELF);
    CylinderParams params = {};
    params.radiusTop      = GET_NEXT_FLOAT(ARGS);
    params.radiusBottom   = GET_NEXT_FLOAT(ARGS);
    params.height         = GET_NEXT_FLOAT(ARGS);
    params.radialSegments = GET_NEXT_INT(ARGS);
    params.heightSegments = GET_NEXT_INT(ARGS);
    params.openEnded      = GET_NEXT_INT(ARGS);
    params.thetaStart     = GET_NEXT_FLOAT(ARGS);
    params.thetaLength    = GET_NEXT_FLOAT(ARGS);

    ulib_geometry_build(geo, SG_GEOMETRY_CYLINDER, &params);
}

CK_DLL_MFUN(cylinder_geo_get_radiusTop)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.radiusTop;
}

CK_DLL_MFUN(cylinder_geo_get_radiusBottom)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.radiusBottom;
}

CK_DLL_MFUN(cylinder_geo_get_height)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.height;
}

CK_DLL_MFUN(cylinder_geo_get_radialSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.cylinder.radialSegments;
}

CK_DLL_MFUN(cylinder_geo_get_heightSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.cylinder.heightSegments;
}

CK_DLL_MFUN(cylinder_geo_get_openEnded)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.cylinder.openEnded ? 1 : 0;
}

CK_DLL_MFUN(cylinder_geo_get_thetaStart)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.thetaStart;
}

CK_DLL_MFUN(cylinder_geo_get_thetaLength)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.cylinder.thetaLength;
}

// Knot Geometry -----------------------------------------------------

CK_DLL_CTOR(knot_geo_ctor)
{
    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_KNOT, NULL);
}

CK_DLL_CTOR(knot_geo_ctor_params)
{
    KnotParams params      = {};
    params.radius          = GET_NEXT_FLOAT(ARGS);
    params.tube            = GET_NEXT_FLOAT(ARGS);
    params.tubularSegments = GET_NEXT_INT(ARGS);
    params.radialSegments  = GET_NEXT_INT(ARGS);
    params.p               = GET_NEXT_INT(ARGS);
    params.q               = GET_NEXT_INT(ARGS);

    SG_Geometry* geo = SG_GetGeometry(OBJ_MEMBER_UINT(SELF, component_offset_id));

    ulib_geometry_build(geo, SG_GEOMETRY_KNOT, &params);
}

CK_DLL_MFUN(knot_geo_build)
{
    SG_Geometry* geo       = GET_GEOMETRY(SELF);
    KnotParams params      = {};
    params.radius          = GET_NEXT_FLOAT(ARGS);
    params.tube            = GET_NEXT_FLOAT(ARGS);
    params.tubularSegments = GET_NEXT_INT(ARGS);
    params.radialSegments  = GET_NEXT_INT(ARGS);
    params.p               = GET_NEXT_INT(ARGS);
    params.q               = GET_NEXT_INT(ARGS);

    ulib_geometry_build(geo, SG_GEOMETRY_KNOT, &params);
}

CK_DLL_MFUN(knot_geo_get_radius)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.knot.radius;
}

CK_DLL_MFUN(knot_geo_get_tube)
{
    RETURN->v_float = GET_GEOMETRY(SELF)->params.knot.tube;
}

CK_DLL_MFUN(knot_geo_get_tubularSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.knot.tubularSegments;
}

CK_DLL_MFUN(knot_geo_get_radialSegments)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.knot.radialSegments;
}

CK_DLL_MFUN(knot_geo_get_p)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.knot.p;
}

CK_DLL_MFUN(knot_geo_get_q)
{
    RETURN->v_int = GET_GEOMETRY(SELF)->params.knot.q;
}
