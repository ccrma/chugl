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
GWindow.center();
// GWindow.maximize();
G2D g;
g.resolution(1920, 1080);
GText.defaultFont(me.dir() + "./assets/m5x7.ttf");

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

class Sampler extends LiSa {
    dur rec_end; // point when user stopped recording
    false => int recording;


    // lisa params
    16 => static int MAX_VOICES;
    15::ms => static dur RAMP_DUR;
    2::second => this.duration;
    this.loop0(false);
    this.rampDown(10::ms);
    this.maxVoices(MAX_VOICES);

    fun void oneshotShred(float rate) {
        this.getVoice() => int voice;
        Math.min(0.5 * (rec_end / second), RAMP_DUR / second)::second => dur ramp;
        T.assert(voice > -1, "not available voices in Sampler");

        this.rate( voice, rate );
        this.loop( voice, false );
        this.playPos( voice, 0::ms );

        this.rampUp( voice, ramp );
        rec_end - ramp => now;
        this.rampDown( voice, ramp );
        ramp => now;
    }

    fun void rec(int toggle) {
        toggle => recording;
        this.record(toggle);

        if (!toggle) this.recPos() => rec_end;
    }
}

// key mapping enum
0 => int Key_Left;
1 => int Key_Right;
2 => int Key_Action;

class Entity {
    // player entity
    int player_id; // 0, 1, 2, 3
    -1 => int gamepad_id;  // -1 means no gamepad connected
    int disabled; // right now doubling to mean dead OR not connected. if that proves to be a problem, can make an enum

    // basic physics stuff
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
    fun string keyString(int which) { 
        if (which == Key_Left) {
            return gs.key2string[gs.player_key_left[player_id]];
        } else if (which == Key_Right) {
            return gs.key2string[gs.player_key_right[player_id]];
        } else if (which == Key_Action) {
            return gs.key2string[gs.player_key_action[player_id]];
        }
        return "N/A";
    }

    fun int key(int which) {
        if (which == Key_Left) {
            return GWindow.key(gs.player_key_left[player_id]);
        } else if (which == Key_Right) {
            return GWindow.key(gs.player_key_right[player_id]);
        } else if (which == Key_Action) {
            return GWindow.key(gs.player_key_action[player_id]);
        }
        return false;
    }

    fun int keyDown(int which) {
        if (which == Key_Left) {
            return GWindow.keyDown(gs.player_key_left[player_id]);
        } else if (which == Key_Right) {
            return GWindow.keyDown(gs.player_key_right[player_id]);
        } else if (which == Key_Action) {
            return GWindow.keyDown(gs.player_key_action[player_id]);
        }
        return false;
    }

    fun int keyUp(int which) {
        if (which == Key_Left) {
            return GWindow.keyUp(gs.player_key_left[player_id]);
        } else if (which == Key_Right) {
            return GWindow.keyUp(gs.player_key_right[player_id]);
        } else if (which == Key_Action) {
            return GWindow.keyUp(gs.player_key_action[player_id]);
        }
        return false;
    }

    fun int hasGamepad() {
        return Gamepad.available(gamepad_id);
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
        // register placement
        gs.registerPlacement(this);
        // sfx!
        gs.playSfx(this, gs.SfxType_Death, Math.random2f(.5, 1.2));
    }

    fun void draw() {
        if (!disabled) {
            g.pushColor(color);
            g.polygonFilled(
                player_pos, player_rot, gs.player_vertices, 0.0
            );
            g.popColor();
        }
    }

    fun void draw(vec3 c) {
        g.pushColor(c);
        g.polygonFilled(
            player_pos, player_rot, gs.player_vertices, 0.0
        );
        g.popColor();
    }
}

class Room {
    "default room" => string room_name;
    fun void ui() {}
    fun void enter() {}
    fun void leave() {}
    fun Room update(float dt) { return null; } // returns true if end state is met
}

class GameState {
    string room_names[0];
    Room rooms[0];
    Room curr_room;
    UI_Int curr_room_idx(0);

    [
        GWindow.Key_Right,  // p1
        GWindow.Key_D,      // p2
        GWindow.Key_P,
        GWindow.Key_M,
    ] @=> int player_key_right[];

    [
        GWindow.Key_Left, 
        GWindow.Key_A, 
        GWindow.Key_I, 
        GWindow.Key_B, 
    ] @=> int player_key_left[];

    [
        GWindow.KEY_DOWN, 
        GWindow.KEY_S, 
        GWindow.KEY_O, 
        GWindow.KEY_N, 
    ] @=> int player_key_action[];

    string key2string[GWindow.KEY_LAST];

    "<right>" => key2string[GWindow.Key_Right];
    "d" => key2string[GWindow.Key_D];
    "p" => key2string[GWindow.Key_P];
    "m" => key2string[GWindow.Key_M];
    "<left>" => key2string[GWindow.Key_Left];
    "a" => key2string[GWindow.Key_A];
    "i" => key2string[GWindow.Key_I];
    "b" => key2string[GWindow.Key_B];
    "<down>" => key2string[GWindow.KEY_DOWN];
    "s" => key2string[GWindow.KEY_S];
    "o" => key2string[GWindow.KEY_O];
    "n" => key2string[GWindow.KEY_N];

    .25 => float player_scale;
    [ 
        player_scale * @(1 / Math.sqrt(3), 0),
        player_scale * @(-1 / (2 * Math.sqrt(3)), .5),
        player_scale * @(-1 / (2 * Math.sqrt(3)), -.5),
    ] @=> vec2 player_vertices[];

    // calculate bbox
    M.bbox(player_vertices) => vec4 player_bbox;
    @(player_bbox.x, player_bbox.y) => vec2 player_bbox_min;
    @(player_bbox.z, player_bbox.w) => vec2 player_bbox_max;
    .5 * (player_bbox_max - player_bbox_min) => vec2 player_bbox_hw_hh;

    4 => static int MAX_PLAYERS;
    2 => int num_players;
    Entity players[MAX_PLAYERS];


    // placement =========================
    Entity@ end_game_placements[MAX_PLAYERS]; // player idx 
    num_players => int placement_idx; // what place the next player who dies will be
    for (int i; i < players.size(); i++) {
        players[i] @=> end_game_placements[i];
    }

    fun void registerPlacement(Entity player) {
        <<< "registering player ", player.player_id, "to idx", placement_idx, "at time", now/second >>>;
        player @=> end_game_placements[placement_idx];
        placement_idx--;
    }

    // audio =========================
    float mic_volume; // should be updated and save at start of each frame
    
    [
        "Victory",
        "Action",
        "Death",
        "Defeat",
    ] @=> static string sfx_list[];

    // sfx enum. keep in sync with sfx_list
    0 => int SfxType_Victory;
    1 => int SfxType_Action;
    2 => int SfxType_Death;
    3 => int SfxType_Defeat;
    4 => int SfxType_Count;
    T.assert(sfx_list.size() == SfxType_Count, "SFXList and sfxType_ enum not in sync");

    adc => PoleZero rec_input;
    .99 => rec_input.blockZero; // block ultra low freqs
    Sampler samplers[MAX_PLAYERS * sfx_list.size()];
    Gain player_gains[MAX_PLAYERS];

    // audio analysis ============================
    256 => int WINDOW_SIZE; // TODO experiment with different sizes
    Flip sampler_waveform_accum[MAX_PLAYERS];
    Windowing.hann(WINDOW_SIZE) @=> float hann_window[];
    float sampler_waveform[MAX_PLAYERS][0];

    for (int i; i < MAX_PLAYERS; ++i) {
        WINDOW_SIZE => sampler_waveform_accum[i].size;
        player_gains[i] => dac;  // sound out
        player_gains[i] => sampler_waveform_accum[i] => blackhole; // analysis

        for (int sfx_idx; sfx_idx < SfxType_Count; sfx_idx++) {
            samplers[i * SfxType_Count + sfx_idx] @=> Sampler sampler;
            rec_input => sampler => player_gains[i];
        }
    }

    fun Sampler getSampler(Entity player, int sfx_type) {
        return samplers[player.player_id * SfxType_Count + sfx_type];
    }

    fun void playSfx(Entity player, int sfx_type, float rate) {
        spork ~ getSampler(player, sfx_type).oneshotShred(rate);
    }

    fun void updateAudioAnalysis() {
        // only call this once per frame since it's just used for graphical updates
        for (int i; i < MAX_PLAYERS; i++) {
            sampler_waveform_accum[i].upchuck();
            sampler_waveform_accum[i].output( sampler_waveform[i] );

            // taper
            for (int s; s < sampler_waveform[i].size(); s++) {
                hann_window[s] *=> sampler_waveform[i][s];
            }
        }

    }

    // config constants =======================
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

    fun void addRoom(Room r) {
        rooms << r;
        room_names << r.room_name;
    }

    fun void enterRoom(Room new_room) {
        <<< "leaving room", curr_room.room_name >>>;
        curr_room.leave();

        // reset the placement tracker
        num_players - 1 => placement_idx;

        // reset player state
        for (int i; i < num_players; ++i) {
            false => gs.players[i].disabled;
            0 => gs.players[i].player_rot;
            @(0, 0) => gs.players[i].player_pos;
        }
        for (num_players => int i; i < MAX_PLAYERS; ++i) {
            true => gs.players[i].disabled;
        }

        <<< "entering room", new_room.room_name >>>;
        new_room @=> curr_room;
        new_room.enter();

        // validate the room is registered
        for (int i; i < rooms.size(); i++) {
            if (rooms[i] == new_room) i => curr_room_idx.val;
        }
        T.assert(rooms[curr_room_idx.val()] == curr_room, "room " + new_room.room_name + " not registered");
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
    "start room" => room_name;

    UI_Float ui_margin(.2);
    UI_Int waveform_display_samples(128);

    int sfx_selection[gs.MAX_PLAYERS];

    fun void ui() {
        UI.slider("margin", ui_margin, 0, 1.0);
        UI.slider("waveform samples", waveform_display_samples, 36, 1024);
    }
    fun void enter() {
        sfx_selection.zero();
    }
    fun Room update(float dt) {
        g.screenSize() => vec2 screen_size;

        screen_size.x -  2 * ui_margin.val() => float menu_total_width;
        screen_size.y -  2* ui_margin.val() => float menu_total_height;
        (menu_total_height - (gs.MAX_PLAYERS - 1) * ui_margin.val()) / gs.MAX_PLAYERS => float character_box_height;

        4.0 => float character_box_width;
        screen_size.x - character_box_width - 3 * ui_margin.val() => float audio_box_width;
        -screen_size.x/2 + character_box_width + 2*ui_margin.val() => float audio_box_start_x;
        audio_box_start_x + 0.5*audio_box_width => float audio_box_center_x;
        character_box_height / gs.sfx_list.size() => float sfx_label_dy;

        // choose # players
        // @bug: not handling case where >4 controllers connect
        // and gamepad ids exceed 3.
        if (GWindow.keyDown(GWindow.KEY_2)) 2 => gs.num_players;
        if (GWindow.keyDown(GWindow.KEY_3)) 3 => gs.num_players;
        if (GWindow.keyDown(GWindow.KEY_4)) 4 => gs.num_players;

        // draw num players
        g.text("Number of Players: " + gs.num_players, @(Math.cos(now/second), Math.sin(now/second)));

        // draw players and allow movement 
        for (int i; i < gs.num_players; ++i) {
            gs.players[i] @=> Entity e;
            float delta_rot;
            false => int switched_sfx_selection;

            gs.getSampler(e, sfx_selection[i]) @=> Sampler prev_sampler;

            // input (switching stops rec and playback on prev)
            if (e.keyDown(Key_Left)) {
                1 -=> sfx_selection[i];
                prev_sampler.rampDown(Sampler.RAMP_DUR);
                prev_sampler.rec(false);
                true => switched_sfx_selection;
            }
            if (e.keyDown(Key_Right)) {
                1 +=> sfx_selection[i];
                prev_sampler.rampDown(Sampler.RAMP_DUR);
                prev_sampler.rec(false);
                true => switched_sfx_selection;
            }

            if (sfx_selection[i] >= gs.sfx_list.size()) {
                gs.sfx_list.size() %=> sfx_selection[i];
            } else if (sfx_selection[i] < 0) {
                gs.sfx_list.size() - 1 => sfx_selection[i];
            }


            gs.getSampler(e, sfx_selection[i]) @=> Sampler sampler;

            if (switched_sfx_selection) {
                0::ms => sampler.playPos;
                sampler.rampUp(Sampler.RAMP_DUR);
            }

            if (e.keyDown(Key_Action)) {
                sampler.rampDown(Sampler.RAMP_DUR);
                sampler.clear();
                0::ms => sampler.recPos;
                sampler.rec(true);
            }
            if (e.keyUp(Key_Action)) {
                sampler.rec(false);
                0::ms => sampler.playPos;
                sampler.rampUp(Sampler.RAMP_DUR);
            }

            // draw
            // e.draw();
        }

        // draw box outlines
        0.5 * screen_size.y - ui_margin.val() => float cursor_y;
        for (int i; i < gs.num_players; ++i) { 
            gs.players[i] @=> Entity player;
            @(
                -screen_size.x/2 + ui_margin.val() + 0.5 * character_box_width,
                cursor_y - 0.5 * character_box_height
            ) => vec2 center;
            center => gs.players[i].player_pos;

            gs.getSampler(player, sfx_selection[i]) @=> Sampler sampler;

            g.box(center, character_box_width, character_box_height);

            // draw rec instructions
            g.pushTextMaxWidth(character_box_width * 0.7);
            g.text(
                "hold " + player.keyString(Key_Action) + " to record",
                @(center.x - .15 * character_box_width, center.y),
                .4
            );
            g.popTextMaxWidth();


            // draw voice SFX options
            center.y + character_box_height * 0.5 - sfx_label_dy * 0.5 => float text_y;
            center.x + character_box_width * 0.35 => float text_x;
            for (int sfx_idx; sfx_idx < gs.sfx_list.size(); ++sfx_idx) {
                g.text(gs.sfx_list[sfx_idx], @(text_x, text_y), .4);

                // draw selection box
                @(character_box_width * .3, sfx_label_dy) => vec2 selection_box_sz;
                Math.remap(
                    Math.sin((now/second) * 6),
                    -1, 1,
                    .86, 1
                ) *=> selection_box_sz;
                .05 * Math.sin((now/second) * 3) => float box_rot;
                if (sfx_idx == sfx_selection[i]) {
                    // TODO: if recording, fill box. else just outline
                    if (sampler.recording) {
                        g.boxFilled(@(text_x, text_y), box_rot, selection_box_sz.x, selection_box_sz.y, gs.players[i].color);
                    } else g.box(@(text_x, text_y), selection_box_sz.x, selection_box_sz.y, gs.players[i].color);
                }

                // move cursor
                sfx_label_dy -=> text_y;
            }

            // draw waveform widget
            g.box(@(audio_box_center_x, center.y), audio_box_width, character_box_height);

            // draw waveform
            sampler.duration() / waveform_display_samples.val() => dur interval;
            audio_box_width / (waveform_display_samples.val()-1) => float waveform_dx;
            // g.pushColor(player.color);
            for (1 => int w; w < waveform_display_samples.val(); w++) {
                @(
                    audio_box_start_x + (w - 1) * waveform_dx,
                    center.y + character_box_height * sampler.valueAt(interval * (w - 1))
                ) => vec2 start;
                @(
                    audio_box_start_x + w * waveform_dx,
                    center.y + character_box_height * sampler.valueAt(interval * w)
                ) => vec2 end;
                g.line(start, end);
            }

            // draw record head
            sampler.recPos() / sampler.duration() => float rec_progress;
            if (rec_progress > 0) {
                g.line(
                    @(
                        audio_box_start_x + rec_progress * audio_box_width,
                        center.y - character_box_height * 0.5
                    ),
                    @(
                        audio_box_start_x + rec_progress * audio_box_width,
                        center.y + character_box_height * 0.5
                    ),
                    player.color
                );
            }

            // draw play head
            if (sampler.playing(0)) {
                sampler.playPos() / sampler.duration() => float play_progress;
                g.line(
                    @(
                        audio_box_start_x + play_progress * audio_box_width,
                        center.y - character_box_height * 0.5
                    ),
                    @(
                        audio_box_start_x + play_progress * audio_box_width,
                        center.y + character_box_height * 0.5
                    )
                );
            }

            // g.popColor();


            (ui_margin.val() + character_box_height) -=> cursor_y;
        }


        Gamepad.available() @=> int gamepads[];

        // assign gamepads and positions to players
        for (int i; i < Math.min(gamepads.size(), gs.MAX_PLAYERS); ++i) {
            gamepads[i] => gs.players[i].gamepad_id;
            gs.player_spawns[i] => gs.players[i].player_pos;
        }
        // remaining players don't have a gamepad assigned
        for (gamepads.size() => int i; i < gs.MAX_PLAYERS; ++i) {
            -1 => gs.players[i].gamepad_id;
        }


        // stage selection
        return null;
    }
}

class SnakeRoom extends Room {
    "snake room" => room_name;
    .5 => float BORDER_PADDING; // padding in worldspace units of sides/top

    // match state enum
    // 0 => static int Match_Select;
    // 1 => static int Match_Running;
    // 2 => static int Match_Replay;
    // Match_Select => int match_state;

    UI_Float player_base_speed(2.0);
    UI_Float player_speed_volume_scale(12); // how much to scale speed with mic volume 

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

    // replay state
    // FileIO replay_ks;
    // FileIO replay_mic_gain;
    // int replay_current_keys[0];
    // StringTokenizer strtok;
    // float current_mic_gain;

    fun void ui() {
        UI.slider("base speed", player_base_speed, 00, 10.);
        UI.slider("speed-volume scale", player_speed_volume_scale, 1.00, 16.);
    }

    fun void enter() {
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
            player_base_speed.val() + this.player_speed_volume_scale.val() * gs.mic_volume => float speed;
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

            (
                e.player_pos.x > border_max.x || e.player_pos.y > border_max.y
                || e.player_pos.x < border_min.x || e.player_pos.y < border_min.y
            ) => int outside_border;
            if (outside_border) {
                e.die();
            }
        }

        _draw();

        // check end game condition
        0 => int num_alive;
        Entity@ possible_winner;
        for (auto e : gs.players) {
            if (!e.disabled) {
                num_alive++;
                e @=> possible_winner;
            }
        }
        
        if (num_alive <= 1) {
            if (possible_winner != null) gs.registerPlacement(possible_winner);
            return placement_room;
        }
        else return null;
    }

    fun void _draw() {
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

            if (!e.disabled) {
                spork ~ FX.booster(
                    e.player_pos,
                    e.color,
                    e.last_speed * gs.player_scale * 2 // scale trail size by speed
                );
            }

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


class BlackholeRoom extends Room {
    "blackhole room" => room_name;
    vec2 platform_endpoints[4*2];

    // player status enum
    0 => static int Status_Disconnected;
    1 => static int Status_Alive;
    2 => static int Status_Dead;

    int player_status[gs.MAX_PLAYERS];
    int player_on_ground[gs.MAX_PLAYERS];
    vec2 player_velocity[gs.MAX_PLAYERS];
    vec2 player_spawn[gs.MAX_PLAYERS];
    vec2 booster_direction[gs.MAX_PLAYERS];
    int player_blackhole_idx[gs.MAX_PLAYERS]; // idx of closest blackhole in range (-1 if none)

    // movement
    UI_Bool debug_draw(false);
    UI_Float initial_jump_speed(4.9);
    UI_Float walk_speed(18.0);
    UI_Float booster_rot_speed(Math.two_pi);

    // blackholes 
    float blackhole_spawn_time_sec[0]; // time in secs that the blackhole spawned
    vec2 blackhole_positions[0];
    UI_Float blackhole_meter_base_fill_rate(.03); // even if silent, meter still fills at this rate
    UI_Float blackhole_event_horizon_radius(.3); // here u dead
    UI_Float blackhole_radius(1.5); // can still get out if u yell 
    UI_Float blackhole_init_time_secs(3.0); // time it takes for blackhole to grow to max radius and begin sucking
    float blackhole_gravity; // (calculated from event horizon radius and booster force, i.e. force at event horizon == max booster force
    float blackhole_meter;  // when at 1.0, all players will spawn a blackhole at their location

    fun void _calcBlackholeGravity() {
        // 0.5 * walk_speed.val() * (blackhole_event_horizon_radius.val() * blackhole_event_horizon_radius.val()) => blackhole_gravity;
        0.5 * walk_speed.val() * (blackhole_event_horizon_radius.val()) => blackhole_gravity; // using inv radius, not inv radius squared
    }

    fun vec2 _blackholeForce(vec2 bpos, vec2 player_pos) {
        // debug draw line to closest
        bpos - player_pos => vec2 l;
        M.mag(l) => float r; // radius
        M.normalize(l) => vec2 n_hat; // direction vector
        // return (blackhole_gravity / (r*r) * n_hat); // inv squared
        return (blackhole_gravity / (r) * n_hat);  // inv
    }

    fun int _blackholeDoneSpawning(int bidx) {
        return (now/second) - blackhole_spawn_time_sec[bidx] >= blackhole_init_time_secs.val();
    }

    fun void ui() {
        UI.checkbox("debug draw", debug_draw);
        UI.slider("initial jump vel", initial_jump_speed, 0, 10);

        if (UI.slider("booster power", walk_speed, 1, 100)) _calcBlackholeGravity();

        UI.slider("rot speed", booster_rot_speed, 0, 10);

        UI.slider("blackhole event horizon radius", blackhole_event_horizon_radius, 0, 1);
        UI.slider("blackhole meter base fill rate", blackhole_meter_base_fill_rate, 0, .5);
        UI.progressBar(blackhole_meter, @(0, 20), "blackhole meter");
    }

    fun void enter() {
        <<< "BlacholeRoom enter()" >>>;
        // TODO: position players, reset Entity state etc
        _calcBlackholeGravity();

        // position gaps in NDC
        g.NDCToWorldPos(-7.0/7, 0) => platform_endpoints[0];
        g.NDCToWorldPos(-5.0/7, 0) => platform_endpoints[1];
        g.NDCToWorldPos(-3.0/7, 0) => platform_endpoints[2];
        g.NDCToWorldPos(-1.0/7, 0) => platform_endpoints[3];
        g.NDCToWorldPos(1.0/7, 0) => platform_endpoints[4];
        g.NDCToWorldPos(3.0/7, 0) => platform_endpoints[5];
        g.NDCToWorldPos(5.0/7, 0) => platform_endpoints[6];
        g.NDCToWorldPos(7.0/7, 0) => platform_endpoints[7];
        T.printArray(platform_endpoints);

        // calculate
        gs.player_bbox_max - gs.player_bbox_min => vec2 width_height;

        // init player state
        player_status.zero();
        for (int i; i < gs.num_players; ++i) {
            Status_Alive => player_status[i];
            0.5 * (platform_endpoints[2*i] + platform_endpoints[2*i+1]) + 
                @(0, width_height.y * .5) => player_spawn[i] => gs.players[i].player_pos;
            Math.pi / 2 => gs.players[i].player_rot;
            true => player_on_ground[i];
            @(0,0) => player_velocity[i];
            @(0,1) => booster_direction[i];
            -1 => player_blackhole_idx[i];
        }

        blackhole_spawn_time_sec.clear();
        blackhole_positions.clear();
        blackhole_spawn_time_sec << (now/second);
        blackhole_positions << @(0, -2);
        0 => blackhole_meter;
    }

    fun Room update(float dt) {
        0.5 * (gs.player_bbox_max - gs.player_bbox_min) => vec2 hw_hh;
        gs.mic_volume => float mic_volume;
        for (int idx; idx < gs.num_players; idx++) {
            if (player_status[idx] == Status_Disconnected) continue; // skip disconnected players
            if (player_status[idx] == Status_Dead) {
                // draw dead in event horizon
                gs.players[idx].draw(Color.GRAY);
                continue; 
            }

            gs.players[idx] @=> Entity player;
            vec2 acceleration;
            player.player_pos => player.player_prev_pos;

            { // rocket input
                Math.atan2(booster_direction[idx].y, booster_direction[idx].x) => float rot_rad;
                if (player.hasGamepad()) {
                    player.axis(Gamepad.AXIS_LEFT_X) => float x_axis;
                    if (x_axis > 0.5) booster_rot_speed.val() * dt +=> rot_rad;
                    if (x_axis < -0.5) booster_rot_speed.val() * dt -=> rot_rad;
                } else {
                    if (player.key(Key_Left))  booster_rot_speed.val() * dt +=> rot_rad;
                    if (player.key(Key_Right)) booster_rot_speed.val() * dt -=> rot_rad;
                }
                M.rot2vec(rot_rad) => booster_direction[idx];

                (gs.mic_volume) * walk_speed.val() * booster_direction[idx] +=> acceleration;
            }

            // check blackholes and death condition
            -1 => int closest_blackhole_idx;
            Math.FLOAT_MAX => float closest_blackhole_dist;
            for (int bidx; bidx < blackhole_positions.size(); bidx++) {
                (now/second) - blackhole_spawn_time_sec[bidx] >= blackhole_init_time_secs.val() => int blackhole_done_spawning;
                if (!blackhole_done_spawning) continue;
                
                // try having all blackholes influence 
                // _blackholeForce(
                //     blackhole_positions[bidx], player.player_pos
                // ) +=> acceleration;

                M.dist( player.player_pos, blackhole_positions[bidx]) => float blackhole_dist;
                // if (blackhole_dist < blackhole_radius.val() && blackhole_dist < closest_blackhole_dist) {
                if (blackhole_dist < closest_blackhole_dist) {
                    blackhole_dist => closest_blackhole_dist;
                    bidx => closest_blackhole_idx;
                }
            }
            closest_blackhole_idx => player_blackhole_idx[idx];


            // step
            if (closest_blackhole_idx >= 0) {
                // blackhole gravity
                _blackholeForce(
                    blackhole_positions[closest_blackhole_idx], player.player_pos
                ) +=> acceleration;
            }

            // clamp max accceleration
            // if (acceleration.magnitude() >= walk_speed.val()) {
            //     acceleration.normalize();
            //     walk_speed.val() *=> acceleration;
            // }

            // update vel
            acceleration * dt +=> player_velocity[idx];

            // clamp max velocity
            // if (player_velocity[idx].magnitude() >= walk_speed.val()) {
            //     player_velocity[idx].normalize();
            //     walk_speed.val() *=> player_velocity[idx];
            // }

            // update pos
            (dt * player_velocity[idx]) +=> player.player_pos;
            if (player.player_pos.y < 0) false => player_on_ground[idx]; // if below platform, not on ground
            else if (player.player_pos.y + hw_hh.y > .01) false => player_on_ground[idx];

            // platform collision
            for (int i; i < platform_endpoints.size(); 2 +=> i) {
                // we collide with a platform if the previous frame was above, and this frame is below
                M.overlap(
                    platform_endpoints[i].x, platform_endpoints[i+1].x,
                    player.player_prev_pos.x - hw_hh.x, player.player_prev_pos.x + hw_hh.x
                ) && (player.player_prev_pos.y - hw_hh.y >= 0) => int prev_frame_above;
                M.overlap(
                    platform_endpoints[i].x, platform_endpoints[i+1].x,
                    player.player_pos.x - hw_hh.x, player.player_pos.x + hw_hh.x
                ) && (player.player_pos.y - hw_hh.y <= 0) => int this_frame_below;

                if (prev_frame_above && this_frame_below) {
                    // collide with floor
                    true => player_on_ground[idx];
                    0 => player_velocity[idx].y;
                    hw_hh.y => player.player_pos.y;
                } 
            }

            // before screen wrap, check for collision with any event horizons
            for (int bidx; bidx < blackhole_positions.size(); bidx++) {
                if (!_blackholeDoneSpawning(bidx)) continue;

                M.isect(player.player_prev_pos, player.player_pos, blackhole_positions[bidx], blackhole_event_horizon_radius.val()) => vec2 isects;
                M.mag(player.player_pos - blackhole_positions[bidx]) < blackhole_event_horizon_radius.val() => int inside_event_horizon;

                // ccd
                if (isects.x > 0) {
                    true => inside_event_horizon;
                    M.lerp(isects.y, player.player_prev_pos, player.player_pos) => player.player_pos;
                }

                if (inside_event_horizon) {
                    Status_Dead => player_status[idx];
                    gs.registerPlacement(player);
                    break;
                }
            }

            // screen wrap logic
            g.world2ndc(player.player_pos) => vec2 ndc;
            if (Math.fabs(ndc.x) > 1.0) M.sign(ndc.x) * -2.0 +=> ndc.x;
            if (Math.fabs(ndc.y) > 1.0) M.sign(ndc.y) * -2.0 +=> ndc.y;
            g.NDCToWorldPos(ndc) => player.player_pos;

            // draw player
            player.draw();
            spork ~ FX.booster(
                player.player_pos - gs.player_scale * booster_direction[idx],
                player.color,
                gs.mic_volume * walk_speed.val() * .6
            );
            g.circle(player.player_pos - gs.player_scale * booster_direction[idx], .02, player.color);

            if (debug_draw.val()) {
                // draw bbox
                g.box(player.player_pos, 2 * hw_hh.x, 2 * hw_hh.y, 0);

                // draw acceleration vector
                g.line(player.player_pos, player.player_pos + acceleration, Color.RED);

                // draw velocity vector
                g.line(player.player_pos, player.player_pos + player_velocity[idx], Color.GREEN);

                // draw line to nearest blackhole
                if (closest_blackhole_idx >= 0) 
                    g.line(player.player_pos, blackhole_positions[closest_blackhole_idx]);
            }
        }

        // draw platforms
        for (int pfrom_idx; 2*pfrom_idx < platform_endpoints.size(); ++pfrom_idx) {
            g.line(platform_endpoints[2*pfrom_idx], platform_endpoints[2*pfrom_idx+1], gs.players[pfrom_idx].color);
        }

        // blackhole spawns
        (blackhole_meter_base_fill_rate.val() + mic_volume) * dt +=> blackhole_meter;
        if (blackhole_meter > 1.0) {
            1.0 -=> blackhole_meter;
            // spawn blackhole!
            for (int i; i < player_status.size(); i++) {
                if (player_status[i] == Status_Alive) {
                    blackhole_positions << gs.players[i].player_pos;
                    blackhole_spawn_time_sec << now / second; 
                }
            }
        }

        // draw blackholes and check player positions
        for (int bidx; bidx < blackhole_positions.size(); ++bidx) {
            Math.clampf(
                (now/second - blackhole_spawn_time_sec[bidx]) / blackhole_init_time_secs.val(),
                0.0,
                1.0
            ) => float spawn_progress;
            g.circleDotted(
                blackhole_positions[bidx], spawn_progress * blackhole_radius.val(), now/second, M.lerp(spawn_progress, Color.BLACK, Color.WHITE)
            );

            g.circleDotted(
                blackhole_positions[bidx], spawn_progress * blackhole_event_horizon_radius.val(), -(now/second), Color.RED
            );
        }

        // end game logic
        0 => int num_alive;
        Entity@ last_alive;
        for (int idx; idx < gs.num_players; idx++) {
            if (player_status[idx] != Status_Alive) continue;
            num_alive++;
            gs.players[idx] @=> last_alive;
        }
        if (num_alive <= 1) {
            if (last_alive != null) gs.registerPlacement(last_alive);
            return placement_room;
        } else return null;
    } 
}


class FlappyBirdRoom extends Room 
{
    "flappy bird room" => room_name;

    // sim params
    UI_Float gravity(2048);
    UI_Float jump_vel(8);

    UI_Float pipe_vertical_gap_base(1.08);
    UI_Float pipe_spawn_spacing_width(4.2); // x distance in world space between pipes
    UI_Float pipe_gap_range(3); // +- where the gap can occur
    UI_Float pipe_velocity(1.6);
    UI_Float pipe_volume_factor(18);
    @(.45, 5.0) => vec2 pipe_hw_hh;
    float pipe_spawn_timer_sec;

    // pipe
    vec2 pipe_gap_pos[0];

    // player params
    [
        g.ndc2world(-.9, .2),
        g.ndc2world(-.9, .4),
        g.ndc2world(-.9, .6),
        g.ndc2world(-.9, .8),
    ] @=> vec2 player_spawns[];
    vec2 player_velocity[gs.MAX_PLAYERS];
    int player_pressed_space[gs.MAX_PLAYERS];

    fun void ui() {
        UI.slider("gravity", gravity, 0, 1000);
        UI.slider("jump vel", jump_vel, 0, 10);
        
        UI.slider("pipe_vertical_gap_half", pipe_vertical_gap_base, 0, 10);
        UI.slider("pipe_spawn_spacing_width", pipe_spawn_spacing_width, .1, 10);
        UI.slider("pipe_gap_range", pipe_gap_range, 0, 10);
        UI.slider("pipe_velocity", pipe_velocity, 0, 10);
        UI.slider("pipe_volume_factor", pipe_volume_factor, 0, 30);
        // UI.slider("pipe hwidth hheight", pipe_hw_hh, 0, 10
    }

    fun void enter() {
        2.0 => pipe_spawn_timer_sec;
        pipe_gap_pos.clear();

        player_velocity.zero();
        player_pressed_space.zero();
        for (int i; i < gs.num_players; ++i) {
            player_spawns[i] @=> gs.players[i].player_pos;
            false => gs.players[i].disabled;
        }
    }

    fun void leave() {
    }

    fun Room update(float dt) { 
        0.5 * g.screenSize() => vec2 screen_hw_hh;
        gs.mic_volume => float mic_volume;


        // pipe movement+spawn logic
        pipe_vertical_gap_base.val() + (mic_volume * .10*pipe_volume_factor.val())
            => float pipe_vertical_gap; // scale gap with vol
        {
            screen_hw_hh.x + pipe_hw_hh.x => float pipe_spawn_x;
            if (
                pipe_gap_pos.size() == 0 || 
                (pipe_spawn_x - pipe_gap_pos[-1].x) > pipe_spawn_spacing_width.val()) 
            {
                // determine 
                pipe_gap_pos << @(
                    pipe_spawn_x,
                    Math.random2f(-pipe_gap_range.val(), pipe_gap_range.val())
                );
            }

            // remove first if offscreen
            if (
                pipe_gap_pos.size() > 0 && 
                pipe_gap_pos[0].x < -screen_hw_hh.x - pipe_hw_hh.x
            ) {
                pipe_gap_pos.popFront();
            }

            // slide pipes
            for (int pipe_idx; pipe_idx < pipe_gap_pos.size(); ++pipe_idx) {
                (pipe_velocity.val() + mic_volume * pipe_volume_factor.val()) * dt -=> pipe_gap_pos[pipe_idx].x;
            }
        }

        Entity@ last_alive;
        0 => int num_alive;
        for (int i; i < gs.num_players; ++i) {
            gs.players[i] @=> Entity p;
            if (p.disabled) continue;
            num_alive++;
            p @=> last_alive;
            vec2 acc;

            // input
            if (p.keyDown(Key_Left) || p.keyDown(Key_Right) || p.keyDown(Key_Action)) {
                jump_vel.val() => player_velocity[i].y;
                true => player_pressed_space[i];

                gs.playSfx(p, gs.SfxType_Action, 1.0 + mic_volume);
            }

            // physics (forward euler)
            if (player_pressed_space[i])
                (gravity.val() * dt * @(0, -1)) +=> acc;

            acc * dt +=> player_velocity[i];
            player_velocity[i] * dt +=> p.player_pos;

            // clamp pos to screen boundaries
            // gs.bbox_max
            if (p.player_pos.y - gs.player_bbox_hw_hh.y < -screen_hw_hh.y) {
                // -screen_hw_hh.y + gs.player_bbox_hw_hh.y => p.player_pos.y;
                // 0 => player_velocity[i].y;
                p.die();
                continue;
            } 
            else if (p.player_pos.y + gs.player_bbox_hw_hh.y > screen_hw_hh.y) {
                screen_hw_hh.y - gs.player_bbox_hw_hh.y => p.player_pos.y;
                0 => player_velocity[i].y;
            }

            // map player vel to rotation
            Math.max(
                Math.remap(
                    player_velocity[i].y,
                    jump_vel.val(), -jump_vel.val(),
                    Math.pi/3, -Math.pi/3
                ),
                -Math.pi/3
            ) => p.player_rot;

            // debug rot
            g.line(p.player_pos, p.player_pos + .3*M.rot2vec(p.player_rot));

            p.draw();
        }

        // draw pipes and collision detect
        for (auto pos : pipe_gap_pos) {
            pos + @(0, pipe_vertical_gap + pipe_hw_hh.y) => vec2 top_pipe_center;
            pos - @(0, pipe_vertical_gap + pipe_hw_hh.y) => vec2 bot_pipe_center;
            // top pipe
            g.boxFilled(
                top_pipe_center,
                0,
                pipe_hw_hh.x * 2,
                pipe_hw_hh.y * 2,
                Color.DARKGREEN
            );
            // bot pipe
            g.boxFilled(
                bot_pipe_center,
                0,
                pipe_hw_hh.x * 2,
                pipe_hw_hh.y * 2,
                Color.DARKGREEN
            );

            for (int player_idx; player_idx < gs.num_players; player_idx++) {
                gs.players[player_idx] @=> Entity p;
                if (p.disabled) continue;
                if (
                    M.aabbIsect(
                        p.player_pos, gs.player_bbox_hw_hh,
                        top_pipe_center, pipe_hw_hh
                    )
                    ||
                    M.aabbIsect(
                        p.player_pos, gs.player_bbox_hw_hh,
                        bot_pipe_center, pipe_hw_hh
                    )
                ) {
                    p.die();
                }
            }
        }

        // determine winner
        if (num_alive <= 1) {
            if (last_alive != null) gs.registerPlacement(last_alive);
            return placement_room;
        } else return null;
    }
}

class HotPotatoRoom extends Room
{
    "hot potato room" => room_name;

    // movement params
    UI_Float drive_turn_speed(1.0);
    UI_Float drive_move_speed(1.0);


    fun void enter() {
        false => gs.players[0].disabled;
        @(0,0) => gs.players[0].player_pos;
        0 => gs.players[0].player_rot;
    }

    fun void leave() {}
    fun void ui() {
        UI.slider("turn rate", drive_turn_speed, 0, 10);
        UI.slider("drive speed", drive_move_speed, 0, 10);
    }

    fun Room update(float dt) {
        gs.players[0] @=> Entity p;

        if (p.key(Key_Left)) {
            dt * drive_turn_speed.val() +=> p.player_rot;
        }
        if (p.key(Key_Right)) {
            dt * drive_turn_speed.val() -=> p.player_rot;
        }
        if (p.key(Key_Action)) {
            dt * drive_move_speed.val() * M.rot2vec(p.player_rot) +=> p.player_pos;
        }

        p.draw();


        return null; 
    } 
}

class PlacementsRoom extends Room
{
    "placements room" =>  room_name;
    UI_Float margin(0.1);

    UI_Int num_players(1); // for debugging podium rendering

    UI_Float podium_spacing_x(1.0);
    UI_Float podium_width(1.0);
    UI_Float podium_height(4.8);

    UI_Float firework_period_secs(.5);

    // TODO add slider(vec2)
    // UI_Float podium_group_cx(1.0);

    fun void ui() {
        if (UI.slider("num players", num_players, 2, gs.MAX_PLAYERS)) {
            for (int i; i < num_players.val(); ++i) {
            }
        }
        UI.slider("margin", margin, 0.0, 1.0);
        UI.slider("spacing X", podium_spacing_x, 0.0, 1.0);
        UI.slider("podium_width", podium_width, 0.0, 10.0);
        UI.slider("podium_height", podium_height, 0.0, 10.0);

        UI.slider("firework period sec", firework_period_secs, 0.1, 5.0);
    }

    fun void enter() {
        gs.num_players => num_players.val;
        spork ~ fireworkShred();

        for (auto p : gs.players) {
            p.player_id == gs.end_game_placements[0].player_id => int is_winner;
            gs.getSampler(p, is_winner ? gs.SfxType_Victory : gs.SfxType_Defeat) @=> Sampler sampler;
            sampler.loop0(true);
            sampler.play(true);
            // TODO change playback rate based on placement position
            // lisa.rate()
        }
    }

    fun void leave() {
        // turn off playback
        for (auto p : gs.players) {
            p.player_id == gs.end_game_placements[0].player_id => int is_winner;
            gs.getSampler(p, is_winner ? gs.SfxType_Victory : gs.SfxType_Defeat) @=> Sampler sampler;
            sampler.loop0(false);
            sampler.rampDown(Sampler.RAMP_DUR);
            sampler.rate(1.0);
        }
    }

    fun Room update(float dt) { 
        num_players.val() => int num_players;
        now/second => float t_sec;
        gs.end_game_placements[0] @=> Entity winner;


        g.screenSize() => vec2 screen_size;

        @(0, 0) => vec2 podium_group_center;
        (num_players - 1) * podium_spacing_x.val() + num_players * podium_width.val() => float placement_group_width;
        podium_group_center.x - (num_players / 2.0 - 0.5) * (podium_width.val() + podium_spacing_x.val())  => float podium_start_center_x;
        // if (num_players % 2 == 0) {
        //     podium_group_center.x - ((num_players / 2)-.5) * (podium_width.val() + podium_spacing_x.val()) => podium_start_x;
        // } else {
        //     podium_group_center.x - (num_players / 2) * (podium_width.val() + podium_spacing_x.val())  => podium_start_x;
        // }

        -0.5 * podium_height.val() => float pod_base_y;

        for (int placement; placement < num_players; placement++) {
            gs.end_game_placements[placement] @=> Entity player;

            // input
            10 => float rot_speed;
            if (player.key(Key_Left)) {
                dt * rot_speed +=> player.player_rot;
            }
            if (player.key(Key_Right)) {
                dt * rot_speed -=> player.player_rot;
            }


            podium_height.val() * (1.0 - (placement / (num_players + 1.0))) => float pod_height;
            podium_start_center_x + (podium_width.val() + podium_spacing_x.val()) * placement => float podium_center_x;
            podium_center_x - 0.5 * podium_width.val() => float pod_bl;
            g.pushColor(player.color);
            g.box(
                @(pod_bl, pod_base_y),
                @(pod_bl + podium_width.val(), pod_height + pod_base_y)
            );

            
            // draw waveform
            for (1 => int w_idx; w_idx < gs.WINDOW_SIZE; w_idx++) {
                g.line(
                    @(
                        podium_center_x + 2*podium_width.val() * gs.sampler_waveform[placement][w_idx - 1], 
                        pod_base_y + (w_idx - 1.0)/gs.WINDOW_SIZE * pod_height
                    ),
                    @(
                        podium_center_x + 2*podium_width.val() * gs.sampler_waveform[placement][w_idx], 
                        pod_base_y + (w_idx $ float) / gs.WINDOW_SIZE * pod_height
                    )
                );
            }
            g.popColor();

            // draw player
            @(podium_center_x,
                pod_base_y + pod_height + gs.player_bbox_hw_hh.y + .2 + .1 * Math.sin(3 * t_sec)
            ) => player.player_pos;
            false => player.disabled;
            player.draw();
        }

        // victory text
        g.pushColor(winner.color);
        g.text("Player " + winner.player_id + " WINS!", @(0, screen_size.y * .4), 1.5 * (1.5 + Math.sin(1.5* t_sec)));
        g.popColor();

        return null; 
    } 

    fun void fireworkShred() {
        while (gs.curr_room == this) {
            M.poisson(firework_period_secs.val())::second => now;
            g.screenSize() * 0.5 => vec2 hw_hh;
            spork ~ FX.explode(
                M.randomPointInArea(@(0, 0), hw_hh.x, hw_hh.y), 5::second, gs.end_game_placements[0].color);
        }
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

        gs.curr_room @=> Room room;
        while (elapsed_time < max_dur) {
            if (gs.curr_room != room) break;
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
StartRoom start_room;         gs.addRoom(start_room);
PlacementsRoom placement_room;         gs.addRoom(placement_room);
SnakeRoom snake_room;   gs.addRoom(snake_room);
BlackholeRoom blackhole_room;   gs.addRoom(blackhole_room);
FlappyBirdRoom flappy_room;   gs.addRoom(flappy_room);
HotPotatoRoom potato_room;    gs.addRoom(potato_room);

gs.enterRoom(start_room);

// gameloop
while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    // sound update (used by all minigames)
    Math.max(
        0,
        Math.pow(s.env_follower.last(), s.env_exp.val()) - s.env_low_cut.val()
    ) => gs.mic_volume;

    gs.updateAudioAnalysis(); 

    // draw sound meter (assumimg all games share same sound meter)
    // g.pushLayer(10);
    // gs.progressBar(gs.mic_volume, @(0, .9), .3, .05, Color.WHITE);
    // g.popLayer();

    // room update
    { // UI
    //     if (UI.begin("test")) {
    //         // TODO: UI Library ideas
    //         // - show waveform history of a UGen
    //         // - show amplitude plot of a UGen last
    //         gs.mic_volume => env_pol_last.val;
    //         UI.slider("Mic Low Cut", env_low_cut, 0.00, 1.);
    //         UI.slider("Mic Exponent", env_exp, 0.00, 1.);
    //         if (UI.slider("Mic Pole", env_pole_pos, 0.95, 1.)) env_pole_pos.val() => env_follower.pole;
    //         UI.slider("Mic Scaled Volume", env_pol_last, 0.00, 1.);
    //     }
    //     UI.end();
        UI.setNextWindowBgAlpha(0.00);
        UI.begin("");
        UI.separatorText("gamestate");
        UI.progressBar(gs.mic_volume, @(0, 0), "volume");
        if (UI.listBox("room", gs.curr_room_idx, gs.room_names)) {
            gs.enterRoom(gs.rooms[gs.curr_room_idx.val()]);
        }


        UI.separatorText(gs.curr_room.room_name);
        gs.curr_room.ui();
        UI.end();
    }

    // update
    gs.curr_room.update(dt) @=> Room new_room;
    if (new_room != null) gs.enterRoom(new_room);
}
