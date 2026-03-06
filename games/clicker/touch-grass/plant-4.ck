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
- hike upgrade: multi-tasking. touch grass completion also triggers a portion of completion for a random hiking tile

*/
@import "../../lib/g2d/ChuGL.chug"
@import "../../lib/g2d/g2d.ck"
@import "../../lib/spring.ck"
@import "../../lib/M.ck"

G2D g;
GG.outputPass().gamma(true);
g.antialias(true);
GText.defaultFont("chugl:proggy-tiny");

// TODO don't mip
Texture.load(me.dir() + "./assets/grass-1.jpeg" ) @=> Texture grass_tex;
Texture.load(me.dir() + "./assets/block.png" ) @=> Texture tile_tex;

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

// grass Upgrades
1 => int green_thumb; // cost increases exponentially. amt of grass per tick
1 => int grass_tick_level; // increases contribution per mouse_delta by X% per level
0 => int animal_friends; // auto touch that grass
false => int grass_and_friend_auto;

// friend upgrades

// hike upgrades
int hike_upgrades[0];
"multitasker" => string hike_multitasker; // every pop of grass fills hike progress meter
1 => int hike_rate; // progress per scroll
"vitality points" => string hike_points; // points per pop
0 => hike_upgrades[hike_multitasker];
1 => hike_upgrades[hike_points];

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

    // string resource_name;
    // UI_Int resource;

    CD tile_touch(2.0);

    // friend tile params
    vec2 chat_bubble_list[0]; // @(who, width)
    CD chat_cd(2.0);
        // chat params
        2 => static float chat_w; // width of entire chat window
        .1 => static float chat_bubble_hh;

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
                dt * animal_friends +=> grass_progress;
                if (inside) 
                    5 * dt * mouse_dist * Math.pow(1.25, grass_tick_level - 1) +=> grass_progress;
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
                if (g.mouseLeft() && tile_touch.update(mouse_dist * dt * 10)) {
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
                Math.fabs(GWindow.scrollY()) * hike_upgrades[hike_rate] => float scroll;
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

Tile@ tiles_by_type[TileType_Count][0];
Tile tile_list[0];
IsoGrid grid;
grid.init(26.5 * M.DEG2RAD, 1.5);

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

    return tile;
}

int fc;
vec2 mouse_prev;
vec2 mouse_curr;
float mouse_dist;

int resources[TileType_Count];
[
    "none resource",
    "sanity",
    "belonging",
    "vitality",
    "competency",
    "peace",
] @=> string resource_names[];

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

    // UI.pushStyleColor(UI_Color.Button, UI.convertHSVtoRGB(@(i / num_presets, 0.6f, 0.6f)));
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

        for (1 => int i; i < resource_names.size(); ++i) {
            g.text(resource_names[i] + " " + resources[i], text_pos, .3);
            .5 -=> text_pos.y;
        }

        g.popTextControlPoint();
    }

    { // draw grid
        g.pushLayer(1);
        for (-12 => int i; i < 12; ++i) {
            i + .5 => float t;
            // "x" axis
            g.line(-100 * grid.i_hat + t * grid.j_hat, 100 * grid.i_hat + t * grid.j_hat);
            // "y" axis
            g.line(-100 * grid.j_hat + t * grid.i_hat, 100 * grid.j_hat + t * grid.i_hat);
        }
        g.popLayer();
    }
    

    { // upgrade UI
        UI.setNextWindowBgAlpha(0);
        UI.begin("Upgrades");
        UI.separatorText("Grass Upgrades");

        // green thumb
        (Math.pow(2, green_thumb - 1) * 5) $ int => int green_thumb_cost;
        if (upgrade(green_thumb_cost, "green thumb " + green_thumb, TileType_Grass)) {
            green_thumb++;
        }

        // grass rate (maybe rename to fertilizer?)
        grass_tick_level * grass_tick_level * 5 => int grass_tick_cost;
        if (upgrade(grass_tick_cost, "grass tick rate " + grass_tick_level, TileType_Grass)) {
            grass_tick_level++;
        }

        // animal friend (grass auto harvest)
        (Math.pow(2.0, animal_friends) * 20) $ int => int animal_friend_cost;
        if (upgrade(animal_friend_cost, "animal friends " + animal_friends, TileType_Grass)) {
            animal_friends++;
        }

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

            UI.beginDisabled(hike_upgrades[hike_multitasker]);
            if (upgrade(20, "multitasker (smell flowers)", TileType_Hike)) {
                true => hike_upgrades[hike_multitasker];
            }
            UI.endDisabled();
        }

        UI.end();

    }
}