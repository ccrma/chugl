#include "ulib_material.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "renderer/scenegraph/Material.h"

//-----------------------------------------------------------------------------
// Material API Declarations
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_mat_ctor);
CK_DLL_DTOR(cgl_mat_dtor);
CK_DLL_MFUN(cgl_mat_clone);

// base material options
// TODO: move all material options down into base class?
CK_DLL_MFUN(cgl_mat_set_polygon_mode);
CK_DLL_MFUN(cgl_mat_get_polygon_mode);
CK_DLL_MFUN(cgl_mat_set_color);
CK_DLL_MFUN(cgl_mat_set_alpha);
CK_DLL_MFUN(cgl_mat_get_alpha);
CK_DLL_MFUN(cgl_mat_set_point_size);
CK_DLL_MFUN(cgl_mat_set_line_width);
CK_DLL_MFUN(cgl_mat_set_transparent);
CK_DLL_MFUN(cgl_mat_get_transparent);
// CK_DLL_MFUN(cgl_mat_set_cull_mode);  // TODO

// uniform setters
CK_DLL_MFUN(cgl_mat_set_uniform_float);
CK_DLL_MFUN(cgl_mat_set_uniform_float2);
CK_DLL_MFUN(cgl_mat_set_uniform_float3);
CK_DLL_MFUN(cgl_mat_set_uniform_float4);
CK_DLL_MFUN(cgl_mat_set_uniform_int);
CK_DLL_MFUN(cgl_mat_set_uniform_int2);
CK_DLL_MFUN(cgl_mat_set_uniform_int3);
CK_DLL_MFUN(cgl_mat_set_uniform_int4);
CK_DLL_MFUN(cgl_mat_set_uniform_bool);
CK_DLL_MFUN(cgl_mat_set_uniform_texID);

// normal mat
CK_DLL_CTOR(cgl_mat_norm_ctor);
CK_DLL_MFUN(cgl_set_use_local_normals);
CK_DLL_MFUN(cgl_set_use_world_normals);


// flat shade mat
CK_DLL_CTOR(cgl_mat_flat_ctor);

// phong specular mat
CK_DLL_CTOR(cgl_mat_phong_ctor);
// uniform setters
CK_DLL_MFUN(cgl_mat_phong_set_diffuse_map);
CK_DLL_MFUN(cgl_mat_phong_set_specular_map);
CK_DLL_MFUN(cgl_mat_phong_set_specular_color);
CK_DLL_MFUN(cgl_mat_phong_set_log_shininess);
// uniform getters TODO

// custom shader mat
CK_DLL_CTOR(cgl_mat_custom_shader_ctor);
CK_DLL_MFUN(cgl_mat_custom_shader_set_shaders);
CK_DLL_MFUN(cgl_mat_custom_shader_set_vert_shader);
CK_DLL_MFUN(cgl_mat_custom_shader_set_frag_shader);
CK_DLL_MFUN(cgl_mat_custom_shader_set_vert_string);
CK_DLL_MFUN(cgl_mat_custom_shader_set_frag_string);

// points mat
CK_DLL_CTOR(cgl_mat_points_ctor);

CK_DLL_MFUN(cgl_mat_points_set_size_attenuation);
CK_DLL_MFUN(cgl_mat_points_get_size_attenuation);

CK_DLL_MFUN(cgl_mat_points_set_sprite);

// mango mat (for debugging UVs)
CK_DLL_CTOR(cgl_mat_mango_ctor);

// basic line mat (note: line rendering is not well supported on modern OpenGL)
// most hardware doesn't support variable line width
// "using the build-in OpenGL functionality for this task is very limited, if working at all."
// for a better soln using texture-buffer line meshes, see: https://github.com/mhalber/Lines#texture-buffer-lines
CK_DLL_CTOR(cgl_mat_line_ctor);
CK_DLL_MFUN(cgl_mat_line_set_mode);	 // many platforms only support fixed width 1.0

//-----------------------------------------------------------------------------
// Material API Definition
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_material(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Base), "Object");
	QUERY->doc_class(QUERY, "Base material class, do not instantiate directly");
    QUERY->add_ex(QUERY, "basic/polygon-modes.ck");
    QUERY->add_ex(QUERY, "basic/transparency.ck");

	QUERY->add_ctor(QUERY, cgl_mat_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);
	CGL::SetMaterialDataOffset(QUERY->add_mvar(QUERY, "int", "@cglmat_data", false));

	// clone
	QUERY->add_mfun(QUERY, cgl_mat_clone, Material::CKName(MaterialType::Base), "clone");
	QUERY->doc_func(QUERY, "Clones this material");

	// Material params (static constants) ---------------------------------
	QUERY->add_svar(QUERY, "int", "POLYGON_FILL", TRUE, (void *)&Material::POLYGON_FILL);
	QUERY->doc_var(QUERY, "pass into Material.polygonMode() to set polygon rendering to filled triangles, default");
	QUERY->add_svar(QUERY, "int", "POLYGON_LINE", TRUE, (void *)&Material::POLYGON_LINE);
	QUERY->doc_var(QUERY, "pass into Material.polygonMode() to render geometry as a line mesh");
	QUERY->add_svar(QUERY, "int", "POLYGON_POINT", TRUE, (void *)&Material::POLYGON_POINT);
	QUERY->doc_var(QUERY, "pass into Material.polygonMode() to render geometry as a point mesh (points drawn at each vertex position)");

	// line rendering static vars
	QUERY->add_svar(QUERY, "int", "LINE_SEGMENTS", TRUE, (void *)&Material::LINE_SEGMENTS_MODE);
	QUERY->doc_var(QUERY, "used by LineMaterial to render lines as a series of segments, where every 2 vertices is a line");
	QUERY->add_svar(QUERY, "int", "LINE_STRIP", TRUE, (void *)&Material::LINE_STRIP_MODE);
	QUERY->doc_var(QUERY, "used by LineMaterial to render lines as a continuous strip, connecting each vertex to the next");
	QUERY->add_svar(QUERY, "int", "LINE_LOOP", TRUE, (void *)&Material::LINE_LOOP_MODE);
	QUERY->doc_var(QUERY, "used by LineMaterial to render lines as a loop, connecting each vertex to the next, and also the last to the first");

	QUERY->add_mfun(QUERY, cgl_mat_set_polygon_mode, "int", "polygonMode");
	QUERY->add_arg(QUERY, "int", "mode");
	QUERY->doc_func(QUERY, "set the rendering mode for this material, can be Material.POLYGON_FILL, Material.POLYGON_LINE, or Material.POLYGON_POINT");

	QUERY->add_mfun(QUERY, cgl_mat_get_polygon_mode, "int", "polygonMode");
	QUERY->doc_func(QUERY, "get the rendering mode for this material, can be Material.POLYGON_FILL, Material.POLYGON_LINE, or Material.POLYGON_POINT");

	QUERY->add_mfun(QUERY, cgl_mat_set_point_size, "void", "pointSize");
	QUERY->add_arg(QUERY, "float", "size");
	QUERY->doc_func(QUERY, 
		"Set point size of PointsMaterial. Also affects point size if rendering in mode Material.POLYGON_POINT"
	);

	QUERY->add_mfun(QUERY, cgl_mat_set_color, "vec3", "color");
	QUERY->add_arg(QUERY, "vec3", "rgb");
	QUERY->doc_func(QUERY, "set material color uniform as an rgb. Alpha set to 1.0");

	QUERY->add_mfun(QUERY, cgl_mat_set_alpha, "float", "alpha");
	QUERY->add_arg(QUERY, "float", "alpha");
	QUERY->doc_func(QUERY, "set the alpha of the material color");

	QUERY->add_mfun(QUERY, cgl_mat_get_alpha, "float", "alpha");
	QUERY->doc_func(QUERY, "get the alpha of the material color");

	QUERY->add_mfun(QUERY, cgl_mat_set_line_width, "void", "lineWidth");
	QUERY->add_arg(QUERY, "float", "width");
	QUERY->doc_func(QUERY, "set line width if rendering with Material.POLYGON_LINE. NOTE: unsupported on macOS");

	QUERY->add_mfun(QUERY, cgl_mat_set_transparent, "int", "transparent");
	QUERY->add_arg(QUERY, "int", "transparent");
	QUERY->doc_func(QUERY, 
		"set if material should be rendered with transparency. 1 for true, 0 for false"
		"Meshes using this material will then be rendered in the transparent pass with depth writing disabled"
	);

	QUERY->add_mfun(QUERY, cgl_mat_get_transparent, "int", "transparent");
	QUERY->doc_func(QUERY, 
		"returns whether material is marked transparent. 1 for true, 0 for false"
	);

	// norm mat fns
	QUERY->add_mfun(QUERY, cgl_set_use_local_normals, "void", "localNormals");
	QUERY->doc_func(QUERY, "For NormalsMaterial: color surface using local-space normals");
	
	QUERY->add_mfun(QUERY, cgl_set_use_world_normals, "void", "worldNormals");
	QUERY->doc_func(QUERY, "For NormalsMaterial: color surface using world-space normals");

	// phong mat fns (TODO add getters, need to fix texture creation)
	QUERY->add_mfun(QUERY, cgl_mat_phong_set_log_shininess, "float", "shine");
	QUERY->add_arg(QUERY, "float", "shininess");
	QUERY->doc_func(QUERY, "For PhongMaterial: set shininess exponent, default 5");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_diffuse_map, "void", "diffuseMap");
	QUERY->add_arg(QUERY, "Texture", "tex");
	QUERY->doc_func(QUERY, "Set diffuse map texture");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_specular_map, "void", "specularMap");
	QUERY->add_arg(QUERY, "Texture", "tex");
	QUERY->doc_func(QUERY, "Set specular map texture");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_specular_color, "vec3", "specular");
	QUERY->add_arg(QUERY, "vec3", "color");
	QUERY->doc_func(QUERY, "For PhongMat: set specular color");

	// shader mat fns  (TODO allow setting vert and frag separately)
	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_shaders, "void", "shaders");
	QUERY->add_arg(QUERY, "string", "vert");
	QUERY->add_arg(QUERY, "string", "frag");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set vertex and fragment shaders");

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_vert_shader, "void", "vertShader");
	QUERY->add_arg(QUERY, "string", "vert");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set vertex shader path");

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_frag_shader, "void", "fragShader");
	QUERY->add_arg(QUERY, "string", "frag");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set fragment shader path");

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_vert_string, "void", "vertString");
	QUERY->add_arg(QUERY, "string", "vert");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set vertex shader string");

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_frag_string, "void", "fragString");
	QUERY->add_arg(QUERY, "string", "frag");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set fragment shader string");


	// points mat fns
	QUERY->add_mfun(QUERY, cgl_mat_points_set_size_attenuation, "int", "attenuatePoints");
	QUERY->add_arg(QUERY, "int", "attenuation");
	QUERY->doc_func(QUERY, "For PointMaterial: set if point size should be scaled by distance from camera");

	QUERY->add_mfun(QUERY, cgl_mat_points_get_size_attenuation, "int", "attenuatePoints");
	QUERY->doc_func(QUERY, "For PointMaterial: returns 1 if point size is being scaled by distance from camera, else 0");

	QUERY->add_mfun(QUERY, cgl_mat_points_set_sprite, "Texture", "pointSprite");
	QUERY->add_arg(QUERY, "Texture", "sprite");
	QUERY->doc_func(QUERY, "For PointMaterial: set sprite texture for point sprite rendering");

	// line mat fns
	QUERY->add_mfun(QUERY, cgl_mat_line_set_mode, "int", "lineMode");
	QUERY->add_arg(QUERY, "int", "mode");
	QUERY->doc_func(QUERY, "For LineMaterial: set line mode. Can be Material.LINE_SEGMENTS, Material.LINE_STRIP, or Material.LINE_LOOP");


	// uniform setters
	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_float, "void", "uniformFloat");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "float", "f0");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_float2, "void", "uniformFloat2");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "float", "f0");
	QUERY->add_arg(QUERY, "float", "f1");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_float3, "void", "uniformFloat3");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "float", "f0");
	QUERY->add_arg(QUERY, "float", "f1");
	QUERY->add_arg(QUERY, "float", "f2");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_float4, "void", "uniformFloat4");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "float", "f0");
	QUERY->add_arg(QUERY, "float", "f1");
	QUERY->add_arg(QUERY, "float", "f2");
	QUERY->add_arg(QUERY, "float", "f3");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_int, "void", "uniformInt");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "i0");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_int2, "void", "uniformInt2");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "i0");
	QUERY->add_arg(QUERY, "int", "i1");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_int3, "void", "uniformInt3");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "i0");
	QUERY->add_arg(QUERY, "int", "i1");
	QUERY->add_arg(QUERY, "int", "i2");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_int4, "void", "uniformInt4");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "i0");
	QUERY->add_arg(QUERY, "int", "i1");
	QUERY->add_arg(QUERY, "int", "i2");
	QUERY->add_arg(QUERY, "int", "i3");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_bool, "void", "uniformBool");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "b0");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_texID, "void", "uniformTexture");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "Texture", "texture");

	QUERY->end_class(QUERY);

	// normal material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Normal), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Color each pixel using the surface normal at that point");
	QUERY->add_ctor(QUERY, cgl_mat_norm_ctor);
	QUERY->end_class(QUERY);

	// flat material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Flat), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Color each pixel using a flat color");
	QUERY->add_ctor(QUERY, cgl_mat_flat_ctor);
	QUERY->end_class(QUERY);

	// phong specular material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Phong), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Color each pixel using a phong specular shading");
	QUERY->add_ctor(QUERY, cgl_mat_phong_ctor);
	QUERY->end_class(QUERY);

	// custom shader material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::CustomShader), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Color each pixel using the custom glsl shaders you provide via Material.shaders()");
    QUERY->add_ex(QUERY, "audioshader/audio-texture.ck");


	QUERY->add_ctor(QUERY, cgl_mat_custom_shader_ctor);
	QUERY->end_class(QUERY);

	// points material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Points), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Used by GPoints");
    QUERY->add_ex(QUERY, "basic/points.ck");
    QUERY->add_ex(QUERY, "textures/snowstorm.ck");
	QUERY->add_ctor(QUERY, cgl_mat_points_ctor);
	QUERY->end_class(QUERY);

	// mango material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Mango), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Color each pixel using its UV coordinates. Looks like a mango, yum.");
	QUERY->add_ctor(QUERY, cgl_mat_mango_ctor);
	QUERY->end_class(QUERY);

	// line material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Line), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Used by GLines");
    QUERY->add_ex(QUERY, "sndpeek/sndpeek-minimal.ck");
    QUERY->add_ex(QUERY, "sndpeek/sndpeek.ck");

	QUERY->add_ctor(QUERY, cgl_mat_line_ctor);
	QUERY->end_class(QUERY);

	return true;
}
// CGL Materials ===================================================
CK_DLL_CTOR(cgl_mat_ctor)
{
	// dud, do nothing for now
}

CK_DLL_DTOR(cgl_mat_dtor) // all geos can share this base destructor
{
	CGL::PushCommand(new DestroySceneGraphNodeCommand(SELF, CGL::GetMaterialDataOffset(), &CGL::mainScene));
}


CK_DLL_MFUN(cgl_mat_clone)
{
	Material *mat = CGL::GetMaterial(SELF);
	RETURN->v_object = CGL::CreateChuckObjFromMat(API, VM, (Material*) mat->Clone(), SHRED, false)->m_ChuckObject;
}

CK_DLL_MFUN(cgl_mat_set_polygon_mode)
{
	Material *mat = CGL::GetMaterial(SELF);
	auto mode = GET_NEXT_INT(ARGS);

	mat->SetPolygonMode((MaterialPolygonMode)mode);

	RETURN->v_int = mode;

	CGL::PushCommand(new UpdateMaterialOptionCommand(mat, *mat->GetOption(MaterialOptionParam::PolygonMode)));
}

CK_DLL_MFUN(cgl_mat_get_polygon_mode)
{
	Material *mat = CGL::GetMaterial(SELF);
	RETURN->v_int = mat->GetPolygonMode();
}

// point size setter
CK_DLL_MFUN(cgl_mat_set_point_size)
{
	Material *mat = CGL::GetMaterial(SELF);
	auto size = GET_NEXT_FLOAT(ARGS);
	mat->SetPointSize(size);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(Material::POINT_SIZE_UNAME)));
}

CK_DLL_MFUN(cgl_mat_set_line_width)
{
	Material* mat = CGL::GetMaterial(SELF);
	t_CKFLOAT width = GET_NEXT_FLOAT(ARGS);
	mat->SetLineWidth(width);

	RETURN->v_float = width;
	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(Material::LINE_WIDTH_UNAME)));
}

CK_DLL_MFUN(cgl_mat_set_color)
{
	Material *mat = CGL::GetMaterial(SELF);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	mat->SetColor(color.x, color.y, color.z, 1.0f);

	RETURN->v_vec3 = color;

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(Material::COLOR_UNAME)));
}

CK_DLL_MFUN(cgl_mat_set_alpha)
{
	Material *mat = CGL::GetMaterial(SELF);
	t_CKFLOAT alpha = GET_NEXT_FLOAT(ARGS);
	mat->SetAlpha(alpha);

	RETURN->v_float = alpha;

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(Material::COLOR_UNAME)));
}

CK_DLL_MFUN(cgl_mat_get_alpha)
{
	Material *mat = CGL::GetMaterial(SELF);
	RETURN->v_float = mat->GetAlpha();
}

CK_DLL_MFUN(cgl_mat_set_transparent)
{
	Material *mat = CGL::GetMaterial(SELF);
	auto transparent = GET_NEXT_INT(ARGS);
	mat->SetTransparent(transparent);

	RETURN->v_int = transparent;

	CGL::PushCommand(new UpdateMaterialOptionCommand(mat, *mat->GetOption(MaterialOptionParam::Transparent)));
}

CK_DLL_MFUN(cgl_mat_get_transparent)
{
	Material *mat = CGL::GetMaterial(SELF);
	RETURN->v_int = mat->IsTransparent() ? 1 : 0;
}

CK_DLL_MFUN(cgl_mat_set_uniform_float)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	float value = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_float2)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	float value0 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value1 = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_float3)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	float value0 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value1 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value2 = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_float4)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	float value0 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value1 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value2 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value3 = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2, value3);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_int)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	int value = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_int2)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	int value0 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value1 = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_int3)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	int value0 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value1 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value2 = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_int4)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	int value0 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value1 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value2 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value3 = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2, value3);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_bool)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	bool value = static_cast<bool>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_texID)
{
	Material *mat = CGL::GetMaterial(SELF);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	CGL_Texture *tex = CGL::GetTexture(GET_NEXT_OBJECT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), tex->GetID());

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_CTOR(cgl_mat_norm_ctor)
{
	NormalMaterial *normMat = new NormalMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(normMat, &CGL::mainScene, SELF, CGL::GetMaterialDataOffset()));
}

CK_DLL_MFUN(cgl_set_use_local_normals)
{
	Material *mat = CGL::GetMaterial(SELF);
	mat->UseLocalNormals();

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *(mat->GetUniform(Material::USE_LOCAL_NORMALS_UNAME))));
}

CK_DLL_MFUN(cgl_set_use_world_normals)
{
	Material *mat = CGL::GetMaterial(SELF);
	mat->UseWorldNormals();

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *(mat->GetUniform(Material::USE_LOCAL_NORMALS_UNAME))));
}


// flat mat fns
CK_DLL_CTOR(cgl_mat_flat_ctor)
{
	FlatMaterial *flatMat = new FlatMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(flatMat, &CGL::mainScene, SELF, CGL::GetMaterialDataOffset()));
}


// phong mat fns
CK_DLL_CTOR(cgl_mat_phong_ctor)
{
	PhongMaterial *phongMat = new PhongMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(phongMat, &CGL::mainScene, SELF, CGL::GetMaterialDataOffset()));
}

CK_DLL_MFUN(cgl_mat_phong_set_diffuse_map)
{
	Material *mat = CGL::GetMaterial(SELF);
	CGL_Texture *tex = CGL::GetTexture(GET_NEXT_OBJECT(ARGS));
	mat->SetDiffuseMap(tex);

	// TODO: how do I return the chuck texture object?
	// RETURN->v_object = SELF;

	// a lot of redundant work (entire uniform vector is copied). can optimize later
	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(Material::DIFFUSE_MAP_UNAME)));
}

CK_DLL_MFUN(cgl_mat_phong_set_specular_map)
{
	Material *mat = CGL::GetMaterial(SELF);
	CGL_Texture *tex = CGL::GetTexture(GET_NEXT_OBJECT(ARGS));
	mat->SetSpecularMap(tex);

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SPECULAR_MAP_UNAME)));
}

CK_DLL_MFUN(cgl_mat_phong_set_specular_color)
{
	Material *mat = CGL::GetMaterial(SELF);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	mat->SetSpecularColor(color.x, color.y, color.z);

	RETURN->v_vec3 = color;

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SPECULAR_COLOR_UNAME)));
}

CK_DLL_MFUN(cgl_mat_phong_set_log_shininess)
{
	Material *mat = CGL::GetMaterial(SELF);
	t_CKFLOAT shininess = GET_NEXT_FLOAT(ARGS);
	mat->SetLogShininess(shininess);

	RETURN->v_float = shininess;

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(Material::SHININESS_UNAME)));
}

// custom shader mat fns ---------------------------------
CK_DLL_CTOR(cgl_mat_custom_shader_ctor)
{
	ShaderMaterial *shaderMat = new ShaderMaterial("", "");
	CGL::PushCommand(new CreateSceneGraphNodeCommand(shaderMat, &CGL::mainScene, SELF, CGL::GetMaterialDataOffset()));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_shaders)
{
	Material *mat = CGL::GetMaterial(SELF);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *vertPath = GET_NEXT_STRING(ARGS);
	Chuck_String *fragPath = GET_NEXT_STRING(ARGS);

	mat->SetFragShader(fragPath->str(), true);
	mat->SetVertShader(vertPath->str(), true);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_vert_shader)
{
	Material *mat = CGL::GetMaterial(SELF);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *vertPath = GET_NEXT_STRING(ARGS);

	mat->SetVertShader(vertPath->str(), true);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_frag_shader)
{
	Material *mat = CGL::GetMaterial(SELF);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *fragPath = GET_NEXT_STRING(ARGS);

	mat->SetFragShader(fragPath->str(), true);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_vert_string)
{
	Material *mat = CGL::GetMaterial(SELF);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *vertString = GET_NEXT_STRING(ARGS);

	mat->SetVertShader(vertString->str(), false);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_frag_string)
{
	Material *mat = CGL::GetMaterial(SELF);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *fragString = GET_NEXT_STRING(ARGS);

	mat->SetFragShader(fragString->str(), false);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}


// points mat fns ---------------------------------
CK_DLL_CTOR(cgl_mat_points_ctor)
{
	PointsMaterial *pointsMat = new PointsMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(pointsMat, &CGL::mainScene, SELF, CGL::GetMaterialDataOffset()));
}

CK_DLL_MFUN(cgl_mat_points_set_size_attenuation)
{
	Material *mat = CGL::GetMaterial(SELF);
	t_CKINT attenuation = GET_NEXT_INT(ARGS);
	mat->SetSizeAttenuation(attenuation);

	RETURN->v_int = attenuation;

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(PointsMaterial::POINT_SIZE_ATTENUATION_UNAME)));
}

CK_DLL_MFUN(cgl_mat_points_get_size_attenuation)
{
	Material *mat = CGL::GetMaterial(SELF);

	RETURN->v_int = mat->GetSizeAttenuation() ? 1 : 0;
}

CK_DLL_MFUN(cgl_mat_points_set_sprite)
{
	Material *mat = CGL::GetMaterial(SELF);
	CGL_Texture *tex = CGL::GetTexture(GET_NEXT_OBJECT(ARGS));
	mat->SetSprite(tex);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(PointsMaterial::POINT_SPRITE_TEXTURE_UNAME)));
}

// set point color

// mango mat fns ---------------------------------
CK_DLL_CTOR(cgl_mat_mango_ctor)
{
	MangoMaterial *mangoMat = new MangoMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(mangoMat, &CGL::mainScene, SELF, CGL::GetMaterialDataOffset()));
}

// line mat fns ---------------------------------

CK_DLL_CTOR(cgl_mat_line_ctor)
{
	LineMaterial *lineMat = new LineMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(lineMat, &CGL::mainScene, SELF, CGL::GetMaterialDataOffset()));
}


CK_DLL_MFUN(cgl_mat_line_set_mode)
{
	Material *mat = CGL::GetMaterial(SELF);
	t_CKINT mode = GET_NEXT_INT(ARGS);
	mat->SetLineMode((MaterialPrimitiveMode)mode);

	RETURN->v_int = mode;
	CGL::PushCommand(new UpdateMaterialOptionCommand(mat, *mat->GetOption(MaterialOptionParam::PrimitiveMode)));
}
