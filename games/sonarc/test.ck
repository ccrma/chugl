@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"

G2D g;


// assume Nx1 texture for now
// map anim_state : (start_idx, length)
// type: oneshot, loop

// manages all the animations of a single sprite sheet
class Animation {
    10 => static int MAX_ANIMATIONS;
    static TextureLoadDesc tex_load_desc;
    true => tex_load_desc.flip_y;
    false => tex_load_desc.gen_mips;

    Texture sprite_sheet;
    int nframes;
    string animation_name[MAX_ANIMATIONS];
    int anim_start[MAX_ANIMATIONS]; // start frame in sprite sheet
    int anim_end[MAX_ANIMATIONS]; // last frame in sprite sheet, inclusive

    fun Animation(string path, int nframes) {
        Texture.load(path, tex_load_desc) @=> this.sprite_sheet;
        nframes => this.nframes;
    }

    fun void add(int anim_idx, string name, int start, int length) {
        if (anim_end[anim_idx] > 0) <<< "Animation warning: overwritting animation at index", anim_idx >>>;

        name => animation_name[anim_idx];
        start => anim_start[anim_idx];
        start + length - 1 => anim_end[anim_idx];
    }
}

0 => int Anim_None;
1 => int Anim_Idle;
2 => int Anim_Walk;
3 => int Anim_Attack;

Animation animation(me.dir() + "./assets/enemy.png", 25);
animation.add(Anim_Attack, "attack", 0, 7);
animation.add(Anim_Walk, "walk", 7,  18);

0 => int AnimType_Hold; // freeze frame
1 => int AnimType_Loop;
2 => int AnimType_Once;

0 => int curr_frame; // current frame in animation sequence 
0 => int anim_type;
0 => int curr_anim;
0 => float anim_frame_cd;

fun void oneshot(int anim) {
    anim => curr_anim;
    animation.anim_start[anim] => curr_frame;
    AnimType_Once => anim_type;
}

fun void loop(int anim) {
    anim => curr_anim;
    animation.anim_start[anim] => curr_frame;
    AnimType_Loop => anim_type;
}

fun void update(float dt, float secs_per_frame) {
    dt -=> anim_frame_cd;

    // step next frame
    if (anim_frame_cd <= 0) {
        secs_per_frame +=> anim_frame_cd;

        if (anim_type == AnimType_Once) {
            if (curr_frame < animation.anim_end[curr_anim]) {
                ++curr_frame;
            } else {
                // finished firing the animation, freeze on first frame
                animation.anim_start[curr_anim] => curr_frame;
                AnimType_Hold => anim_type;
            }
        } else if (anim_type == AnimType_Loop) { // loop
            if (curr_frame < animation.anim_end[curr_anim]) {
                ++curr_frame;
            } else {
                // loop back to start
                animation.anim_start[curr_anim] => curr_frame;
            }
        }
    }
}

fun void draw() {
	g.sprite(
		animation.sprite_sheet, 
        animation.nframes,  // spritesheet dimensions
        curr_frame,  // offset
        @(0, 0),  // position
        @(1, 1),  // scale
        0,  // rotation
        Color.WHITE
	);
}


UI_Int n_segments(12);
UI_Float displacement(.5);
UI_Float fade(3);

fun void lightning(vec2 pos, int n_segments, float rand_displacement_max, float fade_secs) {
    vec2 vertices[0];
    // assum lightning goes straight down from same x position at top of screen
    @(
        pos.x,
        g.ndc2world(@(0,1)).y
    ) => vec2 start;
    vertices << start;

    Math.fabs(pos.y - start.y) / n_segments => float dy;

    for (int i; i < n_segments - 1; i++) {
        start.y - (i * dy + Math.random2f(0, dy)) => float y;
        // displace along normal
        pos.x + Math.random2f(-rand_displacement_max, rand_displacement_max) => float x;
        vertices << @(x, y) << @(x, y);
    }

    // final endpoint
    vertices << pos;    

    // draw and animate
    float elapsed;
    while (1) {
        GG.nextFrame() => now;
        GG.dt() +=> elapsed;
        elapsed / fade_secs => float t; // try different easings
        if (t > 1) break;

        for (int i; i < vertices.size(); 2 +=> i) {
            vertices[i] + (1-t) * (vertices[i+1] - vertices[i]) => vertices[i+1];

            // rotate?

            // flicker, thank u penus mike
            if (maybe) g.line(vertices[i], vertices[i+1]);
        }
    }
}

fun void blink(float shrink_time, vec2 target) {

    float elapsed;

    while (1) {
        GG.nextFrame() => now;
        GG.dt() +=> elapsed;
        
    }
}


vec2 position_history[10];
int pos_hist_idx;

float blink_elaspsed_time;
int blinking;
vec2 blink_start;
vec2 blink_target;
vec2 player_pos;
UI_Float blink_time(.1);
1.0 => float player_sca;

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    // UI
    UI.slider("segments", n_segments, 0, 20);
    UI.slider("displacement", displacement, 0, 1);
    UI.slider("fade", fade, 0, 2);
    UI.slider("blink time", blink_time, 0, 2);

    // input
    if (GWindow.keyDown(GWindow.KEY_1)) oneshot(Anim_Attack);
    if (GWindow.keyDown(GWindow.KEY_2)) loop(Anim_Walk);
    if (GWindow.mouseLeftDown()) spork ~ lightning(g.mousePos(), n_segments.val(), displacement.val(), fade.val());
        // blink
    if (GWindow.keyDown(GWindow.KEY_Q) && !blinking) {
        true => blinking;
        g.mousePos() => blink_target;
        0 => blink_elaspsed_time;
        player_pos => blink_start;
    }

    // update
    update(dt, .1);

    // blink test
    if (blinking) {
        dt +=> blink_elaspsed_time;
        if (blink_elaspsed_time < blink_time.val()) {
            blink_elaspsed_time / blink_time.val() => float t;
            blink_start + t * (blink_target - blink_start) => player_pos;
        // }
        // else if (blink_elaspsed_time < blink_time.val() * 2) {
        //     blink_target => player_pos;
        //     (blink_elaspsed_time - blink_time.val()) / blink_time.val() => player_sca;

            g.pushBlend(g.BLEND_SUB);
            g.pushLayer(1.0);
            M.dist(player_pos, blink_start) => float width;
            M.dir(blink_start, blink_target) => vec2 rot;
            g.boxFilled(.5*(player_pos + blink_start), rot, width, 1-t, Color.RED * t);
            g.popLayer();
            g.popBlend();

            g.dashed(blink_start, player_pos, Color.WHITE, .1);
        } else {
            // exit blink
            blink_target => player_pos;
            0 => blinking;
            1.0 => player_sca;
        }
    }

    // experiment with trail rendering 
    if (0) {
        g.capsuleFilled(player_pos, player_sca * 1, player_sca*.5, Math.pi/2, Color.WHITE);

        player_pos => position_history[pos_hist_idx++];
        if (pos_hist_idx >= position_history.size()) 0 => pos_hist_idx;

        // draw the history
        for (int i; i < position_history.size(); ++i) {
            position_history[(pos_hist_idx + i) % position_history.size()] => vec2 pos;
            i $float / position_history.size() => float sca;
            g.capsuleFilled(pos, sca*1, sca*.5, Math.pi/2, Color.WHITE);
        }

    }

    // tristrips 
    if (1) {
        g.pushBlend(g.BLEND_ALPHA);
        g.pushAlpha(Math.sin(now/second));

        g.pushStrip(@(1, 0));
        g.pushStrip(@(1, 1));
        g.pushStrip(@(0, 1));
        g.endStrip();

        g.pushStrip(@(2, 0));
        g.pushStrip(@(2, 2));
        g.pushStrip(@(0, 2));

        g.popAlpha();
        g.popBlend();
    }

    // draw
    // draw();

}

