/*

DVD bounce algo:
https://prgreen.github.io/blog/2013/09/30/the-bouncing-dvd-logo-explained/

Increase bounces by adding walls, more grass sprites, movement speed, pts per bounce...

Upgrade type: 
- click immediate OR
- progress bar (like Magic Archery concentration meter)

Stack
- add more grass textures

*/
@import "../../lib/g2d/ChuGL.chug"
@import "../../lib/g2d/g2d.ck"
@import "../../lib/spring.ck"
@import "../../lib/M.ck"

G2D g;

fun void progressBar(
    float percentage, vec2 pos, float width, float height, vec3 outline_color, vec3 fill_color
) {
    width * .5 => float hw;
    height * .5 => float hh;

    g.box(pos, 2 * hw, 2 * hh, outline_color);
    -hw + (percentage * 2 * hw) => float end_x;
    g.boxFilled(
        pos - @(hw, hh),   // bot left
        pos + @(end_x, hh), // top right
        fill_color
    );
}

Texture.load(me.dir() + "./grass-1.jpeg" ) @=> Texture grass_tex;

class Grass {
    static UI_Float speed(0.0);

    float vx;
    float vy;
    float H;
    float W;

    vec2 pos;
    vec2 pos_prev;

    2.0 => float grass_touch_cap; 
    float grass_touch; // time since last production

    fun void update(float dt) {
        // ==optimize== cache as mvar
        M.aabb(@(0,0), W/2, H/2) => vec4 aabb;

        // calculate mouse pos relative to local pos
        mouse_prev - pos_prev => vec2 m_prev_local;
        mouse_curr - pos => vec2 m_curr_local;

        if (M.inside(m_prev_local, aabb) || M.inside(m_curr_local, aabb)) {
            (M.dist(m_prev_local, m_curr_local) * dt * 10) +=> grass_touch;
            if (grass_touch >= grass_touch_cap) {
                grass_touch_cap -=> grass_touch;
                ++sanity;
                g.score("+1", pos, .5::second, .5, .5);
            }
        }

        // bounce
        pos.x - (W/2) => float min_x;
        pos.y - (H/2) => float min_y;
        pos.x + (W/2) => float max_x;
        pos.y + (H/2) => float max_y;

        (min_x <= g.screen_min.x || max_x >= g.screen_max.x) => int hit_sides;
        (min_y <= g.screen_min.y || max_y >= g.screen_max.y) => int hit_top_or_bot;
        if (hit_sides) -1 *=> vx;
        if (hit_top_or_bot) -1 *=> vy;
        if (hit_sides || hit_top_or_bot) {
            ++sanity;
            g.score("+1", pos, .5::second, .5, .5);
        }

        // save prev pos
        pos => pos_prev;

        // move
        dt * vx * speed.val() => float dx;
        dt * vy * speed.val() => float dy;
        do {
            pos.x - (W/2) => min_x;
            pos.x + (W/2) => max_x;
            dx +=> pos.x;
        } while (min_x <= g.screen_min.x || max_x >= g.screen_max.x);
        do {
            pos.y - (H/2) => min_y;
            pos.y + (H/2) => max_y;
            dy +=> pos.y;
        } while (min_y <= g.screen_min.y || max_y >= g.screen_max.y);

        // draw
        g.sprite(grass_tex, pos, @(W, -H), 0);

        g.pushLayer(1);
        progressBar(grass_touch / grass_touch_cap, pos, W*.8, .1, Color.WHITE, Color.GREEN);
        g.popLayer();
    }
}

Grass grass_list[0];

fun void addGrass(Texture tex) {
    tex.width() $ float / tex.height() => float aspect;

    Grass grass;
    grass_list << grass;

    // init
    maybe ? 1 : -1 => grass.vx;
    maybe ? 1 : -1 => grass.vy;
    2 => grass.H;
    grass.H * aspect => grass.W;

    M.randomPointInArea(@(0,0), (g.screen_w - grass.W) * .5, (g.screen_h - grass.H) * .5) => grass.pos;
}


10 => int sanity;
int fc;
vec2 mouse_prev;
vec2 mouse_curr;
float mouse_dist;


// 2.0 => float grass_touch_cap; 
// float grass_touch; // time since last production


addGrass(grass_tex);
@(0, 0) => grass_list[0].pos;

fun int upgrade(int cost, string name) {
    false => int ret;

    if (UI.button(name) && sanity >= cost) {
        cost -=> sanity;
        true => ret;
    }

    UI.sameLine();
    UI.text(cost + " sanity");

    return ret;
}


while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
    GG.fc() => fc;

    mouse_curr => mouse_prev;
    g.mousePos() => mouse_curr;

    M.dist(mouse_prev, mouse_curr) => mouse_dist;

    // ui
    {
        5 * grass_list.size() * grass_list.size() => int add_grass_cost;
        if (upgrade(add_grass_cost, "add grass")) addGrass(grass_tex);

        Math.pow(Grass.speed.val() + 1, 2) * 10 => float speed_cost;
        if (upgrade(speed_cost $ int, "grass move speed")) {
            Grass.speed.val() + .5 => Grass.speed.val;
        }
    }

    for (auto grass : grass_list) grass.update(dt);

    g.pushLayer(10);
    g.circleFilled(mouse_curr, .1, Color.WHITE);
    // progressBar(grass_touch / grass_touch_cap, @(0, 4), 10, .2, Color.WHITE, Color.GREEN);

    g.popLayer();


    g.pushTextControlPoint(0, 1);
    g.text(sanity + " sanity", g.screen_top_left + @(.5, -.5), .3);
    g.popTextControlPoint();
}