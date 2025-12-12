@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "../lib/tween.ck"
@import "../lib/b2/b2DebugDraw.ck"
@import "HashMap.chug"
@import "fx.ck"

GWindow.sizeLimits(0, 0, 0, 0, @(4, 4));
GWindow.center();
// GWindow.mouseMode(GWindow.MOUSE_DISABLED);

// == Enums ==================================
@(0,0) => vec2 CENTER;
@(1,0) => vec2 RIGHT;
@(-1,0) => vec2 LEFT;
@(0,1) => vec2 UP;
@(0,-1) => vec2 DOWN;

0 => int EntityType_None;
1 => int EntityType_Player;
2 => int EntityType_EnemyProjectile;

0 => int Layer_None;
8 => int Layer_EnemyProjectile;
9 => int Layer_Boss;
10 => int Layer_DebugDraw;

0 => int BulletType_Default; // the circle bois
1 => int BulletType_Snipe; // aims, then fires
2 => int BulletType_Bar; // long flat line that sweeps out and rotates
// 2 => int BulletType_Laser;
// 3 => int BulletType_Chaser; // limit the steering radius, chases player

[
    "Default",
    "Snipe",
    "Bar",
] @=> string bullet_type_names[];

0 => int SniperBulletPhase_Spawn; // traveling to spawn point
1 => int SniperBulletPhase_Aim; // aiming at player
2 => int SniperBulletPhase_Launch; // launching towards player

0 => int GameState_Menu;
1 => int GameState_Battle;


// ===========================================

// == Util ==================================
class CoolDown {
    float max_secs;
    float curr;
    int on; // true on the frame of activation
    float progress; // percentage cd until next activation

    fun @construct(float secs) {
        secs => max_secs;
        secs => curr;
    }

    fun void reset() {
        max_secs => curr;
        0 => progress;
    }

    fun void update(float dt) {
        false => on;
        dt -=> curr;
        if (curr <= 0) {
            max_secs +=> curr;
            true => on;
        }
        (max_secs - curr) / max_secs => progress;
    }

    fun int active(float dt) {
        update(dt);
        return on;
    }
}

// == sound ====================================

class Volume extends Chugraph {
    // gain tracking
    inlet => Gain adc_square => OnePole env_follower => outlet;
    inlet => adc_square;
    3 => adc_square.op;

    // filter pole position
    UI_Float env_low_cut(.0); // zeros all sound below this level. use for basic noise cancellation
    UI_Float env_exp(.25);
    UI_Float env_pol_last;
    UI_Float env_pole_pos(.9998);
    env_pole_pos.val() => env_follower.pole;

    fun void ui() {
        volume() => env_pol_last.val;
        UI.slider("Mic Low Cut", env_low_cut, 0.00, 1.);
        UI.slider("Mic Exponent", env_exp, 0.00, 1.);
        if (UI.slider("Mic Pole", env_pole_pos, 0.95, 1.)) env_pole_pos.val() => env_follower.pole;
        UI.progressBar(volume(), @(0, 0), "volume");
    }

    fun float volume() {
        return Math.max(
            0,
            Math.pow(env_follower.last(), env_exp.val()) - env_low_cut.val()
        );
    }
}

class Sound {
    512 => int WINDOW_SIZE; 
    Flip sampler_waveform_accum;
    Windowing.hann(WINDOW_SIZE) @=> float hann_window[];
    float sampler_waveform[0];

    WINDOW_SIZE => sampler_waveform_accum.size;
    SndBuf2 buf(me.dir() + "./assets/another-medium.wav") => sampler_waveform_accum => blackhole; // analysis
    buf => dac;

    buf.chan(0) => Volume vol_left => blackhole;
    buf.chan(1) => Volume vol_right => blackhole;
    // 0 => dac.gain;

    // hard-coding 
    130 => float bpm;
    60 / bpm => float qt_note_sec; 
    .2 => float init_delay_sec;

    CoolDown whole_note(4 * qt_note_sec);
    CoolDown half_note(2 * qt_note_sec);
    CoolDown qt_note(qt_note_sec);
    CoolDown eigth_note(.5 * qt_note_sec);
    CoolDown sixteenth_note(.25 * qt_note_sec);

    fun dur SongProgress() {
        return (buf.pos()$float / buf.samples()) * buf.length();
    } 

    fun void reset() {
        0 => buf.pos;
        0 => buf.rate;
        .2 => init_delay_sec;

        whole_note.reset();
        half_note.reset();
        qt_note.reset();
        eigth_note.reset();
        sixteenth_note.reset();
    }
    fun float volume() {
        return .5 * (vol_left.volume() + vol_right.volume());
    }

    fun float volume(int which) {
        if (which == 0) return vol_left.volume();
        if (which == 1) return vol_right.volume();
        return 0;
    }

    fun void ui() {
        UI.separatorText("sound");
        vol_left.ui();
        vol_right.ui();
    }

    // dt should be raw, unscaled. we scale by buf playrate
    // dt is already pre-multiplied by dt_rate here
    fun void update(float dt, float dt_rate) {
        dt_rate => buf.rate;

        // only call this once per frame since it's just used for graphical updates
        sampler_waveform_accum.upchuck();
        sampler_waveform_accum.output( sampler_waveform );

        // track bpm
        if (init_delay_sec > 0) {
            dt -=> init_delay_sec;
        } else {
            whole_note.update(dt);
            half_note.update(dt);
            qt_note.update(dt);
            eigth_note.update(dt);
            sixteenth_note.update(dt);
        }
    }
} 
Sound s;

// == Graphics ==================================
class MyG2D extends G2D {
    fun void progressBar(
        float percentage, vec2 pos, float width, float height, vec3 outline_color, vec3 fill_color, float stages[]
    ) {
        width * .5 => float hw;
        height * .5 => float hh;

        g.box(pos, 2 * hw, 2 * hh, outline_color);

        // stage marker lines
        2 * hw => float width;
        for (0 => int i; i < stages.size(); i++) {
            pos.x + hw - (width * stages[i]) => float x;
            g.line(@(x, pos.y - 1*hh), @(x, pos.y + 1*hh));
        }

        g.pushLayer(g.layer() - .1);
        -hw + (percentage * 2 * hw) => float end_x;
        g.boxFilled(
            pos - @(hw, hh),   // bot left
            pos + @(end_x, hh), // top right
            fill_color
        );
    }
};
MyG2D g;
g @=> FX.g;

// g.resolution(512, 512);
// g.antialias(false);


int slow_effect_generation;
fun void slowEffect(float rate, dur d) {
    ++slow_effect_generation => int gen;
    dur elapsed_time;

    while (gen == slow_effect_generation) {
        GG.nextFrame() => now;
        // must use GG.dt() so it's not affected by it's own time warp!
        GG.dt()::second +=> elapsed_time; 

        if (elapsed_time > d) break;

        M.lerp(M.easeInOutCubic(elapsed_time / d), rate, 1.0) => float t;

        // adjust animation rate
        t => gs.dt_rate;
        // adjust physics rate
        t => b2.rate;

        T.assert(gs.dt_rate <= 1.0, "dt_rate exceeds bounds");
        if (gs.dt_rate > 1.0) {
            <<< "rate",  gs.dt_rate, "t", t, "d", d, "rate", rate >>>;
        }
    }

    // restore to normal rate
    if (gen == slow_effect_generation) {
        1.0 => gs.dt_rate;
        1.0 => b2.rate;
    }
}

// =============================================

// == physics ==================================
DebugDraw debug_draw;
debug_draw.layer(Layer_DebugDraw);
true => debug_draw.drawShapes;
true => debug_draw.outlines_only;

b2WorldDef world_def;
int b2_world_id;
@(0, 0) => world_def.gravity; // no top-down, no gravity
b2.createWorld(world_def) => b2_world_id;
b2.world(b2_world_id);

int begin_contact_events[0];
// ==========================================

class Constants {
    UI_Float player_size(.15);
    UI_Float player_rounding(.1); // from 0 to 1. 0 is circle, 1 is square
    UI_Float player_zeno(.38);
    UI_Float player_fire_cd(.2);
    UI_Float player_invincibility_time(2);

    UI_Float boss_waveform_radius(1.1);
    UI_Float boss_waveform_scale(1.5); // how much its affected by waveform data
    UI_Float boss_waveform_zeno(.15); // how much its affected by waveform data
    UI_Float boss_waveform_resolution(.5); // downsample the raw waveform data by this factor
    UI_Int boss_waveform_repeat(2); // downsample the raw waveform data by this factor
    UI_Float boss_transition_time(3); // downsample the raw waveform data by this factor
    UI_Float boss_max_volume_sca(1.8); // downsample the raw waveform data by this factor

    UI_Float bullet_size(.1);


    [
        bullet_size.val() * RIGHT,
        .5 * bullet_size.val() * UP,
        bullet_size.val() * LEFT,
        .5 * bullet_size.val() * DOWN,
    ] @=> vec2 snipe_bullet_vertices[];
    UI_Float sniper_bullet_speed(2.6);

    UI_Float bar_bullet_speed(3.2);
    UI_Float bar_bullet_len(1.6);
} Constants c;

class Entity {
    // for pool 
    // (TODO can actually just get rid of this. just walk the array backwards, 
    // implement an EntityRet(Entity@, int idx) function that swaps with last
    // active element. we know idx because we are in middle of walking array)
    int _idx;

    int entity_type;
    int frame_spawned; // frame we spawned on

    // physics
    int b2_body_id;

    // bullet
    int bullet_type;
        // sniper bullet stuff
        int sniper_bullet_phase;

    fun void zero() {
        0 => entity_type;
        0 => frame_spawned;
        0 => b2_body_id;
        0 => bullet_type;
        0 => sniper_bullet_phase;
    }

    fun vec2 pos() { return b2Body.position(this.b2_body_id); }
    fun void pos(vec2 p) { b2Body.position(this.b2_body_id, p); }
    fun vec2 rot() { return b2Body.rotation(this.b2_body_id); }
    fun void rot(vec2 rot) { b2Body.rotation(this.b2_body_id, rot); }
    fun void vel(vec2 v) {  b2Body.linearVelocity(this.b2_body_id, v); }
    fun vec2 vel() {  return b2Body.linearVelocity(this.b2_body_id); }

    fun float speed() { return M.mag(b2Body.linearVelocity(this.b2_body_id)); }
}

class EntityManager {
    HashMap b2body_to_entity_map;

    Entity bullet_pool[0]; // NOTE: walk array backwards! to avoid skipping elements
    int bullet_pool_count;

    fun Entity BulletGet(int b2_body_id, int bullet_type) {
        if (bullet_pool_count >= bullet_pool.size()) {
            bullet_pool << new Entity;
            bullet_pool_count => bullet_pool[-1]._idx;
        }

        T.assert(bullet_pool[bullet_pool_count]._idx == bullet_pool_count, "bullet add");
        bullet_pool[bullet_pool_count++] @=> Entity bullet;

        bullet.zero();
        GG.fc() => bullet.frame_spawned;
        
        T.assert(bullet.b2_body_id == 0, "bulletGet b2body id not 0");
        b2_body_id => bullet.b2_body_id;
        b2body_to_entity_map.set(b2_body_id, bullet);

        EntityType_EnemyProjectile => bullet.entity_type;
        bullet_type => bullet.bullet_type;
        return bullet;
    }

    fun void BulletRet(int b2_body_id) {
        b2body_to_entity_map.getObj(b2_body_id) $ Entity @=> Entity b;
        BulletRet(b);
    }

    fun void BulletRet(Entity p) {
        T.assert(bullet_pool[p._idx] == p, "bullet ret idx mismatch");
        T.assert(p._idx < bullet_pool_count, "returning a bullet that was never checked out");
        T.assert(b2Body.isValid(p.b2_body_id), "bullet with invalid b2body");
        T.assert(b2body_to_entity_map.has(p.b2_body_id), "BulletRet bullet should be in hashmap");

        // cleanup
        b2body_to_entity_map.del(p.b2_body_id);
        b2.destroyBody(p.b2_body_id);
        0 => p.b2_body_id;

        // swap with last element
        bullet_pool[bullet_pool_count - 1] @=> Entity last_p;
        T.assert(bullet_pool[last_p._idx] == last_p, "bullet ret idx mismatch last_p");

        // swap last
        last_p @=> bullet_pool[p._idx]; p._idx => last_p._idx;
        // swap p
        p @=> bullet_pool[bullet_pool_count - 1]; bullet_pool_count - 1 => p._idx; 

        bullet_pool_count--;
    }

    fun void BulletClear() {
        bullet_pool_count => int n;
        repeat (n) {
            BulletRet(bullet_pool[0]);
        }
    }


    fun Entity PlayerAdd(vec2 pos) {
        b2BodyDef player_body_def;
        pos => player_body_def.position;
        M.rot2vec(Math.pi/4) => player_body_def.rotation;
        // b2BodyType.kinematicBody => player_body_def.type;
        b2BodyType.dynamicBody => player_body_def.type;

        // entity
        Entity player;
        EntityType_Player => player.entity_type;
        b2.createBody(b2_world_id, player_body_def) => player.b2_body_id;
        b2body_to_entity_map.set(player.b2_body_id, player);

        // filter
        b2Filter player_filter;
        EntityType_Player => player_filter.categoryBits;
        EntityType_EnemyProjectile => player_filter.maskBits;

        // shape
        b2ShapeDef player_shape_def;
        player_filter @=> player_shape_def.filter;
        true => player_shape_def.enableContactEvents;

        // geo
        c.player_rounding.val() => float r;
        .8 * c.player_size.val() => float sz;
        // b2.makeRoundedBox(
        //     (1 - r) * sz, (1 - r) * sz,
        //     r * sz
        // ) @=> b2Polygon player_geo;
        // b2.createPolygonShape(player.b2_body_id, player_shape_def, player_geo); // => player.b2_shape_id;
        b2.createCircleShape(player.b2_body_id, player_shape_def, @(0, 0), .5*sz) => int shape_id;
        T.assert(b2Shape.areContactEventsEnabled(shape_id), "player contact event not enabled");


        return player;
    }
}

// == bullets ================================================

BulletSequence bullet_sequences[0];

fun BulletSequence AddBulletSequence(dur max_dur, int timing_type, BulletSequence bs) {
    bullet_sequences << bs;
    max_dur / second => bs.max_runtime_secs;
    timing_type => bs.timing_type;
    0 => bs.elapsed_time_secs;
    return bs;
}

fun void RemoveBulletSequence(BulletSequence bs) {
    for (int i; i < bullet_sequences.size(); ++i) {
        if (bullet_sequences[i] == bs) {
            bullet_sequences.erase(i);
            return;
        }
    }
}

fun void UpdateBulletSequence(Sound s, float dt, vec2 boss_pos, vec2 player_pos, float boss_angle) {
    // need to walk backwards to safely delete
    for (bullet_sequences.size() - 1 => int i; i >= 0; i--) {
        bullet_sequences[i] @=> BulletSequence bs;
        dt +=> bs.elapsed_time_secs;

        // check if time has run out
        if (bs.elapsed_time_secs >= bs.max_runtime_secs) {
            // delete
            bullet_sequences[-1] @=> bullet_sequences[i];
            bullet_sequences.popBack();
        } else {
            bs.spawn_polar_r * M.rot2vec(boss_angle + bs.spawn_polar_theta) => vec2 polar_offset;
            boss_pos + polar_offset + bs.spawn_pos_offset => vec2 spawn_pos;

            // bs.update(dt, boss_pos, player_pos);
            if (bs.timing_type == Timing_Period) {

            } 
            else if (bs.timing_type == Timing_WholeNote) {
                if (s.whole_note.on) bs.fire(spawn_pos, player_pos);
            }
            else if (bs.timing_type == Timing_HalfNote) {
                if (s.half_note.on) bs.fire(spawn_pos, player_pos);
            }
            else if (bs.timing_type == Timing_QuarterNote) {
                if (s.qt_note.on) bs.fire(spawn_pos, player_pos);
            }
            else if (bs.timing_type == Timing_EighthNote) {
                if (s.eigth_note.on) bs.fire(spawn_pos, player_pos);
            }
            else if (bs.timing_type == Timing_SixteenthNote) {
                if (s.sixteenth_note.on) bs.fire(spawn_pos, player_pos);
            }
            else {
                T.err("unimpl timing type: " + bs.timing_type);
            }

            if (bs.timing_type < 0 || bs.timing_type >= Timing_COUNT)
                T.err("unrecognized timing type: " + bs.timing_type);
        }
    }
}

// this could be bitflags instead?
0 => int Timing_None;
1 => int Timing_Period;
2 => int Timing_WholeNote;
3 => int Timing_HalfNote;
4 => int Timing_QuarterNote;
5 => int Timing_EighthNote;
6 => int Timing_SixteenthNote;
7 => int Timing_COUNT;

class BulletSequence {
    "base bullet sequence" => string name;
    float max_runtime_secs;
    float elapsed_time_secs;

    int timing_type;
    CoolDown fire_cd; // only used if timing_type == Timing_Period

    // spawn positioning
    vec2 spawn_pos_offset;
    float spawn_polar_theta; // used with boss_waveform_theta  (@)
    float spawn_polar_r; 
    int spawn_type;

    fun void update(float dt, vec2 boss_pos, vec2 player_pos) { T.err("BulletSequence update not impl"); }
    fun void fire(vec2 boss_pos, vec2 player_pos) { T.err("BulletSequence fire not impl"); }
}

class CircleSequence extends BulletSequence {
    "circle seq" => name;

    // params
    float delta_theta;
    int n;
    float speed;

    // local
    float start_theta;

    fun @construct(dur period, float delta_theta, int n, float speed) {
        n => this.n;
        delta_theta => this.delta_theta;
        speed => this.speed;
    }

    fun void update(float dt, vec2 boss_pos, vec2 player_pos) {} // not impl

    fun void fire(vec2 boss_pos, vec2 player_pos) {
        bh.circle(n, this.speed, boss_pos, start_theta);
        delta_theta +=> start_theta;
    }
}

class TargetSequence extends BulletSequence {
    "Target seq" => name;

    // params
    int n;              // number of shots
    float speed;
    float arc_len;

    // local

    fun @construct(int n, float speed, float arc_len) {
        n => this.n;
        speed => this.speed;
        arc_len => this.arc_len;
    }

    fun void fire(vec2 boss_pos, vec2 player_pos) {
        M.randomPointInCircle(player_pos, 0, 5 * c.player_size.val()) => vec2 target;
        // @optimize: compute boss->player angle once at start of gameloop and pass as param
        bh.arc(this.arc_len, M.angle(boss_pos, target), this.n, 
            Math.random2f(.8, 1.2) * this.speed, 
            boss_pos
        );
    }
}

class SpiralSequence extends BulletSequence {
    "Spiral seq" => name;

    // params
    int n_arms;              // number of shots
    float bullet_speed;
    float delta_theta;

    // local
    float theta;

    fun @construct(int n_arms, float bullet_speed, float start_theta, float delta_theta) {
        n_arms => this.n_arms;
        bullet_speed => this.bullet_speed;
        start_theta => this.theta;
        delta_theta => this.delta_theta;
    }

    fun void fire(vec2 spawn_pos, vec2 player_pos) {
        for (int i; i < n_arms; i++) {
            Math.random2f(.99, 1.01) * this.bullet_speed => float speed;
            this.theta + (i * Math.two_pi / n_arms) => float angle;
            bh.line(M.rot2vec(angle), speed, spawn_pos);
        }
        delta_theta +=> theta;
    }
}

class DNASequence extends BulletSequence {
    "DNA seq" => name;

    // params
    1.0 => float period_secs;
    float speed;

    // local
    float cd;
    float theta;
    vec2 perp;
    float cos_freq; // determined based on fire_cd to prevent aliasing

    fun @construct(dur period, float speed, vec2 dir) {
        period/second => this.period_secs;
        speed => this.speed;
        M.angle(M.normalize(dir)) => this.theta;
        M.perp(M.normalize(dir)) => this.perp;

        1/this.period_secs => float sr;
        .4 * sr => this.cos_freq;
        <<< "sr", sr, "freq", this.cos_freq >>>;
    }

    fun void update(float dt, vec2 boss_pos, vec2 player_pos) {
        dt -=> cd;
        if (cd <= 0) {
            period_secs => cd;
            // calculate dx displacement
            .7 * Math.cos(Math.two_pi * this.cos_freq * elapsed_time_secs) => float dx;
            bh.arc(0, theta, 1, this.speed, boss_pos + dx * perp);
            bh.arc(0, theta, 1, this.speed, boss_pos - dx * perp);
        }
    }
}

class SnipeSequence extends BulletSequence {
    "snipe seq" => name;

    // params

    // local
    28 => int base;

    fun @construct() {
    }

    fun void fire(vec2 boss_pos, vec2 player_pos) {
        base + Math.random2(0, 8) => int n;
        repeat (n) {
            bh.line(M.randomDir(), Math.random2f(.8, 1.2) * c.sniper_bullet_speed.val(), boss_pos, BulletType_Snipe);
        }
        n++;
    }
}

class BarSequence extends BulletSequence {
    "bar seq" => name;

    // params
    int n;

    // local
    float theta;

    fun @construct(int n) {
        n => this.n;
    }

    fun void fire(vec2 spawn_pos, vec2 player_pos) {
        for (int i; i < n; ++i) {
            M.rot2vec(i * Math.two_pi / this.n + this.theta) => vec2 dir;
            bh.line(dir, c.bar_bullet_speed.val(), spawn_pos, BulletType_Bar);
        }
        (Math.pi / this.n) +=> theta;
    }
}

class BH {

    // fire points in a polygonal shape
    fun void ngon() {

    }

    // fire in arc (if n == 1, fires directly at target)
    fun void arc(float arc_len, float start_theta, int n, float speed, vec2 pos) {
        for (int i; i < n; i++) {
            @(
                Math.cos(arc_len * (i - n/2) / n + start_theta), 
                Math.sin(arc_len * (i - n/2) / n + start_theta)
            ) => vec2 dir;
            em.BulletGet(CreateBulletBody(pos, speed*dir, BulletType_Default), BulletType_Default) @=> Entity b;
        }
    }

    fun void line(vec2 dir, float speed, vec2 pos) {
        em.BulletGet(CreateBulletBody(pos, speed*dir, BulletType_Default), BulletType_Default);
    }

    // fire in circle
    fun void circle(int n, float speed, vec2 pos, float start_theta) {
        arc(Math.two_pi, start_theta, n, speed, pos);
    }

    fun void line(vec2 dir, float speed, vec2 pos, int bullet_type) {
        em.BulletGet(CreateBulletBody(pos, speed*dir, bullet_type), bullet_type);
    }

    // create physics body
    b2BodyDef bullet_body_def;
    b2Filter bullet_filter; 
        EntityType_EnemyProjectile => bullet_filter.categoryBits;
        EntityType_Player => bullet_filter.maskBits; // projectiles only collide with player
    b2ShapeDef bullet_shape_def;
        bullet_filter @=> bullet_shape_def.filter;
        true => bullet_shape_def.enableContactEvents;

    b2.makePolygon(c.snipe_bullet_vertices, 0) @=> b2Polygon sniper_bullet_polygon;

    fun int CreateBulletBody(vec2 pos, vec2 vel, int bullet_type) {
        pos => bullet_body_def.position;
        vel => bullet_body_def.linearVelocity;
        M.normalize(vel) => bullet_body_def.rotation;
        b2BodyType.kinematicBody => bullet_body_def.type;
        b2.createBody(b2_world_id, bullet_body_def) => int body_id;

        // geo
        if (bullet_type == BulletType_Default) {
            b2.createCircleShape(body_id, bullet_shape_def, @(0,0), c.bullet_size.val());
        }
        else if (bullet_type == BulletType_Snipe) {
            b2.createPolygonShape(body_id, bullet_shape_def, sniper_bullet_polygon);
        } 
        else if (bullet_type == BulletType_Bar) {
            .5 * c.bar_bullet_len.val() => float l;
            b2.createSegmentShape(body_id, bullet_shape_def, l * DOWN, l * UP);
            // b2Body.angularVelocity(body_id, .8 * (maybe ? 1 : -1));
            // b2Body.angularVelocity(body_id, .2);
        }
        else {
            T.err("CreateBulletBody unimpl bullet type " + bullet_type);
        }

        return body_id;
    }


    // BH Params
    UI_Int num_bullets(5);
    UI_Float speed(1);
    UI_Float start_theta(0);
    UI_Int bullet_type(0);

    // Bullet Sequence params
    UI_Float bs_time_secs(5);
    UI_Float bullets_per_sec(5);
    UI_Float bs_speed(1);
    UI_Int bs_count(1);
    UI_Float bs_arc_len;

    fun void ui() {
        UI.slider("num bullets", num_bullets, 0, 100);
        UI.slider("bullet speed", speed, 0, 10);
        UI.sliderAngle("start theta", start_theta);
        UI.listBox("start theta", bullet_type, bullet_type_names);
        if (UI.button("circle")) circle(num_bullets.val(), speed.val(), @(0,0), 0);
        if (UI.button("arc")) arc(Math.pi/2, start_theta.val(), num_bullets.val(), speed.val(), @(0,0));
        if (UI.button("line")) line(UP, speed.val(), boss.pos()$vec2, bullet_type.val());

        UI.separatorText("bullet stats");
        UI.slider("bullet size", c.bullet_size, 0, 1);

        UI.separatorText("bullet sequences");
        UI.slider("bs dur", bs_time_secs, 0, 20);
        UI.slider("bullets per sec", bullets_per_sec, 0, 5);
        UI.slider("bs bullet speed", bs_speed, 0, 5);
        UI.slider("bs bullet count", bs_count, 0, 20);
        UI.sliderAngle("bs arc len", bs_arc_len);
        if (UI.button("circle seq")) 
            AddBulletSequence(5::second, Timing_QuarterNote, new CircleSequence(.8::second, .1, 8, bs_speed.val()));
        if (UI.button("target seq")) 
            AddBulletSequence(bs_time_secs.val()::second, Timing_SixteenthNote, new TargetSequence(bs_count.val(), bs_speed.val(), bs_arc_len.val()));
        if (UI.button("DNA seq")) 
            AddBulletSequence(bs_time_secs.val()::second, Timing_Period, new DNASequence((1/bullets_per_sec.val())::second, bs_speed.val(), RIGHT));
        if (UI.button("spiral seq")) 
            AddBulletSequence(bs_time_secs.val()::second, Timing_EighthNote, new SpiralSequence(bs_count.val(), bs_speed.val(), 0, .2));

    }

} BH bh;

// ===============================================

// == gamestate ==================================

class GS {
    GameState_Menu => int gamestate;
    float gametime; // total elapsed gametime, scaled by dt_rate. 
    float battletime; // elapsed gametime in battle. reset on death
    1.0 => float dt_rate;
} GS gs;

UI_Bool draw_colliders(false);
UI_Bool draw_visuals(true);

EntityManager em;
em.PlayerAdd(@(0,0)) @=> Entity@ player;
CoolDown player_cd(c.player_fire_cd.val());
4 => int player_hp;
float player_invincibility; // if >0, invincible


GG.scene().ambient(.35*Color.WHITE);
.8 => GG.scene().light().intensity;

// @TODO: try phongmaterial with shading + wireframe or no wireframe
PolyhedronGeometry boss_geo(PolyhedronGeometry.TETRAHEDRON);
GMesh boss(boss_geo, new PhongMaterial) --> GG.scene();
boss.posZ(Layer_Boss);
true => boss.material().wireframe;
// PolyhedronGeometry.TETRAHEDRON, 
// PolyhedronGeometry.CUBE, 
// PolyhedronGeometry.OCTAHEDRON, 
// PolyhedronGeometry.DODECAHEDRON, 
// PolyhedronGeometry.ICOSAHEDRON.
vec2 boss_waveform[s.WINDOW_SIZE]; // plus 1 to connect the ends?
float boss_waveform_theta;

// using UGens to have continuous waveform
SinOsc boss_x(0) => blackhole; .25 => boss_x.phase; // cos
SinOsc boss_y(0) => blackhole;                      // sin
[boss_x.freq(), boss_y.freq()] @=> float boss_movement_freq[];
UI_Float boss_movement_amplitude(0);
UI_Float boss_volume_sca(c.boss_max_volume_sca.val());

// boss stage stuff
5 => int BOSS_N_STAGES;
[
    0::second,
    22::second,
    52::second,
    minute + 21::second,
    minute + 51::second,
    eon,
] @=> dur stage_timings[];

float hp_markers[0];
for (int i; i < stage_timings.size()-1; i++) {
    hp_markers << (stage_timings[i] / s.buf.length());
}
// hp_markers.reverse();
// T.printArray(hp_markers);



1 => float boss_hp; // [0, 1], equiv to song progress
UI_Int boss_stage(-1);
0 => float boss_time_elapsed_curr_stage;

0 => int BossPhase_Combat;
1 => int BossPhase_TransitionShrink;
2 => int BossPhase_TransitionGrow;
int boss_phase;

CoolDown boss_transition_cd(.5 * c.boss_transition_time.val());

[
    "stage 0",
    "stage 1",
    "stage 2",
    "stage 3",
    "stage 4",
] @=> string boss_stage_names[];


Shred@ bullet_sequence_conductor;
fun void SetupBossBulletSequences(int stage) {
    if (bullet_sequence_conductor != null) bullet_sequence_conductor.exit();
    me @=> bullet_sequence_conductor;
    bullet_sequences.clear();
    if (stage == 0) {
        16*s.qt_note_sec::second=>now;
        AddBulletSequence(eon, Timing_SixteenthNote, new TargetSequence(1, 1.0, 0));
        8*s.qt_note_sec::second => now;
        AddBulletSequence(eon, Timing_SixteenthNote, new TargetSequence(2, 2.0, Math.pi/8)) @=> BulletSequence bs;
        c.boss_waveform_radius.val() * RIGHT => bs.spawn_pos_offset;
        4*s.qt_note_sec::second => now;
        AddBulletSequence(eon, Timing_SixteenthNote, new TargetSequence(2, 2.0, Math.pi/8)) @=> bs;
        -c.boss_waveform_radius.val() * RIGHT => bs.spawn_pos_offset;
    }
    if (stage == 1) {
        AddBulletSequence(eon, Timing_EighthNote, new SpiralSequence(2, 1.0, 0, .2));
        5::second => now;
        AddBulletSequence(eon, Timing_EighthNote, new SpiralSequence(3, 1.0, 0, .3)) @=> BulletSequence bs;
        c.boss_waveform_radius.val() => bs.spawn_polar_r;
        5::second => now;
        AddBulletSequence(eon, Timing_EighthNote, new SpiralSequence(3, 1.0, 0, .3)) @=> bs;
        -c.boss_waveform_radius.val() => bs.spawn_polar_r;
    }
    if (stage == 2) {
        4 * s.qt_note_sec::second => now;
        AddBulletSequence(eon, Timing_HalfNote, new SnipeSequence);
        5::second => now;
        // side guys shoot targeted
        AddBulletSequence(eon, Timing_SixteenthNote, new TargetSequence(2, 2.0, Math.pi/8)) @=> BulletSequence bs;
        c.boss_waveform_radius.val() => bs.spawn_polar_r;
        AddBulletSequence(eon, Timing_SixteenthNote, new TargetSequence(2, 2.0, Math.pi/8)) @=> bs;
        -c.boss_waveform_radius.val() => bs.spawn_polar_r;
    }
    if (stage == 3) {
        AddBulletSequence(eon, Timing_HalfNote, new SnipeSequence);

        1.0 => float spiral_speed;
        1 => int n;
        .3 => float dtheta;
        null @=> BulletSequence spiral_center;
        null @=> BulletSequence spiral_left;
        null @=> BulletSequence spiral_right;

        repeat (5) {
            RemoveBulletSequence(spiral_center);
            RemoveBulletSequence(spiral_left);
            RemoveBulletSequence(spiral_right);

            AddBulletSequence(eon, Timing_EighthNote, new SpiralSequence(n, spiral_speed, 0, dtheta)) @=> spiral_center;
            AddBulletSequence(eon, Timing_EighthNote, new SpiralSequence(n, spiral_speed, 0, dtheta)) @=> spiral_left;
            -c.boss_waveform_radius.val() => spiral_left.spawn_polar_r;
            AddBulletSequence(eon, Timing_EighthNote, new SpiralSequence(n, spiral_speed, 0, dtheta)) @=> spiral_right;
            c.boss_waveform_radius.val() => spiral_right.spawn_polar_r;

            5::second => now;

            n++;
            .2 +=> spiral_speed;
            .1 +=> dtheta;
        }
    }
    if (stage == 4) {
        AddBulletSequence(eon, Timing_HalfNote, new BarSequence(2)) @=> BulletSequence bar_seq;
        3::second => now;
        // side guys shoot targeted
        AddBulletSequence(eon, Timing_EighthNote, new TargetSequence(2, 1.0, Math.pi/4)) @=> BulletSequence bs;
        c.boss_waveform_radius.val() => bs.spawn_polar_r;
        3::second => now;
        AddBulletSequence(eon, Timing_EighthNote, new TargetSequence(2, 1.0, Math.pi/4)) @=> bs;
        -c.boss_waveform_radius.val() => bs.spawn_polar_r;
        3::second => now;
        RemoveBulletSequence(bar_seq);
        AddBulletSequence(eon, Timing_HalfNote, new BarSequence(3)) @=> bar_seq;
        3::second => now;
        AddBulletSequence(eon, Timing_HalfNote, new SnipeSequence);
        3::second => now;
        RemoveBulletSequence(bar_seq);
        AddBulletSequence(eon, Timing_HalfNote, new BarSequence(4)) @=> bar_seq;
        AddBulletSequence(eon, Timing_QuarterNote, new SpiralSequence(5, .8, 0, .3)) @=> BulletSequence spiral_center;
        3::second => now;
        RemoveBulletSequence(bar_seq);
        AddBulletSequence(eon, Timing_HalfNote, new BarSequence(6)) @=> bar_seq;

    }
}

fun void BossTransitionStage(int stage) {
    <<< "STAGE TRANSITION TO", boss_stage.val() >>>;
    0 => boss_time_elapsed_curr_stage;
    spork ~ SetupBossBulletSequences(boss_stage.val());
    if (boss_stage.val() > 1) { // stage 0 - 1 don't do anything special because we don't change geometry
        BossPhase_TransitionShrink => boss_phase;
        boss_transition_cd.reset();
    }
    // clear screen
    em.BulletClear();
    if (stage > 0) {
        spork ~ FX.screenFlashEffect(.7::second);
    }
}

fun void BossUpdate(float dt) {
    dt +=> boss_time_elapsed_curr_stage;

    if (boss_phase == BossPhase_TransitionShrink) {
        // transition
        if (boss_transition_cd.active(dt)) {
            boss_transition_cd.reset();
            BossPhase_TransitionGrow => boss_phase;

            // switch to new geometry and movement
            if (boss_stage.val() > 2) {
                boss_geo.build(PolyhedronGeometry.TETRAHEDRON);
            }
            if (boss_stage.val() == 2) {
                boss_geo.build(PolyhedronGeometry.CUBE);
                .1 => boss_y.freq;
                0 => boss_x.freq; 0 => boss_x.phase;
            }
            else if (boss_stage.val() == 3) {
                boss_geo.build(PolyhedronGeometry.OCTAHEDRON);
                .1 => boss_y.freq;
                .1 => boss_x.freq; .25 => boss_x.phase;
            }
            else if (boss_stage.val() == 4) {
                boss_geo.build(PolyhedronGeometry.ICOSAHEDRON);
                .2 => boss_y.freq;
                .1 => boss_x.freq; .25 => boss_x.phase;
            }
            return;
        }

        // interp boss back to center
        .95 * boss_movement_amplitude.val() => boss_movement_amplitude.val;
        // shrink
        (1 - boss_transition_cd.progress) * c.boss_max_volume_sca.val() => boss_volume_sca.val;
    } 
    else if (boss_phase == BossPhase_TransitionGrow) {
        // transition
        if (boss_transition_cd.active(dt)) {
            BossPhase_Combat => boss_phase;
            c.boss_max_volume_sca.val() => boss_volume_sca.val;
            // play music
            // 1 => s.buf.rate;
            return;
        }

        // grow
        boss_transition_cd.progress * c.boss_max_volume_sca.val() => boss_volume_sca.val;
        // grow movement (this is a janky way to do this)
        if (boss_stage.val() >= 2) {
            boss_transition_cd.progress => boss_movement_amplitude.val;
        }
    }

    // else stage is combat

    // update hp
        // calculate current boss stage (based on N_STAGES)
    if (1) { // disabled to manually test boss stages
        boss_stage.val() => int boss_stage_prev;

        // determine current stage
        for (int curr_stage; curr_stage < stage_timings.size()-1; curr_stage++) {
            if (s.SongProgress() < stage_timings[curr_stage+1]) {
                curr_stage => boss_stage.val;
                break;
            }
        }
        T.assert(boss_stage.val() >= boss_stage_prev, "boss stage cannot decrease");

        if (boss_stage.val() != boss_stage_prev && boss_phase == BossPhase_Combat) { // transitioning!
            BossTransitionStage(boss_stage.val());
        }
    }

    boss.rotateZ(.3*dt);
    if (boss_stage.val() >= 1) {
        boss.rotateY(Math.sin(now/second) * dt);
        boss.rotateX(.3*dt);
    }

    boss.sca(boss_volume_sca.val() * (s.volume()));

    boss_movement_amplitude.val() * @(boss_x.last(), boss_y.last()) => boss.pos;

    if (boss_stage.val() >= 1) .5 * dt +=> boss_waveform_theta;
        // change color on beat
    if (boss_stage.val() >= 3 && s.half_note.on && boss_phase == BossPhase_Combat) (boss.material() $ PhongMaterial).color(Color.random());
        // toggle wireframe
    if (boss_stage.val() >= 2 && s.qt_note.on && boss_phase == BossPhase_Combat) (!boss.material().wireframe() => boss.material().wireframe);
}

fun void init(dur d) {
    d => now;
    init();
}

int died_last_run;
fun void init() {
    -1 => boss_stage.val;
    0 => boss.sca;
    1 => boss_hp;
    CENTER => boss.pos;
    @(0, 0, 0) => boss.rot;
    0 => boss_waveform_theta;
    0 => boss_x.freq => boss_y.freq;
    0 => boss_y.phase; 0 => boss_x.phase; // cos
    boss_waveform.zero();
    boss_geo.build(PolyhedronGeometry.TETRAHEDRON);
    true => boss.material().wireframe;
    (boss.material() $ PhongMaterial).color(Color.WHITE);
    BossTransitionStage(0);
    bullet_sequences.clear();

    s.reset();
    GameState_Menu => gs.gamestate;
    em.BulletClear();
    4 => player_hp;
    0 => player_invincibility;
    0 => gs.battletime;
}

init();
while (1) {
    GG.nextFrame() => now;
    GG.dt() * gs.dt_rate => float dt;
    dt +=> gs.gametime;
    if (gs.gamestate == GameState_Battle) dt +=> gs.battletime;

    // == ui ==================================
    if (0)
    {
        UI.setNextWindowBgAlpha(0.00);
        UI.begin("", null, UI_WindowFlags.NoFocusOnAppearing);
        {
            UI.checkbox("draw colliders", draw_colliders);
            UI.checkbox("draw visuals", draw_visuals);
            UI.slider("player zeno", c.player_zeno, 0, 1);

            UI.separatorText("boss");
            UI.text("stage: " + boss_stage.val());
            if (UI.listBox("stage select", boss_stage, boss_stage_names)) {
                BossTransitionStage(boss_stage.val());
            }
            UI.slider("boss waveform radius", c.boss_waveform_radius, 0, 5);
            UI.slider("boss waveform scale", c.boss_waveform_scale, 0, 2);
            UI.slider("boss waveform zeno", c.boss_waveform_zeno, 0, 1);
            UI.slider("boss waveform resolution", c.boss_waveform_resolution, 0, 1);
            UI.slider("boss waveform repeat", c.boss_waveform_repeat, 0, 8);
            UI.slider("boss volume sca", boss_volume_sca, 0, 2);
            UI.slider("movement amplitude", boss_movement_amplitude, 0, 2);
            if (UI.slider("movement xy freq", boss_movement_freq, 0, .5)) {
                boss_movement_freq[0] => boss_x.freq;
                boss_movement_freq[1] => boss_y.freq;
            }

            s.ui();

            UI.separatorText("bullet spawner");
            bh.ui();


            UI.separatorText("profiling");
            UI.text("Entity Map Count: " + em.b2body_to_entity_map.size());
            UI.text("Bullet Pool Size: " + em.bullet_pool_count + "/" + em.bullet_pool.size());
            UI.text("Bullet sequences: " + bullet_sequences.size());
        }
        UI.end();
    }

    // == input ==================================
    {
        player.pos() + c.player_zeno.val() * (g.mousePos() - player.pos()) => player.pos;

        M.angle(player.pos(), CENTER) => float player_angle;
        M.rot2vec(player_angle) => vec2 player_dir;
        // if (GWindow.mouseLeftDown()) spork ~ FX.lightning(boss.pos()$vec2, g.mousePos(), 12, .2, 3);
        if (GWindow.keyDown(GWindow.Key_Space)) 0 => player_hp;
        if (s.qt_note.on) {
            spork ~ FX.lightning(boss.pos()$vec2, g.mousePos(), 12, .2, 3);
            // spork ~ FX.ripple(boss.pos()$vec2, .1, .5::second, .5 * Color.WHITE);
            // spork ~ FX.circleFlash(player.pos() + player_dir * c.player_size.val(), .04, .2::second, .5 * Color.WHITE);
            // spork ~ FX.explode(boss.pos()$vec2, .15::second, Color.WHITE); // @JUICE: color according to bullet
            // spork ~ FX.explode(player.pos(), .15::second, Color.WHITE); // @JUICE: color according to bullet
            1 - (s.buf.pos()$float / s.buf.samples()) => boss_hp;

            if (boss_hp <= 0) {
                false => died_last_run;
                spork ~ FX.explode(boss.pos()$vec2, 1::second, Color.WHITE); // @JUICE: color according to bullet
                spork ~ FX.screenFlashEffect(.5::second);
                em.BulletClear();
                spork ~ init(1::second);
            }
        }

        if (gs.gamestate == GameState_Menu) {
            if (GWindow.mouseLeftDown()) {
                // transition to battle
                1 => s.buf.rate;
                GameState_Battle => gs.gamestate;
            }
        }

        // TODO: experiment with using arrow keys to move player radially 
        // (left/right rotate angle, up down change radius from center)
    }

    // == physics update contact events ==========
    if (gs.gamestate == GameState_Battle && player_hp > 0) {
        b2World.contactEvents(b2_world_id, begin_contact_events, null, null);
        int bullet_body_id;
        for (int i; i < begin_contact_events.size(); 2 +=> i) {
            <<< "contact event" >>>;
            begin_contact_events[i] => int touch_shape_a;
            begin_contact_events[i+1] => int touch_shape_b;
            if (!b2Shape.isValid(touch_shape_a) || !b2Shape.isValid(touch_shape_b)) continue;
            b2Shape.body(touch_shape_a) => int touch_body_id_a;
            b2Shape.body(touch_shape_b) => int touch_body_id_b;
            if (!b2Body.isValid(touch_body_id_a) || !b2Body.isValid(touch_body_id_b)) continue;

            T.assert(touch_body_id_a == player.b2_body_id || touch_body_id_b == player.b2_body_id, "contact does not include player");
            <<< "player hit" >>>;

            if (touch_body_id_a == player.b2_body_id) touch_body_id_b => bullet_body_id;
            else                                      touch_body_id_a => bullet_body_id;
            
            b2Body.position(bullet_body_id) => vec2 bullet_pos;
            em.BulletRet(bullet_body_id);
            spork ~ FX.explode(.5 * (player.pos() + bullet_pos), 1::second, Color.WHITE); // @JUICE: color according to bullet

            // process player damage
            player_invincibility > 0 => int invincible;
            if (invincible) {
                // nothing
            } else {
                player_hp--;
                // @JUICE: time warp
                spork ~ FX.cameraShakeEffect(.08, .5 * c.player_invincibility_time.val()::second, 30);
                spork ~ FX.screenFlashEffect(.2::second);
                spork ~ slowEffect(.15, 2::second);
                // set invincible
                c.player_invincibility_time.val() => player_invincibility;
            }

        }
    }

    // == update =================================
    if (gs.gamestate == GameState_Battle) { 
        // player update
        if (player_hp <= 0) {
            // die
            true => died_last_run;
            init();
            continue;
        }


        // update sound
        s.update(dt, gs.dt_rate);

        BossUpdate(dt);

        // boss waveform
        (s.WINDOW_SIZE * c.boss_waveform_resolution.val()) $ int => int samps_per_repeat;
        samps_per_repeat * c.boss_waveform_repeat.val() => boss_waveform.size;
        // now/second => float start_theta;
        boss_waveform_theta => float start_theta;
        for (int i; i < samps_per_repeat; i++) {
            // linear interpolate sample
            s.sampler_waveform[i * s.sampler_waveform.size() / samps_per_repeat] => float sample;

            // repeat
            for (int j; j < c.boss_waveform_repeat.val(); j++) {

                j * Math.two_pi / c.boss_waveform_repeat.val() => float repeat_start_theta;

                i * Math.two_pi / boss_waveform.size() - (start_theta) + repeat_start_theta => float theta; // [-pi, pi]
                (c.boss_waveform_radius.val() + c.boss_waveform_scale.val() * sample) => float r;
                r * @(Math.cos(theta), Math.sin(theta)) // @optimize use speedy cos/sin
                    + boss.pos() $ vec2  // center on boss
                    + @(0,0)  // offset
                    => vec2 target;
                
                boss_waveform[j * samps_per_repeat + i] => vec2 start;
                start + c.boss_waveform_zeno.val() * (target - start) => vec2 v; 

                v => boss_waveform[j * samps_per_repeat + i];
            }
        }

        UpdateBulletSequence(s, dt, boss.pos() $ vec2, player.pos(), boss_waveform_theta);

        // update bullets
        for (em.bullet_pool_count - 1 => int i; i >= 0; i--) {
            em.bullet_pool[i] @=> Entity b;
            if (g.offscreen(b.pos(), 1)) {
                em.BulletRet(b);
                continue;
            }
            
            if (b.bullet_type == BulletType_Snipe) {
                if (
                    b.sniper_bullet_phase == SniperBulletPhase_Spawn && s.qt_note.on
                    && GG.fc() != b.frame_spawned
                ) {
                    SniperBulletPhase_Aim => b.sniper_bullet_phase;
                    @(0, 0) => b.vel; // stop moving
                }
                else if (b.sniper_bullet_phase == SniperBulletPhase_Aim) {
                    M.dir(b.pos(), player.pos()) => vec2 target_rot;
                    b.rot() + .8 * (target_rot - b.rot()) => b.rot;
                    if (s.half_note.on) {
                        SniperBulletPhase_Launch => b.sniper_bullet_phase;
                        Math.random2f(.9, 1.1) * c.sniper_bullet_speed.val() * b.rot() => b.vel;
                    }
                }
            }
        }

        // update player
        dt -=> player_invincibility;
        if (player_cd.active(dt)) {
            // fire proj
        }

    }

    // == draw ===================================
    if (draw_visuals.val()) {

        if (gs.gamestate == GameState_Menu) {
            g.pushFont("chugl:proggy-tiny");
            .01 * Math.sin(2*2.1667*gs.gametime) + .3 => float sz;
            if (died_last_run) g.text("click to retry", CENTER, sz);
            else               g.text("click to begin", CENTER, sz);
            g.popFont();
        }


        // draw player
        if (player_hp > 0 && gs.gamestate == GameState_Battle) {
            Math.pi/4 + M.angle(player.pos(), CENTER) => float player_angle;
            player_invincibility > 0 => int invincible;

            if (invincible) {
                g.pushBlend(g.BLEND_ALPHA);
                g.pushAlpha(.1);
            }

            c.player_rounding.val() => float r;
            (1 - r) * c.player_size.val() => float l;
            g.pushPolygonRadius(r * c.player_size.val());
            g.squareFilled(player.pos(), player_angle, l, Color.WHITE);
            g.popPolygonRadius();

            if (invincible) {
                g.popBlend();
                g.popAlpha();
            }

            // draw hp bar
            c.player_size.val() => float sz;
            for (int i; i < player_hp; ++i) {
                M.rot2vec(player_angle + (i * Math.two_pi / 4)) => vec2 dir;
                M.perp(dir) => vec2 perp;
                M.rot2vec(Math.pi/4 + player_angle + (i * Math.two_pi / 4)) => vec2 dir2;


                g.pushLayer(Layer_DebugDraw);
                // g.circleFilled(
                //     player.pos() + .4* sz * dir2,
                //     .12 * sz,
                //     Color.BLACK
                // );
                .75 + s.volume() => float sca;
                g.line(
                    player.pos() + sca * sz * dir - .5 * perp * sz,
                    player.pos() + sca * sz * dir + .5 * perp * sz,
                    Color.WHITE
                );
                g.popLayer();
            }
        }

        // draw waveform
        g.line(boss_waveform, true);

        // draw bullets
            // need to walk backwards to safely delete
        // .3 + .4 * s.qt_note.progress => float inner_r;
        g.pushLayer(Layer_EnemyProjectile);
        for (em.bullet_pool_count - 1 => int i; i >= 0; i--) {
            em.bullet_pool[i] @=> Entity b;
            if (b.bullet_type == BulletType_Default) {
                g.pushBlend(g.BLEND_ADD);
                g.circleFilled(b.pos(), c.bullet_size.val(), .2 * Color.RED);
                g.popBlend();

                // g.pushLayer(Layer_DebugDraw);
                // g.circle(b.pos(), c.bullet_size.val(), .1, .1 * Color.RED);
                // g.popLayer();

                // @BUG the layering isn't working....
                g.pushLayer(Layer_DebugDraw);
                g.circle(b.pos(), c.bullet_size.val(), .1, .2 * Color.RED);
                g.popLayer();
            }
            else if (b.bullet_type == BulletType_Snipe) {
                g.pushColor(Color.ORANGE);
                g.polygonFilled(b.pos(), b.rot(), c.snipe_bullet_vertices, 0);
                g.popColor();
            }
            else if (b.bullet_type == BulletType_Bar) {
                g.pushPolygonRadius(.025);
                g.boxFilled(
                    b.pos(),
                    b.rot(),
                    .05,
                    c.bar_bullet_len.val(),
                    Color.YELLOW
                );
                g.popPolygonRadius();
            }
            // g.circleFilled(b.pos(), inner_r * c.bullet_size.val(), Color.WHITE);
            // g.circleFilled(b.pos(), c.bullet_size.val(), Color.WHITE);
            // g.circleFilled(b.pos(), .7 * c.bullet_size.val(), Color.RED);
        }
        g.popLayer();

        // draw boss bullet spawner
        s.volume(0) => float vl;
        s.volume(1) => float vr;
        c.bullet_size.val() * 2 + .35 * vl * vl => float r_left;
        c.bullet_size.val() * 2 + .35 * vr * vr => float r_right;
        g.pushLayer(Layer_Boss);
        c.boss_waveform_radius.val() * M.rot2vec(boss_waveform_theta) => vec2 pos;
        g.circleFilled(boss.pos()$vec2 + pos, r_right, Color.BLACK);
        g.circleDotted(boss.pos()$vec2 + pos, r_right, gs.battletime, Color.WHITE);

        g.circleFilled(boss.pos()$vec2 - pos, r_left, Color.BLACK);
        g.circleDotted(boss.pos()$vec2 - pos, r_left, -gs.battletime, Color.WHITE);
        g.popLayer();
        // g.circleFilled(boss.pos()$vec2 + c.boss_waveform_radius.val() * RIGHT, 
        //     c.bullet_size.val() * 2, Color.WHITE);

        // boss hp bar (TODO make radial)
        // OR idea: instead of UI, make the stages a clear part of the game world...
        // e.g. in the distance the background starts to change color, next stage is when the new background arrives ? 
        g.progressBar(boss_hp, g.ndc2world(0, .8), 5, .4, Color.WHITE, 
            M.lerp(s.qt_note.progress, Color.BLACK, Color.RED), hp_markers);
    }

    // == bookkeep ===================================
    {
        if (draw_colliders.val()) {
            b2World.draw(b2_world_id, debug_draw);
            debug_draw.update();
        }
    }
}
