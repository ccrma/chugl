/*

Begin with a pixel-art cutscene of getting burnt out at work?
- similar to hyper light drifter opening
- set the stage for the player beginning their healing self-care journey


Instead of unlocking new ways to touch tile, unlock more and more wholesome/healing activities
- was previously stuck on the meme-y cynical/satirical tone of "maximizing","optimizing" tile touching.
- what if instead we went full comfy-wholesome. and made the game about healing/convalescence, in a 
genuine earnest kind of way?
- instead of manual tile touch --> robot arm --> etc. 
- touch tile --> go for walk! --> spend time with family/friends --> go bar hopping 
- different activities can have different types of mental health resource, e.g.
    - energy
    - sanity
    - social belonging
    - romance?
- some activities can have a %chance of "failure", e.g. "go for a date", has a chance of going poorly,
resulting in minus energy/sanity. but can never fall below 0
- just like in real life, you can only spend your time doing 1 thing. No "automation." Whatever you 
hover over is what you make progress on.
- however, like 1000000 Shrimp game, there can be upgrades to accelerate time, or get more resources 
per harveset, etc.
    - what if time naturally accelerates as you grow older?
    - what if game ends when you die of old age?
- mechanic idea: each activity has its own "harvest" mechanic. tile from rubbing, sleep from doing nothing,
going on a walk - clicking, etc. 
    - it is a clicker built around minigames (ref: kento cho for mechanic ideas)
- to unlock next activity, you need sufficient number of mental health resources
    - e.g. touch tile generates sanity. Sanity can unlock tile-touch upgrades (e.g. study botany, which
    generates more sanity per harvest) and eventually do the equivalent of "upgrade ship", e.g. "social confidence"

background art is a video of touching tile?

what if colors slowly game in over time, grayscale --> full color based on LUT?

4 activities
- sleep
- touch tile
- go hiking
- meditate

Juice
- look at a327ex Anchor code for spring parameterized by freq/bounce rather than stiffness

TODO
- cooking interaction kinda sucks. update based on art

*/
@import "../../lib/g2d/ChuGL.chug"
@import "../../lib/g2d/g2d.ck"
@import "../../lib/spring.ck"
@import "../../lib/M.ck"

G2D g;

Texture.load(me.dir() + "./assets/grass-1.jpeg" ) @=> Texture grass_tex;

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

0 => int TileType_None;
1 => int TileType_Grass;
2 => int TileType_Friend;
3 => int TileType_Hike;
4 => int TileType_Cook;
5 => int TileType_Meditate;

class CD {
    float curr;
    float cap;
    fun @construct(float cap) { cap => this.cap; }
    fun float percentage() { return curr/cap; }
    fun int update(float amt) {
        amt +=> curr;
        if (curr >= cap) {
            cap -=> curr;
            return true;
        }
        return false;
    }
}

class Tile {
    int type;
    float H;
    float W;
    vec2 pos;

    string resource_name;
    int resource;

    CD tile_touch(2.0);

    // friend tile params
    vec2 chat_bubble_list[0]; // @(who, width)
        // chat params
        2 => static float chat_w; // width of entire chat window
        .1 => static float chat_bubble_hh;

    fun void update(float dt) {
        M.aabb(pos, W/2, H/2) => vec4 aabb;

        if (M.inside(mouse_prev, aabb) || M.inside(mouse_curr, aabb)) {
            if (type == TileType_Grass) {
                if (tile_touch.update(mouse_dist * dt * 10)) {
                    ++resource;
                    g.score("+1", pos, .5::second, .5, .5);
                }
            }
            else if (type == TileType_Cook) {
                if (g.mouseLeft() && tile_touch.update(mouse_dist * dt * 10)) {
                    ++resource;
                    g.score("+1", pos, .5::second, .5, .5);
                }
            }
            else if (type == TileType_Friend && g.mouseLeftDown()) {
                // textbox animation
                // TODO: would be cool to make the chat in emojis
                chat_bubble_list << @(maybe, Math.random2(1, 7) * .1 * chat_w);
                if (chat_bubble_list.size() > 10) chat_bubble_list.popFront();

                ++resource;
                g.score("+1", pos, .5::second, .5, .5);
            }
            else if (type == TileType_Hike) {
                Math.fabs(GWindow.scrollY()) => float scroll;
                if (tile_touch.update(scroll * .01)) {
                    ++resource;
                    g.score("+1", pos, .5::second, .5, .5);
                }
                // ++resource;
                // g.score("+1", pos, .5::second, .5, .5);
            }
            else if (type == TileType_Meditate) {
                if (mouse_dist == 0 
                    && GWindow.scrollX() == 0
                    && GWindow.scrollY() == 0
                    && !g.anyInput()
                    && tile_touch.update(dt)) {
                    ++resource;
                    g.score("+1", pos, .5::second, .5, .5);
                }
            }
        }

        // draw
        if (type == TileType_Grass) {
            g.sprite(grass_tex, pos, @(W, -H), 0);

            g.pushLayer(1);
            progressBar(tile_touch.percentage(), pos, W*.8, .1, Color.WHITE, Color.GREEN);
            g.popLayer();
        }
        else if (type == TileType_Friend) {
            g.box(pos, W, H);
            g.text("Friend", pos, .5);

            // chat params
            pos.y + 1 => float chat_bubble_pos_y;
            pos.x - .5 * chat_w => float chat_left_x;
            pos.x + .5 * chat_w => float chat_right_x;

            g.pushPolygonRadius(chat_bubble_hh * .5);
            for (chat_bubble_list.size() - 1 => int i; i >= 0; i--) {
                float chat_bubble_pos_x;
                vec3 chat_bubble_color;
                .5 * chat_bubble_list[i].y => float chat_bubble_hw;

                (chat_bubble_list[i].x > 0) => int their_text;
                if (their_text) {
                    chat_left_x + chat_bubble_hw => chat_bubble_pos_x;
                    Color.GRAY => chat_bubble_color;
                } else {
                    chat_right_x - chat_bubble_hw => chat_bubble_pos_x;
                    Color.BLUE => chat_bubble_color;
                }

                g.boxFilled(
                    @(
                        chat_bubble_pos_x,
                        chat_bubble_pos_y
                    ),
                    2 * chat_bubble_hw,
                    chat_bubble_hh,
                    chat_bubble_color
                );

                // vertical margin. more gap if sender switches
                2 * chat_bubble_hh +=> chat_bubble_pos_y;
                chat_bubble_list[i - 1].x == chat_bubble_list[i].x => int same_sender;
                if (same_sender) .005 +=> chat_bubble_pos_y;
                else             .05 +=> chat_bubble_pos_y; 
            }
            g.popPolygonRadius();
        }
        else if (type == TileType_Hike) {
            g.box(pos, W, H);
            g.text("Hike", pos, .5);

            g.pushLayer(1);
            progressBar(tile_touch.percentage(), pos + @(0, .5), W*.8, .1, Color.WHITE, Color.GREEN);
            g.popLayer();
        }
        else if (type == TileType_Cook) {
            g.box(pos, W, H);
            g.text("Cook", pos, .5);

            g.pushLayer(1);
            progressBar(tile_touch.percentage(), pos + @(0, .5), W*.8, .1, Color.WHITE, Color.GREEN);
            g.popLayer();
        }
        else if (type == TileType_Meditate) {
            g.box(pos, W, H);
            g.text("Meditate", pos, .5);

            g.pushLayer(1);
            progressBar(tile_touch.percentage(), pos + @(0, .5), W*.8, .1, Color.WHITE, Color.GREEN);
            g.popLayer();
        }
    }
}

Tile tile_list[0];

fun void addTile(Texture tex, vec2 pos, int type, string resource_name) {
    tex.width() $ float / tex.height() => float aspect;

    Tile tile;
    tile_list << tile;

    // init
    // 2 => tile.H;
    // tile.H * aspect => tile.W;
    2 => tile.H;
    2 => tile.W;
    pos => tile.pos;
    type => tile.type;
    resource_name => tile.resource_name;
}


10 => int sanity;

int fc;
vec2 mouse_prev;
vec2 mouse_curr;
float mouse_dist;

addTile(grass_tex, @(0, 0), TileType_Grass, "sanity");
addTile(grass_tex, @(3, 1), TileType_Friend, "belonging");
addTile(grass_tex, @(-3, -3), TileType_Hike, "vitality");
addTile(grass_tex, @(3, -3), TileType_Cook, "competency");
addTile(grass_tex, @(-3, 3), TileType_Meditate, "peace");

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

    for (auto tile : tile_list) tile.update(dt);

    g.pushLayer(10);
    g.circleFilled(mouse_curr, .1, Color.WHITE);
    g.popLayer();


    { // resource UI
        g.screen_top_left + @(.5, -.5) => vec2 text_pos;
        g.pushTextControlPoint(0, 1);

        for (auto t : tile_list) {
            g.text(t.resource_name + " " + t.resource, text_pos, .3);
            .5 -=> text_pos.y;
        }

        g.popTextControlPoint();
    }
}