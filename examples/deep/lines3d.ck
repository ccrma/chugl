//-----------------------------------------------------------------------------
// name: lines3d.ck
// desc: example of creating a custom GGen to do immediate-mode 3D line drawing
// Supports per-vertex colors. Does *not* support line width.
// requires: ChuGL + chuck-1.5.5.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//
// date: Fall 2025
//-----------------------------------------------------------------------------

// note: because lines3d is immediate mode, you *don't* need to instantiate 
// a separate instance for each line. All lines can be drawn with a single 
// instance, and will be batch-drawn in a single draw call
Lines3D l --> GG.scene();  
Lines3D l1 --> GG.scene(); l1.translateX(-1.0);
Lines3D l2 --> GG.scene(); l2.translateX(1.0);

GG.camera().posY(4);
GG.camera() --> GGen dolly --> GG.scene();

// builds a spiral with the given params
fun vec3[] spiral(
    float radians, float width, float height, int num_segments
) {
    vec3 points[0];
    for (int i; i < num_segments; ++i) {
        i / (num_segments $ float) => float progress;
        progress * radians => float theta; 
        (1.0 - progress) * width => float b;
        points << @(
            b * Math.cos(theta), 
            progress * height,
            b * Math.sin(theta)
        );
    }
    return points;
}

while (true) {
    GG.nextFrame() => now;

    // draw single segment
    // l.line(@(-1, -1, -1), @(-1, 0, 1));

    // draw single segment with color
    // l.line(@(-2, -2, -2), @(-1, 0, 1), Color.GREEN);

    // draw a list of points
    spiral(10 * Math.pi, 1.0, 1.0, 300) @=> vec3 points[];
    l1.line(points, false /* whether or not to close the loop */);

    // draw a list of point with per-vertex colors
    [Color.BROWN, Color.GREEN, Color.YELLOW] @=> vec3 colors[];
    l2.line(points, colors, false);

    // rotate camera, look at center
    dolly.rotateY(GG.dt()); 
    GG.camera().lookAt(@(0,0,0));
}

public class Lines3D extends GGen
{
	null @=> static Shader@ shader;

	// shader init (only needs to be done once; draws all line segments in 1 draw call)
    if (shader == null) {
        "
        #include FRAME_UNIFORMS
            // includes the following:
            // struct FrameUniforms {
            //     // scene params (only set in ScenePass, otherwise 0)
            //     projection: mat4x4f,
            //     view: mat4x4f,
            //     projection_view_inverse_no_translation: mat4x4f,
            //     camera_pos: vec3f,
            //     ambient_light: vec3f,
            //     num_lights: i32,
            //     background_color: vec4f,

            //     // general params (include in all passes except ComputePass)
            //     resolution: vec3i,      // window viewport resolution 
            //     time: f32,              // time in seconds since the graphics window was opened
            //     delta_time: f32,        // time since last frame (in seconds)
            //     frame_count: i32,       // frames since window was opened
            //     mouse: vec2f,           // normalized mouse coords (range 0-1)
            //     mouse_click: vec2i,     // mouse click state
            //     sample_rate: f32        // chuck VM sound sample rate (e.g. 44100)
            // };
            // @group(0) @binding(0) var<uniform> u_frame: FrameUniforms;

        #include DRAW_UNIFORMS
            // includes the following:
            // struct DrawUniforms {
            //     model: mat4x4f,
            //     normal: mat4x4f,
            //     id: i32,
            //     receives_shadow: i32,
            // };

            // @group(2) @binding(0) var<storage> u_draw_instances: array<DrawUniforms>;

        struct VertexInput {
            @location(0) position : vec3f,
            @location(1) color : vec3f,     // per-vertex color
        };

        struct VertexOutput {
            @builtin(position) position: vec4f,
            @location(0) v_color: vec3f,
        };

        @vertex
        fn vs_main(
            in : VertexInput,
            @builtin(instance_index) instance_idx : u32,    // carry over from everything being indexed...
            @builtin(vertex_index) vertex_idx : u32,        // used to determine which polygon we are drawing
        ) -> VertexOutput {
            var out : VertexOutput;

            let p = in.position;
            
            var u_Draw : DrawUniforms = u_draw_instances[instance_idx];
            out.position = (u_frame.projection * u_frame.view) * u_Draw.model * vec4f(p, 1.0f);
            out.v_color = in.color;

            return out;
        }

        // begin fragment shader ----------------------------

        @fragment
        fn fs_main(in : VertexOutput) -> @location(0) vec4f {
            return vec4f(in.v_color, 1.0);
        }
        " => string shader_code;

        // set drawing shader
        ShaderDesc shader_desc;
        shader_code => shader_desc.vertexCode;
        shader_code => shader_desc.fragmentCode;
        [VertexFormat.Float3, VertexFormat.Float3] @=> shader_desc.vertexLayout;

        new Shader(shader_desc) @=> shader;
    }

	Material material;
	material.shader(shader);

	// ==optimize== use lineStrip topology + index reset? but then requires using additional index buffer
	material.topology(Material.Topology_LineList); // list not strip!

    // color stack
    [Color.WHITE] @=> vec3 color_stack[];

	// vertex buffers
	vec3 u_positions[0];
	vec3 u_colors[0];

	Geometry geo; // just used to set vertex count
	geo.vertexCount(0);
	GMesh mesh(geo, material) --> this;

    fun void line(vec3 points[], int loop) {
        for (1 => int i; i < points.size(); i++) {
            u_positions << points[i - 1] << points[i];
            u_colors << color_stack[-1] << color_stack[-1];
        } 
        if (loop) {
            u_positions << points[-1] << points[0];
            u_colors << color_stack[-1] << color_stack[-1];
        }
    }

    // if points.size() is greater than colors.size()
    // loop over the colors array and continue stepping
    // by 1 for each point
    fun void line(vec3 points[], vec3 colors[], int loop) {
        0 => int color_idx;
        for (1 => int i; i < points.size(); i++) {
            u_positions << points[i - 1] << points[i];
            u_colors << colors[color_idx % colors.size()] << colors[++color_idx % colors.size()];
        } 
        if (loop) {
            u_positions << points[-1] << points[0];
            u_colors << colors[color_idx % colors.size()] << colors[color_idx % colors.size()];
        }
    }

	fun void line(vec3 p1, vec3 p2) {
		u_positions << p1 << p2;
		u_colors << color_stack[-1] << color_stack[-1];
	}

	fun void line(vec3 p1, vec3 p2, vec3 color) {
		u_positions << p1 << p2;
		u_colors << color << color;
	}

    fun void pushColor(vec3 color) {
        color_stack << color;
    }

    fun void popColor() {
        color_stack.popBack();
    }

	fun void update(float dt)
	{
		// update GPU vertex buffers
		geo.vertexAttribute(0, u_positions);
		geo.vertexCount(u_positions.size());
		geo.vertexAttribute(1, u_colors);

		// reset
		u_positions.clear();
		u_colors.clear();

        // clear color stack (except for white)
        color_stack[0] => vec3 default_color;
        color_stack.clear();
        color_stack << default_color;
	}

}