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
- fix the tile collision to be in gridspace, not worldspace
- cooking interaction kinda sucks. update based on art
- should you be able to add "more" tiles for each activity?
- within a tile, some upgrades are unlocked after unlocking other tiles.
    - e.g. on grass tile, after unlocking "friend", there is "touch grass w/ friend" upgrade that automates the touching
- within a tile, upgrades to generate other resources
    - e.g. touching grass now also generates vitality and friendship? but less.
- idea: grass tile you get from friendship is itself boosted when you click on friendship tile?
    - and vice versa, so if you have grass+friend grass tile and friend+grass friend tile, they automate each other
        - might be too convoluted ...
- grass upgrades: for each other tiletype, an upgrade that makes it so every grass pop also generates that resource

- core upgrade mechanic: you can't specialize in everything. need to "pick a lane" and focus on a particular synergy
  between upgrade types
    - counter: too much thinking for a clicker game. In Magic Archery you just get everything and its awesome

- maybe grass is concentration resource (like in magic archery)
    - upgrades happen continuously, over time (with progress bar) rather than instant on-click?
    - I like the idea of concentrating...

- idea: should there be a friend-generating tile type??

*/
@import "../../lib/g2d/ChuGL.chug"
@import "../../lib/g2d/g2d.ck"
@import "../../lib/spring.ck"
@import "../../lib/M.ck"
@import "../../lib/T.ck"

// constants
class C {
    static UI_Float GUI_PAD(.25);
    static UI_Float GUI_FONT_SIZE(.3);
    static UI_Float GUI_BUTTON_PAD(.06);


    static UI_Float GUI_MARGIN(.2);
    static UI_Float GUI_LINE_VSPACE(.36);
    static UI_Float LOCAL_GUI_W(4);

    static UI_Float LOCAL_GUI_H_RATIO(.33); // determined every frame

    static UI_Float RESOURCE_SPRITE_SCA(.28);
}

G2D g;
GG.outputPass().gamma(true);
g.antialias(true);
GText.defaultFont("chugl:proggy-tiny");

// TODO don't mip
TextureLoadDesc load_desc;
false => load_desc.gen_mips;

Texture.load(me.dir() + "./assets/grass-1.jpeg", load_desc) @=> Texture grass_tex;
Texture.load(me.dir() + "./assets/block.png", load_desc) @=> Texture tile_tex;

true => load_desc.flip_y;
Texture.load(me.dir() + "./assets/cooking-icon.png", load_desc) @=> Texture cooking_icon_tex;
Texture.load(me.dir() + "./assets/friend-icon.png", load_desc) @=> Texture friend_icon_tex;
Texture.load(me.dir() + "./assets/grass-icon.png", load_desc) @=> Texture grass_icon_tex;
Texture.load(me.dir() + "./assets/hike-icon.png", load_desc) @=> Texture hike_icon_tex;
Texture.load(me.dir() + "./assets/meditation-icon.png", load_desc) @=> Texture meditation_icon_tex;


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

fun void drawTile(vec2 pos_world, vec3 color) {
    -0.16 => float offset_y; // TODO remove after audrey centers tiles
    // (ij.x + Math.sgn(ij.x) * .5) $ int => int grid_x;
    // (ij.y + Math.sgn(ij.y) * .5) $ int => int grid_y;
    // M.mult(grid.grid_to_world_mat, @(grid_x, grid_y)) => vec2 grid_pos_world;

    // normalized distance from top of screen [0, 1]
    (1.0 / GG.camera().viewSize()) * (pos_world.y - g.screen_max.y) => float y_sort;

    g.pushLayer(g.layer() - y_sort);
    g.sprite(tile_tex, pos_world + @(0, offset_y), @(2 * grid.tile_hw, -2 * grid.tile_hw), 0, color);
    g.popLayer();

    // g.pushLayer(1);
    // g.pushColor(Color.GRAY);
    // g.text(grid_x + "," + grid_y, pos_world, .2);
    // g.popColor();
    // g.popLayer();
}



class IsoGrid {
    26.5 * M.DEG2RAD => float angle_rad;
    1 => float tile_hw;

    // normalized gridspace basis vectors
    vec2 i_norm;
    vec2 j_norm;

    // gridspace basis vectors (not normalized)
    vec2 i_hat; 
    vec2 j_hat;

    // conversion matrices
    vec4 grid_to_world_mat;
    vec4 world_to_grid_mat;

    init(angle_rad, tile_hw);

    fun void init(float angle_rad, float tile_hw) {
        angle_rad => this.angle_rad;
        tile_hw => this.tile_hw;

        // tile params
        @(Math.cos(angle_rad), Math.sin(angle_rad)) => i_norm;
        @(-i_norm.x, i_norm.y) => j_norm; // reflect over y axis
        tile_hw / i_norm.x => float mag;
        // grid basis vectors
        i_norm * mag => i_hat;
        j_norm * mag => j_hat;

        M.mat(i_hat, j_hat) => grid_to_world_mat;
        M.inv(grid_to_world_mat) => world_to_grid_mat;
    }

    fun vec2 world2cell(vec2 world) {
        return grid2cell( M.mult(world_to_grid_mat, world) );
    }

    fun vec2 grid2cell(vec2 grid) {
        return @(
            (grid.x + Math.sgn(grid.x) * .5) $ int,
            (grid.y + Math.sgn(grid.y) * .5) $ int
        ); 
    }

    fun vec2 cell2world(vec2 cell) {
        return M.mult(grid_to_world_mat, cell);
    }
}

fun gridlines(vec2 min, vec2 max, vec3 color) {
    g.pushColor(color);
    g.line(min.x * grid.i_hat + max.y * grid.j_hat, max.x * grid.i_hat + max.y * grid.j_hat);
    g.line(min.x * grid.i_hat + min.y * grid.j_hat, max.x * grid.i_hat + min.y * grid.j_hat);

    g.line(min.x * grid.i_hat + min.y * grid.j_hat, min.x * grid.i_hat + max.y * grid.j_hat);
    g.line(max.x * grid.i_hat + min.y * grid.j_hat, max.x * grid.i_hat + max.y * grid.j_hat);
    g.popColor();
}

// Tile Type enum
0 => int TileType_None;
1 => int TileType_Grass;
2 => int TileType_Friend;
3 => int TileType_Hike;
4 => int TileType_Cook;
5 => int TileType_Meditate;
6 => int TileType_Count;

// color constants
Color.linear(Color.hex(0x63ab3f)) => vec3 GRASS_COLOR;
Color.linear(Color.hex(0x4fa4b8)) => vec3 FRIEND_COLOR;
Color.linear(Color.hex(0x2f5753)) => vec3 HIKE_COLOR;
Color.ORANGE => vec3 COOK_COLOR;
Color.WHITE => vec3 MEDITATION_COLOR;
[
    Color.MAGENTA,
    GRASS_COLOR,
    FRIEND_COLOR,
    HIKE_COLOR,
    COOK_COLOR,
    MEDITATION_COLOR,
] @=> vec3 tile_colors[];

[
    null,
    grass_icon_tex,
    friend_icon_tex,
    hike_icon_tex,
    cooking_icon_tex,
    meditation_icon_tex,
] @=> Texture resource_textures[];

// grass Upgrades
false => int grass_and_friend_auto;

// friend upgrades

// hike upgrades
int hike_upgrades[0];
"multitasker" => string hike_multitasker; // every pop of grass fills hike progress meter
1 => int hike_rate; // progress per scroll
"vitality points" => string hike_points; // points per pop
0 => hike_upgrades[hike_multitasker];
1 => hike_upgrades[hike_points];
int hike_additional_tiles;


// cook_upgrades
int cook_friend;

// forest helper upgrades
int helpers_per_resource[TileType_Count]; // how many were bought by each resource


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
    int unlocked;

    float H;
    float W;
    vec2 pos;

    CD tile_touch(2.0);

    int num_helpers;

    // friend tile params
    vec2 chat_bubble_list[0]; // @(who, width)
    CD chat_cd(2.0);
        // chat params
        2 => static float chat_w; // width of entire chat window
        .1 => static float chat_bubble_hh;
    
    // grass tile upgrades
    1 => int green_thumb; // cost increases exponentially. amt of grass per tick
    1 => int grass_tick_level; // increases contribution per mouse_delta by X% per level
    // 0 => int animal_friends; // auto touch that grass


    fun void addResource(int amt) {
        amt +=> resources[type];
        g.score("+" + amt, pos, .5::second, .5, .5);
    }

    fun void _text(int receiving) {
        chat_bubble_list << @(receiving, Math.random2(1, 7) * .1 * chat_w);
        addResource(1);
    }

    fun void update(float dt) {
        M.aabb(pos, W/2, H/2) => vec4 aabb;
        M.inside(mouse_prev, aabb) || M.inside(mouse_curr, aabb) => int inside;

        if (unlocked) {
            if (type == TileType_Grass) {
                float grass_progress;

                Math.pow(1.25, grass_tick_level - 1) => float tick_rate;
                .5 * dt * num_helpers * tick_rate +=> grass_progress;

                if (inside) 
                    5 * dt * mouse_dist * tick_rate +=> grass_progress;
                // Math.min(tile_touch.cap, grass_progress) => grass_progress;
                if (tile_touch.update(grass_progress)) {
                    addResource(green_thumb);

                    // add progress to hike meter
                    tiles_by_type[TileType_Hike].size() => int hike_tile_count;
                    if (hike_upgrades[hike_multitasker] && hike_tile_count) {
                        tiles_by_type[TileType_Hike][Math.random2(0, hike_tile_count - 1)] @=> Tile@ hike_tile;
                        .2 +=> hike_tile.tile_touch.curr;
                        // TODO refactor resource pop into separate function, and call that instead
                        hike_tile.update(0);
                    }
                }
            }
            else if (type == TileType_Cook) {
                // current mechanic: hold left click and *vertical* movement
                // animation: chopping veg
                //            stiring wok
                if (g.mouseLeft() && tile_touch.update(mouse_delta.dot(@(0, 1) * dt * 5))) {
                    addResource(1);
                }
            }
            else if (type == TileType_Friend) {
                // textbox animation
                // TODO: would be cool to make the chat in emojis

                // receive text
                if (chat_cd.update(dt)) {
                    _text(true);
                    // TODO update chat cd based on level
                    // M.poisson(2.0) => chat_cd.cap;
                    // <<< chat_cd.cap >>>;
                }

                // send text!
                if (inside && g.mouseLeftDown()) {
                    _text(false);
                }

                if (chat_bubble_list.size() > 10) chat_bubble_list.popFront();

            }
            else if (type == TileType_Hike) {
                Math.fabs(GWindow.scrollY()) * hike_rate => float scroll;
                if (inside && tile_touch.update(scroll * .002)) {
                    addResource(hike_upgrades[hike_points]);
                }
                // ++resource;
                // g.score("+1", pos, .5::second, .5, .5);
            }
            else if (type == TileType_Meditate && inside) {
                if (mouse_dist == 0 
                    && GWindow.scrollX() == 0
                    && GWindow.scrollY() == 0
                    && !g.anyInput()
                    && tile_touch.update(dt)) {
                    addResource(1);
                }
            }
        }

        // draw

        if (num_helpers) { // draw helper info
            g.pushLayer(1);
            g.text("helpers x " + num_helpers, pos - @(0, .3), .2);
            g.popLayer();
        }

        if (type == TileType_Grass) {
            drawTile(pos, GRASS_COLOR);

            g.pushLayer(1);
            progressBar(tile_touch.percentage(), pos, W*.8, .1, Color.WHITE, Color.GREEN);
            g.popLayer();
        }
        else if (type == TileType_Friend) {
            drawTile(pos, unlocked ? FRIEND_COLOR : Color.DARKGRAY);

            g.pushLayer(1);
            g.text("Friend", pos, .5);
            g.popLayer();

            // chat params
            pos.y + 1 => float chat_bubble_pos_y;
            pos.x - .5 * chat_w => float chat_left_x;
            pos.x + .5 * chat_w => float chat_right_x;

            g.pushPolygonRadius(chat_bubble_hh * .5);
            g.pushLayer(2);
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
            g.popLayer();
            g.popPolygonRadius();
        }
        else if (type == TileType_Hike) {
            drawTile(pos, unlocked ? HIKE_COLOR : Color.DARKGRAY);

            g.pushLayer(1);
            g.text("Hike", pos, .5);
            progressBar(tile_touch.percentage(), pos + @(0, .5), W*.8, .1, Color.WHITE, Color.GREEN);
            g.popLayer();
        }
        else if (type == TileType_Cook) {
            drawTile(pos, unlocked ? COOK_COLOR : Color.DARKGRAY);

            g.pushLayer(1);
            g.text("Cook", pos, .5);
            progressBar(tile_touch.percentage(), pos + @(0, .5), W*.8, .1, Color.WHITE, Color.GREEN);
            g.popLayer();
        }
        else if (type == TileType_Meditate) {
            drawTile(pos, unlocked ? MEDITATION_COLOR : Color.DARKGRAY);

            g.pushLayer(1);
            g.text("Meditate", pos, .5);
            progressBar(tile_touch.percentage(), pos + @(0, .5), W*.8, .1, Color.WHITE, Color.GREEN);
            g.popLayer();
        }

        if (!unlocked) g.text("LOCKED", pos - @(0, .5), .5);
    }
}

Tile@ tiles_by_coord[20][20]; // just pick a number big enough for the entire map
Tile@ tiles_by_type[TileType_Count][0];
Tile tile_list[0];

[
    @(0, 0), @(0, 0),                // NONE
    @(-1.5, -1.5), @(1.5, 1.5),     // GRASS
    @(1.5, -1.5), @(3.5, 1.5),      // FRIEND
    @(-3.5, -1.5), @(-1.5, 1.5),    // HIKE
    @(-1.5, -3.5), @(1.5, -1.5),    // COOK
    @(-1.5, 1.5), @(1.5, 3.5),      // MEDITATION
] @=> vec2 tile_regions[];

IsoGrid grid;
grid.init(26.5 * M.DEG2RAD, 1.5);

fun Tile tileFromCell(vec2 cell) {
    return tileFromCell(cell.x $ int, cell.y $ int);
}

fun Tile tileFromCell(int grid_x, int grid_y) {
    if (grid_x < 0) tiles_by_coord.size() + grid_x => grid_x;
    if (grid_y < 0) tiles_by_coord.size() + grid_y => grid_y;
    return tiles_by_coord[grid_x][grid_y];
}

fun Tile unlockTile(Texture tex, vec2 cell_pos, int type) {
    addTile(tex, cell_pos, type) @=> Tile@ tile;
    true => tile.unlocked;
    return tile;
}

fun Tile addTile(Texture tex, vec2 cell_pos, int type) {
    tex.width() $ float / tex.height() => float aspect;

    Tile tile;
    tile_list << tile;

    // init
    // 2 => tile.H;
    // tile.H * aspect => tile.W;
    2 => tile.H;
    2 => tile.W;
    grid.cell2world(cell_pos) => tile.pos;
    type => tile.type;

    tiles_by_type[type] << tile;

    cell_pos.x $ int => int grid_x;
    cell_pos.y $ int => int grid_y;
    if (grid_x < 0) tiles_by_coord.size() + grid_x => grid_x;
    if (grid_y < 0) tiles_by_coord.size() + grid_y => grid_y;
    tile @=> tiles_by_coord[grid_x][grid_y];

    return tile;
}

int fc;
vec2 mouse_prev;
vec2 mouse_curr;
vec2 mouse_delta;
float mouse_dist;
vec2 mouse_grid;
vec2 mouse_cell;
vec2 mouse_cell_world_pos; // centered cell position in worldspace

// tile gui params
Tile@ gui_tile;
vec2 gui_tile_cell;
int display_tile_gui;

int resources[TileType_Count];
[
    "none resource",
    "sanity",
    "belonging",
    "vitality",
    "competency",
    "peace",
] @=> string resource_names[];

int free_forest_helpers;


addTile(grass_tex, @(0, 0), TileType_Grass) @=> Tile grass_tile;
addTile(grass_tex, @(3, 0), TileType_Friend) @=> Tile friend_tile;
addTile(grass_tex, @(-3, 0), TileType_Hike) @=> Tile hike_tile;
addTile(grass_tex, @(0, -3), TileType_Cook) @=> Tile cook_tile;
addTile(grass_tex, @(0, 3), TileType_Meditate) @=> Tile meditate_tile;
true => grass_tile.unlocked;
100 => resources[TileType_Grass];

// addTile(grass_tex, @(1, 0), TileType_Grass) @=> Tile _tile;
// true => _tile.unlocked;


fun int upgrade(int cost[], string name) {
    false => int ret;

    // UI.pushStyleColor(UI_Color.Button, UI.convertHSVtoRGB());
    // UI.pushStyleColor(UI_Color.ButtonHovered, UI.convertHSVtoRGB(@(i / num_presets, 0.7f, 0.7f)));
    // UI.pushStyleColor(UI_Color.ButtonActive, UI.convertHSVtoRGB(@(i / num_presets, 0.8f, 0.8f)));

    // construct price tag string from cost array
    string cost_desc;
    true => int can_afford;
    for (int i; i < TileType_Count; ++i) {
        if (cost[i]) cost[i] + " " + resource_names[i] + " " +=> cost_desc;
        if (cost[i] > resources[i]) false => can_afford;
    }

    UI.text(name);
    UI.sameLine();
    UI.pushID(name);
    if (UI.button(cost_desc) && can_afford) {
        for (int i; i < TileType_Count; ++i) { cost[i] -=> resources[i]; }
        true => ret;
    }
    UI.popID();

    // UI.popStyleColor(3);

    return ret;
}

fun int upgrade(int cost, string name, int type) {
    int costs[TileType_Count];
    cost => costs[type];
    return upgrade(costs, name);
}


// =================
// gui (maybe move this into g2d)
// =================

class GUI {
    // ======================================
    // CORE GUI METHODS
    // ======================================
    [ false ] @=> int disabled_stack[]; 
    fun void pushDisabled(int b) { disabled_stack << b; }
    fun void popDisabled() { disabled_stack.popBack(); }

    // CALL EVERY FRAME
    fun void update() {
        disabled_stack.erase(1, disabled_stack.size());
    }

    // assumes using monospace font
    // TODO can update sizing dynamically after I add those stats to chugl core
    .38 => float FONT_SIZE_CHAR_WIDTH; // single character w (worldspace)
    .7 => float FONT_SIZE_CHAR_HEIGHT; // single character h (worldspace)

    // button that auto-resizes based on font size and text length
    fun int button(string text, vec2 pos, vec2 cp) {
        g.font_size_stack[-1] => float font_size;
        (text.length()) * font_size * FONT_SIZE_CHAR_WIDTH => float text_w;
        .5 * text_w + C.GUI_BUTTON_PAD.val() => float hw;
        .5 * font_size * FONT_SIZE_CHAR_HEIGHT => float text_hh;
        text_hh + C.GUI_BUTTON_PAD.val() => float hh;

        // based on control points, update post to be true center
        // (height aligned by inner text, NOT button)
        // (width aligned by button boundaries)
        (2 * cp.x - 1) * hw -=> pos.x;
        (2 * cp.y - 1) * text_hh -=> pos.y;

        M.inside(mouse_curr, pos, hw, hh) => int inside;

        .02 => float r;

        2 * (hw - r) => float button_w;
        2 * (hh - r) => float button_h;
        .5 * (inside ? Color.GRAY : Color.DARKGRAY) => vec3 button_color;
        if (disabled_stack[-1]) .25 * Color.DARKGRAY => button_color;

        g.pushPolygonRadius(.02); {
            g.boxFilled(pos, button_w, button_h, button_color);
        } g.popPolygonRadius();

        g.pushLayer(g.layer() + .01); g.pushColor(disabled_stack[-1] ? Color.GRAY : Color.WHITE); {
            g.text(text, pos);
        } g.popColor(); g.popLayer();

        return !disabled_stack[-1] && inside && g.mouseLeftDown();
    }

    // ======================================
    // this game specific methods vvvvvvvv
    // ======================================

    // eventually will have to impl compact price representations...
    fun string _costToStr(int cost) {
        return cost + "";
    }

    // cost button with inline sprites
    fun int button(int cost[], vec2 pos, vec2 cp) {
        g.font_size_stack[-1] => float font_size;
        2 => int CHARS_PER_ICON;

        int cost_str_length;
        for (auto c : cost) {
            if (c > 0) (CHARS_PER_ICON + _costToStr(c).length()) +=> cost_str_length;
        }

        cost_str_length * font_size * FONT_SIZE_CHAR_WIDTH => float text_w;
        .5 * text_w + C.GUI_BUTTON_PAD.val() => float hw;
        .5 * font_size * FONT_SIZE_CHAR_HEIGHT => float text_hh;
        text_hh + C.GUI_BUTTON_PAD.val() => float hh;

        // based on control points, update post to be true center
        // (height aligned by inner text, NOT button)
        // (width aligned by button boundaries)
        (2 * cp.x - 1) * hw -=> pos.x;
        (2 * cp.y - 1) * text_hh -=> pos.y;

        M.inside(mouse_curr, pos, hw, hh) => int inside;

        .02 => float polygon_radius;

        2 * (hw - polygon_radius) => float button_w;
        2 * (hh - polygon_radius) => float button_h;
        .5 * (inside ? Color.GRAY : Color.DARKGRAY) => vec3 button_color;
        if (disabled_stack[-1]) .25 * Color.DARKGRAY => button_color;

        g.pushPolygonRadius(polygon_radius); {
            g.boxFilled(pos, button_w, button_h, button_color);
        } g.popPolygonRadius();

        @(
            pos.x - hw + C.GUI_BUTTON_PAD.val(),
            pos.y
        ) => pos;
        g.pushLayer(g.layer() + .01); g.pushColor(disabled_stack[-1] ? Color.GRAY : Color.WHITE); 
        g.pushTextControlPoint(0, .5);
        {
            for (int type; type < TileType_Count; ++type) {
                cost[type] => int c;
                if (c) {
                    _costToStr(c) => string cost_str;
                    g.text(cost_str, pos);  // number
                    (cost_str.length() + 1.2) * font_size * FONT_SIZE_CHAR_WIDTH +=> pos.x;
                    <<< cost_str.length(), FONT_SIZE_CHAR_WIDTH >>>;
                    g.sprite(resource_textures[type], pos, C.RESOURCE_SPRITE_SCA.val());
                    C.RESOURCE_SPRITE_SCA.val() +=> pos.x;
                }
            }
        } g.popTextControlPoint(); g.popColor(); g.popLayer();

        // TODO put the purchasing logic here too
        return !disabled_stack[-1] && inside && g.mouseLeftDown();
    }

    fun int upgradeButton(
        string text, vec2 pos, int cost, int type
    ) {
        if (button(text, pos, @(.5, .5)) && resources[type] >= cost) {
            cost -=> resources[type];
            return true;
        }
        return false;
    }

    vec2 cursor;
    float tile_upgrade_button_right_border;
    fun int tileUpgradeButton(string text, int cost, int type) {
        int ret;
        int costs[TileType_Count];
        cost => costs[type];

        // text
        g.pushTextControlPoint(0, 1);
        g.text(text, cursor);
        g.popTextControlPoint();

        // button
        pushDisabled(resources[type] < cost); {
            if (button(costs, @(tile_upgrade_button_right_border, cursor.y), @(1, 1))) {
                T.assert(!disabled_stack[-1], "disabled button should never return true");
                cost -=> resources[type];
                true => ret;
            }
        } popDisabled();

        C.GUI_LINE_VSPACE.val() -=> cursor.y;

        return ret;
    }

} GUI gui;

fun void tileUpgradeGui(Tile@ tile, vec2 cell, vec2 top_left, vec2 bot_right) {
    g.pushLayer(10);
    g.pushFontSize(C.GUI_FONT_SIZE.val());
    // upgrade gui for empty tile spot
    if (tile == null) {
        grid.cell2world(cell) => vec2 world_pos;
        for (1 => int type; type < TileType_Count; ++type) {
            tile_regions[type * 2 + 0] => vec2 region_min;
            tile_regions[type * 2 + 1] => vec2 region_max;
            if (M.inside(cell, region_min, region_max)) {
                20 * tiles_by_type[type].size() * tiles_by_type[type].size() => int cost;
                if (gui.upgradeButton(cost + " " + resource_names[type], world_pos, cost, type)) {
                    unlockTile(tile_tex, cell, type);
                    // clear the null tile gui
                    false => display_tile_gui;
                }
                break;
            }
        }
    } else {
        tile.type => int type;
        int cost[TileType_Count];

        g.boxFilled(top_left, bot_right, .1*Color.GRAY);

        // assign/unassign helper
        top_left + C.GUI_PAD.val() * @(1, -1) => vec2 cursor;
        bot_right.x - C.GUI_PAD.val() => float right_border_x;
        .5 * (top_left.x + bot_right.x) => float center_x;

        g.pushTextControlPoint(0, 1);
        g.text("Forest Helper", cursor);
        g.popTextControlPoint();

        @(right_border_x, cursor.y) => vec2 button_pos;

        if (tile.num_helpers) {
            if (gui.button("remove", button_pos, @(1, 1))) {
                ++free_forest_helpers;
                --tile.num_helpers;
            }
        } else {
            gui.pushDisabled(free_forest_helpers < 1);
            if (gui.button("add", button_pos, @(1, 1))) {
                --free_forest_helpers;
                ++tile.num_helpers;
            }
            gui.popDisabled();
        }

        C.GUI_LINE_VSPACE.val() -=> cursor.y;

        if (type == TileType_Grass) {
            // init cursor stuff
            cursor => gui.cursor;
            right_border_x => gui.tile_upgrade_button_right_border;

            // green thumb
            (Math.pow(2, tile.green_thumb - 1) * 5) $ int => int green_thumb_cost;
            if (gui.tileUpgradeButton("Green Thumb " + tile.green_thumb, green_thumb_cost, TileType_Grass)) {
                ++tile.green_thumb;
            }

            // grass rate (maybe rename to fertilizer?)
            tile.grass_tick_level * tile.grass_tick_level * 5 => int grass_tick_cost;
            if (gui.tileUpgradeButton("Fertilizer " + tile.grass_tick_level, grass_tick_cost, TileType_Grass)) {
                ++tile.grass_tick_level;
            }
        }
        else if (type == TileType_Friend) {

        }
        else if (type == TileType_Hike) {

        }
        else if (type == TileType_Cook) {

        }
        else if (type == TileType_Meditate) {

        }

    }
    g.popFontSize();
    g.popLayer();
}


while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
    GG.fc() => fc;

    mouse_curr => mouse_prev;
    g.mousePos() => mouse_curr;

    mouse_curr - mouse_prev => mouse_delta;
    M.dist(mouse_prev, mouse_curr) => mouse_dist;
    M.mult(grid.world_to_grid_mat, mouse_curr) => mouse_grid;
    grid.grid2cell(mouse_grid) => mouse_cell; 
    grid.cell2world(mouse_cell) => mouse_cell_world_pos; 

    { // tile upgrade gui
        if (GWindow.mouseRightDown() || GWindow.keyDown(GWindow.Key_Space)) {
            tileFromCell(mouse_cell) @=> gui_tile;
            mouse_cell => gui_tile_cell;
            true => display_tile_gui;
        }

        // determine gui bounds
        C.LOCAL_GUI_H_RATIO.val() * g.screen_h - C.GUI_MARGIN.val() => float local_gui_h;
        @(g.screen_min.x + C.GUI_MARGIN.val(), g.screen_center.y - (.5 - C.LOCAL_GUI_H_RATIO.val()) * g.screen_h) => vec2 top_left;
        top_left + @(C.LOCAL_GUI_W.val(), -local_gui_h) => vec2 bot_right;
        .5 * (top_left + bot_right) => vec2 local_gui_pos;

        // logic for closing tile gui
        // TODO align with actual bbox of gui window
        if (display_tile_gui && (GWindow.mouseLeftDown() || GWindow.keyDown(GWindow.Key_Space))) {
            // detect clicking outside of null tile
            if (gui_tile == null) M.dist2(mouse_cell, gui_tile_cell) < .1 => display_tile_gui;
            // detect clicking outside of local gui window
            else M.inside(mouse_curr, local_gui_pos, .5 * C.LOCAL_GUI_W.val(), .5 * local_gui_h) => display_tile_gui;
        }

        if (display_tile_gui) tileUpgradeGui(gui_tile, gui_tile_cell, top_left, bot_right);
    }

    // update tiles
    for (auto tile : tile_list) tile.update(dt);

    g.pushLayer(10);
    // g.circleFilled(mouse_curr, .1, Color.WHITE);
    g.popLayer();

    { // resource UI
        g.pushLayer(10);
        g.screen_top_left + @(.5, -.5) => vec2 text_pos;
        g.pushTextControlPoint(0, .5);

        for (1 => int i; i < resource_names.size(); ++i) {
            g.sprite(resource_textures[i], text_pos, C.RESOURCE_SPRITE_SCA.val());
            g.text(resources[i] + " " + resource_names[i], text_pos + @(.2, 0), .3);
            .3 -=> text_pos.y;
        }

        g.text(free_forest_helpers + " forest helpers ", text_pos + @(.2, 0), .3);

        g.popTextControlPoint();
        g.popLayer();
    }

    { // draw grid
        g.pushLayer(1);
        g.pushColor(Color.DARKGRAY);
        for (-12 => int i; i < 12; ++i) {
            i + .5 => float t;
            // "x" axis
            g.line(-100 * grid.i_hat + t * grid.j_hat, 100 * grid.i_hat + t * grid.j_hat);
            // "y" axis
            g.line(-100 * grid.j_hat + t * grid.i_hat, 100 * grid.j_hat + t * grid.i_hat);
        }
        g.popColor();
        g.popLayer();

        // draw tile regions
        g.pushLayer(2);
        1 => float margin;
        for (int i; i < TileType_Count; ++i) {
            gridlines(margin * tile_regions[2*i + 0], margin * tile_regions[2*i + 1], 2 * tile_colors[i]);
        }
        g.popLayer();
    }
    

    { // upgrade UI
        UI.setNextWindowBgAlpha(0);
        UI.begin("Upgrades");
        UI.separatorText("Grass Upgrades");

        // animal friend (grass auto harvest)
        // (Math.pow(2.0, animal_friends) * 20) $ int => int animal_friend_cost;
        // if (upgrade(animal_friend_cost, "animal friends " + animal_friends, TileType_Grass)) {
        //     animal_friends++;
        // }

        // TODO should automation upgrades cost other resource AND base resource?
        // e.g. sanity AND belonging?
        {
            10 => int cost;
            TileType_Friend => int cost_type;
            "belonging tile + automation" => string name;

            UI.beginDisabled(grass_and_friend_auto || !friend_tile.unlocked);
            if (upgrade(cost, name, TileType_Friend)) {
                // unlock!
                addTile(grass_tex, @(1, 0), TileType_Grass) @=> Tile tile;
                true => tile.unlocked;
                true => grass_and_friend_auto;
            }
            UI.endDisabled();
        }

        // unlock friendship!
        UI.separatorText("Friend Upgrades");
        {
            UI.beginDisabled(friend_tile.unlocked);
            20 => int friend_unlock_cost;
            if (upgrade(friend_unlock_cost, "unlock friends", TileType_Grass)) {
                true => friend_tile.unlocked;
            }
            UI.endDisabled();
        }


        UI.separatorText("Hike Upgrades");
        {
            UI.beginDisabled(hike_tile.unlocked);
            int cost[TileType_Count];
            20 => cost[TileType_Grass];
            20 => cost[TileType_Friend];
            if (upgrade(cost, "unlock hike")) {
                true => hike_tile.unlocked;
            }
            UI.endDisabled();

            // hike points
            (Math.pow(2, hike_upgrades[hike_points] - 1) * 5) $ int => int hike_points_cost;
            if (upgrade(hike_points_cost, "hike points " + hike_upgrades[hike_points], TileType_Hike)) {
                ++hike_upgrades[hike_points];
            }

            // hike rate
            hike_rate * hike_rate * 5 => int hike_rate_cost;
            if (upgrade(hike_rate_cost, "hike tick rate " + hike_rate, TileType_Hike)) {
                hike_rate++;
            }

            // hike automate
            UI.beginDisabled(hike_upgrades[hike_multitasker]);
            if (upgrade(20, "multitasker (smell flowers)", TileType_Hike)) {
                true => hike_upgrades[hike_multitasker];
            }
            UI.endDisabled();

            // hike extra tiles
            // this config experiments with tiles only costing vitality, and NOT being 
            // themed to combo/pair with the other resource types
            UI.beginDisabled(hike_additional_tiles >= 3);
            (Math.pow(2, hike_additional_tiles) * 20) $ int => int hike_add_tile_cost;
            if (upgrade(hike_add_tile_cost, "add hiking tile", TileType_Hike)) {
                vec2 cell_pos;
                if (hike_additional_tiles == 0) @(-2, 0) => cell_pos;
                if (hike_additional_tiles == 1) @(-3, 1) => cell_pos;
                if (hike_additional_tiles == 2) @(-3, -1) => cell_pos;

                unlockTile(tile_tex, cell_pos, TileType_Hike);
                ++hike_additional_tiles;
            }
            UI.endDisabled();
        }

        UI.separatorText("Cooking Upgrades");
        {
            UI.beginDisabled(cook_tile.unlocked);
            int cost[TileType_Count];
            20 => cost[TileType_Grass];
            20 => cost[TileType_Friend];
            20 => cost[TileType_Hike];
            if (upgrade(cost, "unlock cooking")) {
                true => cook_tile.unlocked;
            }
            UI.endDisabled();

            // foraging (does it build another cook tile or another grass tile?)
            // (or is the automation unlock for cooking)

            // cooking friend tile
            cost.zero();
        }

        UI.separatorText("Meditation Upgrades");
        {
            UI.beginDisabled(meditate_tile.unlocked);
            int cost[TileType_Count];
            20 => cost[TileType_Grass];
            20 => cost[TileType_Friend];
            20 => cost[TileType_Hike];
            20 => cost[TileType_Cook];
            if (upgrade(cost, "unlock meditation")) {
                true => meditate_tile.unlocked;
            }
            UI.endDisabled();
        }


        UI.separatorText("Forest Helper Upgrades");
        {
            for (1 => int type; type < TileType_Count; ++type) {
                helpers_per_resource[type] => int helper_count;
                (Math.pow(2, helper_count) * 10) $ int => int cost;
                if (upgrade(cost, "forest helper", type)) {
                    ++helpers_per_resource[type];
                    ++free_forest_helpers;
                }
            }
        }



        UI.end();
    }

    { // cleanup
    gui.update();
    }
}