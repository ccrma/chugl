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
9 => int Layer_EnemyProjectile;
10 => int Layer_DebugDraw;

// ===========================================

// == Util ==================================
class CoolDown {
    float max_secs;
    float curr;

    fun @construct(float secs) {
        secs => max_secs;
        secs => curr;
    }

    fun int active(float dt) {
        dt -=> curr;
        if (curr <= 0) {
            max_secs => curr;
            return true;
        }
        return false;
    }
}

// == sound ====================================
class Sound {
    512 => int WINDOW_SIZE; 
    Flip sampler_waveform_accum;
    Windowing.hann(WINDOW_SIZE) @=> float hann_window[];
    float sampler_waveform[0];

    WINDOW_SIZE => sampler_waveform_accum.size;
    SndBuf buf(me.dir() + "./assets/another-medium.wav") => sampler_waveform_accum => blackhole; // analysis
    buf => dac;

    fun void update() {
        // only call this once per frame since it's just used for graphical updates
        sampler_waveform_accum.upchuck();
        sampler_waveform_accum.output( sampler_waveform );

        // taper (looks bad)
        // for (int s; s < sampler_waveform.size(); s++) {
        //     hann_window[s] *=> sampler_waveform[s];
        // }
    }
} 
Sound s;

// == Graphics ==================================
class MyG2D extends G2D {};
MyG2D g;
g @=> FX.g;

// g.resolution(512, 512);
// g.antialias(false);
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
    UI_Float player_size(.2);
    UI_Float player_rounding(.1); // from 0 to 1. 0 is circle, 1 is square
    UI_Float player_zeno(.5);
    UI_Float player_fire_cd(.2);

    UI_Float boss_waveform_radius(1.2);
    UI_Float boss_waveform_scale(1.5); // how much its affected by waveform data
    UI_Float boss_waveform_zeno(.15); // how much its affected by waveform data
    UI_Float boss_waveform_resolution(.5); // downsample the raw waveform data by this factor
    UI_Int boss_waveform_repeat(2); // downsample the raw waveform data by this factor

} Constants c;

class Entity {
    // for pool
    int _idx;

    int entity_type;

    // physics
    int b2_body_id;


    fun vec2 pos() { return b2Body.position(this.b2_body_id); }
    fun void pos(vec2 p) { b2Body.position(this.b2_body_id, p); }
    fun vec2 rot() { return b2Body.rotation(this.b2_body_id); }
    fun void rot(vec2 rot) { b2Body.rotation(this.b2_body_id, rot); }
}

class EntityManager {
    HashMap b2body_to_entity_map;

    Entity bullet_pool[0]; // NOTE: walk array backwards! to avoid skipping elements
    int bullet_pool_count;

    fun Entity BulletGet(int b2_body_id) {
        if (bullet_pool_count >= bullet_pool.size()) {
            bullet_pool << new Entity;
            bullet_pool_count => bullet_pool[-1]._idx;
        }

        T.assert(bullet_pool[bullet_pool_count]._idx == bullet_pool_count, "bullet add");
        bullet_pool[bullet_pool_count++] @=> Entity bullet;
        
        T.assert(bullet.b2_body_id == 0, "bulletGet b2body id not 0");
        b2_body_id => bullet.b2_body_id;
        b2body_to_entity_map.set(b2_body_id, bullet);

        return bullet;
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


    fun Entity PlayerAdd(vec2 pos) {
        b2BodyDef player_body_def;
        pos => player_body_def.position;
        M.rot2vec(Math.pi/4) => player_body_def.rotation;
        b2BodyType.kinematicBody => player_body_def.type;

        // entity
        Entity player;
        EntityType_Player => player.entity_type;
        b2.createBody(b2_world_id, player_body_def) => player.b2_body_id;
        b2body_to_entity_map.set(player.b2_body_id, player);

        // filter
        b2Filter player_filter;
        EntityType_Player => player_filter.categoryBits;

        // shape
        b2ShapeDef player_shape_def;
        player_filter @=> player_shape_def.filter;
        true => player_shape_def.enableContactEvents;

        // geo
        c.player_rounding.val() => float r;
        .8 * c.player_size.val() => float sz;
        b2.makeRoundedBox(
            (1 - r) * sz, (1 - r) * sz,
            r * sz
        ) @=> b2Polygon player_geo;
        b2.createPolygonShape(player.b2_body_id, player_shape_def, player_geo); // => player.b2_shape_id;

        return player;
    }
}

// == bullets ================================================

BulletSequence bullet_sequences[0];

fun void AddBulletSequence(dur max_dur, BulletSequence bs) {
    bullet_sequences << bs;
    max_dur / second => bs.max_runtime_secs;
    0 => bs.elapsed_time_secs;
}

fun void UpdateBulletSequence(float dt, vec2 pos) {
    // need to walk backwards to safely delete
    for (bullet_sequences.size() - 1 => int i; i >= 0; i--) {
        bullet_sequences[i] @=> BulletSequence bs;
        // check if time has run out
        dt +=> bs.elapsed_time_secs;
        if (bs.elapsed_time_secs >= bs.max_runtime_secs) {
            // delete
            bullet_sequences[-1] @=> bullet_sequences[i];
            bullet_sequences.popBack();
        } else {
            bs.update(dt, pos);
        }
    }
}

class BulletSequence {
    "base bullet sequence" => string name;
    float max_runtime_secs;
    float elapsed_time_secs;

    fun void update(float dt, vec2 pos) { T.err("BulletSequence update not impl"); }
}

class CircleSequence extends BulletSequence {
    "circle seq" => name;

    // params
    1.0 => float period_secs;
    float delta_theta;
    int n;

    // local
    float cd;
    float start_theta;

    fun @construct(dur period, float delta_theta, int n) {
        n => this.n;
        delta_theta => this.delta_theta;
        period/second => this.period_secs;
    }

    fun void update(float dt, vec2 pos) {
        dt -=> cd;
        if (cd <= 0) {
            period_secs => cd;
            bh.circle(n, 1.0, pos, start_theta);
            delta_theta +=> start_theta;
        }
    }
}

class BH {

    // fire points in a polygonal shape
    fun void ngon() {

    }

    // fire in arc
    fun void arc(float arc_len, float start_theta, int n, float speed, vec2 pos) {
        for (int i; i < n; i++) {
            @(
                Math.cos(arc_len * (i - n/2) / n + start_theta), 
                Math.sin(arc_len * (i - n/2) / n + start_theta)
            ) => vec2 dir;
            em.BulletGet(CreateBulletBody(pos, speed*dir)) @=> Entity b;
        }
    }

    // fire in circle
    fun void circle(int n, float speed, vec2 pos, float start_theta) {
        arc(Math.two_pi, start_theta, n, speed, pos);
    }

    // create physics body
    b2BodyDef bullet_body_def;
    b2Filter bullet_filter; 
        EntityType_EnemyProjectile => bullet_filter.categoryBits;
        EntityType_Player => bullet_filter.maskBits; // projectiles only collide with player
    b2ShapeDef bullet_shape_def;
        bullet_filter @=> bullet_shape_def.filter;
        true => bullet_shape_def.enableContactEvents;
    fun int CreateBulletBody(vec2 pos, vec2 vel) {
        pos => bullet_body_def.position;
        vel => bullet_body_def.linearVelocity;
        b2BodyType.kinematicBody => bullet_body_def.type;
        b2.createBody(b2_world_id, bullet_body_def) => int body_id;

        // geo
        b2.createCircleShape(body_id, bullet_shape_def, @(0,0), .5 * c.player_size.val());

        return body_id;
    }


    UI_Int num_bullets(5);
    UI_Float speed(1);
    UI_Float start_theta(0);
    fun void ui() {
        UI.slider("num bullets", num_bullets, 0, 100);
        UI.slider("bullet speed", speed, 0, 10);
        UI.sliderAngle("start theta", start_theta);
        if (UI.button("circle")) circle(num_bullets.val(), speed.val(), @(0,0), 0);
        if (UI.button("arc")) arc(Math.pi/2, start_theta.val(), num_bullets.val(), speed.val(), @(0,0));

        UI.separatorText("bullet sequences");
        if (UI.button("circle seq")) 
            AddBulletSequence(5::second, new CircleSequence(.2::second, .1, 10));

    }

} BH bh;

// ===============================================

// == gamestate ==================================

UI_Bool draw_colliders(true);
UI_Bool draw_visuals(true);

EntityManager em;
em.PlayerAdd(@(0,0)) @=> Entity@ player;
CoolDown player_cd(c.player_fire_cd.val());

GG.scene().ambient(.35*Color.WHITE);
.8 => GG.scene().light().intensity;

// @TODO: try phongmaterial with shading + wireframe or no wireframe
GMesh boss(new PolyhedronGeometry(PolyhedronGeometry.OCTAHEDRON), new FlatMaterial) --> GG.scene();
true => boss.material().wireframe;
// PolyhedronGeometry.TETRAHEDRON, 
// PolyhedronGeometry.CUBE, 
// PolyhedronGeometry.OCTAHEDRON, 
// PolyhedronGeometry.DODECAHEDRON, 
// PolyhedronGeometry.ICOSAHEDRON.

vec2 boss_waveform[s.WINDOW_SIZE]; // plus 1 to connect the ends?

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    // == ui ==================================
    UI.setNextWindowBgAlpha(0.00);
    UI.begin("", null, UI_WindowFlags.NoFocusOnAppearing);
    {
        UI.checkbox("draw colliders", draw_colliders);
        UI.checkbox("draw visuals", draw_visuals);
        UI.slider("player zeno", c.player_zeno, 0, 1);
        UI.slider("boss waveform radius", c.boss_waveform_radius, 0, 5);
        UI.slider("boss waveform scale", c.boss_waveform_scale, 0, 2);
        UI.slider("boss waveform zeno", c.boss_waveform_zeno, 0, 1);
        UI.slider("boss waveform resolution", c.boss_waveform_resolution, 0, 1);
        UI.slider("boss waveform repeat", c.boss_waveform_repeat, 0, 8);

        UI.separatorText("bullet spawner");
        bh.ui();


        UI.separatorText("profiling");
        UI.text("Entity Map Count: " + em.b2body_to_entity_map.size());
        UI.text("Bullet Pool Size: " + em.bullet_pool_count + "/" + em.bullet_pool.size());
        UI.text("Bullet sequences: " + bullet_sequences.size());
    }
    UI.end();

    // == input ==================================
    {
        player.pos() + c.player_zeno.val() * (g.mousePos() - player.pos()) => player.pos;
    }

    // == update =================================
    { 
        // update sound
        s.update();

        // update boss
        boss.posY(Math.sin(now/second));    
        boss.rotateY(Math.sin(now/second) * dt);
        boss.rotateX(.3*dt);
        boss.rotateZ(.3*dt);

        (s.WINDOW_SIZE * c.boss_waveform_resolution.val()) $ int => int samps_per_repeat;
        samps_per_repeat * c.boss_waveform_repeat.val() => boss_waveform.size;
        now/second => float start_theta;
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

        UpdateBulletSequence(dt, boss.pos() $ vec2);

        // update bullets
        for (em.bullet_pool_count - 1 => int i; i >= 0; i--) {
            em.bullet_pool[i] @=> Entity b;
            if (g.offscreen(b.pos(), 1)) {
                em.BulletRet(b);
                continue;
            }
        }

        // update player
        if (player_cd.active(dt)) {
            // fire proj
        }

    }

    // == draw ===================================
    if (draw_visuals.val()) {
        // draw player
        {
            c.player_rounding.val() => float r;
            (1 - r) * c.player_size.val() => float l;
            g.pushPolygonRadius(r * c.player_size.val());
            g.squareFilled(player.pos(), Math.pi/4, l, Color.WHITE);
            // spork ~ FX.booster(player.pos(), Color.WHITE, l);
        }

        // draw waveform
        g.line(boss_waveform, true);
    }

    // == bookkeep ===================================
    {
        if (draw_colliders.val()) {
            b2World.draw(b2_world_id, debug_draw);
            debug_draw.update();
        }
    }
}
