/*

Ideas:
- if mouse leaves territory, lose sanity (uncaptured territory fog is WORK / DEADLINES / STRESS)
    - if sanity falls to 0, you lose?
    - spending sanity on that territory breaks it down, when reaching 0 it "unlocks"
- spend sanity points to acquire adjacent tiles, which may have grass and allow you to "touch" wider swathes
- some tiles might have other tile types besides grass e.g.
    - water
    - bunnies / other familiars
        - a bunny when unlocked will sit on a grass tile and accelerate sanity generation
- victory when you cover the entire territory with grass, vanquishing the last stress/work tile
    - achieve enlightenment. screen goes full white. nice music. credits

- hexagonal grid??
    - https://stackoverflow.com/questions/7705228/hexagonal-grids-how-do-you-find-which-hexagon-a-point-is-in
    - https://catlikecoding.com/unity/tutorials/hex-map/part-1/
    - https://www.redblobgames.com/grids/hexagons/

- can zoom in/out to micro/macro scales ? ? ?

*/
@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/spring.ck"
@import "../lib/M.ck"

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

0 => int TileType_None;
1 => int TileType_Work;
2 => int TileType_Grass;

class Tile {
    int type;

    int row;
    int col;
    vec2 pos; // world pos


    // work params
    5 => int work_sanity_to_complete;
    int sanity_spent;

    // grass params
    // 10 => float max_hp;
    // max_hp => float hp;
    // 1 => float hp_regen_per_sec;
    2.0 => float grass_fill_time_secs; // time for grass to produce 1 sanity
    float grass_curr_time; // time since last production
    Spring grass_rot_spring(0, 1200, 12);
}

3 => int N_ROWS;
3 => int N_COLS;

Tile tiles[N_COLS][N_ROWS];

// player params
5 => int sanity;
vec2 mouse_prev;
vec2 mouse_curr;

{ // init
    N_COLS / 2 => int mid_x;
    N_ROWS / 2 => int mid_y;

    for (int row; row < N_ROWS; row++) {
        for (int col; col < N_COLS; col++) {
            tiles[col][row] @=> Tile t;
            TileType_Work => t.type;

            @(
                col - mid_x,
                row - mid_y
            ) => t.pos;

            // sanity to complete scales with distance from center
            (Math.abs(row - mid_y) + Math.abs(col - mid_x)) * 5 => t.work_sanity_to_complete;

        }
    }

    // center tile is grass
    TileType_Grass => tiles[mid_x][mid_y].type;
}



while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
    mouse_curr => mouse_prev;
    g.mousePos() => mouse_curr;

    M.dist(mouse_prev, mouse_curr) => float mouse_dist;
    

    .4 * Color.GREEN => vec3 color;

    // click
    {
        // if (g.mouseLeftDown() && M.inside( mouse_curr, aabb )) {
        //     Color.RED => color;
        //     1 -=> hp;
        //     ++sanity;
        // }
    }

    // detect which grid point is being hovered
    (mouse_curr.x + .5 * Math.sgn(mouse_curr.x)) $ int => int grid_x;
    (mouse_curr.y + .5 * Math.sgn(mouse_curr.y)) $ int => int grid_y;

    g.pushLayer(10);
    g.square(@(grid_x, grid_y), 1, Color.RED);
    g.popLayer();

    // hover + cooldown
    for (int row; row < N_ROWS; row++) {
        for (int col; col < N_COLS; col++) {
            tiles[col][row] @=> Tile@ t;
            M.aabb(t.pos, .5, .5) => vec4 aabb;
            t.grass_rot_spring.update(dt);

            if (t.type == TileType_Grass) {

                float grass_progress;

                // passively generate sanity
                .05 * dt +=> grass_progress;

                if (M.inside( mouse_curr, aabb )) {
                    // hover to generate
                    .5 * dt +=> grass_progress;

                    // swipe/rub to generate
                    if (M.inside(mouse_prev, aabb)) {
                        (mouse_dist * dt * 10) +=> grass_progress;
                    }
                }

                grass_progress +=> t.grass_curr_time;
                if (t.grass_curr_time > t.grass_fill_time_secs)  {
                    t.grass_fill_time_secs -=> t.grass_curr_time;
                    ++sanity;
                    t.grass_rot_spring.pull(.2 * (maybe ? -1 : 1));
                    g.score("+1", t.pos, .5::second, .5, .2);
                }

                // plant draw
                g.pushLayer(1);
                progressBar(t.grass_curr_time / t.grass_fill_time_secs, t.pos, .8, .1, Color.WHITE, Color.WHITE);
                g.popLayer();

                g.squareFilled(t.pos, t.grass_rot_spring.x, 1, color);
            }
            else if (t.type == TileType_Work) {
                // spend sanity to work
                if (g.mouseLeftDown() && M.inside(mouse_curr, aabb) && sanity > 0) {
                    --sanity;
                    ++t.sanity_spent;

                    if (t.sanity_spent == t.work_sanity_to_complete) {
                        TileType_Grass => t.type;
                    }
                    
                    t.grass_rot_spring.pull(.2 * (maybe ? -1 : 1));

                    g.pushColor(Color.RED);
                    g.score("-1", t.pos, .5::second, .5, .2);
                    g.popColor();
                }

                g.squareFilled(t.pos, t.grass_rot_spring.x, 1, Color.GRAY);
                g.text("work", t.pos, .2);

                // progress bar
                if (t.sanity_spent > 0) {
                    g.pushLayer(1);
                    progressBar(t.sanity_spent $ float / t.work_sanity_to_complete, t.pos, .8, .1, Color.WHITE, Color.WHITE);
                    g.popLayer();
                }
            }
        }
    }

    { // draw
        g.pushTextControlPoint(0, 1);
        g.text(sanity + " sanity", g.screen_top_left + @(.5, -.5), .3);
        g.popTextControlPoint();

        // gridlines 
        N_COLS / 2.0 => float w;
        N_ROWS / 2.0 => float h;
        g.pushLayer(1);
        for (int i; i < N_COLS + 1; i++) {
            -N_COLS / 2.0 + i => float x;
            g.line(@(x, -h), @(x, h));
        }
        for (int i; i < N_ROWS + 1; i++) {
            -N_ROWS / 2.0 + i => float y;
            g.line(@(-w, y), @(w, y));
        }
        g.popLayer();
    }
}