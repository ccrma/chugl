ShaderDesc shader_desc;
// test defaults
T.assert(shader_desc.vertexString == "", "shader desc vertexString");
T.assert(shader_desc.fragmentString == "", "shader desc fragmentString");
T.assert(shader_desc.vertexFilepath == "", "shader desc vertexFilepath");
T.assert(shader_desc.fragmentFilepath == "", "shader desc fragmentFilepath");
T.assert(shader_desc.vertexLayout == null, "shader desc vertexLayout");
T.assert(!shader_desc.lit, "shader desc lit");

"vertex_string" => shader_desc.vertexString;
"fragment_string" => shader_desc.fragmentString;
"vertex_filepath" => shader_desc.vertexFilepath;
"fragment_filepath" => shader_desc.fragmentFilepath;
[1,2,3,4,5,6,7,8] @=> shader_desc.vertexLayout;
true => shader_desc.lit;

// Shader shader; // default constructor not allowed
Shader shader(shader_desc);
T.assert(shader.fragmentString() == shader_desc.fragmentString, "shader fragmentString");
T.assert(shader.vertexString() == shader_desc.vertexString, "shader vertexString");
T.assert(shader.fragmentFilepath() == shader_desc.fragmentFilepath, "shader fragmentFilepath");
T.assert(shader.vertexFilepath() == shader_desc.vertexFilepath, "shader vertexFilepath");
T.assert(T.arrayEquals(shader.vertexLayout(), shader_desc.vertexLayout), "shader vertexLayout");
T.assert(shader.lit(), "shader lit");

ShaderDesc shader_desc2;
Shader shader2(shader_desc2);
T.assert(shader2.fragmentString() == shader_desc2.fragmentString, "shader2 fragmentString");
T.assert(shader2.vertexString() == shader_desc2.vertexString, "shader2 vertexString");
T.assert(shader2.fragmentFilepath() == shader_desc2.fragmentFilepath, "shader2 fragmentFilepath");
T.assert(shader2.vertexFilepath() == shader_desc2.vertexFilepath, "shader2 vertexFilepath");
T.assert(T.arrayEquals(shader2.vertexLayout(), [0,0,0,0,0,0,0,0]), "shader2 vertexLayout");
T.assert(!shader2.lit(), "shader2 lit");

Material material;

T.assert(material.CULL_NONE == 0, "material CULL_NONE");
T.assert(material.CULL_FRONT == 1, "material CULL_FRONT");
T.assert(material.CULL_BACK == 2, "material CULL_BACK");

T.assert(material.Topology_PointList == 0, "material TOPOLOGY_POINTLIST");
T.assert(material.Topology_LineList == 1, "material TOPOLOGY_LINELIST");
T.assert(material.Topology_LineStrip == 2, "material TOPOLOGY_LINESTRIP");
T.assert(material.Topology_TriangleList == 3, "material TOPOLOGY_TRIANGLELIST");
T.assert(material.Topology_TriangleStrip == 4, "material TOPOLOGY_TRIANGLESTRIP");

T.assert(material.cullMode() == material.CULL_NONE, "material cullMode default"); 
material.cullMode(material.CULL_BACK);
T.assert(material.cullMode() == material.CULL_BACK, "material cullMode");

T.assert(material.topology() == material.Topology_TriangleList, "material topology default");
material.topology(material.Topology_PointList);
T.assert(material.topology() == material.Topology_PointList, "material topology");

T.assert(material.shader() == null, "material shader default");
shader => material.shader;
T.assert(material.shader() == shader, "material shader");

// Uniforms

material.uniformFloat(0, 1.2);
T.assert(T.feq(material.uniformFloat(0), 1.2), "material uniformFloat");
T.assert(T.arrayEquals(material.activeUniformLocations(), [0]), "material activeUniformLocations");
material.uniformFloat(1, 3.4);
T.assert(T.feq(material.uniformFloat(1), 3.4), "material uniformFloat");
material.removeUniform(0);
T.assert(T.arrayEquals(material.activeUniformLocations(), [1]), "material activeUniformLocations");
material.removeUniform(1);

int empty_int_arr[0];
T.assert(T.arrayEquals(material.activeUniformLocations(), empty_int_arr), "material activeUniformLocations");

// TODO: janky to conflate uniforms and storage buffer
material.storageBuffer(0, [1.0]);
T.assert(T.arrayEquals(material.activeUniformLocations(), [0]), "material activeUniformLocations after storageBuffer");

material.uniformInt(1, 1);
T.assert(material.uniformInt(1) == 1, "material uniformInt");
material.uniformInt2(2, 1, 2);
T.assert(T.arrayEquals(material.uniformInt2(2), [1, 2]), "material uniformInt2");
material.uniformInt3(3, 1, 2, 3);
T.assert(T.arrayEquals(material.uniformInt3(3), [1, 2, 3]), "material uniformInt3");
material.uniformInt4(4, 1, 2, 3, 4);
T.assert(T.arrayEquals(material.uniformInt4(4), [1, 2, 3, 4]), "material uniformInt4");

// test setting/getting TextureSampler

TextureSampler sampler;
TextureSampler.WRAP_CLAMP => sampler.wrapU;
Texture texture;

material.texture(0, texture);
material.sampler(1, sampler);


T.assert(material.texture(0) == texture, "material texture");
T.assert(
    material.sampler(1).wrapU  == sampler.wrapU &&
    material.sampler(1).wrapV  == sampler.wrapV &&
    material.sampler(1).wrapW  == sampler.wrapW &&
    material.sampler(1).filterMin  == sampler.filterMin &&
    material.sampler(1).filterMag  == sampler.filterMag &&
    material.sampler(1).filterMip  == sampler.filterMip,
    "material sampler"
);

// FlatMaterial 

FlatMaterial flat_material;

flat_material.sampler(sampler);

T.assert(
    flat_material.sampler().wrapU  == sampler.wrapU &&
    flat_material.sampler().wrapV  == sampler.wrapV &&
    flat_material.sampler().wrapW  == sampler.wrapW &&
    flat_material.sampler().filterMin  == sampler.filterMin &&
    flat_material.sampler().filterMag  == sampler.filterMag &&
    flat_material.sampler().filterMip  == sampler.filterMip,
    "material sampler"
);
