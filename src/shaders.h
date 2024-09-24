#pragma once

#include <glm/glm.hpp>

#include "core/macros.h"

#include <unordered_map>

struct ShaderEntry {
    const char* name;
    const char* code;
};

#define PER_FRAME_GROUP 0
#define PER_MATERIAL_GROUP 1
#define PER_DRAW_GROUP 2
#define VERTEX_PULL_GROUP 3

#define VS_ENTRY_POINT "vs_main"
#define FS_ENTRY_POINT "fs_main"
#define COMPUTE_ENTRY_POINT "main"

// #define STRINGIFY(s) #s
// #define INTERPOLATE(var) STRINGIFY(${##var##})

struct FrameUniforms {
    glm::mat4x4 projection;  // at byte offset 0
    glm::mat4x4 view;        // at byte offset 64
    glm::vec3 camera_pos;    // at byte offset 128
    float time;              // at byte offset 140
    glm::vec3 ambient_light; // at byte offset 144
    int32_t num_lights;      // at byte offset 156

    float _pad[256]; // padding to reach webgpu minimum buffer size requirement
};

struct LightUniforms {
    glm::vec3 color;    // at byte offset 0
    int32_t light_type; // at byte offset 12
    glm::vec3 position; // at byte offset 16
    float _pad0;
    glm::vec3 direction;  // at byte offset 32
    float point_radius;   // at byte offset 44
    float point_falloff;  // at byte offset 48
    float spot_cos_angle; // at byte offset 52
    float _pad1[2];
};

// struct DrawUniforms {
//     glm::mat4x4 modelMat; // at byte offset 0
// };

struct DrawUniforms {
    glm::mat4x4 model; // at byte offset 0
    int32_t id;        // at byte offset 128
    float _pad0[3];
};

// clang-format off

static std::unordered_map<std::string, std::string> shader_table = {
    {
        "FRAME_UNIFORMS", 
        R"glsl(

        struct FrameUniforms {
            projection: mat4x4f,
            view: mat4x4f,
            camera_pos: vec3f,
            time: f32,
            ambient_light: vec3f,
            num_lights: i32,
        };

        @group(0) @binding(0) var<uniform> u_Frame: FrameUniforms;

        )glsl"
    },
    {
        "LIGHTING_UNIFORMS",
        R"glsl(

        // light types
        const LightType_None = 0;
        const LightType_Directional = 1;
        const LightType_Point = 2;
        const LightType_Spot = 3;

        struct LightUniforms {
            color : vec3f,
            light_type: i32,
            position: vec3f,
            direction: vec3f, 

            // point light
            point_radius: f32,
            point_falloff: f32,

            // spot light
            spot_cos_angle: f32,
        };

        @group(0) @binding(1) var<storage, read> u_lights: array<LightUniforms>;

        )glsl"
    },
    {
        "DRAW_UNIFORMS", 
        R"glsl(

        struct DrawUniforms {
            model: mat4x4f,
            id: u32
        };

        @group(2) @binding(0) var<storage> drawInstances: array<DrawUniforms>;

        )glsl"
    },

    {
        "STANDARD_VERTEX_INPUT", // vertex input for standard 3D objects (pos, normal, uv, tangent)
        R"glsl(

        struct VertexInput {
            @location(0) position : vec3f,
            @location(1) normal : vec3f,
            @location(2) uv : vec2f,
            @location(3) tangent : vec4f,
            @builtin(instance_index) instance : u32,
        };

        )glsl"
    },

    {
        "STANDARD_VERTEX_OUTPUT", // vertex output for standard 3D objects (pos, normal, uv, tangent)
        R"glsl(

        struct VertexOutput {
            @builtin(position) position : vec4f,
            @location(0) v_worldpos : vec3f,
            @location(1) v_normal : vec3f,
            @location(2) v_uv : vec2f,
            @location(3) v_tangent : vec4f,
        };

        )glsl"
    },

    {
        "STANDARD_VERTEX_SHADER",
        R"glsl(
        @vertex 
        fn vs_main(in : VertexInput) -> VertexOutput
        {
            var out : VertexOutput;
            var u_Draw : DrawUniforms = drawInstances[in.instance];

            let modelMat3 : mat3x3<f32> = mat3x3(
                u_Draw.model[0].xyz,
                u_Draw.model[1].xyz,
                u_Draw.model[2].xyz
            );

            let worldpos = u_Draw.model * vec4f(in.position, 1.0f);
            out.position = (u_Frame.projection * u_Frame.view) * worldpos;
            out.v_worldpos = worldpos.xyz;

            // TODO: after wgsl adds matrix inverse, calculate normal matrix here
            // out.v_normal = (transpose(inverse(u_Frame.viewMat * u_Draw.model)) * vec4f(in.normal, 0.0)).xyz;
            out.v_normal = (u_Draw.model * vec4f(in.normal, 0.0)).xyz;

            // tangent vectors aren't impacted by non-uniform scaling or translation
            out.v_tangent = vec4f(modelMat3 * in.tangent.xyz, in.tangent.w);
            out.v_uv     = in.uv;

            return out;
        }
        )glsl"
    },

    {
        "SCREEN_PASS_VERTEX_SHADER",
        R"glsl(
        struct VertexOutput {
            @builtin(position) position : vec4<f32>,
            @location(0) v_uv : vec2<f32>,
        };

        @vertex 
        fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
            var output : VertexOutput;

            // outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
            // gl_Position = vec4(outUV * 2.0f + -1.0f, 0.0f, 1.0f);

            output.v_uv = vec2f(f32((vertexIndex << 1u) & 2u), f32(vertexIndex & 2u));
            output.position = vec4f(output.v_uv * 2.0 - 1.0, 0.0, 1.0);
            
            // triangle which covers the screen
            // if (vertexIndex == 0u) {
            //     output.position = vec4f(-1.0, -1.0, 0.0, 1.0);
            //     output.v_uv = vec2f(0.0, 0.0);
            // } else if (vertexIndex == 1u) {
            //     output.position = vec4f(3.0, -1.0, 0.0, 1.0);
            //     output.v_uv = vec2f(2.0, 0.0);
            // } else {
            //     output.position = vec4f(-1.0, 3.0, 0.0, 1.0);
            //     output.v_uv = vec2f(0.0, 2.0);
            // }
            // flip y (webgpu render textures are flipped)
            output.v_uv.y = 1.0 - output.v_uv.y;
            return output;
        }
        )glsl"
    }

    // TODO lighting
    // TODO normal matrix
    // TODO helper fns (srgb to linear, linear to srgb, etc)
};


static const char* uv_shader_string  = R"glsl(
#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT

struct VertexOutput {
    @builtin(position) position : vec4f,
    @location(0) v_uv : vec2f,
};

@vertex 
fn vs_main(in : VertexInput) -> VertexOutput
{
    var out : VertexOutput;
    var u_Draw : DrawUniforms = drawInstances[in.instance];

    out.position = (u_Frame.projection * u_Frame.view) * u_Draw.model * vec4f(in.position, 1.0f);
    out.v_uv = in.uv;

    return out;
}

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    return vec4f(in.v_uv, 0.0, 1.0);
}
)glsl";

static const char* normal_shader_string  = R"glsl(
#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT

struct VertexOutput {
    @builtin(position) position : vec4f,
    @location(0) v_world_normal : vec3f,
    @location(1) v_local_normal : vec3f,
};

@vertex 
fn vs_main(in : VertexInput) -> VertexOutput
{
    var out : VertexOutput;
    var u_Draw : DrawUniforms = drawInstances[in.instance];

    out.position = (u_Frame.projection * u_Frame.view) * u_Draw.model * vec4f(in.position, 1.0f);
    // TODO: after wgsl adds matrix inverse, calculate normal matrix here
    // out.v_normal = (transpose(inverse(u_Frame.viewMat * u_Draw.model)) * vec4f(in.normal, 0.0)).xyz;
    out.v_world_normal = (u_Draw.model * vec4f(in.normal, 0.0)).xyz;
    out.v_local_normal = in.normal;

    return out;
}


// our custom material uniforms
@group(1) @binding(0) var<uniform> world_space_normals: i32;

// don't actually need normals/tangents
@fragment 
fn fs_main(in : VertexOutput, @builtin(front_facing) is_front: bool) -> @location(0) vec4f
{   
    var world_normal = in.v_world_normal;
    var local_normal = in.v_local_normal;
    if (!is_front) {
        world_normal = -world_normal;
        local_normal = -local_normal;
    }

    if (world_space_normals == 1) {
        return vec4f(max(world_normal, vec3f(0.0)), 1.0);
    } else {
        return vec4f(max(local_normal, vec3f(0.0)), 1.0);
    }
}
)glsl";

static const char* tangent_shader_string  = R"glsl(
#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT

struct VertexOutput {
    @builtin(position) position : vec4f,
    @location(0) v_world_tangent : vec4f,
    @location(1) v_local_tangent : vec4f,
};

@vertex 
fn vs_main(in : VertexInput) -> VertexOutput
{
    var out : VertexOutput;
    var u_Draw : DrawUniforms = drawInstances[in.instance];

    let modelMat3 : mat3x3<f32> = mat3x3(
        u_Draw.model[0].xyz,
        u_Draw.model[1].xyz,
        u_Draw.model[2].xyz
    );

    out.position = (u_Frame.projection * u_Frame.view) * u_Draw.model * vec4f(in.position, 1.0f);

    // tangent vectors aren't impacted by non-uniform scaling or translation
    out.v_world_tangent = vec4f(modelMat3 * in.tangent.xyz, in.tangent.w);
    out.v_local_tangent = in.tangent;

    return out;
}

// our custom material uniforms
@group(1) @binding(0) var<uniform> world_space_tangents: i32;

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    if (world_space_tangents == 1) {
        return vec4f(max(in.v_world_tangent.xyz, vec3f(0.0)), 1.0);
    } else {
        return vec4f(max(in.v_local_tangent.xyz, vec3f(0.0)), 1.0);
    }
}
)glsl";


static const char* flat_shader_string  = R"glsl(
#include FRAME_UNIFORMS
#include DRAW_UNIFORMS
#include STANDARD_VERTEX_INPUT
#include STANDARD_VERTEX_OUTPUT
#include STANDARD_VERTEX_SHADER

// our custom material uniforms
@group(1) @binding(0) var<uniform> u_color : vec4f;
@group(1) @binding(1) var u_sampler : sampler;
@group(1) @binding(2) var u_color_map : texture_2d<f32>;

// don't actually need normals/tangents
@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    let tex = textureSample(u_color_map, u_sampler, in.v_uv);
    var ret = u_color * tex;
    ret.a = clamp(ret.a, 0.0, 1.0);

    // alpha test
    if (ret.a < .01) {
        discard;
    }

    return ret;
}
)glsl";

static const char* phong_shader_string = R"glsl(
    // phong impl based off obj material model
    // actually, blinn* phong
    // see https://www.fileformat.info/format/material/

    #include FRAME_UNIFORMS
    #include LIGHTING_UNIFORMS
    #include DRAW_UNIFORMS
    #include STANDARD_VERTEX_INPUT
    #include STANDARD_VERTEX_OUTPUT
    #include STANDARD_VERTEX_SHADER

    @group(1) @binding(0) var<uniform> u_specular_color : vec3f;
    @group(1) @binding(1) var<uniform> u_diffuse_color : vec4f;
    @group(1) @binding(2) var<uniform> u_shininess : f32; // range from (0, 2^n). must be > 0. logarithmic scale.
    @group(1) @binding(3) var<uniform> u_emission_color : vec3f;
    @group(1) @binding(4) var<uniform> u_normal_factor : f32;
    @group(1) @binding(5) var<uniform> u_ao_factor : f32; // 0 disables ao

    @group(1) @binding(6) var texture_sampler: sampler;
    @group(1) @binding(7) var u_diffuse_map: texture_2d<f32>;   
    @group(1) @binding(8) var u_specular_map: texture_2d<f32>;
    @group(1) @binding(9) var u_ao_map: texture_2d<f32>;
    @group(1) @binding(10) var u_emissive_map: texture_2d<f32>;
    @group(1) @binding(11) var u_normal_map: texture_2d<f32>;

    // calculate envmap contribution
    // vec3 CalcEnvMapContribution(vec3 viewDir, vec3 norm)
    // {
    //     // TODO: envmap can be optimized by moving some calculations into vertex shader
    //     vec3 envMapSampleDir = vec3(0.0);
    //     if (u_EnvMapParams.method == ENV_MAP_METHOD_REFLECTION) {
    //         envMapSampleDir = reflect(-viewDir, norm);
    //     }
    //     else if (u_EnvMapParams.method == ENV_MAP_METHOD_REFRACTION) {
    //         envMapSampleDir = refract(-viewDir, norm, u_EnvMapParams.ratio);
    //     }
    //     return texture(u_Skybox, envMapSampleDir).rgb;
    // }

    fn srgbToLinear(srgb_in : vec3f) -> vec3f {
        return pow(srgb_in.rgb,vec3f(2.2));
    }

    fn calculateNormal(inNormal: vec3f, inUV : vec2f, inTangent: vec4f, scale: f32, is_front : bool) -> vec3f {
        var tangentNormal : vec3f = textureSample(u_normal_map, texture_sampler, inUV).rgb * 2.0 - 1.0;
        tangentNormal.x *= scale;
        tangentNormal.y *= scale;

        let N : vec3f = normalize(inNormal);
        let T : vec3f = normalize(inTangent.xyz);
        let B : vec3f = inTangent.w * normalize(cross(N, T));  // mikkt method
        let TBN : mat3x3f = mat3x3(T, B, N);

        var normal = normalize(TBN * tangentNormal);
        if (!is_front) {
            normal = -normal;
        }
        return normal;
    }

    // main =====================================================================================
    @fragment 
    fn fs_main(
        in : VertexOutput,
        @builtin(front_facing) is_front: bool,
    ) -> @location(0) vec4f
    {
        var normal = calculateNormal(in.v_normal, in.v_uv, in.v_tangent, u_normal_factor, is_front);

        let viewDir = normalize(u_Frame.camera_pos - in.v_worldpos);  // direction from camera to this frag

        // material color properties (ignore alpha channel for now)
        let diffuseTex = textureSample(u_diffuse_map, texture_sampler, in.v_uv);
        let specularTex = textureSample(u_specular_map, texture_sampler, in.v_uv);
        let aoTex = textureSample(u_ao_map, texture_sampler, in.v_uv);
        let emissiveTex = textureSample(u_emissive_map, texture_sampler, in.v_uv);
        // factor ao into diffuse
        var diffuse_color = u_diffuse_color.rgb * srgbToLinear(diffuseTex.rgb);
        diffuse_color = mix(diffuse_color, diffuse_color * aoTex.r, u_ao_factor);
        let specular_color : vec3f = (specularTex.rgb * u_specular_color);

        var lighting = vec3f(0.0); // accumulate lighting
        for (var i = 0; i < u_Frame.num_lights; i++) {
            let light = u_lights[i];
            var L : vec3f = vec3(0.0);
            var radiance : vec3f = vec3(0.0);
            switch (light.light_type) {
                case 1: { // directional
                    let lightDir = normalize(-light.direction);
                    let halfwayDir = normalize(lightDir + viewDir);
                    // diffuse shading
                    let diffuse_factor : f32 = max(dot(normal, lightDir), 0.0);
                    // specular shading
                    let specular_factor : f32 = max(0.0, pow(max(dot(normal, halfwayDir), 0.0), u_shininess));
                    let diffuse : vec3f = diffuse_factor * diffuse_color;
                    let specular : vec3f = specular_factor * specular_color.rgb;
                    // combine results
                    lighting +=  (light.color * (diffuse+ specular));
                } 
                case 2: { // point
                    // calculate attenuation
                    let dist = distance(in.v_worldpos, light.position);
                    let attenuation = pow(
                        clamp(1.0 - dist / light.point_radius, 0.0, 1.0), 
                        light.point_falloff
                    );
                    let radiance : vec3f = light.color * attenuation;

                    let lightDir = normalize(light.position - in.v_worldpos);
                    let halfwayDir = normalize(lightDir + viewDir);

                    // diffuse 
                    let diffuse_factor = max(dot(normal, lightDir), 0.0);
                    // specular 
                    let specular_factor = max(pow(max(dot(normal, halfwayDir), 0.0), u_shininess), 0.0);

                    // combine results
                    lighting += radiance * (diffuse_color * diffuse_factor + specular_color.rgb * specular_factor);
                }
                default: {} // no light
            } // end switch light type
        }  // end light loop

        // ambient light
        lighting += u_Frame.ambient_light * diffuse_color;

        // emissive
        lighting += srgbToLinear(emissiveTex.rgb) * u_emission_color;

        // calculate envmap contribution
        // if (u_EnvMapParams.enabled) {
        //     vec3 envMapContrib = CalcEnvMapContribution(viewDir, norm);
        //     // blending
        //     if (u_EnvMapParams.blendMode == ENV_MAP_BLEND_MODE_ADDITIVE) {
        //         lighting += (u_EnvMapParams.intensity * envMapContrib);
        //     }
        //     else if (u_EnvMapParams.blendMode == ENV_MAP_BLEND_MODE_MULTIPLICATIVE) {
        //         lighting *= (u_EnvMapParams.intensity * envMapContrib);
        //     }
        //     else if (u_EnvMapParams.blendMode == ENV_MAP_BLEND_MODE_MIX) {
        //         lighting = mix(lighting, envMapContrib, u_EnvMapParams.intensity);
        //     }
        // }

        // alpha test
        if (diffuseTex.a < .01) {
            discard;
        }

        return vec4f(
            lighting, 
            diffuseTex.a
        );
    }
)glsl";

static const char* lines2d_shader_string  = R"glsl(

#include FRAME_UNIFORMS

// line material uniforms
@group(1) @binding(0) var<uniform> u_line_width: f32;
@group(1) @binding(1) var<uniform> u_color: vec3f;

#include DRAW_UNIFORMS

// stored as [x0, y0, x1, y1, ...]
@group(3) @binding(0) var<storage, read> positions : array<f32>; // vertex pulling group 
@group(3) @binding(1) var<storage, read> u_color_array: array<f32>; // per-vertex color rgb

struct VertexInput {
    @builtin(instance_index) instance : u32,
};

struct VertexOutput {
    @builtin(position) position : vec4f,
    @location(0) v_color : vec3f,
};

// for bevel joins
fn getPos(vertex_idx : u32) -> vec2f 
{
    let pos_idx = vertex_idx / 4u; // 4 vertices per line segment in bevel join
    return vec2f(
        positions[2u * pos_idx + 0u],  // x
        positions[2u * pos_idx + 1u]   // y
    );
}

fn calculate_line_pos(vertex_id : u32) -> vec2f
{
    let vertex_idx = vertex_id + 4u; // adding 4 to account for sentinel start point

    let this_pos = getPos(vertex_idx); // input segment pos
    let next_pos = getPos(vertex_idx + 4u);
    let prev_pos = getPos(vertex_idx - 4u);
    var pos = vec2f(0.0); // final extruded pos
    let bevel_idx = vertex_id % 4u;

    let prev_dir = normalize(this_pos - prev_pos);
    let next_dir = normalize(next_pos - this_pos);
    let prev_dir_perp = vec2f(-prev_dir.y, prev_dir.x);
    let next_dir_perp = vec2f(-next_dir.y, next_dir.x);

    // determine orientation from whether vectors are cw or ccw
    var orientation = 1.0;
    var ccw : bool = dot(-prev_dir_perp, next_dir) > 0.0; // next dir is ccw to prev dir
    let cw : bool = !ccw;
    if (cw) {
        orientation = -1.0;
    }

    // every 4 vertices 
    /*
    if ccw, we go bevel --> miter --> bevel --> miter
    if cw, we go miter --> bevel --> miter --> bevel
    */

    if (
        (ccw && (bevel_idx == 1u || bevel_idx == 3u))
        ||
        (cw && (bevel_idx == 0u || bevel_idx == 2u))
    ) {
        let miter_dir = 1.0 * normalize(prev_dir_perp + next_dir_perp);
        let max_miter_len = min(length(next_pos - this_pos), length(prev_pos - this_pos));
        let miter_length = clamp((u_line_width * 0.5) / dot(miter_dir, prev_dir_perp), 0.0, max_miter_len);
        
        pos = this_pos - orientation * miter_length * miter_dir;
    }
    else if (
        (ccw && bevel_idx == 0u)
        ||
        (cw && bevel_idx == 1u)
    ) {
        pos = this_pos + orientation * (u_line_width * 0.5) * prev_dir_perp;
    }
    else if (
        (ccw && bevel_idx == 2u)
        || 
        (cw && bevel_idx == 3u)
    ) {
        pos = this_pos + orientation * (u_line_width * 0.5) * next_dir_perp;
    }

    return pos;
}

@vertex 
fn vs_main(
    in : VertexInput,
    @builtin(vertex_index) vertex_id : u32
) -> VertexOutput
{
    var out : VertexOutput;
    var u_Draw : DrawUniforms = drawInstances[in.instance];

    let worldpos = u_Draw.model * vec4f(calculate_line_pos(vertex_id), 0.0, 1.0);
    out.position = (u_Frame.projection * u_Frame.view) * worldpos;

    // color
    out.v_color = vec3f(1.0);
    var pos_idx = max(i32(vertex_id / 4u), 0);
    let num_colors = i32(arrayLength(&u_color_array));
    if (num_colors > 0) {
        out.v_color = vec3f(
            u_color_array[(3 * pos_idx + 0) % num_colors],
            u_color_array[(3 * pos_idx + 1) % num_colors],
            u_color_array[(3 * pos_idx + 2) % num_colors]
        );
    }

    return out;
}

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    return vec4f(u_color * in.v_color, 1.0);
}
)glsl";

static const char* points_shader_string  = R"glsl(

#include FRAME_UNIFORMS

// line material uniforms
@group(1) @binding(0) var<uniform> u_point_global_color : vec4f;
@group(1) @binding(1) var<uniform> u_point_global_size : f32;
@group(1) @binding(2) var u_point_sampler : sampler;
@group(1) @binding(3) var u_point_texture : texture_2d<f32>;

#include DRAW_UNIFORMS

// every 5 f32s is a vertex:  (x, y, z, uv.x, uv,y)
@group(3) @binding(0) var<storage, read> u_point_vertices: array<f32>; // vertex attributes (currently just a plane)
// (r,g,b)
@group(3) @binding(1) var<storage, read> u_point_colors: array<f32>; // per-point color (for now rgb)
@group(3) @binding(2) var<storage, read> u_point_sizes: array<f32>; // per-point size (single f32)
@group(3) @binding(3) var<storage, read> u_point_positions: array<f32>; // per-point positoins (x, y, z)

struct VertexOutput {
    @builtin(position) position : vec4f,
    @location(0) v_color : vec3f,
    @location(1) v_uv : vec2f,
};

@vertex 
fn vs_main(
    @builtin(instance_index) instance_id : u32,
    @builtin(vertex_index) vertex_id : u32
) -> VertexOutput
{
    var out : VertexOutput;
    var u_Draw : DrawUniforms = drawInstances[instance_id];

    let point_idx = i32(vertex_id / 6u); // 6 vertices per point plane
    let vertex_idx = i32(vertex_id % 6u); // 6 vertices per point plane

    let point_uv = vec2f(
        u_point_vertices[5 * vertex_idx + 3],
        u_point_vertices[5 * vertex_idx + 4]
    );

    let num_colors = i32(arrayLength(&u_point_colors));
    var point_color = u_point_global_color.rgb;
    if (num_colors > 0) {
        point_color *= vec3f(
            u_point_colors[(3 * point_idx + 0) % num_colors],
            u_point_colors[(3 * point_idx + 1) % num_colors],
            u_point_colors[(3 * point_idx + 2) % num_colors]
        );
    }

    var point_size = u_point_global_size;
    let num_sizes = i32(arrayLength(&u_point_sizes));
    if (num_sizes > 0) {
        point_size *= u_point_sizes[point_idx % num_sizes];
    }

    // first apply scale
    var point_pos = point_size * vec3f(
        u_point_vertices[5 * vertex_idx + 0],
        u_point_vertices[5 * vertex_idx + 1],
        u_point_vertices[5 * vertex_idx + 2],
    );

    // then translation
    point_pos += vec3f(
        u_point_positions[3 * point_idx + 0],
        u_point_positions[3 * point_idx + 1],
        u_point_positions[3 * point_idx + 2]
    );

    out.position = (u_Frame.projection * u_Frame.view * u_Draw.model) * vec4f(point_pos, 1.0);
    out.v_color = point_color;
    out.v_uv = point_uv;

    return out;
}

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    let tex = textureSample(u_point_texture, u_point_sampler, in.v_uv);

    // alpha test
    if (tex.a < .01) {
        discard;
    }

    return vec4f(in.v_color * tex.rgb, tex.a);
}

)glsl";


// ----------------------------------------------------------------------------

static const char* pbr_shader_string = R"glsl(
    // includes
    #include FRAME_UNIFORMS
    #include DRAW_UNIFORMS
    #include LIGHTING_UNIFORMS
    #include STANDARD_VERTEX_INPUT
    #include STANDARD_VERTEX_OUTPUT
    #include STANDARD_VERTEX_SHADER

    // textures
    @group(1) @binding(0) var texture_sampler: sampler;
    @group(1) @binding(1) var albedoMap: texture_2d<f32>;
    @group(1) @binding(2) var normalMap: texture_2d<f32>;
    @group(1) @binding(3) var aoMap: texture_2d<f32>;
    @group(1) @binding(4) var mrMap: texture_2d<f32>;
    @group(1) @binding(5) var emissiveMap: texture_2d<f32>;

    // uniforms
    @group(1) @binding(6) var<uniform> u_baseColor: vec4f;
    @group(1) @binding(7) var<uniform> u_emissiveFactor: vec3f;
    @group(1) @binding(8) var<uniform> u_metallic: f32;
    @group(1) @binding(9) var<uniform> u_roughness: f32;
    @group(1) @binding(10) var<uniform> u_normalFactor: f32;
    @group(1) @binding(11) var<uniform> u_aoFactor: f32;

    fn srgbToLinear(srgb_in : vec3f) -> vec3f {
        return pow(srgb_in.rgb,vec3f(2.2));
    }

    fn calculateNormal(inNormal: vec3f, inUV : vec2f, inTangent: vec4f, scale: f32, is_front : bool) -> vec3f {
        var tangentNormal : vec3f = textureSample(normalMap, texture_sampler, inUV).rgb * 2.0 - 1.0;
        // scale normal
        // ref: https://github.com/mrdoob/three.js/blob/dev/src/renderers/shaders/ShaderChunk/normal_fragment_maps.glsl.js
        tangentNormal.x *= scale;
        tangentNormal.y *= scale;

        // TODO: do we need to adjust tangent normal based on face direction (backface or frontface)?
        // e.g. tangentNormal *= (sign(dot(normal, faceNormal)))
        // don't think so since normal map is in tangent space

        // from mikkt:
        // For normal maps it is sufficient to use the following simplified version
        // of the bitangent which is generated at pixel/vertex level. 
        // bitangent = fSign * cross(vN, tangent);
        let N : vec3f = normalize(inNormal);
        let T : vec3f = normalize(inTangent.xyz);
        let B : vec3f = inTangent.w * normalize(cross(N, T));  // mikkt method
        let TBN : mat3x3f = mat3x3(T, B, N);

        // return inTangent.xyz;
        var normal = normalize(TBN * tangentNormal);
        if (!is_front) {
            normal = -normal;
        }
        return normal;
    }

    const PI = 3.1415926535897932384626433832795;
    const reflectivity = 0.04;  // heuristic, assume F0 of 0.04 for all dielectrics

    // Normal Distribution function ----------------------------------------------
    fn D_GGX(dotNH : f32, roughness : f32) -> f32 {
        let alpha : f32 = roughness * roughness;
        let alpha2 : f32 = alpha * alpha;
        let denom : f32 = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
        return (alpha2)/(PI * denom*denom);
    }

    // Geometric Shadowing function ----------------------------------------------
    fn G_SchlickSmithGGX(dotNL : f32, dotNV : f32, roughness : f32) -> f32 {
        let r : f32 = (roughness + 1.0);
        let k : f32 = (r*r) / 8.0;

        let GL : f32 = dotNL / (dotNL * (1.0 - k) + k);
        let GV : f32 = dotNV / (dotNV * (1.0 - k) + k);
        return GL * GV;

    }

    // Fresnel function ----------------------------------------------------------
    // cosTheta assumed to be in range [0, 1]
    fn F_Schlick(cosTheta : f32, F0 : vec3<f32>) -> vec3f {
        return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
    }

    @fragment 
    fn fs_main(
        in : VertexOutput,
        @builtin(front_facing) is_front: bool
    ) -> @location(0) vec4f
    {
        let N : vec3f = calculateNormal(in.v_normal, in.v_uv, in.v_tangent, u_normalFactor, is_front);
        let V : vec3f = normalize(u_Frame.camera_pos - in.v_worldpos);

        // linear-space albedo (normally authored in sRGB space so we have to convert to linear space)
        // transparency not supported
        let albedo: vec3f = u_baseColor.rgb * srgbToLinear(textureSample(albedoMap, texture_sampler, in.v_uv).rgb);
        
        // The metallicRoughnessTexture contains the metalness value in the "blue" color channel, 
        // and the roughness value in the "green" color channel.
        let metallic_roughness = textureSample(mrMap, texture_sampler, in.v_uv);
        let metallic : f32 = metallic_roughness.b * u_metallic;
        let roughness : f32 = metallic_roughness.g * u_roughness;

        var F0 : vec3f = vec3f(reflectivity);
        F0 = mix(F0, albedo.rgb, metallic); // reflectivity for metals

        var Lo : vec3f = vec3(0.0);
        // loop over all lights
        for (var i = 0; i < u_Frame.num_lights; i++) {
            let light = u_lights[i];
            var L : vec3f = vec3(0.0);
            var radiance : vec3f = vec3(0.0);
            switch (light.light_type) {
                case 1: { // directional
                    L = normalize(-light.direction);
                    radiance = light.color;
                }
                case 2: { // point
                    L = normalize(light.position - in.v_worldpos);
                    let dist = distance(in.v_worldpos, light.position);
                    let attenuation = pow(
                        clamp(1.0 - dist / light.point_radius, 0.0, 1.0), 
                        light.point_falloff
                    );
                    radiance = light.color * attenuation;
                }
                default: { } // no light
            } // end switch light type

            // half vector
            let H : vec3f = normalize(V + L);
            let dotNH : f32 = clamp(dot(N, H), 0.0, 1.0);
            let dotNV : f32 = clamp(dot(N, V), 0.0, 1.0);
            let dotNL : f32 = clamp(dot(N, L), 0.0, 1.0);

            if (dotNL > 0.0) {
                // D = Normal distribution (Distribution of the microfacets)
                let D : f32 = D_GGX(dotNH, roughness);
                // G = Geometric shadowing term (Microfacets shadowing)
                let G : f32 = G_SchlickSmithGGX(dotNL, dotNV, roughness);
                // F = Fresnel factor (Reflectance depending on angle of incidence)
                let F : vec3<f32> = F_Schlick(max(dot(H, V), 0.0), F0);

                // specular contribution
                let spec : vec3f = D * F * G / (4.0 * dotNL * dotNV + 0.0001);
                // diffuse contribution
                let kD : vec3f = (vec3f(1.0) - F) * (1.0 - metallic);
                // final color contribution
                Lo += (kD * albedo / PI + spec) * dotNL * radiance;
            }
        }  // end light loop

        // // ambient occlusion (hardcoded for now) (ambient should only be applied to direct lighting, not indirect lighting)
        let ambient : vec3f = u_Frame.ambient_light * albedo * textureSample(aoMap, texture_sampler, in.v_uv).r * u_aoFactor;
        var finalColor : vec3f = Lo + ambient;  // TODO: update ao calculation after adding IBL

        // add emission
        let emissiveColor : vec3f = srgbToLinear(textureSample(emissiveMap, texture_sampler, in.v_uv).rgb);
        finalColor += emissiveColor * u_emissiveFactor;

        return vec4f(finalColor, u_baseColor.a);
    }
)glsl";

static const char* mipMapShader = CODE(
    var<private> pos : array<vec2<f32>, 3> = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0), 
        vec2<f32>(-1.0, 3.0), 
        vec2<f32>(3.0, -1.0)
    );

    struct VertexOutput {
        @builtin(position) position : vec4<f32>,
        @location(0) texCoord : vec2<f32>,
    }

    @vertex
    fn vs_main(@builtin(vertex_index) vertexIndex : u32) -> VertexOutput {
        var output : VertexOutput;
        // remap uvs to [0, 2] and flip y
        // rasterizer will clip uvs to [0, 1]
        output.texCoord = pos[vertexIndex] * vec2<f32>(0.5, -0.5) + vec2<f32>(0.5);
        // positions in ndc space
        output.position = vec4<f32>(pos[vertexIndex], 0.0, 1.0);
        return output;
    }

    @group(0) @binding(0) var imgSampler : sampler;
    @group(0) @binding(1) var img : texture_2d<f32>;  // image to mip

    @fragment
    fn fs_main(@location(0) texCoord : vec2<f32>) -> @location(0) vec4<f32> {
        return textureSample(img, imgSampler, texCoord);
    }
);

const char* gtext_shader_string = R"glsl(

    // Based on: http://wdobbie.com/post/gpu-text-rendering-with-vector-textures/

    #include FRAME_UNIFORMS
    #include DRAW_UNIFORMS

    // custom material uniforms
    @group(1) @binding(0) var<storage, read> u_Glyphs: array<i32>;
    @group(1) @binding(1) var<storage, read> u_Curves: array<f32>;
    @group(1) @binding(2) var<uniform> u_Color: vec4f;

    // Controls for debugging and exploring:

    // Size of the window (in pixels) used for 1-dimensional anti-aliasing along each rays.
    //   0 - no anti-aliasing
    //   1 - normal anti-aliasing
    // >=2 - exaggerated effect 
    @group(1) @binding(3) var<uniform> antiAliasingWindowSize: f32;

    // Enable a second ray along the y-axis to achieve 2-dimensional anti-aliasing.
    // set to 1 to enable, 0 to disable
    @group(1) @binding(4) var<uniform> enableSuperSamplingAntiAliasing: i32;

    @group(1) @binding(5) var<uniform> bb : vec4f; // x = minx, y = miny, z = maxx, w = maxy
    @group(1) @binding(6) var texture_map: texture_2d<f32>;
    @group(1) @binding(7) var texture_sampler: sampler;


    struct VertexInput {
        @location(0) position : vec2f,
        @location(1) uv : vec2f,
        @location(2) glyph_index : i32, // index into glyphs array (which itself is slice into curves array)
        @builtin(instance_index) instance : u32,
    };

    struct VertexOutput {
        @builtin(position) position : vec4f,
        @location(0) v_uv : vec2f, // per-glyph uv
        @location(1) @interpolate(flat) v_buffer_index: i32,
        @location(2) v_uv_textbox : vec2f, // entire GText uv (e.g. if you want to texture your text)
    };

    @vertex 
    fn vs_main(in : VertexInput) -> VertexOutput
    {
        var out : VertexOutput;
        var u_Draw : DrawUniforms = drawInstances[in.instance];
        out.position = (u_Frame.projection * u_Frame.view) * u_Draw.model * vec4f(in.position, 0.0f, 1.0f);
        out.v_uv     = in.uv;
        out.v_buffer_index = in.glyph_index;

        let bb_w = bb.z - bb.x;
        let bb_h = bb.w - bb.y;
        out.v_uv_textbox = (in.position - vec2f(bb.x, bb.y)) / vec2f(bb_w, bb_h);

        return out;
    }


    struct Glyph {
        start : i32,
        count : i32,
    };

    struct Curve {
        p0 : vec2f,
        p1 : vec2f,
        p2 : vec2f,
    };

    fn loadGlyph(index : i32) -> Glyph {
        var result : Glyph;
        // let data = u_Glyphs[index].xy;
        // result.start = u32(data.x);
        // result.count = u32(data.y);
        result.start = u_Glyphs[2 * index + 0];
        result.count = u_Glyphs[2 * index + 1];
        return result;
    }

    fn loadCurve(index : i32) -> Curve {
        var result : Curve;
        // result.p0 = u_Curves[3u * index + 0u].xy;
        // result.p1 = u_Curves[3u * index + 1u].xy;
        // result.p2 = u_Curves[3u * index + 2u].xy;
        result.p0 = vec2f(u_Curves[6 * index + 0], u_Curves[6 * index + 1]);
        result.p1 = vec2f(u_Curves[6 * index + 2], u_Curves[6 * index + 3]);
        result.p2 = vec2f(u_Curves[6 * index + 4], u_Curves[6 * index + 5]);
        return result;
    }

    fn computeCoverage(inverseDiameter : f32, p0 : vec2f, p1 : vec2f, p2 : vec2f) -> f32 {
        if (p0.y > 0.0 && p1.y > 0.0 && p2.y > 0.0) { return 0.0; }
        if (p0.y < 0.0 && p1.y < 0.0 && p2.y < 0.0) { return 0.0; }

        // Note: Simplified from abc formula by extracting a factor of (-2) from b.
        let a = p0 - 2.0*p1 + p2;
        let b = p0 - p1;
        let c = p0;

        var t0 : f32;
        var t1 : f32;
        if (abs(a.y) >= 1e-5) {
            // Quadratic segment, solve abc formula to find roots.
            let radicand : f32 = b.y*b.y - a.y*c.y;
            if (radicand <= 0.0) { return 0.0; }
        
            let s : f32 = sqrt(radicand);
            t0 = (b.y - s) / a.y;
            t1 = (b.y + s) / a.y;
        } else {
            // Linear segment, avoid division by a.y, which is near zero.
            // There is only one root, so we have to decide which variable to
            // assign it to based on the direction of the segment, to ensure that
            // the ray always exits the shape at t0 and enters at t1. For a
            // quadratic segment this works 'automatically', see readme.
            let t : f32 = p0.y / (p0.y - p2.y);
            if (p0.y < p2.y) {
                t0 = -1.0;
                t1 = t;
            } else {
                t0 = t;
                t1 = -1.0;
            }
        }

        var alpha : f32 = 0.0;
        
        if (t0 >= 0.0 && t0 < 1.0) {
            let x : f32 = (a.x*t0 - 2.0*b.x)*t0 + c.x;
            alpha += clamp(x * inverseDiameter + 0.5, 0.0, 1.0);
        }

        if (t1 >= 0.0 && t1 < 1.0) {
            let x = (a.x*t1 - 2.0*b.x)*t1 + c.x;
            alpha -= clamp(x * inverseDiameter + 0.5, 0.0, 1.0);
        }

        return alpha;
    }

    fn rotate(v : vec2f) -> vec2f {
        return vec2f(v.y, -v.x);
    }

    @fragment
    fn fs_main(in : VertexOutput) -> @location(0) vec4f {
        var alpha : f32 = 0.0;

        // Inverse of the diameter of a pixel in uv units for anti-aliasing.
        let inverseDiameter = 1.0 / (antiAliasingWindowSize * fwidth(in.v_uv));

        let glyph = loadGlyph(in.v_buffer_index);
        for (var i : i32 = 0; i < glyph.count; i++) {
            let curve = loadCurve(glyph.start + i);

            let p0 = curve.p0 - in.v_uv;
            let p1 = curve.p1 - in.v_uv;
            let p2 = curve.p2 - in.v_uv;

            alpha += computeCoverage(inverseDiameter.x, p0, p1, p2);
            if (bool(enableSuperSamplingAntiAliasing)) {
                alpha += computeCoverage(inverseDiameter.y, rotate(p0), rotate(p1), rotate(p2));
            }
        }

        if (bool(enableSuperSamplingAntiAliasing)) {
            alpha *= 0.5;
        }

        alpha = clamp(alpha, 0.0, 1.0);
        let result = u_Color * alpha;
        let sample = textureSample(texture_map, texture_sampler, in.v_uv_textbox);

        // alpha test
        if (result.a < 0.001) {
            discard;
        }

        return result * sample;
        // return vec4f(in.v_uv_textbox, 0.0, 1.0);
    }
)glsl";


const char* default_postprocess_shader_string = R"glsl(
    #include SCREEN_PASS_VERTEX_SHADER

    @fragment 
    fn fs_main(in : VertexOutput) -> @location(0) vec4f {
        return vec4f(in.v_uv, 0.0, 1.0);
    }
)glsl";


const char* output_pass_shader_string = R"glsl(
    #include SCREEN_PASS_VERTEX_SHADER

    const TONEMAP_NONE = 0;
    const TONEMAP_LINEAR = 1;
    const TONEMAP_REINHARD = 2;
    const TONEMAP_CINEON = 3;
    const TONEMAP_ACES = 4;
    const TONEMAP_UNCHARTED = 5;

    @group(0) @binding(0) var texture: texture_2d<f32>;
    @group(0) @binding(1) var texture_sampler: sampler;
    @group(0) @binding(2) var<uniform> u_Gamma: f32;
    @group(0) @binding(3) var<uniform> u_Exposure: f32;
    @group(0) @binding(4) var<uniform> u_Tonemap: i32 = TONEMAP_NONE;

    // Helpers ==================================================================
    fn Uncharted2Tonemap(x: vec3<f32>) -> vec3<f32> {
        let A: f32 = 0.15;
        let B: f32 = 0.5;
        let C: f32 = 0.1;
        let D: f32 = 0.2;
        let E: f32 = 0.02;
        let F: f32 = 0.3;
        return (x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F) - E / F;
    } 

    // source: https://github.com/selfshadow/ltc_code/blob/master/webgl/shaders/ltc/ltc_blit.fs
    fn rrt_odt_fit(v: vec3<f32>) -> vec3<f32> {
        let a: vec3<f32> = v * (v + 0.0245786) - 0.000090537;
        let b: vec3<f32> = v * (0.983729 * v + 0.432951) + 0.238081;
        return a / b;
    } 

    fn mat3_from_rows(c0: vec3<f32>, c1: vec3<f32>, c2: vec3<f32>) -> mat3x3<f32> {
        var m: mat3x3<f32> = mat3x3<f32>(c0, c1, c2);
        m = transpose(m);
        return m;
    } 

    // main =====================================================================
    @fragment 
    fn fs_main(in : VertexOutput) -> @location(0) vec4f {
        let hdrColor: vec4<f32> = textureSample(texture, texture_sampler, in.v_uv);
        var color: vec3<f32> = hdrColor.rgb;
        if (u_Tonemap != TONEMAP_NONE) {
            color = color * (u_Exposure);
        }
        switch (u_Tonemap) {
        case 1: { // linear
            color = clamp(color, vec3f(0.), vec3f(1.));
        }
        case 2: { // reinhard
            color = hdrColor.rgb / (hdrColor.rgb + vec3<f32>(1.));
        }
        case 3: { // cineon
            let x: vec3<f32> = max(vec3<f32>(0.), color - 0.004);
            color = x * (6.2 * x + 0.5) / (x * (6.2 * x + 1.7) + 0.06);
            // color = pow(color, vec3<f32>(u_Gamma));
            color = pow(color, vec3<f32>(2.2));  // invert gamma correction (assumes final output to srgb texture)
            // Note: will need to change this if we want to output to linear texture and do gamma correction ourselves
        } 
        case 4: { // aces
            var ACES_INPUT_MAT: mat3x3<f32> = mat3_from_rows(vec3<f32>(0.59719, 0.35458, 0.04823), vec3<f32>(0.076, 0.90834, 0.01566), vec3<f32>(0.0284, 0.13383, 0.83777));
            var ACES_OUTPUT_MAT: mat3x3<f32> = mat3_from_rows(vec3<f32>(1.60475, -0.53108, -0.07367), vec3<f32>(-0.10208, 1.10813, -0.00605), vec3<f32>(-0.00327, -0.07276, 1.07602));
            color = color / 0.6;
            color = ACES_INPUT_MAT * color;
            color = rrt_odt_fit(color);
            color = ACES_OUTPUT_MAT * color;
            color = clamp(color, vec3f(0.), vec3f(1.));
        }
        case 5: { // uncharted
            let ExposureBias: f32 = 2.;
            let curr: vec3<f32> = Uncharted2Tonemap(ExposureBias * color);
            let W: f32 = 11.2;
            let whiteScale: vec3<f32> = vec3<f32>(1. / Uncharted2Tonemap(vec3<f32>(W)));
            color = curr * whiteScale;
        }
        default: {}
        }

        // gamma correction
        // 9/3/2024: assuming swapchain texture is always in srgb format so we DON'T gamma correct,
        // let the final canvas/backbuffer gamma correct for us
        color = pow(color, vec3<f32>(1. / u_Gamma));


        return vec4<f32>(color, 1.0); // how does alpha work?
        // return vec4<f32>(color, clamp(hdrColor.a, 0.0, 1.0));

        // return textureSample(texture, texture_sampler, in.v_uv); // passthrough
    } 



)glsl";


const char* bloom_downsample_screen_shader = R"glsl(

#include SCREEN_PASS_VERTEX_SHADER
@group(0) @binding(0) var u_texture: texture_2d<f32>; // texture at previous mip level
@group(0) @binding(1) var u_sampler: sampler;
@group(0) @binding(2) var<uniform> u_threshold: f32;
@group(0) @binding(3) var<uniform> u_full_res: vec2u; // full resolution of input texture

@fragment 
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    let input_dim = textureDimensions(u_texture).xy;
    let dx = 1.0 / f32(input_dim.x); // change in uv.x that corresponds to 1 pixel in input texture x direction
    let dy = 1.0 / f32(input_dim.y); // change in uv.y that corresponds to 1 pixel in input texture y direction
    let uv = in.v_uv;

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    let a = textureSample(u_texture, u_sampler, vec2f(uv.x - 2.0 * dx, uv.y + 2.0 * dy)).rgb;
    let b = textureSample(u_texture, u_sampler, vec2f(uv.x, uv.y + 2.0 * dy)).rgb;
    let c = textureSample(u_texture, u_sampler, vec2f(uv.x + 2.0 * dx, uv.y + 2.0 * dy)).rgb;

    let d = textureSample(u_texture, u_sampler, vec2f(uv.x - 2.0 * dx, uv.y)).rgb;
    let e = textureSample(u_texture, u_sampler, vec2f(uv.x, uv.y)).rgb;
    let f = textureSample(u_texture, u_sampler, vec2f(uv.x + 2.0 * dx, uv.y)).rgb;

    let g = textureSample(u_texture, u_sampler, vec2f(uv.x - 2.0 * dx, uv.y - 2.0 * dy)).rgb;
    let h = textureSample(u_texture, u_sampler, vec2f(uv.x, uv.y - 2.0 * dy)).rgb;
    let i = textureSample(u_texture, u_sampler, vec2f(uv.x + 2.0 * dx, uv.y - 2.0 * dy)).rgb;

    let j = textureSample(u_texture, u_sampler, vec2f(uv.x - dx, uv.y + dy)).rgb;
    let k = textureSample(u_texture, u_sampler, vec2f(uv.x + dx, uv.y + dy)).rgb;
    let l = textureSample(u_texture, u_sampler, vec2f(uv.x - dx, uv.y - dy)).rgb;
    let m = textureSample(u_texture, u_sampler, vec2f(uv.x + dx, uv.y - dy)).rgb;

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
    var downsample = vec3f(0.0);
    downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;

    if (all(input_dim == u_full_res)) {
        let brightness = max(max(downsample.r, downsample.g), downsample.b);
        let contribution = max(0.0, brightness - u_threshold) / max(brightness, 0.00001);
        downsample *= contribution;
    }

    return vec4f(downsample, 1.0);
}
)glsl";


const char* bloom_downsample_shader_string = R"glsl(

@group(0) @binding(0) var u_input_texture: texture_2d<f32>;
@group(0) @binding(1) var u_input_tex_sampler: sampler;
@group(0) @binding(2) var u_out_texture: texture_storage_2d<rgba16float, write>; // hdr
@group(0) @binding(3) var<uniform> u_mip_level: i32; 

@compute @workgroup_size(8, 8, 1)
fn main(
    @builtin(global_invocation_id) GlobalInvocationID : vec3<u32>,
    @builtin(workgroup_id) WorkGroupID : vec3<u32>,
    @builtin(local_invocation_index) LocalInvocationIndex : u32, // ranges from 0 - 63
    @builtin(local_invocation_id) LocalInvocationID : vec3<u32>
)
{
    // TODO use same texel size logic in upsample shader
    let output_size = textureDimensions(u_out_texture);
    let input_size = textureDimensions(u_input_texture);
    let pixel_coords = vec2i(GlobalInvocationID.xy);
    let x = 1.0 / f32(input_size.x);
    let y = 1.0 / f32(input_size.y);

    let uv        = (vec2f(GlobalInvocationID.xy) + vec2f(0.5)) / vec2f(output_size.xy);

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    let a = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - 2.0*x, uv.y + 2.0*y), 0.0).rgb;
    let b = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x,       uv.y + 2.0*y), 0.0).rgb;
    let c = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + 2.0*x, uv.y + 2.0*y), 0.0).rgb;
    let d = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - 2.0*x, uv.y), 0.0).rgb;
    let e = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x,       uv.y), 0.0).rgb;
    let f = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + 2.0*x, uv.y), 0.0).rgb;
    let g = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - 2.0*x, uv.y - 2.0*y), 0.0).rgb;
    let h = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x,       uv.y - 2.0*y), 0.0).rgb;
    let i = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + 2.0*x, uv.y - 2.0*y), 0.0).rgb;
    let j = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - x,   uv.y + y), 0.0).rgb;
    let k = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + x,   uv.y + y), 0.0).rgb;
    let l = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x - x,   uv.y - y), 0.0).rgb;
    let m = textureSampleLevel(u_input_texture, u_input_tex_sampler, vec2f(uv.x + x,   uv.y - y), 0.0).rgb;

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1
    var downsample = vec3f(0.0);
    downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;

    // apply thresholding
    if (u_mip_level == 0) {
    //     // thresholding
    //     float brightness = max3(e);
    //     float contribution = max(0, brightness - u_Threshold) / max (brightness, 0.00001);
    //     downsample *= contribution;
    //     break;
    }

    let alpha = textureSampleLevel(u_input_texture, u_input_tex_sampler, uv, 0.0).a;
	textureStore(u_out_texture, pixel_coords, vec4f(downsample, alpha));
}
)glsl";

const char* bloom_upsample_screen_shader = R"glsl(

#include SCREEN_PASS_VERTEX_SHADER

@group(0) @binding(0) var u_prev_upsample_texture: texture_2d<f32>; // upsample texture at mip i+1 (half-resolution of target)
@group(0) @binding(1) var u_sampler: sampler;
@group(0) @binding(2) var u_curr_downsample_texture: texture_2d<f32>; // downsample texture at same mip/resolution
@group(0) @binding(3) var<uniform> u_full_resolution_size: vec2<u32>;  // used to check if this is last mip level
@group(0) @binding(4) var<uniform> u_internal_blend: f32; // linear blend between mip levels
@group(0) @binding(5) var<uniform> u_final_blend: f32; // linear blend with original image

@fragment
fn fs_main(in : VertexOutput) -> @location(0) vec4f
{
    let input_dim = textureDimensions(u_curr_downsample_texture);
    let dx = 1.0 / f32(input_dim.x); // change in uv.x that corresponds to 1 pixel in input texture x direction
    let dy = 1.0 / f32(input_dim.y); // change in uv.y that corresponds to 1 pixel in input texture y direction
    let uv = in.v_uv;

    let weights = array(
        0.0625, 0.125, 0.0625,
        0.125,  0.25,  0.125,
        0.0625, 0.125, 0.0625
    );

    var upsampled_color = vec3f(0.0);
    upsampled_color += weights[0] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(-dx, dy)).rgb;
    upsampled_color += weights[1] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(0.0, dy)).rgb;
    upsampled_color += weights[2] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(dx, dy)).rgb;
    upsampled_color += weights[3] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(-dx, 0.0)).rgb;
    upsampled_color += weights[4] * textureSample(u_prev_upsample_texture, u_sampler, uv).rgb;
    upsampled_color += weights[5] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(dx, 0.0)).rgb;
    upsampled_color += weights[6] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(-dx, -dy)).rgb;
    upsampled_color += weights[7] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(0.0, -dy)).rgb;
    upsampled_color += weights[8] * textureSample(u_prev_upsample_texture, u_sampler, uv + vec2f(dx, -dy)).rgb;

    let curr_color = textureSample(u_curr_downsample_texture, u_sampler, uv).rgb;

    if (all(input_dim == u_full_resolution_size)) {
        // upsampled_color = mix(curr_color, upsampled_color, u_final_blend);
        // additive blend on final stage
        upsampled_color = curr_color + upsampled_color * u_final_blend;
    } else {
        upsampled_color = mix(curr_color, upsampled_color, u_internal_blend);
    }

    return vec4f(upsampled_color, 1.0);
}   
)glsl";


const char* bloom_upsample_shader_string = R"glsl(

@group(0) @binding(0) var u_input_texture: texture_2d<f32>; // output_render_texture at mip i
@group(0) @binding(1) var u_sampler: sampler;
@group(0) @binding(2) var u_output_texture: texture_storage_2d<rgba16float, write>; // output_render_texture at mip i - 1
// @group(0) @binding(3) var<uniform> u_mip_level: i32; // doesn't seem mip level affects 1-mip views
@group(0) @binding(3) var<uniform> u_full_resolution_size: vec2<u32>;  // same across all calls
@group(0) @binding(4) var<uniform> u_internal_blend: f32; // linear blend between mip levels
@group(0) @binding(5) var<uniform> u_final_blend: f32; // linear blend with original image
@group(0) @binding(6) var u_downsample_texture: texture_2d<f32>; // mip level i-1 of downsample chain

@compute @workgroup_size(8, 8, 1)
fn main(
    @builtin(global_invocation_id) GlobalInvocationID : vec3<u32>,
    @builtin(workgroup_id) WorkGroupID : vec3<u32>,
    @builtin(local_invocation_index) LocalInvocationIndex : u32, // ranges from 0 - 63
    @builtin(local_invocation_id) LocalInvocationID : vec3<u32>
)
{
    let output_size = textureDimensions(u_output_texture);
	let pixel_coords = vec2<i32>(GlobalInvocationID.xy);

    let u_FilterRadius = 0.001;
    let x = u_FilterRadius;
    let y = u_FilterRadius;

    let uv        = (vec2f(pixel_coords.xy) + vec2f(0.5)) / vec2f(f32(output_size.x), f32(output_size.y));

    let a = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x - x, uv.y + y), 0.0).rgb;
    let b = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x,     uv.y + y), 0.0).rgb;
    let c = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x + x, uv.y + y), 0.0).rgb;
    let d = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x - x, uv.y), 0.0).rgb;
    let e = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x,     uv.y), 0.0).rgb;
    let f = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x + x, uv.y), 0.0).rgb;
    let g = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x - x, uv.y - y), 0.0).rgb;
    let h = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x,     uv.y - y), 0.0).rgb;
    let i = textureSampleLevel(u_input_texture, u_sampler, vec2f(uv.x + x, uv.y - y), 0.0).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    var bloom = e*4.0;
    bloom += (b+d+f+h)*2.0;
    bloom += (a+c+g+i);
    bloom *= (1.0 / 16.0);

	let curr_pixel = textureLoad(u_downsample_texture, pixel_coords, 0);
    var out_pixel = curr_pixel.rgb;

    // if this is last mip level
    if (all(output_size == u_full_resolution_size)) {
        out_pixel = mix(curr_pixel.rgb, bloom, u_final_blend);
    } else {
        out_pixel = mix(curr_pixel.rgb, bloom, u_internal_blend);
    }

	textureStore(u_output_texture, pixel_coords, vec4f(out_pixel, 1.0));
}

)glsl";

// clang-format on

std::string Shaders_genSource(const char* src)
{
    std::string source(src);
    size_t pos = source.find("#include");
    while (pos != std::string::npos) {
        size_t start            = source.find_first_of(' ', pos);
        size_t end              = source.find_first_of('\n', start + 1);
        std::string includeName = source.substr(start + 1, end - start - 1);
        // strip \r
        includeName.erase(std::remove(includeName.begin(), includeName.end(), '\r'),
                          includeName.end());
        // strip \n
        includeName.erase(std::remove(includeName.begin(), includeName.end(), '\n'),
                          includeName.end());
        // strip ;
        includeName.erase(std::remove(includeName.begin(), includeName.end(), ';'),
                          includeName.end());
        std::string includeSource = shader_table[includeName];
        source.replace(pos, end - pos + 1, includeSource);
        pos = source.find("#include");
    }

    return source;
}
