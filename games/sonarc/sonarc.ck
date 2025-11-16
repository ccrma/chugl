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
@import "../lib/whisper/Whisper.chug"
@import "../lib/tween.ck"
@import "HashMap.chug"

Tween t;

// GWindow.windowed(1920, 1080);
GWindow.center();
// GWindow.fullscreen();
G2D g;
// g.resolution(1920, 1080);
GText.defaultFont(me.dir() + "./assets/m5x7.ttf");
UI_Float3 background_color(18/255.0, 39/255.0, 8/255.0);
g.backgroundColor(background_color.val());

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

// ===============================================
// Begin BARD Game
// ===============================================


// spell type enum

// bard abilities (voice controlled)
0 => int ZSpellType_Thunderbolt; // @TODO change to lightning
1 => int ZSpellType_Laserbeam;
2 => int ZSpellType_Fireball;
3 => int ZSpellType_Revive;
ZSpellType_Revive + 1 => int BARD_SPELL_COUNT;

// knight abilities
4 => int ZSpellType_Shield;

5 => int ZSpellType_Count;

class ZGamestate {
    b2WorldDef world_def;
    @(0, 0) => world_def.gravity; // no top-down, no gravity
    b2.createWorld(world_def) => int b2_world_id;
    b2.world(b2_world_id);

    ZEntity entity_pool[0]; // maybe it can all go in 1 entity list...
    int entity_pool_idx; // # of entities currently checked out
    HashMap b2body_to_entity_map;

    ZEntity players[0];

    UI_Float enemy_speed(.4);

    UI_Float animation_secs_per_frame(.1);

    // assets ====================================================
    static TextureLoadDesc tex_load_desc;
    true => tex_load_desc.flip_y;
    false => tex_load_desc.gen_mips;

    10 => int pillbug_animation_frame_count;
    Texture.load(me.dir() + "./assets/pillbug.png", tex_load_desc) @=> Texture pillbug_sprite;
    // end assets ====================================================

    // @TODO: can just iterate backwards over the entity pool, no longer need pool_idx
    fun ZEntity addEntity(int entity_type, int b2_body_id) {
        if (entity_pool_idx == entity_pool.size()) {
            entity_pool << new ZEntity;
            entity_pool_idx => entity_pool[-1].pool_idx;
        }
        entity_pool[entity_pool_idx] @=> ZEntity entity;
        T.assert(!b2Body.isValid(entity.b2_body_id), "entity should have no b2bodyid");
        T.assert(!b2body_to_entity_map.has(b2_body_id), "hashmap should not have registered body in addEntity()");

        if (b2Body.isValid(b2_body_id)) {
            b2_body_id => entity.b2_body_id;
            b2body_to_entity_map.set(b2_body_id, entity);
        }
        T.assert(entity.pool_idx == entity_pool_idx, "invalid pool idx in addEntity()");
        ++entity_pool_idx;
        entity_type => entity.entity_type;
        return entity;
    }

    fun int has(int b2_body_id) {
        return b2body_to_entity_map.has(b2_body_id);
    }

    fun ZEntity get(int b2_body_id) {
        return b2body_to_entity_map.getObj(b2_body_id) $ ZEntity;
    }

    // CAREFUL: need to batch the returns because returning in the middle of a for-loop
    // causes the loop to skip over elements.
    ZEntity entities_to_return[0];
    fun void returnEntity(ZEntity e) {
        entities_to_return << e;
    }

    // call once per frame
    fun void processReturnedEntities() {
        for (auto spell : entities_to_return) {
            // swap with last active
            T.assert(entity_pool[spell.pool_idx] == spell, "spell pool idx incorrect");
            T.assert(entity_pool[entity_pool_idx - 1].pool_idx == entity_pool_idx - 1, "spell pool idx incorrect on end");
            --entity_pool_idx;
            spell.pool_idx => int returned_pool_idx;

            // destroy the b2 body on return
            if (b2Body.isValid(spell.b2_body_id)) {
                T.assert(b2body_to_entity_map.has(spell.b2_body_id), "hashmap should have valid body id");
                b2body_to_entity_map.del(spell.b2_body_id);
                b2.destroyBody(spell.b2_body_id);
                0 => spell.b2_body_id;
            }

            entity_pool[entity_pool_idx] @=> ZEntity last_spell;
            spell.pool_idx => last_spell.pool_idx;
            last_spell @=> entity_pool[spell.pool_idx];

            spell @=> entity_pool[entity_pool_idx];
            entity_pool_idx => spell.pool_idx;
            spell.zero(); // reset all fields except pool_idx
            
            T.assert(
                entity_pool[entity_pool_idx] == spell &&
                entity_pool_idx == spell.pool_idx, 
                "spell pool idx incorrect after swap"
            );
            T.assert(
                entity_pool[returned_pool_idx] == last_spell &&
                returned_pool_idx == last_spell.pool_idx,
                "spell pool idx incorrect on end after swap"
            );
        }
        entities_to_return.clear();
        T.assert(entities_to_return.size() == 0, "processed spells to return");
    }
}
ZGamestate z;

// ZEntity type enum (also used for B2Filter categories)
(1 << 0) => int ZEntityType_Static;
(1 << 1) => int ZEntityType_Player;
(1 << 2) => int ZEntityType_Spell;
(1 << 3) => int ZEntityType_Pickup;
(1 << 4) => int ZEntityType_Enemy;

Spell spells[0]; // spell types
class Spell {
    int type;
    string activating_words[];
    1 => float damage;

    int unlocked;  // @TODO should this be on ZEntity instead?

    fun static void add(int type, string words[], float damage) {
        Spell s;
        spells << s;
        type => s.type;
        words @=> s.activating_words;
        damage => s.damage;
    }

    static b2Filter spell_filter;
    ZEntityType_Spell => spell_filter.categoryBits;
    0xFFFFFFFF ^ ZEntityType_Spell => spell_filter.maskBits; // collides with everything except other spells
    static b2ShapeDef spell_shape_def;
    spell_filter @=> spell_shape_def.filter;
    true => spell_shape_def.enableContactEvents;
    static b2BodyDef spell_body_def;
    b2BodyType.dynamicBody => spell_body_def.type;
    // true => spell_body_def.isBullet;
    fun static void cast(int spell_type, vec2 pos, vec2 rot, ZEntity caster) { // basically an init
        if (spell_type < 0 || spell_type >= ZSpellType_Count) {
            T.err("Unrecognized spell type: " + spell_type);
            return;
        }

        spells[spell_type] @=> Spell@ spell_class;

        caster.b2_group_idx => spell_filter.groupIndex;
        <<< "setting group idx to", spell_filter.groupIndex >>>;

        // how do we do combos....

        if (spell_type == ZSpellType_Thunderbolt) {
            // instant cast, no need for ZEntity and b2body
        }
        if (spell_type == ZSpellType_Laserbeam) {
            spork ~ FX.laser(pos, 50*rot + pos, .2);

            // raycast
            b2QueryFilter laser_filter; // @optimize make static
            ZEntityType_Spell => laser_filter.categoryBits;
            ZEntityType_Enemy | ZEntityType_Player => laser_filter.maskBits;
            b2World.castRayAll(z.b2_world_id, pos, 50 * rot, laser_filter) @=> b2RayResult results[];
            for (auto r : results) {
                ZEntity.fromShape(r.shape) @=> ZEntity hit_entity;

                // @TODO implement Entity.hit()
                if (hit_entity.entity_type & ZEntityType_Enemy) Enemy.damage(hit_entity, spell_class.damage);
                if (hit_entity.entity_type & ZEntityType_Player) {
                    // @TODO: implement player.damage()
                    true => hit_entity.dead;
                    b2Body.linearVelocity(hit_entity.b2_body_id, @(0,0));
                }

                <<< "laser hit", r.shape >>>;
            }
        }
        if (spell_type == ZSpellType_Fireball) {
            pos => spell_body_def.position;
            rot => spell_body_def.rotation;
            b2.createBody(z.b2_world_id, spell_body_def) => int b2_body_id;

            b2Circle circle(.12);
            b2.createCircleShape(b2_body_id, spell_shape_def, circle);

            // set movement
            b2Body.linearVelocity(b2_body_id, 3.0 * rot);


            z.addEntity(ZEntityType_Spell, b2_body_id) @=> ZEntity spell;
            spell_type => spell.spell_type;

            // spell_map.set(p.body_id, p);
        }

        if (spell_type == ZSpellType_Revive) {
            for (auto p : z.players) {
                // @hardcoded resurrect range
                if (p.dead && M.dist(p.position(), caster.position()) <= 1.0) {
                    false => p.dead;
                    spork ~ FX.ripple(p.position());
                }
            }
        }

        if (spell_type == ZSpellType_Shield) {
            pos => spell_body_def.position;
            rot => spell_body_def.rotation;
            b2.createBody(z.b2_world_id, spell_body_def) => int b2_body_id;

            // @hardcoded: barrier size 
            .5 => float hw;
            b2.createSegmentShape(b2_body_id, spell_shape_def, 
                @(0, -hw), @(0, hw)
            );

            // set movement
            b2Body.linearVelocity(b2_body_id, 3.0 * rot);

            z.addEntity(ZEntityType_Spell, b2_body_id) @=> ZEntity spell;
            spell_type => spell.spell_type;
            pos => spell.spawn_pos;
        }

        // reset filter
        0 => spell_filter.groupIndex;
    }

    fun static void update(ZEntity spell) {
        T.assert(spell.entity_type & ZEntityType_Spell, "update() spell");
    }

    fun static void collide(ZEntity spell, ZEntity object) {
        T.assert(spell.entity_type & ZEntityType_Spell, "collide() spell");
        spell.spell_type => int spell_type;
        spells[spell_type] @=> Spell spell_class;
        object.entity_type & ZEntityType_Enemy => int hit_enemy;
        object.entity_type & ZEntityType_Static => int hit_wall;
        object.entity_type & ZEntityType_Player => int hit_player;
        spell.spell_type < BARD_SPELL_COUNT => int is_bard_spell;

        if (spell_type == ZSpellType_Thunderbolt) {
            // instant cast, no need for ZEntity and b2body
            z.returnEntity(spell);
        } else if (spell_type == ZSpellType_Laserbeam) {

        } else if (spell_type == ZSpellType_Fireball) {
            // @TODO apply dmg to enemies in aoe
            spork ~ FX.smokeCloud(b2Body.position(spell.b2_body_id), Color.ORANGE);
            z.returnEntity(spell);
        } else if (spell_type == ZSpellType_Shield) {
            if (hit_wall) z.returnEntity(spell);
        } else {
            T.err("Unrecognized spell type in collide(): " + spell_type);
        }

        if (hit_enemy) {
            Enemy.damage(object, spell_class.damage);
        }

        if (hit_player && is_bard_spell) {
            // insta ded
            true => object.dead;
            b2Body.linearVelocity(object.b2_body_id, @(0,0));
        }
    }
}

// bard spells
Spell.add(ZSpellType_Thunderbolt, ["thunderbolt", "wonderful"], 2);
Spell.add(ZSpellType_Laserbeam, ["laser", "lazer", "liza", "lazy", "leisabim", "blazer"], 1.5);
Spell.add(ZSpellType_Fireball, ["fireball"], 3);
Spell.add(ZSpellType_Revive, ["revive", "resurrect"], 0); true => spells[ZSpellType_Revive].unlocked;

// knight spells
Spell.add(ZSpellType_Shield, null, 0);

// enemy type enum
0 => int ZEnemyType_Default;
1 => int ZEnemyType_Count;

class Enemy {
    int health;

    fun static ZEntity spawn(vec2 pos, float size) 
    {
        // @optimize: reuse static b2 defs 
        b2BodyDef enemy_body_def;
        pos => enemy_body_def.position;
        b2BodyType.dynamicBody => enemy_body_def.type;
        b2.createBody(z.b2_world_id, enemy_body_def) => int body_id;

        b2ShapeDef enemy_shape_def;
        b2Filter enemy_filter;
        ZEntityType_Enemy => enemy_filter.categoryBits;
        // make enemy a sensor if we want player to be able to move through enemies
        // @TODO play brotato, vampire survivors, 20 minutes till dawn etc and see what they do
        // true => enemy_shape_def.isSensor;
        // true => enemy_shape_def.enableSensorEvents;
        enemy_filter @=> enemy_shape_def.filter;

        b2Capsule enemy_geo(
            @(-size/2, 0),  // center1
            @(size/2, 0),   // center2
            .1         // radius
        );
        b2.createCapsuleShape(body_id, enemy_shape_def, enemy_geo);

        z.addEntity(ZEntityType_Enemy, body_id) @=> ZEntity e;
        enemy_geo @=> e.capsule;
        3 => e.hp_max;
        3 => e.hp_curr;
        return e;
    }

    fun static void damage(ZEntity e, float amt) {
        T.assert(e.entity_type & ZEntityType_Enemy, "calling damage() on non-enemy");

        // hurt anim
        t.easeOutQuad(e.hit_cd, 1.0, 0).over(.3);

        amt -=> e.hp_curr;
        // die
        if (e.hp_curr <= 0) {
            z.returnEntity(e);
            spork ~ FX.explode(b2Body.position(e.b2_body_id), .8::second, Color.GRAY);
        }
    }

    fun static void draw(ZEntity e, float dt) {
        // draw enemies
        (e.capsule.center1 - e.capsule.center2).magnitude() => float size;

        // TODO add hit CD

        @(
            size + 2 * e.capsule.radius,
            size + 2 * e.capsule.radius
        ) => vec2 sca;

        // g.box(e.position(), sca.x, sca.y);

        g.sprite(
            z.pillbug_sprite, @(z.pillbug_animation_frame_count, 1), 
            @(e.current_frame $ float / z.pillbug_animation_frame_count, 0), 
            e.position(), 
            sca,
            e.angle() - Math.pi/2.0, 
            Color.WHITE * e.hit_cd.val() +
            Color.DARKBROWN
        );

        // collider
        // g.capsuleFilled(
        //     b2Body.position(e.b2_body_id), 
        //     size,
        //     e.capsule.radius, 
        //     b2Body.angle(e.b2_body_id), 

        //     Color.WHITE * e.hit_cd.val() +
        //     Color.RED
        // );
    }
}

// player type enum
0 => int ZPlayerType_Bard;
1 => int ZPlayerType_Knight;


// sensor type enum
0 => int ZSensorType_Default;
1 => int ZSensorType_UnlockFireball;
2 => int ZSensorType_UnlockLaser;
ZSensorType_UnlockLaser + 1 => int UNLOCK_SPELL_COUNT;

3 => int ZSensorType_SpawnZone;
4 => int ZSensorType_Count;

class ZEntity
{
    int pool_idx; // used by pool allocator, assumes each ZEntity only belongs to a single pool

    int b2_body_id;
    int b2_group_idx;
    int entity_type; // mask of ZEntityType_XXX

    // transform
    vec2 spawn_pos;
    vec2 pos;
    float rot_rad;   // movement dir
    vec2  look_dir;  // look dir
    float last_dir_x; // used for sprite Y-flip 
    
    // health
    float hp_max;
    float hp_curr;

    // movement
    float speed;

    // player character stuff
    int player_type;
    int closest_enemy_body; // b2_body_id of closest enemy
    float closest_enemy_dist;
    int dead;


    // spell stuff
    int spell_type;

    // pickup stuff
    int sensor_type;
    float time_to_spawn_secs; 

    // enemy stuff
    UI_Float hit_cd(0.0); // set to 1 on hit and lerped back to 0
    int enemy_type; 
    b2Capsule@ capsule;

    // sprite animation state
    float time_since_last_frame;
    int current_frame;

    // zeros all fields EXCEPT pool_idx, which is needed by entity pool
    fun void zero() {
        0 => b2_body_id; // @TODO should we b2DestroyBody here?
        0 => b2_group_idx;
        0 => entity_type;
        @(0, 0) => spawn_pos;
        @(0, 0) => pos;
        @(1, 0) => look_dir;
        0 => last_dir_x;
        0 => rot_rad;

        0 => hp_max;
        0 => hp_curr;

        0 => speed;
        
        0 => player_type;
        0 => closest_enemy_body; // b2_body_id of closest enemy
        Math.FLOAT_MAX => closest_enemy_dist;
        0 => dead;

        0 => spell_type;
        0 => sensor_type;
        0 => time_to_spawn_secs;

        0 => hit_cd.val;
        0 => enemy_type;
        null @=> capsule;

        0 => time_since_last_frame;
        0 => current_frame;
    }

    fun vec2 position() {
        return b2Body.position(b2_body_id);
    }

    fun vec2 rotation() {
        return b2Body.rotation(b2_body_id);
    }

    fun float angle() { return b2Body.angle(b2_body_id); }

    fun static ZEntity fromShape(int id) {
        T.assert(b2Shape.isValid(id), "invalid shape id!!");
        return z.b2body_to_entity_map.getObj(b2Shape.body(id)) $ ZEntity;
    }
}

class VoiceCommandRoom extends Room
{
    // @TODO need a callback from whisper to let us know when the internal audio buffer is cleared...
    // this is so spells don't multi-cast

    "voice room" => room_name;
    adc => Whisper w(me.dir() + "../lib/whisper/models/ggml-base.en.bin") => blackhole;
    string transcription_raw;
    string transcription_parsed; // the current parsed / sanitized transcription
    StringTokenizer transcription_tokenizer;
    float last_transcribe_time_ms;

    // voice command state
    int spell_token_counts[BARD_SPELL_COUNT]; // track transcribed tokens so we don't over-cast
    int spell_token_counts_per_frame[BARD_SPELL_COUNT]; // used to count every frame

    // assets =======================================
    static TextureLoadDesc tex_load_desc;
    true => tex_load_desc.flip_y;
    false => tex_load_desc.gen_mips;
    Texture.load(me.dir() + "./assets/bard.bmp", tex_load_desc) @=> Texture bard_sprite;
    Texture.load(me.dir() + "./assets/knight.bmp", tex_load_desc) @=> Texture knight_sprite;

    // b2 world creation ==============================

    int begin_sensor_events[0];
    int begin_touch_events[0];

    // Gamestate ===================
    UI_Float bard_speed(.6);
    UI_Float player_speed(2.2);
    UI_Float player_size(.3);
    UI_Float player_corner_radius(.02);
    UI_Bool  unlock_all_spells;

    // add players
    addPlayer(ZPlayerType_Bard, @(-1, 0)) @=> ZEntity player;
    addPlayer(ZPlayerType_Knight, @(1, 0)) @=> ZEntity knight;

    // player controls
    fun float axis(int gamepad_id, int axis) {
        Gamepad.axis(gamepad_id, axis) => float val;
        // deadzone
        if (Math.fabs(val) < .08) return 0;
        return val;
    }

    8.0 => float dungeon_hw;
    // build walls
    b2BodyDef static_body_def;
    b2BodyType.staticBody => static_body_def.type;
    b2.createBody(z.b2_world_id, static_body_def) => int wall_body_id;
    // shape def
    b2ShapeDef wall_shape_def;
    // geometry
    b2.createSegmentShape(wall_body_id, wall_shape_def, @(-dungeon_hw, dungeon_hw), @(dungeon_hw, dungeon_hw)); 
    b2.createSegmentShape(wall_body_id, wall_shape_def, @(dungeon_hw, dungeon_hw), @(dungeon_hw, -dungeon_hw)); 
    b2.createSegmentShape(wall_body_id, wall_shape_def, @(dungeon_hw, -dungeon_hw), @(-dungeon_hw, -dungeon_hw)); 
    b2.createSegmentShape(wall_body_id, wall_shape_def, @(-dungeon_hw, -dungeon_hw), @(-dungeon_hw, dungeon_hw));
    z.addEntity(ZEntityType_Static, wall_body_id) @=> ZEntity wall;

    fun ZEntity addPlayer(int player_type, vec2 pos) {
        b2BodyDef player_body_def;
        pos => player_body_def.position;
        b2BodyType.dynamicBody => player_body_def.type;
        z.addEntity(
            ZEntityType_Player,
            b2.createBody(z.b2_world_id, player_body_def) 
        ) @=> ZEntity player;
        player_type => player.player_type;

        b2ShapeDef player_shape_def;
        b2Filter player_filter;
        ZEntityType_Player => player_filter.categoryBits;
        0xFFFF ^ ZEntityType_Player => player_filter.maskBits; // disallow player-player collision
        -z.players.size() - 1 => player_filter.groupIndex; // give each player a group index to disallow player collision w/ own spells
        player_filter.groupIndex => player.b2_group_idx;
        player_filter @=> player_shape_def.filter;
        true => player_shape_def.enableSensorEvents;
        true => player_shape_def.enableContactEvents;

        b2.makeRoundedBox(player_size.val(), player_size.val(), player_corner_radius.val()) @=> b2Polygon player_geo;
        b2.createPolygonShape(player.b2_body_id, player_shape_def, player_geo) => int player_shape_id;
        z.players << player;
        return player;
    }

    fun void playerCollide(ZEntity player, ZEntity object) {
        T.assert(player.entity_type & ZEntityType_Player, "player is not type player");
        T.assert(!(object.entity_type & ZEntityType_Spell), "not handling player-spell collisions here");
        object.entity_type & ZEntityType_Enemy => int hit_enemy;
        <<< "hit enemy", hit_enemy, player.player_type >>>;

        if (hit_enemy && player.player_type == ZPlayerType_Bard) {
            true => player.dead;
        }
    }
    // Spells & Projectiles ===================

    // Enemies ==================================
    UI_Bool enemy_update(false);
    Enemy.spawn(@(0,2), .3) @=> ZEntity enemy;
    UI_Float spawn_rate(0.0); // enemy spawn rate (in enemies per second)
    // call once per frame
    float time_to_next_spawn;
    float elapsed_time_secs;
    fun void spawner(float dt) {
        if (spawn_rate.val() <= 0) return;

        if (time_to_next_spawn == 0) M.poisson(1.0 / spawn_rate.val()) => time_to_next_spawn;

        dt +=> elapsed_time_secs;
        while (elapsed_time_secs > time_to_next_spawn) {
            spawnPickup(M.randomPointInArea(@(0, 0), .8 * dungeon_hw, .8 * dungeon_hw), ZSensorType_SpawnZone) @=> ZEntity e;
            4.0 => e.time_to_spawn_secs;
            time_to_next_spawn -=> elapsed_time_secs;
            M.poisson(1.0 / spawn_rate.val()) => time_to_next_spawn;
        }
    }

    // Pickups ==================================
    spawnPickup(@(-3, -3), ZSensorType_UnlockFireball);
    spawnPickup(@(3, -3), ZSensorType_UnlockLaser);


    // transcription ====================================
    // should we just put this in overall GS?
    // else @optimize disable transcription when not in this room
    fun void transcriberShred() {
        <<< "started transcriber" >>>;
        500::ms => dur step;
        step => now;
        while (1) {
            now + step => time later;
            w.transcribe() => now;
            w.text() => transcription_raw;

            { // parse and cleanup
                transcription_raw.trim().lower() => transcription_parsed;
                // search for and remove punctuation
                transcription_parsed.replace(",", "");
                transcription_parsed.replace("!", "");
                transcription_parsed.replace(".", "");
                transcription_parsed.replace("?", "");
                transcription_tokenizer.set(transcription_parsed);
            }

            if (w.wasContextReset()) spell_token_counts.zero();

            (now - (later - step)) / ms => last_transcribe_time_ms;

            if (now < later) later - now => now; // wait for next step
        }
    } spork ~ transcriberShred();


    // Methods =====================

    fun void ui() {
        UI.separatorText("Player");
        if (UI.button("revive bard")) false => player.dead;
        UI.checkbox("unlock all spells", unlock_all_spells);
        UI.slider("bard speed", bard_speed, 0.0, 1.0);
        UI.slider("knight speed", player_speed, 0.0, 3.0);
        UI.slider("corner radius", player_corner_radius, 0.0, 1.0);

        UI.separatorText("Enemy");
        UI.checkbox("update", enemy_update);
        UI.slider("enemy_speed", z.enemy_speed, 0.0, 3.0);
        UI.slider("enemy spawn rate", spawn_rate, 0.0, 3.0);

        UI.separatorText("Whisper");
        UI.textColored(@(1, 1, 0, 1), "Transcription: ");
        UI.sameLine();
        UI.textWrapped(transcription_raw);

        UI.textColored(@(1, 1, 0, 1), "Transcription Time: ");
        UI.sameLine();
        UI.text(last_transcribe_time_ms + "(ms)");

        UI.textColored(@(0, 1, 0, 1), "Parsed: ");
        UI.sameLine();
        UI.text(transcription_parsed);

        UI.textColored(@(0, 1, 0, 1), "Tokens: ");
        while( transcription_tokenizer.more() )
        {
            UI.text("\t" + transcription_tokenizer.next());
        }
        transcription_tokenizer.reset();

    }

    fun Room update(float dt) { 
        g.mousePos() => vec2 mouse_pos;
        b2Body.position(player.b2_body_id) => vec2 player_pos;
        M.normalize(mouse_pos - player_pos) => player.look_dir;

        spawner(dt);

        { // process sensor events from the prev frame
            b2World.sensorEvents(z.b2_world_id, begin_sensor_events, null);
            for (int i; i < begin_sensor_events.size(); 2 +=> i) {
                begin_sensor_events[i] => int sensor_shape_id;
                begin_sensor_events[i+1] => int visitor_shape_id;

                // might be invalid because we destroy in prior iteration
                if (!b2Shape.isValid(sensor_shape_id) || !b2Shape.isValid(visitor_shape_id)) continue;

                ZEntity.fromShape(sensor_shape_id) @=> ZEntity sensor;
                ZEntity.fromShape(visitor_shape_id) @=> ZEntity visitor;

                <<< "sensor collision!" >>>;

                if (sensor.sensor_type == ZSensorType_UnlockFireball) {
                    true => spells[ZSpellType_Fireball].unlocked;
                } else if (sensor.sensor_type == ZSensorType_UnlockLaser) {
                    true => spells[ZSpellType_Laserbeam].unlocked;
                }

                z.returnEntity(sensor);
                // b2Body.position(sensor_id) => vec2 sensor_pos;
                // b2Body.position(not_sensor_id) => vec2 not_sensor_pos;
            }

            b2World.contactEvents(z.b2_world_id, begin_touch_events, null, null);
            for (int i; i < begin_touch_events.size(); 2 +=> i) {
                begin_touch_events[i] => int touch_shape_a;
                begin_touch_events[i+1] => int touch_shape_b;
                if (!b2Shape.isValid(touch_shape_a) || !b2Shape.isValid(touch_shape_b)) continue;
                b2Shape.body(begin_touch_events[i]) => int touch_body_id_a;
                b2Shape.body(begin_touch_events[i+1]) => int touch_body_id_b;

                if (!b2Body.isValid(touch_body_id_a) || !b2Body.isValid(touch_body_id_b)) continue;

                z.get(touch_body_id_a) @=> ZEntity a;
                z.get(touch_body_id_b) @=> ZEntity b;

                a.entity_type & ZEntityType_Spell => int a_is_spell;
                b.entity_type & ZEntityType_Spell => int b_is_spell;
                a.entity_type & ZEntityType_Player => int a_is_player;
                b.entity_type & ZEntityType_Player => int b_is_player;

                T.assert(!(a_is_spell && b_is_spell), "spells should not collide with each other");
                if (a_is_spell) Spell.collide(a, b);
                if (b_is_spell) Spell.collide(b, a);
                if (a_is_player) playerCollide(a, b);
                if (b_is_player) playerCollide(b, a);

                <<< "begin_touch:", touch_body_id_a, touch_body_id_b, touch_body_id_a == player.b2_body_id, touch_body_id_b == player.b2_body_id >>>;
                // <<< "begin_touch:", touch_body_id_a == enemy.b2_body_id, touch_body_id_b == enemy.b2_body_id >>>;
            }
        }

        { // controls

            vec2 dir;

            if (!player.dead) {
                if (GWindow.key(GWindow.KEY_A)) 1 -=> dir.x;
                if (GWindow.key(GWindow.KEY_D)) 1 +=> dir.x;
                if (GWindow.key(GWindow.KEY_S)) 1 -=> dir.y;
                if (GWindow.key(GWindow.KEY_W)) 1 +=> dir.y;
                dir.normalize();
                if (dir.x != 0) dir.x => player.last_dir_x;
                b2Body.linearVelocity(player.b2_body_id, dir * bard_speed.val());
            }

            @(0, 0) => dir;
            if (GWindow.key(GWindow.KEY_LEFT)) 1 -=> dir.x;
            if (GWindow.key(GWindow.KEY_RIGHT)) 1 +=> dir.x;
            if (GWindow.key(GWindow.KEY_DOWN)) 1 -=> dir.y;
            if (GWindow.key(GWindow.KEY_UP)) 1 +=> dir.y;

            // knight gamepad controls
            Gamepad.available() @=> int gamepads[];
            0 => int gamepad_id;
            if (gamepads.size()) {
                gamepads[0] => gamepad_id;

                axis(gamepad_id, Gamepad.AXIS_LEFT_X) => dir.x;
                -axis(gamepad_id, Gamepad.AXIS_LEFT_Y) => dir.y;
            }

            dir.normalize();

            if (!knight.dead) {
                if (dir.x != 0) dir.x => knight.last_dir_x;
                b2Body.linearVelocity(knight.b2_body_id, dir * player_speed.val());

                if (GWindow.keyDown(GWindow.KEY_RIGHTSHIFT) || Gamepad.buttonDown(gamepad_id, Gamepad.BUTTON_A)) {
                    Spell.cast(ZSpellType_Shield, knight.position(), knight.look_dir, knight);
                }
            }
        } // end controls

        { // detect spells
            Spell@ spell;
            for (int i; i < transcription_tokenizer.size(); i++) {
                for (int spell_idx; spell_idx < BARD_SPELL_COUNT; spell_idx++) {
                    spells[spell_idx] @=> spell;
                    if (spell.unlocked || unlock_all_spells.val()) {
                        for (auto word : spell.activating_words) {
                            if (transcription_tokenizer.get(i) == word) {
                                spell_token_counts_per_frame[spell.type]++;
                            }
                        }
                    }
                }
            }
            transcription_tokenizer.reset();
        }

        // cast!
        for (int i; i < BARD_SPELL_COUNT; i++) {
            spells[i] @=> Spell spell;
            spell_token_counts_per_frame[spell.type] > spell_token_counts[spell.type] => int cast;
            if (!cast) continue;
            Spell.cast(
                spell.type,
                player_pos + .4 * player.look_dir,
                player.look_dir, player
            );
        }
        // if (GWindow.mouseLeftDown()) {
        //     Spell.cast(
        //         ZSpellType_Fireball,
        //         player_pos + .4 * player.look_dir,
        //         player.look_dir, player
        //     );
        // }
        
        // draw players 
        2 * @(player_size.val(), player_size.val()) => vec2 player_sca;

        if (player.dead) {
            g.sprite(bard_sprite, player.position(), player_sca, Math.pi/2.0, Color.BLACK);
        } else {
            if (player.last_dir_x < 0) -1 *=> player_sca.x;
            g.sprite(bard_sprite, player_pos, player_sca, 0);
        }

        g.pushTextMaxWidth(2);
        g.pushTextControlPoint(.5, 0);
        transcription_raw => string bard_text;
        bard_text.replace("[BLANK_AUDIO]", "...");
        g.text(bard_text, player_pos + @(0, .25), .3);
        g.popTextControlPoint();
        g.popTextMaxWidth();

        2 * @(player_size.val(), player_size.val()) => player_sca;
        if (knight.last_dir_x < 0) -1 *=> player_sca.x;
        if (knight.dead) {
            g.sprite(knight_sprite, knight.position(), player_sca, Math.pi/2.0, Color.BLACK);
            g.circleDotted(knight.position(), 1.0, .2*(now/second), Color.WHITE);
            g.text("REVIVE!", knight.position() + @(0, .1 * Math.sin(2 * (now/second))), .3);
        } else {
            g.sprite(knight_sprite, knight.position(), player_sca, 0);
        }


        // step and draw spells + pickups
        for (auto entity : z.entity_pool) {
            if (entity.entity_type & ZEntityType_Spell) {
                entity @=> ZEntity spell;
                M.rot2vec(spell.rot_rad) => vec2 dir; // @optimize: store dir as a vec2 with sin/cos precomputed
                dt * spell.speed * dir +=> spell.pos;

                // update & draw SPELLS
                // @TODO can the spell combination stuff go here? replace else if with just an if???
                if (spell.spell_type == ZSpellType_Thunderbolt) {
                    if (g.offscreen(spell.pos, 1.5)) {
                        z.returnEntity(spell);
                    } else {
                        spork ~ FX.booster(spell.pos, Color.YELLOW, 2);
                    }
                } else if (spell.spell_type == ZSpellType_Laserbeam) {
                    // this one is a one-framer
                    z.returnEntity(spell);
                } else if (spell.spell_type == ZSpellType_Fireball) {
                    if (g.offscreen(spell.pos, 1.5)) z.returnEntity(spell);
                    else spork ~ FX.booster(b2Body.position(spell.b2_body_id), Color.ORANGE, 3);
                } 
                else if (spell.spell_type == ZSpellType_Shield) {
                    // @hardcoded
                    // despawn after a set distance
                    2.0 => float despawn_dist;
                    if (M.dist(spell.spawn_pos, spell.position()) > despawn_dist) {
                        z.returnEntity(spell);
                    } else {
                        spell.position() => vec2 pos;
                        spell.rotation() => vec2 rot;
                        M.perp(rot) => vec2 perp;
                        // @hardcoded shield width
                        .5 => float shield_hw;
                        g.line(pos + shield_hw * perp, pos - shield_hw * perp);
                    }
                }
                else {
                    T.err("Unrecognized spell type: " + spell.spell_type);
                }
            }

            if (entity.entity_type & ZEntityType_Pickup) {
                vec3 color;
                string text;
                
                if (entity.sensor_type < UNLOCK_SPELL_COUNT ) {
                    if (entity.sensor_type == ZSensorType_UnlockFireball) {
                        Color.ORANGE => color;
                        "fireball" => text;
                    } 
                    else if (entity.sensor_type == ZSensorType_UnlockLaser) {
                        Color.RED => color;
                        "laserbeam" => text;
                    }
                    g.square(
                        b2Body.position(entity.b2_body_id),
                        b2Body.angle(entity.b2_body_id), // @optimize can use b2Body.rotation and pass complex cos/sin to polygonFilled
                        .3 + .05 * Math.sin(3 * (now/second)), // @optimize: cache sin results?
                        color
                    );
                    g.squareFilled(
                        b2Body.position(entity.b2_body_id),
                        b2Body.angle(entity.b2_body_id), // @optimize can use b2Body.rotation and pass complex cos/sin to polygonFilled
                        .1,
                        color
                    );
                    g.pushColor(color);
                    g.text(text, entity.position() + @(0, .35 + .1 * Math.sin(2 * (now/second))), .3);
                    g.popColor();
                }
                else if (entity.sensor_type == ZSensorType_SpawnZone) {
                    g.boxFilled( entity.position(), Math.pi/4, .3, .1, Color.RED );
                    g.boxFilled( entity.position(), -Math.pi/4, .3, .1, Color.RED );
                    dt -=> entity.time_to_spawn_secs;

                    // don't spawn if player on top
                    if (entity.time_to_spawn_secs <= 0) {
                        b2QueryFilter filter;
                        ZEntityType_Player => filter.maskBits;
                        if (b2World.overlapAABB(z.b2_world_id, M.aabb(entity.position(), .15, .15), filter ).size() == 0) {
                            Enemy.spawn(entity.position(), .4);
                        }

                        z.returnEntity(entity);
                    }
                }
            }

            // enemy update
            if (entity.entity_type & ZEntityType_Enemy) {
                if (enemy_update.val()) {
                    // enemy update (walk towards player)
                    b2Body.linearVelocity( 
                        entity.b2_body_id,
                        z.enemy_speed.val() * M.dir(b2Body.position(entity.b2_body_id), player_pos));
                    
                    b2Body.rotation(
                        entity.b2_body_id,
                        player_pos - entity.position()
                    );


                    dt +=> entity.time_since_last_frame;
                    if (entity.time_since_last_frame >= z.animation_secs_per_frame.val()) {
                        // advance to next frame
                        0 => entity.time_since_last_frame;
                        (entity.current_frame + 1) % z.pillbug_animation_frame_count => entity.current_frame;
                    } 
                }

                // update closest enemies for all players
                // @optimize use b2Shape collider distance instead of transform pos
                for (auto p : z.players) {
                    if (p.player_type == ZPlayerType_Bard) continue; // don't auto-target bard
                    M.dist(p.position(), entity.position()) => float dist;
                    if (
                        p.closest_enemy_body == 0 
                        ||
                        dist < p.closest_enemy_dist
                    ) {
                        dist => p.closest_enemy_dist;
                        enemy.b2_body_id => p.closest_enemy_body;
                        // update look dir
                        M.normalize(entity.position() - p.position()) => p.look_dir;
                    }
                }

                Enemy.draw(entity, dt);
            }
        }

        // aim (auto targetting for now)
        // @TODO add debug mode drawing player colliders only
        for (auto p : z.players) {
            if (b2Body.isValid(p.closest_enemy_body)) {
                g.dashed(p.position(), b2Body.position(p.closest_enemy_body), Color.RED, .1);
            }
        }

        // draw dungeon
        g.box(@(0,0), 2 * dungeon_hw, 2 * dungeon_hw);


        { // collisions
            b2World.castRayClosest(z.b2_world_id, player_pos, mouse_pos - player_pos, new b2QueryFilter) 
                @=> b2RayResult result;

            if (result.shape != 0) {
                g.squareFilled(result.point, 0, .1, Color.RED);
            }
            // <<< "fraction: ", result.fraction, "shape: ", result.shape, "normal: ", result.normal, "point: ", result.point >>>;
            
        }

        // camera track player @TODO

        // reset closest enemy
        for (auto p : z.players) {
            0 => p.closest_enemy_body; 
            Math.FLOAT_MAX => p.closest_enemy_dist;
        }

        // cleanup
        z.processReturnedEntities();
        for (int i; i < spell_token_counts_per_frame.size(); i++) {
            spell_token_counts_per_frame[i] => spell_token_counts[i];
        }
        spell_token_counts_per_frame.zero();

        return null;
    }


    fun int staticBody(vec2 vertices[]) {
        // body def
        b2BodyDef static_body_def;
        b2BodyType.staticBody => static_body_def.type;
        @(0.0, 0.0) => static_body_def.position;
        b2.createBody(z.b2_world_id, static_body_def) => int body_id;

        // shape def
        b2ShapeDef shape_def;

        // geometry
        b2.makePolygon(vertices, 0.0) @=> b2Polygon polygon;

        // shape
        b2.createPolygonShape(body_id, shape_def, polygon);

        return body_id;
    }


    fun ZEntity spawnPickup(vec2 pos, int sensor_type) {
        // params @TODO these come from PickupType class
        .3 => float w;
        .3 => float h;
        .12 => float speed;
        .08 => float steering_rate;
        1 => float angular_velocity;

        b2BodyDef pickup_body_def;
        b2BodyType.kinematicBody => pickup_body_def.type;
        angular_velocity => pickup_body_def.angularVelocity;
        pos => pickup_body_def.position;
        false => pickup_body_def.enableSleep; // disable otherwise slowly rotating objects will be put to sleep

        b2Filter pickup_filter;
        ZEntityType_Pickup => pickup_filter.categoryBits;
        ZEntityType_Player | ZEntityType_Static => pickup_filter.maskBits;

        b2ShapeDef pickup_shape_def;
        /// A sensor shape generates overlap events but never generates a collision response.
        true => pickup_shape_def.isSensor; 
        true => pickup_shape_def.enableSensorEvents;
        pickup_filter @=> pickup_shape_def.filter;

        b2.createBody(z.b2_world_id, pickup_body_def) => int body_id;
        T.assert(b2Body.isValid(body_id), "spawnPickup body invalid");
        b2.makeBox(w, h) @=> b2Polygon polygon;
        b2.createPolygonShape(body_id, pickup_shape_def, polygon) => int shape_id;

        z.addEntity(ZEntityType_Pickup, body_id) @=> ZEntity e;
        sensor_type => e.sensor_type;

        return e;
    } 
}

// ===============================================
// End Voice Command Game
// ===============================================

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

    fun static void smokeCloud(vec2 pos, vec3 color) {
        // params
        Math.random2(8, 16) => int num; // number of lines

        .4::second => dur max_dur;
        vec2 positions[num];
        vec2 dir[num];
        float radius[num];
        dur durations[num];
        float velocities[num];

        // init
        for (int i; i < num; i++) {
            pos => positions[i];
            M.randomDir() => dir[i];
            Math.random2f(.08, .11) => radius[i];
            Math.random2f(.3, max_dur/second)::second => durations[i];
            Math.random2f(.01, .02) => velocities[i];
        }

        dur elapsed_time;
        gs.curr_room @=> Room room;
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
                    // shrink radii down to 0
                    radius[i] * M.easeOutQuad(1 - t) => float r;
                    // draw
                    g.circleFilled(positions[i], r, color);
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

    fun static void laser(vec2 start, vec2 end, float size) {
        <<< "casting laser" >>>;
        M.angle(start, end) => float rot;
        0.5 * (start + end) => vec2 pos;
        (start - end).magnitude() => float width;

        1::second => dur effect_dur;
        dur elapsed_time;
        while (elapsed_time < effect_dur) {
            GG.nextFrame() => now;
            GG.dt()::second +=> elapsed_time;

            1.0 - M.easeOutQuad(elapsed_time / effect_dur) => float t;
            g.boxFilled(pos, rot, width, t * size, Color.RED * t);
        }
    }

    fun static void ripple(vec2 pos) {
        1.5 => float end_radius;
        1.5::second => dur effect_dur;

        dur elapsed_time;
        while (elapsed_time < effect_dur) {
            GG.nextFrame() => now;
            GG.dt()::second +=> elapsed_time;
            M.easeOutQuad(elapsed_time / effect_dur) => float t;

            g.circle(pos, end_radius * t, .1 * (1 - t), Color.WHITE * (1 - t));
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
VoiceCommandRoom voice_command_room;    gs.addRoom(voice_command_room);

gs.enterRoom(voice_command_room);

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

        if (UI.colorEdit("background color", background_color)) g.backgroundColor(background_color.val());




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
    t.update(dt);
    gs.curr_room.update(dt) @=> Room new_room;
    if (new_room != null) gs.enterRoom(new_room);
}
