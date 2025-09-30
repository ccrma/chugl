@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"

G2D g;

ShaderDesc screen_shader_desc;
me.dir() + "./blind_screen_shader.wgsl" => screen_shader_desc.vertexPath;
me.dir() + "./blind_screen_shader.wgsl" => screen_shader_desc.fragmentPath;
null => screen_shader_desc.vertexLayout;
Shader screen_shader(screen_shader_desc);
screen_shader.name("screen shader");

// all gamestate
[@(0,0,1,0), @(2, 0, 1, 0)] @=> vec4 lights[]; 
vec2 player_pos;
2.0 => float speed;

// ==optimize== render to rgba8 texture instead of float16
GG.scene().backgroundColor(Color.ORANGE);
GG.rootPass() --> GG.scenePass() --> ScreenPass screen_pass(screen_shader);
screen_pass.material().texture(0, GG.scenePass().colorOutput());
screen_pass.material().sampler(1, TextureSampler.linear());
screen_pass.material().uniformFloat(2, GG.camera().viewSize());
screen_pass.material().storageBuffer(3, lights);

// audio stuff ===========================
// filter pole position
UI_Float env_low_cut(.08);
UI_Float env_exp(.22);
UI_Float env_pole_pos(.9998);

// make a plucked string
class GainMeter extends Chugraph // Chubgraph
{
    inlet => Gain squared_input => OnePole env_follower => outlet;
    // square the input
    inlet => squared_input;
    // multiply
    3 => squared_input.op;

    env_pole_pos.val() => env_follower.pole;

    // ==optimize== cache this
    fun float level()
    {
        return Math.max(
            0,
            Math.pow(env_follower.last(), env_exp.val()) - env_low_cut.val()
        );
    }
}

SndBuf buf => dac;
dac => GainMeter gain_meter => blackhole;
"special:dope" => buf.read;
0 => buf.loop;
0 => buf.rate;


int play_buf_generation;
fun void sporkBuf() {
    play_buf_generation++ => int my_gen;
    0 => buf.pos;
    1 => buf.rate;
    buf.length() => now;
    // only stop if subsequent plays haven't been issued
    if (play_buf_generation == my_gen) {
        0 => buf.rate;
    }
}

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    // input =================================
    if (GWindow.key(GWindow.KEY_LEFT)) {
        dt * speed -=> player_pos.x;
    }
    if (GWindow.key(GWindow.KEY_RIGHT)) {
        dt * speed +=> player_pos.x;
    }
    if (GWindow.key(GWindow.KEY_UP)) {
        dt * speed +=> player_pos.y;
    }
    if (GWindow.key(GWindow.KEY_DOWN)) {
        dt * speed -=> player_pos.y;
    }

    if (GWindow.keyDown(GWindow.KEY_SPACE)) {
        spork ~ sporkBuf();
    }

    // dev console ===========================
    UI.separatorText("Gain Meter Params");

    UI.slider("Low Cut", env_low_cut, 0.0, .2);
    UI.slider("Exponent", env_exp, 0.0, 1.0);
    if (UI.slider("Pole pos", env_pole_pos, 0.8, 1.0)) {
        env_pole_pos.val() => gain_meter.env_follower.pole;
    }


    // audio update ===========================
    <<< gain_meter.level() >>>;
    gain_meter.level() => lights[0].z;

    // render =====================================
    g.circleFilled(player_pos, .2);


    // screen pass uniforms =======================
    screen_pass.material().storageBuffer(3, lights);
}