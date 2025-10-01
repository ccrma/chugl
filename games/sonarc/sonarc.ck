/*

TODO
- b2 collision testing
- add explosion sound effect / fart noise when someone dies

VR Lab Feedback
- when player wins, change font color to that player, add fireworks in that color in the background
- slow-motion hit-stop when someone dies (watch sakurai's video)

Ideas for progression/scaling difficulty:
- trails grow longer over time (add food to eat to grow longer like slither.io?)
- arena gets smaller
- players can choose when to drop trails (by holding down key)
    - trails remain forever
    - no default trail history
    - you have to yell into mic to replenish ammo for drawing trails?
        - but then does mic input do both movemenet AND trail ammo?
- overtime start playing music that forces the mic gain to be triggered

Optimization for line collision testing
- only add a history position if it's *not* collinear with the last segment
    - if it is collinear, extend the last segment
- track AABB for each player's arc, in broad-phase culling compute AABB intersection
- check all players simultaneously against all trail histories to avoid looping per player and to improve cache performance
- actually most optimal is just put the trail in b2, use b2 for collision detection
*/

@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"

GWindow.windowed(1920, 1080);
G2D g;
g.resolution(1920, 1080);

// ========================
// Sound
// ========================
class Sound {
    // idea: we want the mic volume to slew up quickly but slew down slowly (fills fast, depletes slow)
    adc => Gain adc_square => OnePole env_follower => blackhole;
    adc => adc_square;
    3 => adc_square.op;
    SndBuf replay_audio => dac;

    // filter pole position
    UI_Float env_low_cut(.08);
    UI_Float env_exp(.22);
    UI_Float env_pol_last;
    UI_Float env_pole_pos(.9998);
    env_pole_pos.val() => env_follower.pole;
}
Sound s;

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

    fun void die() {
        // destroy player
        true => disabled;
        spork ~ FX.explode(player_pos, 1::second, color);
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
    Room room;

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

    [
        Color.hex(0x00ffff),
        Color.hex(0xffa500),
        Color.hex(0x32CD32),
        Color.hex(0xFF0f00),
    ] @=> vec3 player_colors[];

    [
        @(3, -3),
        @(-3, 3),
        @(3, 3),
        @(-3, -3),
    ] @=> vec2 player_spawns[];

    [
        90.0 + 45,
        -45.0,
        180 + 45,
        45,
    ] @=> float player_rots_deg[];

    // init player info (for now)
    for (int i; i < players.size(); ++i) {
        player_colors[i] => players[i].color;
        i => players[i].player_id;
    }

    // replay (disabling for now) ==================================
    // "sonarc-replay-audio.wav" => string replay_audio_filename;
    // "sonarc-replay-keystrokes.txt" => string replay_ks_filename;
    // "sonarc-replay-volume.txt" => string replay_gain_filename;

    fun void enterRoom(Room new_room) {
        T.assert(new_room != null, "cannot transition from a null room");
        T.assert(new_room != room, "cannot transition from a room to itself");
        room.leave();
        new_room.enter();
        new_room @=> room;
    }

    // pos in [-1, 1] is NDC
    // width and height are [0, 1], relative to screen dimensions
    fun void progressBar(float percentage, vec2 pos_ndc, float width_ndc, float height_ndc, vec3 color) {
        g.NDCToWorldPos(pos_ndc.x, pos_ndc.y) => vec2 pos;
        g.NDCToWorldPos(width_ndc, height_ndc) => vec2 hw_hh;
        hw_hh.x => float hw;
        hw_hh.y => float hh;
        // shift pos.x to center
        // hw +=> pos.x;

        g.box(pos, 2 * hw, 2 * hh, color);

        -hw + (percentage * 2 * hw) => float end_x;
        g.boxFilled(
            pos - @(hw, hh),   // bot left
            pos + @(end_x, hh), // top right
            Color.WHITE
        );
    }
}
GameState gs;

class StartRoom extends Room {
    fun Room update(float dt) {
        // @bug: not handling case where >4 controllers connect
        // and gamepad ids exceed 3.

        if (GWindow.keyDown(GWindow.KEY_2)) 2 => gs.num_players;
        if (GWindow.keyDown(GWindow.KEY_3)) 3 => gs.num_players;
        if (GWindow.keyDown(GWindow.KEY_4)) 4 => gs.num_players;

        Gamepad.available() @=> int gamepads[];

        // assign gamepads and positions to players
        for (int i; i < Math.min(gamepads.size(), gs.MAX_PLAYERS); ++i) {
            gamepads[i] => gs.players[i].gamepad_id;
            gs.player_spawns[i] => gs.players[i].player_pos;
        }
        // remaining players don't have a gamepad assigned
        for (gamepads.size() => int i; i < gs.MAX_PLAYERS; ++i) {
            -1 => gs.players[i].gamepad_id;
            gs.player_spawns[i] => gs.players[i].player_pos;
        }

        // draw num players
        1 => float text_y_offset;
        g.text("Number of Players: " + gs.num_players, @(0, text_y_offset));

        // draw players and allow movement 
        for (int i; i < gs.num_players; ++i) {
            gs.players[i] @=> Entity e;
            float delta_rot;

            // input
            if (e.key(Key_Left)) gs.player_rot_speed => delta_rot;
            if (e.key(Key_Right)) -gs.player_rot_speed => delta_rot;
            if (Gamepad.available(e.gamepad_id)) {
                -gs.player_rot_speed * e.axis(Gamepad.AXIS_LEFT_X) => delta_rot;
            }
            (dt * delta_rot) +=> e.player_rot;

            // draw
            e.draw();
        }


        // stage selection
        return null;
    }
}

class SnakeRoom extends Room {
    .5 => float BORDER_PADDING; // padding in worldspace units of sides/top

    // match state enum
    // 0 => static int Match_Select;
    // 1 => static int Match_Running;
    // 2 => static int Match_Replay;
    // Match_Select => int match_state;

    UI_Float player_speed_volume_scale(8); // how much to scale speed with mic volume 

    [
        @(3, -3),
        @(-3, 3),
        @(3, 3),
        @(-3, -3),
    ] @=> vec2 player_spawns[];

    [
        90.0 + 45,
        -45.0,
        180 + 45,
        45,
    ] @=> float player_rots_deg[];

    vec2 border_min, border_max;

    Entity@ winner;

    // replay state
    // FileIO replay_ks;
    // FileIO replay_mic_gain;
    // int replay_current_keys[0];
    // StringTokenizer strtok;
    // float current_mic_gain;

    fun void enter() {
        _initPlayers();
    }

    fun Room update(float dt) {
        // sound update
        // if (match_state == Match_Replay) {
        //     if (replay_mic_gain.more()) replay_mic_gain.readLine().trim().toFloat() => gs.mic_volume;
        // } else {

        // update border vertices
        g.screenSize() => vec2 screen_size_world_space;
        screen_size_world_space * -.5 + @(BORDER_PADDING, BORDER_PADDING) => border_min;
        screen_size_world_space *  .5 - @(BORDER_PADDING, BORDER_PADDING) => border_max;
        [
            border_min, @(border_max.x, border_min.y),
            border_max, @(border_min.x, border_max.y)
        ] @=> vec2 border_vertices[];

        // update players
        for (int i; i < gs.num_players; i++) {
            gs.players[i] @=> Entity e;

            if (e.disabled) continue;

            e.frame_alive_count++;

            // update rotation
            gs.player_rot_speed + this.player_speed_volume_scale.val() * gs.mic_volume => float rot_speed;
            if (e.key(Key_Left)) {
                (dt * rot_speed) +=> e.player_rot;
            }
            if (e.key(Key_Right)) {
                -(dt * rot_speed) +=> e.player_rot;
            }

            // update position and trail
            e.player_pos => e.player_prev_pos;
            gs.player_base_speed + this.player_speed_volume_scale.val() * gs.mic_volume => float speed;
            speed => e.last_speed;
            dt * speed * M.rot2vec(e.player_rot) +=> e.player_pos;
            e.player_pos => e.player_pos_history[e.player_pos_history_write_idx++];
            if (e.player_pos_history_write_idx >= e.player_pos_history.size()) {
                0 => e.player_pos_history_write_idx;
            }
            // if we have filled the buffer once, update the read head to 1 in front of write
            if (e.frame_alive_count >= e.player_pos_history.size()) {
                e.player_pos_history_write_idx => e.player_pos_history_read_idx;
            }    

        }

        processTrailCollisions(); // AFTER all players have moved

        // test if outside border
        for (auto e : gs.players) {
            if (e.disabled) continue;

            for (-1 => int i; i < border_vertices.size() -1; i++) {
                e.player_pos.x > border_max.x
                || 
                e.player_pos.y > border_max.y
                || 
                e.player_pos.x < border_min.x
                ||
                e.player_pos.y < border_min.y => int outside_border;
                if (outside_border) {
                    e.die();
                }
            }
        }

        draw();

        // check end game condition
        0 => int num_alive;
        for (auto e : gs.players) {
            if (!e.disabled) {
                num_alive++;
                e @=> winner;
            }
        }
        
        // if (num_alive <= 1) return start_room;
        // else return null;

        return null;
    }

    fun void draw() {
        // draw border
        g.pushColor(Color.WHITE);
        g.box(border_min, border_max);
        g.popColor();

        // draw players
        for (int i; i < gs.num_players; i++) {
            gs.players[i] @=> Entity e;
            gs.players[i].draw(); // draw player

            // draw trail
            g.pushColor(gs.players[i].color);

            g.line(e.player_pos_history, e.player_pos_history_read_idx, e.frame_alive_count);
            spork ~ FX.booster(
                e.player_pos,
                e.color,
                e.last_speed * gs.player_scale * 2 // scale trail size by speed
            );

            g.popColor();
        }
    }

    // ========================== Internal ==============================

    // TODO: rather than spork this, do it in the match_room update loop
    // so we guarantee frame-accuracy
    // fun void record() {
    //     adc => WvOut w => blackhole;
    //     gs.replay_audio_filename => w.wavFilename;

    //     // open for write (default mode: ASCII)
    //     replay_ks.open( gs.replay_ks_filename, FileIO.WRITE );
    //     replay_mic_gain.open( gs.replay_gain_filename, FileIO.WRITE );

    //     // test
    //     if( !replay_ks.good() ) {
    //         cherr <= "can't open file for writing..." <= replay_ks.filename() <= IO.newline();
    //         return;
    //     }

    //     if( !replay_mic_gain.good() ) {
    //         cherr <= "can't open file for writing..." <= replay_mic_gain.filename() <= IO.newline();
    //         return;
    //     }

    //     // print
    //     while (match_state == Match_Running) {
    //         GG.nextFrame() => now;  // THIS GOES FIRST
    //         // write all player keys being held this frame
    //         GWindow.keys() @=> int keys_held[];
    //         for (auto key : keys_held) {
    //             replay_ks <= key <= " ";
    //         }
    //         replay_ks <= IO.newline();
    //         replay_mic_gain <= gs.mic_volume <= IO.newline();
    //     }

    //     // close the thing
    //     replay_ks.close();
    //     replay_mic_gain.close();
    //     w.closeFile();
    //     adc =< w;
    // }

    // initializes player data *excluding* trail history
    fun void _initPlayers() {
        // init player entities
        for (int i; i < gs.players.size(); i++) {
            gs.players[i] @=> Entity@ e;
            i => e.player_id;
            gs.player_colors[i] => e.color;
            gs.player_spawns[i] => e.player_pos;
            gs.player_spawns[i] => e.player_prev_pos;
            M.DEG2RAD * gs.player_rots_deg[i] => e.player_rot;
            0 => e.player_pos_history_write_idx;
            0 => e.player_pos_history_read_idx;
            0 => e.frame_alive_count;
            0 => e.last_speed;
            (i >= gs.num_players) => e.disabled;
        }
    }

    fun void _checkAllPlayersAgainstSegment(vec2 p0, vec2 p1) {
        for (auto e : gs.players) {
            if (e.disabled) continue;

            if (M.intersect(
                e.player_prev_pos, e.player_pos, 
                p0, p1
            )) {
                e.die();
            }
        }
    }

    // call AFTER all player updates have finished
    fun void processTrailCollisions() {
        gs.players[0].player_pos_history.size() => int history_size;
        for (int i; i < gs.num_players; i++) {
            gs.players[i] @=> Entity this_player;

            // don't collide with a newly spawned player
            if (this_player.frame_alive_count < history_size) continue;

            this_player.player_pos_history_read_idx => int start_idx;
            start_idx - 3 => int end_idx; // ignore most recent trail segments to avoid self-collision
            if (end_idx < 0) history_size +=> end_idx;

            if (start_idx > end_idx) {
                for (start_idx => int i; i < history_size - 1; i++) {
                    _checkAllPlayersAgainstSegment(
                        this_player.player_pos_history[i], this_player.player_pos_history[i + 1]
                    );
                }
                for (0 => int i; i <= end_idx; i++) {
                    _checkAllPlayersAgainstSegment(
                        this_player.player_pos_history[i-1], this_player.player_pos_history[i]
                    );
                }
            } else {
                for (start_idx => int i; i < end_idx; i++) {
                    _checkAllPlayersAgainstSegment(
                        this_player.player_pos_history[i], this_player.player_pos_history[i + 1]
                    );
                }
            }
        }
    }
}


class PlatformRoom extends Room {
    vec2 platform_endpoints[4*2];

    fun void enter() {
        // TODO: position players, reset Entity state etc

        // position gaps in NDC
        g.NDCToWorldPos(-7.0/7, 0) => platform_endpoints[0];
        g.NDCToWorldPos(-5.0/7, 0) => platform_endpoints[1];
        g.NDCToWorldPos(-3.0/7, 0) => platform_endpoints[2];
        g.NDCToWorldPos(-1.0/7, 0) => platform_endpoints[3];
        g.NDCToWorldPos(1.0/7, 0) => platform_endpoints[4];
        g.NDCToWorldPos(3.0/7, 0) => platform_endpoints[5];
        g.NDCToWorldPos(5.0/7, 0) => platform_endpoints[6];
        g.NDCToWorldPos(7.0/7, 0) => platform_endpoints[7];
    }

    fun void leave() {}
    fun Room update(float dt) { 
        // for (vec2 platform : platform_endpoints) {
        <<< "start" >>>;
        for (int i; i < platform_endpoints.size(); 2 +=> i) {
            g.line(platform_endpoints[i],  platform_endpoints[i + 1]);
            // g.line(@(platform_endpoints[i].x, 0),  @(platform_endpoints[i].y, 0));
        }
        <<< "end" >>>;

        g.circleFilled(g.NDCToWorldPos(0, 0), 1.0);
        g.circleFilled(g.NDCToWorldPos(-1.0, 0), 1.0);
        g.circleFilled(g.NDCToWorldPos(1.0, 0), 1.0);
        g.circleFilled(g.NDCToWorldPos(0.0, 1.0), 1.0);
        g.circleFilled(g.NDCToWorldPos(0.0, -1.0), 1.0);

        return null;
    } 
}

class FX {
    // spawns an explosion of lines going in random directinos that gradually shorten
    fun static void explode(vec2 pos, dur max_dur, vec3 color) {
        // params
        Math.random2(8, 16) => int num; // number of lines

        vec2 positions[num];
        vec2 dir[num];
        float lengths[num];
        dur durations[num];
        float velocities[num];

        // init
        for (int i; i < num; i++) {
            pos => positions[i];
            M.randomDir() => dir[i];
            Math.random2f(.1, .2) => lengths[i];
            Math.random2f(.3, max_dur / second)::second => durations[i];
            Math.random2f(.01, .02) => velocities[i];
        }

        dur elapsed_time;
        while (elapsed_time < max_dur) {
            GG.nextFrame() => now;
            GG.dt()::second +=> elapsed_time;

            for (int i; i < num; i++) {
                // update line 
                (elapsed_time) / durations[i] => float t;
                // if animation still in progress for this line
                if (t < 1) {
                    // update position
                    velocities[i] * dir[i] +=> positions[i];
                    // shrink lengths linearly down to 0
                    lengths[i] * (1 - t) => float len;
                    // draw
                    g.line(positions[i], positions[i] + len * dir[i], color);
                }
            }
        }
    }

    fun static void booster(vec2 pos, vec3 color, float radius_scale) {
        radius_scale * Math.random2f(.04, .046) => float init_radius;
        init_radius * 8 * .5::second => dur effect_dur; // time to dissolve

        Math.random2f(0.8, 1.0) * color => vec3 init_color;

        dur elapsed_time;
        while (elapsed_time < effect_dur) {
            GG.nextFrame() => now;
            GG.dt()::second +=> elapsed_time;
            1.0 - elapsed_time / effect_dur => float t;

            g.circle(pos, init_radius * t, 1.0, t * t * init_color);
        }
    }
}

// init
StartRoom start_room;
PlatformRoom platform_room;
gs.enterRoom(platform_room);

// gameloop
while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    // sound update (used by all minigames)
    Math.max(
        0,
        Math.pow(s.env_follower.last(), s.env_exp.val()) - s.env_low_cut.val()
    ) => gs.mic_volume;

    // draw sound meter (assumimg all games share same sound meter)
    gs.progressBar(gs.mic_volume, @(0, .9), .3, .05, Color.WHITE);

    // room update
    gs.room.update(dt) @=> Room new_room;
    if (new_room != null) gs.enterRoom(new_room);

    // { // UI
    //     if (UI.begin("test")) {
    //         // TODO: UI Library ideas
    //         // - show waveform history of a UGen
    //         // - show amplitude plot of a UGen last
    //         gs.mic_volume => env_pol_last.val;
    //         UI.slider("Mic Low Cut", env_low_cut, 0.00, 1.);
    //         UI.slider("Mic Exponent", env_exp, 0.00, 1.);
    //         if (UI.slider("Mic Pole", env_pole_pos, 0.95, 1.)) env_pole_pos.val() => env_follower.pole;
    //         UI.slider("Mic Scaled Volume", env_pol_last, 0.00, 1.);

    //         UI.slider("Speed-Volume Scale", gs.player_speed_volume_scale, 1.00, 4.);
    //     }
    //     UI.end();
    // }
}
