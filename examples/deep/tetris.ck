/*
Single-file tetris example

Resources:
- https://tetris.wiki/Tetris_Guideline
    The playfield (known as the Matrix in the guideline) is 10x20
    with an additional 20 cell buffer zone above the top of the playfield, 
    usually hidden or obstructed by the window. 
    If the hardware permits, a sliver of the 21st row is shown to aid 
    players manipulate the active piece in that area.

    Tetrominoes appear on the 21st and 22nd rows of the playfield, 
    centered and rounded to the left when needed. 
    They must start with their flat side down, 
    and move down immediately after appearing.
- https://harddrop.com/wiki/Spawn_Location
    L, J, T, S and Z pieces rotate in a 3x3 box and spawn horizontally in most games. 
    There's no unambiguous way to spawn a 3 columns-wide piece in a 10 columns-wide 
    matrix because it will always be off-center. 
    The current standard is left-handed: 3 empty columns on left side, 4 empty columns on right side

    I and O pieces can be centered in a 10 columns-wide matrix

*/



// ========= ======= ======= ===== 
// ======= App Config ======= ======= === 
// ========= ======= ======= ===== 

// disable tonemapping / HDR
GG.outputPass().tonemap(OutputPass.ToneMap_None);
// disable gamma
GG.outputPass().gamma(false);

// init camera
GG.camera().orthographic();
GG.camera().viewSize(10);
GG.camera().posZ(GG.camera().clipFar() - 1.0);
GG.camera().posX(0).posY(0);

// disable skybox (messes with screen clear / load)
null => GG.scene().skybox;

// default font
GText.defaultFont("chugl:proggy-tiny");

// ========= ======= ======= ===== ===== 
//     UTILITIES
// ========== ========= ========= ======
fun void assert(int condition, string msg) { if (!condition) <<< msg >>>; }

class Animation {
    int id;         // unique id of animation (used for lookups)
    // int sid;

    float t;        // normalized progress [0,1]
    float start;    // start value
    float duration; // duration of animation
}

class AnimationManager {
    Animation animations[0];

    fun void start(int id, float start, float duration) {
        Animation a;
        // first check if animation already exists
        for (auto anim : animations) {
            if (anim.id == id) {
                anim @=> a;
                break;
            }
        }

        id => a.id;
        0 => a.t;
        start => a.start;
        duration => a.duration;
        animations << a;
    }

    
    fun float get(int id, float target) { 
        Animation@ a;

        // look for it
        for (auto anim : animations) {
            if (anim.id == id) {
                anim @=> a;
                break;
            }
        }

        if (a) {
            // ease out quad
            (1 - (1-a.t) * (1-a.t)) => float ease;
            return a.start + ease * (target - a.start);
        } else return target;

    }

    fun void update(float dt) {
        for (animations.size() - 1 => int i; i >= 0; --i) {
            animations[i] @=> Animation a;

            if (a.t >= 1) {
                animations.erase(i);
                continue;
            }

            dt / a.duration +=> a.t;
        }
    }
}
AnimationManager am;


// constants and enums
class C {

    // screen enum
    0 => static int SCREEN_PLAYING;
    1 => static int SCREEN_GAMEOVER;

    // animation enum
    1 => static int ANIM_SCREENSHAKE_Y;
    2 => static int ANIM_CLEAR_ROW;


    // rotation enum 
    // tetronimo rotation has max 4 states with cyclic symmetry
    // Rotation states (symmetric)
    //        CCW     CW
    // R - 2 - L - 0 - R - 2 - L
    // for more info, see https://tetris.wiki/Super_Rotation_System
    0 => static int ROT_STATE_SPAWN;
    1 => static int ROT_STATE_L;
    2 => static int ROT_STATE_2;
    3 => static int ROT_STATE_R;
    4 => static int ROT_STATE_COUNT;

    // piece enum
    0 => static int PIECE_BORDER;
    1 => static int PIECE_I;
    2 => static int PIECE_J;
    3 => static int PIECE_L;
    4 => static int PIECE_O;
    5 => static int PIECE_S;
    6 => static int PIECE_T;
    7 => static int PIECE_Z;
    8 => static int PIECE_EMPTY;
    9 => static int PIECE_COUNT;

    // color palette
    [
        Color.WHITE,      // border
        Color.CYAN,      // I
        Color.BLUE,      // J
        Color.ORANGE,    // L
        Color.YELLOW,    // O
        Color.GREEN,     // S
        Color.PURPLE,    // T
        Color.RED,       // Z
        Color.DARKGRAY,  // empty
    ] @=> static vec3 piece_colors[];

    // tetronimo colliders 
    [
        0, 0, 0, 0,
        1, 1, 1, 1,
        0, 0, 0, 0,
        0, 0, 0, 0,
    ] @=> static int PIECE_I_COLLIDER[];
    [
        1, 0, 0,
        1, 1, 1,
        0, 0, 0,
    ] @=> static int PIECE_J_COLLIDER[];
    [
        0, 0, 1,
        1, 1, 1,
        0, 0, 0,
    ] @=> static int PIECE_L_COLLIDER[];
    [
        1, 1,
        1, 1,
    ] @=> static int PIECE_O_COLLIDER[];
    [
        0, 1, 1,
        1, 1, 0,
        0, 0, 0,
    ] @=> static int PIECE_S_COLLIDER[];
    [
        0, 1, 0,
        1, 1, 1,
        0, 0, 0,
    ] @=> static int PIECE_T_COLLIDER[];
    [
        1, 1, 0,
        0, 1, 1,
        0, 0, 0,
    ] @=> static int PIECE_Z_COLLIDER[];

    [
        [0],
        PIECE_I_COLLIDER,
        PIECE_J_COLLIDER,
        PIECE_L_COLLIDER,
        PIECE_O_COLLIDER,
        PIECE_S_COLLIDER,
        PIECE_T_COLLIDER,
        PIECE_Z_COLLIDER,
    ] @=> static int PIECE_COLLIDERS[][];


    [
        @(0, 0),
        @(3, 21), // I
        @(3, 21), // J
        @(3, 21), // L
        @(4, 21), // O
        @(3, 21), // S
        @(3, 21), // T
        @(3, 21), // Z
    ] @=> static vec2 PIECE_SPAWN_POS[];


    // gameplay config
    .1 => static float SPAWN_TIME;

    // graphical config
    .3 => static float piece_sz;
}


class GameState {
    float playtime;

    C.SCREEN_PLAYING => int screen;

    10 => int MATRIX_W;
    22 => int MATRIX_H;

    int _matrix[MATRIX_H][MATRIX_W];
    int matrix_row_ids[0]; // debug

    int piece_random_bag[0];
    float next_piece_spawn_time;
    int prev_piece_kind;

    .75 => float step_cd_max;
    0 => float step_cd;

    // active player-controllable piece params
    int piece_x;
    int piece_y;
    int piece_kind;
    int piece_rot;
    int piece_drop_y;

    // held piece
    int hold_kind;
    int hold_rot;
    int hold_used;

    // detect held arrow keys for sliding left and right
    .06 => float key_repeat_cd_max;
    .16 => float key_held_threshold;
    0 => float key_right_cd;
    0 => float key_right_held_dur;
    0 => float key_left_cd;
    0 => float key_left_held_dur;

    fun void reset() {
        C.SCREEN_PLAYING => screen;
        for (int y; y < MATRIX_H; ++y) {
            _matrix[y].zero();
            y => matrix_row_ids["" + _matrix[y]];
        }
        piece_random_bag.clear();
        reshuffleBag();
        newPiece(randomPiece(), C.ROT_STATE_SPAWN);

        0 => prev_piece_kind;
        0 => hold_kind;
        0 => hold_rot;
        false => hold_used;
    }

    // index into collider with the given rotation
    // for colliders, +x goes right, +y goes *down*
    fun int indexCollider(int kind, int x, int y, int rot) {
        C.PIECE_COLLIDERS[kind] @=> int collider[];
        // <<< "indexCollider()", kind, x, y, rot >>>;

        if (kind == C.PIECE_O) {
            return 1; // O piece doesn't rotate, has hitbox at every cell
        }
        else if (kind == C.PIECE_I) { // I collider is 4x4
            if      (rot == C.ROT_STATE_SPAWN || rot == C.ROT_STATE_2) return collider[x + 4*y];
            else if (rot == C.ROT_STATE_R     || rot == C.ROT_STATE_L) return collider[4*x + (3-y)];
            // else if (rot == C.ROT_STATE_2)     return collider[3-x + 4*(3-y)];
            // else if (rot == C.ROT_STATE_L)     return collider[4*(3-x) + y];
        }
        else if (kind == C.PIECE_S || kind == C.PIECE_Z) {
            if      (rot == C.ROT_STATE_SPAWN || rot == C.ROT_STATE_2) return collider[x + 3*y];
            else if (rot == C.ROT_STATE_R     || rot == C.ROT_STATE_L) return collider[3*x + (2-y)];
        }
        else { // all other colliders are 3x3
            if      (rot == C.ROT_STATE_SPAWN) return collider[x + 3*y];
            else if (rot == C.ROT_STATE_R)     return collider[3*x + (2-y)];
            else if (rot == C.ROT_STATE_2)     return collider[2-x + 3*(2-y)];
            else if (rot == C.ROT_STATE_L)     return collider[3*(2-x) + y];
        }

        <<< "indexCollider() invalid args", kind, x, y, rot >>>;
        return 0;
    }

    fun int pieceDimension(int kind) {
        3 => int dim;
        if (kind == C.PIECE_O) 2 => dim;
        if (kind == C.PIECE_I) 4 => dim;
        if (kind == C.PIECE_BORDER) 0 => dim;
        return dim;
    }

    // check if piece with top-left cell at x,y collides with matrix
    fun int collides(int kind, int matrix_x, int matrix_y, int rot) {
        assert(kind, "collides() called with invalid piece");

        pieceDimension(kind) => int dim;

        // TODO wall kick stuff

        for (int x; x < dim; x++) {
            for (int y; y < dim; y++) {
                (x + matrix_x) => int _x;
                (matrix_y - y) => int _y;
                (_x < 0 || _x >= MATRIX_W) || (_y < 0 || _y >= MATRIX_H) => int out_of_bounds;

                indexCollider(kind, x, y, rot) => int piece_hitbox;

                if (piece_hitbox && out_of_bounds) return true;
                if (piece_hitbox && _matrix[_y][_x]) return true;
            }
        }
        return false;
    }

    fun void addToMatrix(int kind, int matrix_x, int matrix_y, int rot) {
        assert(kind, "addToMatrix() called with invalid piece");

        pieceDimension(kind) => int dim;

        for (int x; x < dim; x++) {
            for (int y; y < dim; y++) {
                (x + matrix_x) => int _x;
                (matrix_y - y) => int _y;

                indexCollider(kind, x, y, rot) => int piece_hitbox;
                if (piece_hitbox) {
                    // sanity check no collision
                    assert(_matrix[_y][_x] == 0, "addToMatrix() collision");
                    kind => _matrix[_y][_x];
                } 
            }
        }
    }

    fun void updatePieceDropY() {
        if (piece_kind == 0) {
            0 => piece_drop_y;
            return;
        }

        for (piece_y - 1 => int y; y >= 0; --y) {
            if (collides(piece_kind, piece_x, y, piece_rot)) {
                y+1 => piece_drop_y;
                break;
            }
        }
    }

    fun void reshuffleBag() {
        if (piece_random_bag.size() > 0) return;
        piece_random_bag << C.PIECE_I;
        piece_random_bag << C.PIECE_J;
        piece_random_bag << C.PIECE_L;
        piece_random_bag << C.PIECE_O;
        piece_random_bag << C.PIECE_S;
        piece_random_bag << C.PIECE_T;
        piece_random_bag << C.PIECE_Z;

        // fisher-yates shuffle
        for (int i; i < piece_random_bag.size(); ++i) {
            Math.random2(i, piece_random_bag.size() - 1) => int swap_idx;
            piece_random_bag[i] => int swap;
            piece_random_bag[swap_idx] => piece_random_bag[i];
            swap => piece_random_bag[swap_idx];
        }
    }

    fun int randomPiece() {
        // grab from random bag
        piece_random_bag[-1] => int kind;
        piece_random_bag.popBack();
        reshuffleBag();
        return kind;
    }

    fun void newPiece(int kind, int rot) {
        kind => piece_kind;
        rot => piece_rot;

        // default spawn 3x3 pieces at matrix(3,21)
        C.PIECE_SPAWN_POS[piece_kind].x $ int => piece_x;
        C.PIECE_SPAWN_POS[piece_kind].y $ int => piece_y;

        updatePieceDropY();

        // reset fall timer
        0 => step_cd;
    }

    // when clearing rows, we first queue them and wait for 
    // the clearline animation to finish before actually removing
    // blocks from the matrix
    int cleared_rows_indices[0];
    fun void prepRowsToClear(int start_y) {
        assert(cleared_rows_indices.size() == 0, "prepRowsToClear()");

        // gather cleared rows
        for (start_y => int y; y >= Math.max(0, start_y - 3); y--) {
            true => int row_cleared;
            for (auto c : _matrix[y]) {
                if (c == 0) {
                    false => row_cleared;
                    break;
                }
            }

            if (row_cleared) cleared_rows_indices << y;
        }

        if (cleared_rows_indices.size() > 0) {
            am.start(C.ANIM_CLEAR_ROW, 1, .16);
        }
    }

    fun int isRowCleared(int y) {
        for (auto r : cleared_rows_indices) {
            if (r == y) return true;
        }
        return false;
    }

    fun void _clearRows() {
        assert(cleared_rows_indices.size() > 0, "_clearRows called incorrectly");

        // clear it by removing and then re-appending
        // debug print out all row ids
        <<< "rows to clear:" >>>;
        for (int row; row < cleared_rows_indices.size(); ++row) {
            <<< matrix_row_ids["" + _matrix[cleared_rows_indices[row]]] >>>;
        }

        // cleared_rows_indices should already be in descending order
        // this is a little piggy (swapping would be faster)
        // but optimizing speed here doesn't matter, I'd rather do the simplest thing
        for (int i; i < cleared_rows_indices.size(); ++i) {
            _matrix[cleared_rows_indices[i]] @=> int row[];
            row.zero();
            _matrix.erase(cleared_rows_indices[i]);
            _matrix << row;
        }

        <<< "final list:" >>>;
        for (int row; row < _matrix.size(); ++row) {
            <<< row, ":", matrix_row_ids["" + _matrix[row]] >>>;
        }

        cleared_rows_indices.clear();
    }

    fun void placePiece(int y) {
        assert(piece_kind > 0, "placePiece() invalid piece");

        piece_kind => prev_piece_kind;

        // collided, lock in place
        addToMatrix(piece_kind, piece_x, y, piece_rot);

        // clear rows
        // TODO animate via pending_rows back-and-forth with graphics system
        prepRowsToClear(y);

        // unlock hold
        false => hold_used;

        // check game over
        if (y >= MATRIX_H - 1) {
            C.SCREEN_GAMEOVER => screen;
        } else {
            // prep timer for spawning new piece
            0 => piece_kind;
            C.SPAWN_TIME + playtime => next_piece_spawn_time;
        }
    }

    fun void update(float dt) {
        // disable user input and simulation while waiting for clear animation
        if (cleared_rows_indices.size() > 0) return;

        dt +=> step_cd;
        dt +=> playtime;

        if (screen == C.SCREEN_GAMEOVER) {
            if (GWindow.keyDown(GWindow.KEY_SPACE)) reset();
            return;
        }

        if (piece_kind) { // player input
            // drop it
            if (GWindow.keyDown(GWindow.KEY_SPACE)) {
                // camera shake (intensity scales with drop distance)
                am.start(
                    C.ANIM_SCREENSHAKE_Y, 
                    Math.remap(piece_y - piece_drop_y, 0, MATRIX_H, 0, -.27),
                    C.SPAWN_TIME);
                placePiece(piece_drop_y);
            }

            // swap aka hold piece
            if (!hold_used && (GWindow.keyDown(GWindow.KEY_C) || GWindow.keyDown(GWindow.KEY_LEFTSHIFT))) {
                true => hold_used;

                piece_kind => int curr_kind;
                piece_rot => int curr_rot;
                if (hold_kind == 0) {
                    0 => piece_kind;
                    C.SPAWN_TIME + playtime => next_piece_spawn_time;
                } else {
                    // swap out
                    newPiece(hold_kind, hold_rot);
                }
                // store current piece
                curr_kind => hold_kind;
                curr_rot => hold_rot;
            }

            piece_x => int new_x;
            if (GWindow.key(GWindow.KEY_DOWN)) { 16 * dt +=> step_cd; }
            if (GWindow.keyDown(GWindow.KEY_LEFT)) { 
                1 -=> new_x;
                0 => key_left_cd;
                0 => key_left_held_dur;

            }
            if (GWindow.key(GWindow.KEY_LEFT)) { 
                dt +=> key_left_held_dur;
                if (key_left_held_dur >= key_held_threshold) {
                    dt +=> key_left_cd;
                    if (key_left_cd >= key_repeat_cd_max) {
                        key_repeat_cd_max -=> key_left_cd;
                        1 -=> new_x;
                    }
                }
            }
            if (GWindow.keyDown(GWindow.KEY_RIGHT)) { 
                1 +=> new_x; 
                0 => key_right_cd;
                0 => key_right_held_dur;
            }
            if (GWindow.key(GWindow.KEY_RIGHT)) { 
                dt +=> key_right_held_dur;
                if (key_right_held_dur > key_held_threshold) {
                    dt +=> key_right_cd;
                    if (key_right_cd >= key_repeat_cd_max) {
                        key_repeat_cd_max -=> key_right_cd;
                        1 +=> new_x;
                    }
                }
            }

            // try going sideways
            if (new_x != piece_x && 
                !collides(piece_kind, new_x, piece_y, piece_rot)) {
                new_x => piece_x;
            } 

            // rotation
            piece_rot => int new_rot;
            if (GWindow.keyDown(GWindow.KEY_UP)) ++new_rot;
            if (GWindow.keyDown(GWindow.KEY_Z) || GWindow.keyDown(GWindow.KEY_LEFTCONTROL)) --new_rot;
            // wrap rotation
            while (new_rot < 0) C.ROT_STATE_COUNT +=> new_rot;
            if (new_rot >= C.ROT_STATE_COUNT) new_rot % C.ROT_STATE_COUNT => new_rot;

            // try rotating
            // TODO apply Super Rotation System
            if (new_rot != piece_rot &&
                !collides(piece_kind, piece_x, piece_y, new_rot)) {
                new_rot => piece_rot;
            }


            // simulation step
            if (step_cd >= step_cd_max) {
                step_cd_max -=> step_cd;
                // try lower tetronimo 1 block
                if (collides(piece_kind, piece_x, piece_y - 1, piece_rot)) {
                    placePiece(piece_y);
                } else {
                    1 -=> piece_y;
                }
            }

            updatePieceDropY();

        } else {
            // progress piece spawn timer
            if (playtime > next_piece_spawn_time) 
                newPiece(randomPiece(), C.ROT_STATE_SPAWN);
        }
    }
} 

class Graphics { 
    static PlaneGeometry plane_geo;
    static FlatMaterial black_material; black_material.color(Color.BLACK);
    static FlatMaterial colored_materials[0];
    for (int i; i < C.PIECE_COUNT; i++) {
        colored_materials << new FlatMaterial(C.piece_colors[i]);
    }

    GMesh@ plane_pool[0];
    int plane_pool_curr;

    GText@ text_pool[0];
    int text_pool_curr;

    [0.0] @=> float layer[];
    fun void pushLayer(float l) { layer << l; }
    fun void popLayer() { layer.popBack(); }

    0 => static int DRAW_POSITION_ABSOLUTE;
    1 => static int DRAW_POSITION_RELATIVE; // draws everything relative to camera, so even if camera moves, the drawing appears in a fixed screen region
    [DRAW_POSITION_ABSOLUTE] @=> int draw_pos_mode[];
    [@(.5, .5)] @=> vec2 text_control_point[];
    fun void pushDrawMode(int m) { draw_pos_mode << m; }
    fun void popDrawMode() { draw_pos_mode.popBack(); }
    fun void pushControlPoint(vec2 m) { text_control_point << m; }
    fun void popControlPoint() { text_control_point.popBack(); }

    fun void text(string s, vec2 pos, vec3 color, float scale) {
        if (draw_pos_mode[-1] == DRAW_POSITION_RELATIVE) 
            GG.camera().pos() $ vec2 +=> pos;

        if (text_pool_curr >= text_pool.size()) {
            text_pool << new GText;
        }

        text_pool[text_pool_curr++] @=> GText text;
        text.pos(pos).sca(scale);
        text.posZ(layer[-1]);
        text.color(@(color.r, color.g, color.b, 1.0));
        text.text(s);
        text.controlPoints(text_control_point[-1]);
        text --> GG.scene();
    }

    fun void box(vec2 pos, float w, float h, Material@ m) {
        if (draw_pos_mode[-1] == DRAW_POSITION_RELATIVE) 
            GG.camera().pos() $ vec2 +=> pos;

        if (plane_pool_curr >= plane_pool.size()) {
            plane_pool << new GMesh(plane_geo, m);
        }

        plane_pool[plane_pool_curr++] @=> GMesh plane;
        plane.pos(pos).scaX(w).scaY(h);
        plane.posZ(layer[-1]);
        plane.material(m);
        plane --> GG.scene();
    }

    fun void square(vec2 pos, float l, Material@ m) {
        box(pos, l, l, m);
    }

    fun void square(vec2 pos, float l, int piece_kind) {
        square(pos, l, colored_materials[piece_kind]);
    }

    fun void printStats() {
        <<< "plane_pool.size()", plane_pool.size() >>>;
        <<< "text_pool.size()", text_pool.size() >>>;
        <<< "#animations", am.animations.size() >>>;
        // <<< "piece enums", C.PIECE_I, C.PIECE_J, C.PIECE_L >>>;
    }

    fun void drawBlock(float x, float y, int kind) {
        C.piece_sz * @(-4.5, -9.5) => vec2 bot_left_cell;
        square(bot_left_cell + C.piece_sz * @(x,y), .9 * C.piece_sz, kind);
    }

    fun void drawBlockOutline(float x, float y, int kind) {
        C.piece_sz * @(-4.5, -9.5) => vec2 bot_left_cell;
        bot_left_cell + C.piece_sz * @(x,y) => vec2 p;

        // draw on lower layer so outline doesn't cover solid block
        pushLayer(-.2);
        square(p, .9 * C.piece_sz, kind);
        popLayer();

        pushLayer(-.1);
        square(p, .68 * C.piece_sz, black_material);
        popLayer();
    }

    fun void drawPiece(int piece, int rot, float matrix_x, float matrix_y, int drop_y) {
        if (piece == 0) return;

        gs.pieceDimension(piece) => int dim;
        for (int x; x < dim; ++x) {
            for (int y; y < dim; ++y) {
                if (gs.indexCollider(piece, x, y, rot)) {

                    (x + matrix_x) => float _x;
                    (matrix_y - y) => float _y;
                    drawBlock(_x, _y, piece);

                    // draw drop outline
                    if (drop_y) drawBlockOutline(_x, drop_y - y, piece);
                }

            }
        }
    }

    fun void draw() {
        // update camera pos (for screenshake)
        am.get(C.ANIM_SCREENSHAKE_Y, 0) => GG.camera().posY;


        // draw 10x20 playfield boundaries
        for (-1 => int x; x < 11; ++x) {
            drawBlock(x, -1, C.PIECE_BORDER);
            drawBlock(x, 20, C.PIECE_BORDER);
        }

        for (-1 => int y; y < 21; ++y) {
            drawBlock(-1, y, C.PIECE_BORDER);
            drawBlock(10, y, C.PIECE_BORDER);
        }

        // process cleared rows
        am.get(C.ANIM_CLEAR_ROW, 0.0) => float clear_row_t;
        if (gs.cleared_rows_indices.size() > 0 && clear_row_t <= 0) 
            gs._clearRows();

        // draw matrix
        for (int y; y < 20; ++y) {
            if (gs.isRowCleared(y)) {
                C.piece_sz * (y - 9.5) => float row_y;
                C.piece_sz * gs.MATRIX_W => float row_w;
                box(@(0,row_y), row_w, clear_row_t * C.piece_sz, colored_materials[gs.prev_piece_kind]);
            } else {
                for (int x; x < gs.MATRIX_W; ++x) {
                    gs._matrix[y][x] => int kind;
                    if (kind) {
                        drawBlock(x, y, kind);
                    } 
                }
            }
        }

        // draw active piece
        drawPiece(gs.piece_kind, gs.piece_rot, gs.piece_x, gs.piece_y, gs.piece_drop_y);

        if (gs.screen == C.SCREEN_PLAYING) {
            pushDrawMode(DRAW_POSITION_RELATIVE);

            C.piece_sz * 10 => float y;
            C.piece_sz * -12.5 => float x;
            text("NEXT:", @(x, y), Color.WHITE, .6);
            gs.piece_random_bag[-1] => int next_piece_kind;
            drawPiece(
                next_piece_kind,
                (next_piece_kind != C.PIECE_I) ? C.ROT_STATE_SPAWN : C.ROT_STATE_L,
                -5, 20, false
            );


            C.piece_sz * 5 => y;
            Color.WHITE => vec3 swap_text_color;
            if (gs.hold_used) Color.GRAY => swap_text_color;
            text("SWAP:", @(x, y), swap_text_color, .6);
            drawPiece( gs.hold_kind, gs.hold_rot, -5, 15, false);

            // debug draw row text
            // for (int i; i < gs.MATRIX_H; ++i) {
            //     C.piece_sz * (i - 9.5) => float y;
            //     C.piece_sz * 6.5 => float x;
            //     text("" + gs.matrix_row_ids["" + gs._matrix[i]], @(x,y), Color.WHITE, .3);
            // }


            { // draw controls 
                pushControlPoint(@(0, .5));

                C.piece_sz * 10 => float y;
                C.piece_sz * 8 => float x;
                text("ROT:  UP    ", @(x, y), Color.WHITE, .6);
                2*C.piece_sz -=> y;
                text("HOLD: C/LSHIFT ", @(x, y), Color.WHITE, .6);
                2*C.piece_sz -=> y;
                text("DROP: SPACE   ", @(x, y), Color.WHITE, .6);

                popControlPoint();
            }


            popDrawMode();
        }


        // draw gameover text
        if (gs.screen == C.SCREEN_GAMEOVER) {
            C.piece_sz * 13.5 => float y_max;

            (4*gs.playtime)$int => int flicker;
            if (flicker % 2) 
                text("GAME OVER", @(0, y_max), C.piece_colors[flicker % C.piece_colors.size()], 1);
            text("SPACE TO RESET", @(0, -y_max), Color.WHITE, 1);
        }

        // cleanup: reset pools
        for (plane_pool_curr => int i; i < plane_pool.size(); ++i) 
            plane_pool[i].detachParent();
        0 => plane_pool_curr;

        for (text_pool_curr => int i; i < text_pool.size(); ++i) 
            text_pool[i].detachParent();
        0 => text_pool_curr;

        // reset layer stack
        layer.erase(1, layer.size());
        draw_pos_mode.erase(1, draw_pos_mode.size());
        text_control_point.erase(1, text_control_point.size());

    }

} 

GameState gs;
Graphics g;
gs.reset();

while (1) {
    GG.nextFrame() => now;
    GG.fc() => int fc;
    GG.dt() => float dt;

    // step gamestate
    gs.update(dt);

    // draw
    am.update(dt);
    g.draw();
    // if (fc % 60 == 0) g.printStats();
}