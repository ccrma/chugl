/*

Lvls
1: bubbles?
2: coins / art from mukbang game
3: eyes / lips body parts from face-making game
4: ?
5: many pimples --> holes --> tripophobia

*/

@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/spring.ck"
@import "../lib/M.ck"

GWindow.sizeLimits(0, 0, 0, 0, @(9, 16));
GWindow.center();

G2D g;
GG.outputPass().gamma(true);

class Pimples {
    // static stuff ====================================================
    1 => static int Level_Bubble;
    2 => static int Level_Wrap;
    3 => static int Level_Bug;
    4 => static int Level_Clown;
    5 => static int Level_Monster;

    // TODO optimize only load textures once, statically

    static TextureLoadDesc load_desc;
    false => load_desc.flip_y;

    // bubble
    Texture.load("./assets/pimple/bubble.png", load_desc) @=> static Texture bubble_tex;
    2 => static float BUBBLE_SPRITE_SCA_MODIFIER;

    // wrap
    Texture.load("./assets/pimple/bubblewrap-popped.png", load_desc) @=> static Texture bubblewrap_popped;
    Texture.load("./assets/pimple/bubblewrap-unpopped.png", load_desc) @=> static Texture bubblewrap_unpopped;
    5 => static int WRAP_N_COLS;
    4 => static int WRAP_N_ROWS;

    // bug
    Texture.load("./assets/pimple/pillbug.png", load_desc) @=> static Texture pillbug;
    Texture.load("./assets/pimple/pillbug-smushed.png", load_desc) @=> static Texture pillbug_smushed;
    10 => static int PILLBUG_N_FRAMES;
    .5 => static float PILLBUG_SCA_MOD;

    // clown
    true => load_desc.flip_y;
    Texture.load("./assets/pimple/clown.png", load_desc) @=> static Texture clown_tex;
    Texture.load("./assets/pimple/balloon.png", load_desc) @=> static Texture balloon_tex;
    balloon_tex.width() / 4.0 / balloon_tex.height() => static float BALLOON_ASPECT;
    4.2 => float BALLOON_VISUAL_SCA;

    // size
    .5 => static float r;
    r * r => static float r2;

    // local stuff ====================================================
    vec2 positions[0]; // .z is > 0 if popped
    int active[0];
    vec4 rand[0]; // random val for animations
    int num_pimples;
    int level;

    fun void sprite(Texture tex, vec2 pos, float sca) {
        tex.width() $ float / tex.height() => float aspect;
        g.sprite(tex, pos, sca * @(aspect, 1), 0);
    }

    fun void init(int count, int level) {
        level => this.level;
        positions.clear();
        active.clear();
        rand.clear();

        if (level == Level_Bubble) {
            count => num_pimples;
            positions.size(count);
            active.size(count);
            rand.size(count);

            for (int i; i < count; i++) {
                true => active[i];
                M.randomPointInArea(@(0,0), 2, 4) => positions[i];
                @(
                    Math.randomf(),
                    Math.randomf(),
                    Math.randomf(),
                    Math.random2f(.5, 1)
                ) => rand[i];
            }
        }
        else if (level == Level_Wrap) {

            // 1 0
            // 2 .5
            // 3 1
            // 4 1.5
            // ignores count for now
            // tile to fill the screen, player only needs to pop the unpopped ones
            -r * ((WRAP_N_COLS-1)) => float x;
            for (int i; i < 5; i++) {
                -(r * (WRAP_N_ROWS -1)) => float y;
                for (int j; j < WRAP_N_ROWS; ++j) {
                    // add
                    positions << @(x, y);
                    num_pimples++;
                    active << true;

                    2 * r +=> y;
                }
                2*r +=> x;
            }
        }
        else if (level == Level_Bug) {
            // all spawn at center
            positions.size(count);
            active.size(count);
            rand.size(count);
            count => num_pimples;
            for (int i; i < count; ++i) {
                true => active[i];

                // set random bug targets
                M.randomPointInArea(@(0,0), .5 * 5, .5 * g.screen_h) $ vec4 => rand[i];
                <<< rand[i], g.screen_w >>>;
                // set random frame offset
                Math.random2(0, PILLBUG_N_FRAMES - 1) => rand[i].z;
                Math.random2f(.8, 1.2) => rand[i].w; // speed
            }
        }
        else if (level == Level_Clown) {
            count => num_pimples;
            positions.size(count);
            active.size(count);
            rand.size(count);

            for (int i; i < count; i++) {
                true => active[i];
                if (i < count / 2) M.randomPointInArea(@(0,g.screen_h * .25), 2, 1.5) => positions[i];
                else {
                    M.randomPointInArea(@(1.5,0), .5, 4) => positions[i];
                }
                @(
                    Math.randomf(),
                    Math.randomf(),
                    Math.randomf(),
                    Math.random2f(.5, 1)
                ) => rand[i];
            }
        }
    }

    fun void pop(int idx) {
        num_pimples--;
        false => active[idx];
    }

    fun void update(float dt) {
        g.mousePos() => vec2 mouse_pos;
        now / second => float t;

        Math.sin(2*t) => float sin_2t;



        int popped; // only pop 1 at a time

        g.pushLayer(1);
        for (int i; i < pimples.positions.size(); i++) {
            pimples.positions[i] => vec2 p;
            Color.WHITE => vec3 color;
            r => float mod_r;

            // update
            if (level == Level_Bubble) {
                // move in place
                .6 * rand[i].z * Math.sin(t * rand[i].x) +=> p.x;
                .6 * rand[i].z * Math.sin(t * rand[i].y) +=> p.y;
                // vary size
                r * rand[i].w => mod_r;
            }
            else if (level == Level_Wrap) {
                // slide up and down
                (i / WRAP_N_ROWS) % 2 => int col;
                .3 * sin_2t * (col ? 1 : -1) +=> p.y;
            }
            else if (level == Level_Bug) {
                PILLBUG_SCA_MOD * r => mod_r;

                // move bug towards target
                if (active[i]) {
                    rand[i] $ vec2 => vec2 target;

                    // lerp bug towards target
                    if (M.dist2(p, target) < .05) {
                        // stop, pick new target
                        M.randomPointInArea(@(0,0), .5 * 5, .5 * g.screen_h) => vec2 target; 
                        target.x => rand[i].x;
                        target.y => rand[i].y;
                    } else {
                        rand[i].w => float BUG_SPEED;
                        dt * BUG_SPEED * M.dir(p, target) + p => pimples.positions[i];
                        pimples.positions[i] => p;
                    }
                }
            }
            else if (level == Level_Clown) {
                .95 * r => mod_r;
                g.square(p + .5 * g.UP, 2 * mod_r);
            }

            if (!popped && pimples.active[i]) {
                int collision;

                if (level != Level_Clown) {
                    M.dist(p, mouse_pos) < mod_r => collision;
                } else {
                    M.dist(p + .5 * g.UP, mouse_pos) < mod_r => collision;
                }

                if (collision) {
                    true => popped;
                    Color.RED => color;
                    if (g.mouse_left_down) {
                        pimples.pop(i);
                        if (level == Level_Bubble) {
                            g.animate(p, bubble_tex, 7, 1, .05, 2* mod_r * BUBBLE_SPRITE_SCA_MODIFIER);
                        }
                        else if (level == Level_Clown) {
                            AnimationEffect e(
                                balloon_tex,
                                4,
                                0,
                                .05,
                                BALLOON_VISUAL_SCA * r, p, Color.WHITE
                            ); 
                            -1 *=> e._frame_sca.y;
                            g.add(e);
                        }
                    }
                }
            }

            pimples.active[i] => int active;

            // draw

            if (level == 1) {
                if (active) {
                    g.sprite(bubble_tex, 7, 0, p, 2* mod_r * BUBBLE_SPRITE_SCA_MODIFIER);
                    // g.square(p, 2*r * rand[i].w);
                }
            } 
            else if (level == Level_Wrap) {
                g.sprite(active ? bubblewrap_unpopped : bubblewrap_popped, p, 2*r);
            }
            else if (level == Level_Bug) {
                // white bg
                g.pushLayer(0);
                g.boxFilled(@(0, 0), g.screen_w, g.screen_h, Color.BEIGE);
                g.popLayer();

                // g.pushLayer(0);
                // g.sprite(skin_tex, @(0,0), g.screen_h);
                // g.popLayer();


                rand[i] $ vec2 => vec2 target;
                M.angle(p, target) + Math.pi/2 => float angle;

                if (active)
                    g.sprite(pillbug, PILLBUG_N_FRAMES, 0, p, r* @(1,1), angle, Color.WHITE);
                else 
                    g.sprite(pillbug_smushed, p, r, angle);
            }
            else if (level == Level_Clown) {
                g.pushLayer(0);
                g.sprite(clown_tex, @(0,0), @(g.screen_w, -g.screen_h), 0);
                g.popLayer();

                if (active) 
                    g.sprite(balloon_tex, @(4, 1), @(0, 0), p, BALLOON_VISUAL_SCA * r * @(BALLOON_ASPECT, -1), 0, Color.WHITE);
            }
        }
        g.popLayer();
    }
}

Pimples pimples;
Pimples.Level_Clown => int level;
// 1 => level;

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    if (pimples.num_pimples == 0) {
        pimples.init(Math.random2(6,7), level++);
    }

    pimples.update(dt);

    <<< g.screen_w >>>;
}