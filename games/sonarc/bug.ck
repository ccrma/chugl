@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"

G2D g;

// key mapping enum
0 => int Key_Left;
1 => int Key_Right;

class Entity {
    // player entity
    int player_id; // 0, 1, 2, 3
    -1 => int gamepad_id;  // -1 means no gamepad connected
    int disabled;

    vec2 player_pos;
    vec2 player_prev_pos;
    float player_rot;
    int frame_alive_count;
    Color.MAGENTA => vec3 color;
    float last_speed;

    // used by SnakeGame
    vec2 player_pos_history[120];
    int player_pos_history_write_idx;
    int player_pos_history_read_idx;

    // control mapping for each player
    fun int key(int which) {
        // if (gs.room == match_room && match_room.match_state == match_room.Match_Replay) {
        //     if (which == Key_Left) {
        //         return T.arrayHas(match_room.replay_current_keys, gs.player_key_left[player_id]);
        //     }
        //     if (which == Key_Right) {
        //         return T.arrayHas(match_room.replay_current_keys, gs.player_key_right[player_id]);
        //     }
        // }
        if (which == Key_Left) {
            return GWindow.key(gs.player_key_left[player_id]);
        }
        if (which == Key_Right) {
            return GWindow.key(gs.player_key_right[player_id]);
        }
        return false;
    }

    fun float axis(int axis) {
        Gamepad.axis(gamepad_id, axis) => float val;
        // deadzone
        if (Math.fabs(val) < .08) return 0;
        return val;
    }

    fun void draw() {
        g.pushColor(color);

        // draw player
        if (!disabled) {
            g.polygonFilled(
                player_pos, player_rot, gs.player_vertices, 0.0
            );
        }
        g.popColor();
    }
}

class Room {
    fun void enter() {}
    fun void leave() {}
    fun Room update(float dt) { return null; } // returns true if end state is met
}

class GameState {
    // Room room;

    // [
    //     SnakeRoom snake_room
    // ] @=> Room game_rooms[];

    [
        GWindow.Key_Right,  // p1
        GWindow.Key_D,      // p2
        GWindow.Key_P,
        GWindow.Key_M,
    ] @=> int player_key_right[];

    [
        GWindow.Key_Left, 
        GWindow.Key_A, 
        GWindow.Key_O, 
        GWindow.Key_N, 
    ] @=> int player_key_left[];

    .25 => float player_scale;
    [ 
        player_scale * @(1 / Math.sqrt(3), 0),
        player_scale * @(-1 / (2 * Math.sqrt(3)), .5),
        player_scale * @(-1 / (2 * Math.sqrt(3)), -.5),
    ] @=> vec2 player_vertices[];

    4 => static int MAX_PLAYERS;
    2 => int num_players;
    Entity players[MAX_PLAYERS];

    // audio =========================
    float mic_volume; // should be updated and save at start of each frame

    // config constants =======================
    .3 => float player_base_speed;
    3.5 => float player_rot_speed; // idea: scale with mic volume too
}
GameState gs;


// gameloop
while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
}
