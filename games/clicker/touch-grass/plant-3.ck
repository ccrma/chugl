/*

What happens when sanity goes negative?

you can only have 1 side active at a time. (dim the other side)

grass upgrades

work upgrades


*/
@import "../../lib/g2d/ChuGL.chug"
@import "../../lib/g2d/g2d.ck"
@import "../../lib/spring.ck"
@import "../../lib/M.ck"

G2D g;

FlatMaterial overlay_mat;
true => overlay_mat.transparent;
GMesh overlay_mesh(new PlaneGeometry, overlay_mat) --> GG.scene();
overlay_mesh.posZ(9);
10 => overlay_mesh.sca;
.5 => overlay_mat.alpha;
Color.WHITE => overlay_mat.color;


// GWindow.mouseMode(GWindow.MouseMode_Disabled);

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
Texture.load(me.dir() + "./computer.png" ) @=> Texture computer_tex;
Texture.load(me.dir() + "./hand.png" ) @=> Texture hand_tex;

0 => int Type_Grass;
1 => int Type_Work;

class Tile {
    int type;

    Texture@ tex;
    float H;
    float W;

    vec2 pos;

    2.0 => float grass_touch_cap; 
    float grass_touch; // time since last production

    Spring rot_spring(0, 1200, 25);

    fun void update(float dt) {
        // ==optimize== cache as mvar
        M.aabb(pos, W/2, H/2) => vec4 aabb;

        if (M.inside(mouse_curr, aabb)) {
            if (type == Type_Grass) {
                dt +=> grass_touch;
                if (grass_touch >= grass_touch_cap) {
                    grass_touch_cap -=> grass_touch;
                    ++sanity;
                    g.score("+1", pos, .5::second, .5, .5);

                    rot_spring.pull(.1 * (maybe ? -1 : 1));
                }
            }
            else if (type == Type_Work) {
                if (g.mouseLeftDown()) {
                    --sanity;
                    ++money;
                    g.score("$", pos, .5::second, .5, .5);
                    rot_spring.pull(.1 * (maybe ? -1 : 1));
                }
            }
        }

        rot_spring.update(dt);

        // draw
        g.sprite(tex, pos, @(W, -H) * (1 + rot_spring.x), 0);

        if (type == Type_Grass) {
            g.pushLayer(1);
            progressBar(grass_touch / grass_touch_cap, pos, W*.8, .1, Color.WHITE, Color.GREEN);
            g.popLayer();
        }
    }
}

Tile tile_list[0];

fun void add(Texture@ tex, int type, vec2 pos) {
    tex.width() $ float / tex.height() => float aspect;

    Tile tile;
    tile_list << tile;

    // init
    type => tile.type;
    tex @=> tile.tex;
    2 => tile.H;
    tile.H * aspect => tile.W;
    pos => tile.pos;

    // M.randomPointInArea(@(0,0), (g.screen_w - grass.W) * .5, (g.screen_h - grass.H) * .5) => grass.pos;

}


10 => int sanity;
int money;
int fc;
vec2 mouse_prev;
vec2 mouse_curr;
float mouse_dist;

add(grass_tex, Type_Grass, @(2, 0));
add(computer_tex, Type_Work, @(-2, 0));

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
        UI.text("upgrades");
    }

    for (auto tile : tile_list) tile.update(dt);

    g.pushLayer(10);
    // g.circleFilled(mouse_curr, .1, Color.WHITE);
    // progressBar(grass_touch / grass_touch_cap, @(0, 4), 10, .2, Color.WHITE, Color.GREEN);

    g.sprite(hand_tex, mouse_curr, 3 * @(.1, -.1), 0);
    g.popLayer();

    // border line
    g.boxFilled(@(0,0), .1, g.screen_h, Color.WHITE);

    overlay_mesh.scaX(g.screen_w / 2);
    overlay_mesh.scaY(g.screen_h);
    overlay_mesh.pos(@(g.screen_w / 4 * -Math.sgn(mouse_curr.x), 0));

		// vec2 position,
		// float width,
		// float height,
		// vec3 color


    g.pushTextControlPoint(0, 1);
    g.text(money + " money", g.screen_top_left + @(.5, -.5), .3);
    g.popTextControlPoint();

    g.pushTextControlPoint(1, 1);
    g.text(sanity + " sanity", g.screen_top_right - @(.5, .5), .3);
    // g.text(money + " money");
    g.popTextControlPoint();
}