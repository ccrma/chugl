@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "../lib/whisper/Whisper.chug"
@import "../lib/tween.ck"
@import "../lib/b2/b2DebugDraw.ck"
@import "HashMap.chug"
@import "voice.ck"


Tween t;
VoiceCommand voice(me.dir() + "../lib/whisper/models/ggml-base.en.bin");

GWindow.windowed(1920, 1080);
GWindow.center();

// GWindow.fullscreen();
BardG2D g;
g.sortDepthByY(true);
GText.defaultFont(me.dir() + "./assets/m5x7.ttf");

// g.resolution(1920 / 2, 1080 / 2);
// g.antialias(false);

GG.camera().viewSize(18);

// == load assets ================================================
TextureLoadDesc tex_load_desc;
true => tex_load_desc.flip_y;
false => tex_load_desc.gen_mips;
Texture.load(me.dir() + "./assets/bard_new.png", tex_load_desc) @=> Texture bard_sprite;
Texture.load(me.dir() + "./assets/knight.bmp", tex_load_desc) @=> Texture knight_sprite;
Texture.load(me.dir() + "./assets/ground.png", tex_load_desc) @=> Texture ground_texture;
Texture.load(me.dir() + "./assets/cart_small.png", tex_load_desc) @=> Texture cart_sprite;
Texture.load(me.dir() + "./assets/wheel_small.png", tex_load_desc) @=> Texture wheel_sprite;
Texture.load(me.dir() + "./assets/chisel.png", tex_load_desc) @=> Texture chisel_sprite;
Texture.load(me.dir() + "./assets/arrow.png", tex_load_desc) @=> Texture arrow_sprite;
Texture.load(me.dir() + "./assets/prisoner_walk.png", tex_load_desc) @=> Texture prisoner_sprite;

// enemy sprites
Texture.load(me.dir() + "./assets/enemy.png", tex_load_desc) @=> Texture charger_sprite;

// == custom graphics ================================================
class BardG2D extends G2D {
    fun void progressBar(
        float percentage, vec2 pos, float width, float height, vec3 color
    ) {
        // g.NDCToWorldPos(pos_ndc.x, pos_ndc.y) => vec2 pos;
        // g.NDCToWorldPos(width_ndc, height_ndc) => vec2 hw_hh;
        // hw_hh.x => float hw;
        // hw_hh.y => float hh;
        // shift pos.x to center
        // hw +=> pos.x;
        width * .5 => float hw;
        height * .5 => float hh;

        g.box(pos, 2 * hw, 2 * hh, color);

        -hw + (percentage * 2 * hw) => float end_x;
        g.boxFilled(
            pos - @(hw, hh),   // bot left
            pos + @(end_x, hh), // top right
            color
        );
    }

    static UI_Float shadow_height(.1);
    fun void shadow(vec2 pos, float w) {
        g.pushLayer(Layer_Shadow);
        g.pushBlend(g.BLEND_MULT);
        g.ellipseFilled(pos, @(.5*w, .5 * shadow_height.val()), .1 * Color.WHITE);
        g.popBlend();
        g.popLayer();
    }

    fun void attackTell(vec2 pos, float w, float h) {
        g.pushLayer(Layer_Shadow);
        // transparent inside
        g.pushBlend(g.BLEND_ALPHA);
        g.ellipseFilled(pos, @(.5*w, .5 *h), @(1, 0, 0, .1));
        g.popBlend();
        // draw thicker outline
        g.ellipse(pos, @(.5*w, .5 * h), .02, @(1, 0, 0, 1));
        g.popLayer();
    }

    fun void chevron(
        vec2 pos, float rot_rad, float angle_rad, float height, float thickness, vec3 color
    ) {
        height * .5 => float hh;
        Math.tan(angle_rad * .5) => float tan_theta;
        // sadly polygons only supports convex shapes (which a chevron is not)
        // so drawing as two thick lines with miter join
        // @TODO how to support non-convex polygons? check freya shapes
        // does it require earcut triangulation?
        g.pushColor(color);
        polygonFilled(pos, rot_rad, 
            [
                @(0, hh),
                @(hh / tan_theta, 0),
                @(thickness + hh / tan_theta, 0),
                @(thickness, hh),
            ],
            0);
        polygonFilled(pos, rot_rad, 
            [
                @(hh / tan_theta, 0),
                @(0, -hh),
                @(thickness, -hh),
                @(thickness + hh / tan_theta, 0),
            ],
            0);
        g.popColor();
    }
}

// == room ================================================
Room rooms[0];
string room_names[0];
UI_Int curr_room_idx(1);

class Room {
    0 => int _room_idx;
    "default room" => string room_name;
    fun void ui() {}
    fun void enter() {}
    fun void leave() {}
    fun Room update(float dt) { return null; } // returns true if end state is met
}

fun void RoomAdd(Room r) {
    rooms.size() => r._room_idx;
    rooms << r;
    room_names << r.room_name;
}

fun void RoomEnter(Room r) {
    T.assert(r._room_idx < rooms.size(), "invalid room idx");

    rooms[curr_room_idx.val()].leave();
    r.enter();
    r._room_idx => curr_room_idx.val;
}

// == FX ================================================

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

    fun static void booster(vec2 pos, vec3 color, float radius) {
        Math.random2f(.98 * radius, radius) => float init_radius;
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

    fun static void heal(vec2 pos, float radius, int layer, dur effect_dur) {
        dur elapsed_time;
        while (elapsed_time < effect_dur) {
            GG.nextFrame() => now;
            GG.dt()::second +=> elapsed_time;
            1.0 - elapsed_time / effect_dur => float t;

            g.pushLayer(layer); 

            g.circleFilled(pos, radius, @(0, t, 0, .25 * t));

            g.popLayer();
        }
    }

    fun static void laser(vec2 start, vec2 end, float size) {
        <<< "casting laser" >>>;
        M.dir(start, end) => vec2 rot;
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

    fun static void ripple(vec2 pos, float end_radius, dur effect_dur, vec3 color) {
        dur elapsed_time;
        while (elapsed_time < effect_dur) {
            GG.nextFrame() => now;
            GG.dt()::second +=> elapsed_time;
            M.easeOutQuad(elapsed_time / effect_dur) => float t;

            g.circle(pos, end_radius * t, .1 * (1 - t), color);
        }
    }

    fun static void bulletHitEffect(vec2 pos, float scale) {
        dur elapsed_time;
        while (elapsed_time < .28::second) {
            GG.nextFrame() => now;
            GG.dt()::second +=> elapsed_time;

            if (elapsed_time < .12::second) 
                g.squareFilled( pos, 0, scale, Color.WHITE);
            else g.squareFilled(pos, 0, scale, Color.RED);
        }
    }

    fun static void circleFlash(vec2 pos, float radius, dur d, vec3 color) {
        dur elapsed_time;
        while (elapsed_time < d) {
            GG.nextFrame() => now;
            GG.dt()::second +=> elapsed_time;

            // flash a transparent explosion circle
            g.pushBlend(g.BLEND_ADD);
            g.circleFilled(pos, radius, color);
            g.popBlend();
        }
    }

    // algo: divide the line into n segments, jitter, and flash/dissolve
    fun static void lightning(vec2 pos, int n_segments, float rand_displacement_max, float fade_secs) {
        vec2 vertices[0];
        // assum lightning goes straight down from same x position at top of screen
        @(
            pos.x,
            g.ndc2world(@(0,1)).y
        ) => vec2 start;
        vertices << start;

        Math.fabs(pos.y - start.y) / n_segments => float dy;

        for (int i; i < n_segments - 1; i++) {
            start.y - (i * dy + Math.random2f(0, dy)) => float y;
            // displace along normal
            pos.x + Math.random2f(-rand_displacement_max, rand_displacement_max) => float x;
            vertices << @(x, y) << @(x, y);
        }

        // final endpoint
        vertices << pos;    

        // draw and animate
        float elapsed;
        while (1) {
            GG.nextFrame() => now;
            GG.dt() +=> elapsed;
            elapsed / fade_secs => float t; // try different easings
            if (t > 1) break;

            for (int i; i < vertices.size(); 2 +=> i) {
                vertices[i] + (1-t) * (vertices[i+1] - vertices[i]) => vertices[i+1];

                // rotate?

                // flicker, thank u penus mike
                if (maybe) g.line(vertices[i], vertices[i+1]);
            }
        }
    }

    // @improvement: after refactoring to FX manager,
    // have option to set timescale *for each FX individually*
    // then this one can play at a lower framerate, meaning the random
    // flicker is controllable and slow-able
    fun static void dashTrail(vec2 start, vec2 end_pos) {
        M.dist(start, end_pos) => float dist;
        M.dir(start, end_pos) => vec2 dir;
        M.perp(dir) => vec2 perp;

        float elapsed;
        .32 => float total_time;
        2 => int n_segments;
        dist / n_segments => float seg_len;
        
        while (elapsed < total_time) {
            GG.nextFrame() => now;
            GG.dt() +=> elapsed;
            (elapsed / total_time) => float t;

            g.pushLayer(Layer_Weapon);
            for (int j; j < 3; j++) {
                for (int i; i < n_segments; i++) {
                    perp * Math.random2f(-.2, .2) + 
                        start + Math.random2f(i, i + 1) * seg_len * dir => vec2 s;
                    g.line(
                        s,
                        s + (1-t) * dir * seg_len
                    );
                }
            }
            g.popLayer();
        }
    }

    fun static void particleTrail(vec2 start, vec2 end, int layer) {
        M.dir(start, end) => vec2 dir;
        M.perp(dir) => vec2 perp;

        // @improve: once function ptrs are added, can have a generic trail rendering
        // function that uses a function ptr as draw method
        .06 => float r;
        .5 => float total_time;
        4 => int n;
        vec2 pos_history[n];
        int history_idx;
        for (int i; i < pos_history.size(); ++i) start => pos_history[i];

        float elapsed;
        while (elapsed < total_time) {
            GG.nextFrame() => now;
            GG.dt() +=> elapsed;

            // linear for now
            elapsed / total_time => float t;

            g.pushLayer(layer);
            start + t * (end - start) => vec2 pos;
            pos => pos_history[history_idx++];
            if (history_idx >= pos_history.size()) 0 => history_idx;

            // draw head
            // g.circleFilled(pos, r, Color.WHITE);

            // draw trail
            // @TODO another transparency bug. trail doesn't blend with ground texture
            g.pushColor(Color.WHITE);
            for (int i; i < pos_history.size(); ++i) {
                (i + history_idx) % pos_history.size() => int idx;
                pos_history[idx] => vec2 p;
                r * i / n$float => float scale;
                g.pushStrip(p + scale * perp);
                g.pushStrip(p - scale * perp);
            }
            g.endStrip();

            g.popColor();
            g.popLayer();
        }
    }
}

// == constants ================================================

@(1,0) => vec2 RIGHT;
@(-1,0) => vec2 LEFT;
@(0,1) => vec2 UP;
@(0,-1) => vec2 DOWN;
6 => int MAX_ACTIVE_UPGRADES;

// == enums ================================================

HashMap entity_type_names;

0 =>        int EntityType_None;    entity_type_names.set(EntityType_None, "None");
(1 << 0) => int EntityType_Static;      entity_type_names.set(EntityType_Static, "Static");
(1 << 1) => int EntityType_Player;      entity_type_names.set(EntityType_Player, "Player");
(1 << 2) => int EntityType_Cart;    entity_type_names.set(EntityType_Cart, "Cart");
(1 << 3) => int EntityType_Enemy;   entity_type_names.set(EntityType_Enemy, "Enemy");
(1 << 4) => int EntityType_Pickup;      entity_type_names.set(EntityType_Pickup, "Pickup");
(1 << 5) => int EntityType_Weapon;      entity_type_names.set(EntityType_Weapon, "Weapon");
(1 << 6) => int EntityType_PlayerProjectile;    entity_type_names.set(EntityType_PlayerProjectile, "PlayerProjectile");
(1 << 7) => int EntityType_EnemyProjectile;     entity_type_names.set(EntityType_EnemyProjectile, "EnemyProjectile");
(1 << 8) => int EntityType_Spell;   entity_type_names.set(EntityType_Spell, "Spell");
(1 << 9) => int EntityType_EnemyShadow;  entity_type_names.set(EntityType_EnemyShadow, "EnemyShadow");

0 =>        int PlayerType_None;
1 =>        int PlayerType_Bard;
2 =>        int PlayerType_Carpenter;

0 =>        int EnemyType_None;
1 =>        int EnemyType_Basic;
2 =>        int EnemyType_Pursuer;
3 =>        int EnemyType_Charger;
4 =>        int EnemyType_Shooter;

// enemy state machine
0 => int EnemyState_Default; 
1 => int EnemyState_Pre; // charging up an attack, the "tell" phase
2 => int EnemyState_Act; // doing the attack, e.g. charging at player

-1 =>        int Layer_Background;
0  =>        int Layer_Shadow;
4  =>        int Layer_Pickup;
5  =>        int Layer_Player => int Layer_Enemy;
6  =>        int Layer_Cart;
7  =>        int Layer_Fog => int Layer_Weapon; 
8  =>        int Layer_Light;
9  =>        int Layer_HP;
10  =>        int Layer_Projectile;
11  =>       int Layer_DebugDraw; // drawing b2 physics colliders
12 =>        int Layer_UI;

// @design: is this better or the "smear" melee attack (like in nuclear throne)
0 => int WeaponState_Rest;    // holding at rest position
1 => int WeaponState_Attack;  // lerping to attack, holding, and returning to rest


// upgrade category enum
0 => int UpgradeCategory_None;
1 => int UpgradeCategory_Bard;
2 => int UpgradeCategory_Cart;
3 => int UpgradeCategory_Evo;

// upgrade enum
0 => int Upgrade_None;
1 =>  int Upgrade_Cart_Ambulance;
2 =>  int Upgrade_Cart_Fortress;
3 =>  int Upgrade_Cart_Speed;
4 =>  int Upgrade_Cart_Dodge;
    // bard upgrades
5 =>  int Upgrade_Bard_Fireball;
6 =>  int Upgrade_Bard_Lightning;

10 =>  int Upgrade_Count;

// cart move state enum
"Cart_MoveState_Stop" => string Cart_MoveState_Stop;
"Cart_MoveState_Roam" => string Cart_MoveState_Roam;

// b2 collider shape enums
10 => int ShapeType_Triangle; // starting at 10 because higher than all b2shapeType


// == ui ================================================

// == attack params ===========================================
// experimenting here... thinking all attacks share the same
// base of range / targetting / cooldown / 
// each instance of a weapon would need its own attack params
// because multiple players can have the same weapon with different
// stats...

// and maybe bullets are their own class? or they can just 
// be an Entity...
class AttackParams { 
    UI_Float _dmg;
    UI_Float _dmg_mult(1.0);

    UI_Float _range;
    UI_Float _range_mult(1.0);

    UI_Float _cd_sec;
    UI_Float _atk_speed(1.0); // need to invert in attack speed calc

    float cd;

    fun AttackParams(float dmg, float range, float cd) {
        dmg => this._dmg.val;
        range => this._range.val;
        cd => this._cd_sec.val;
        cd => this.cd;
    }

    fun float dmg() { return this._dmg.val() * this._dmg_mult.val(); }
    fun float range() { return this._range.val() * this._range_mult.val(); }
}

// == upgrades ================================================

Upgrade upgrade_type_list[0];
class Upgrade {
    int upgrade_type;
    int category;
    string title;
    string desc; // @TODO will want an array of desc for each level
    int max_level;
}

fun void UpgradeAddType(
    int upgrade_type, int category, string title, string desc, int max_level
) {
    Upgrade u;
    upgrade_type => u.upgrade_type;
    category => u.category;
    title => u.title;
    desc => u.desc;
    max_level => u.max_level;
    upgrade_type_list << u;

    T.assert(upgrade_type_list[upgrade_type] == u, "upgrade Add type");
}

UpgradeAddType(Upgrade_None, UpgradeCategory_None, "", "", 0);
UpgradeAddType(Upgrade_Cart_Ambulance, UpgradeCategory_Cart, "Ambulance", "Cart heals nearby players", 3);
UpgradeAddType(Upgrade_Cart_Fortress, UpgradeCategory_Cart, "Ballistics", "Weaponize the cart", 3);
UpgradeAddType(Upgrade_Cart_Speed, UpgradeCategory_Cart, "Cart Speed", "", 3);
UpgradeAddType(Upgrade_Cart_Dodge, UpgradeCategory_Cart, "Cart Dodge", "", 3);

// bard upgrades
UpgradeAddType(Upgrade_Bard_Fireball, UpgradeCategory_Bard, "Fireball", "", 5);
UpgradeAddType(Upgrade_Bard_Lightning, UpgradeCategory_Bard, "Lightning", "", 5);

// == player type ================================================


class Player {
    int player_type;
    string name;

    float base_hp;
    UI_Float base_speed;

    // shared stats
    static UI_Float base_pickup_radius(1.06);
    static UI_Float base_inspo_cd(1); // tick rate for generating inspo

    fun static void ui() {
        if (!UI.collapsingHeader("player type", 0)) return;

        UI.slider("pickup radius", base_pickup_radius, 0, 2);


        UI.separatorText("player-specific");
        for (1 => int i; i < player_type_list.size(); ++i) {
            player_type_list[i] @=> Player p;

            UI.pushID(p.name);

            UI.text(p.name);
            UI.slider("base speed", p.base_speed, 0, 10);
            UI.separator();
            UI.popID();
        }
    }
}

Player player_type_list[0];
fun void PlayerAddType(
    int player_type, float base_hp, float base_speed, string name
) {
    Player p;

    player_type => p.player_type;
    name => p.name;
    base_hp => p.base_hp;
    base_speed => p.base_speed.val;

    player_type_list << p;
    T.assert(player_type_list[player_type] == p, "adding player type out of order");
}

//                                 hp   hp_scaling  dmg  dmg_scaling  drop  speed_range  shape                     size
//            player_type           hp     speed    name
PlayerAddType(PlayerType_None,      0,     0,       "");
PlayerAddType(PlayerType_Bard,      8,     .3,      "Bard");
PlayerAddType(PlayerType_Carpenter, 10,    3.6,      "Carpenter");

// == enemy type ================================================

class Enemy {
    int enemy_type;
    string name;
    float base_hp;
    float hp_scaling;

    float drop; // how much exp/$ it gives on death

    float base_dmg; // dmg dealt via contact OR projectile
    float dmg_scaling; // linearly scales with wave
    UI_Float speed; // range of speed to spawn with @(min, max)

    // collider stuff
    int b2_shape_type;
    UI_Float2 size;          // for capsule: @(p1 -- p2, radius)
                             // for box:     @(w, h)
                             // for circle   @(radius, radius)
    vec2 rotation;

    // for ranged-atk enemies and enemies with abilities
    UI_Float ability_cd;
    UI_Float ability_speed;

    // shared contact dmg cooldown
    static UI_Float contact_dmg_cd(1.0);
    static UI_Float contact_attack_anim_amt(.4);

    // enemy-specific params
        // charger
    static UI_Float charger_tell_dur(2.0);
    static UI_Float charger_charge_dur(2.0);
    static UI_Float charger_charge_cd(5.0);
    static UI_Float charger_charge_torque(1.0);
    static UI_Float charger_charge_impulse(1.0);
    static UI_Float charger_charge_range(2.0);


    fun float dmg(int wave) { return this.base_dmg + wave * this.dmg_scaling; }

    // manipulate enemy params
    fun static void ui() {
        if (!UI.collapsingHeader("enemy type params", 0)) return;

        UI.slider("contact dmg cd(sec)", contact_dmg_cd, .1, 5);
        UI.slider("contact attack anim lurch amt", contact_attack_anim_amt, 0, 1);

        for (1 => int i; i < enemy_type_list.size(); ++i) {
            enemy_type_list[i] @=> Enemy et;
            UI.pushID(et.name);
            UI.text(et.name);
            UI.slider("speed", et.speed, 0, 1.0);

            if (et.ability_cd.val() > 0) {
                UI.slider("ability cd", et.ability_cd, .1, 5);
                UI.slider("ability speed", et.ability_speed, .1, 10);
            }

            if (et.enemy_type == EnemyType_Charger) {
                UI.slider("charge range", charger_charge_range, 0, 10);
                UI.slider("tell dur", charger_tell_dur, 0, 5);
                UI.slider("charge dur", charger_charge_dur, 0, 5);
                UI.slider("charge cd", charger_charge_cd, 0, 10);
                UI.slider("charge torque", charger_charge_torque, 0, 10);
                UI.slider("charge impulse", charger_charge_impulse, 0, 10);
            }

            UI.separator();
            UI.popID();
        }
    }
}

Enemy enemy_type_list[0];
fun void EnemyAddType(
    int enemy_type, string name, float base_hp, float hp_scaling, float dmg, float dmg_scaling, float drop, float speed,
    int b2_shape_type, vec2 size, float rot, 
    float cd, float ability_speed
) {
    Enemy e;
    enemy_type => e.enemy_type;
    name => e.name;
    base_hp => e.base_hp;
    hp_scaling => e.hp_scaling;
    drop => e.drop;
    dmg => e.base_dmg;
    dmg_scaling => e.dmg_scaling;
    speed => e.speed.val; 
    b2_shape_type => e.b2_shape_type;
    size => e.size.val;
    M.rot2vec(rot) => e.rotation;

    cd => e.ability_cd.val;
    ability_speed => e.ability_speed.val;

    enemy_type_list << e;

    T.assert(enemy_type_list[enemy_type] == e, "adding enemy type out of order");
}

// enemy types
//                                             hp   hp_scaling  dmg  dmg_scaling  drop  speed        shape                     b2_size    rot          ability_cd   ability_speed
EnemyAddType(EnemyType_None,    "None",        0,   0,          0,    0,          0,     0,          0,                        @(0,0),    0,           0,           0);
EnemyAddType(EnemyType_Basic,   "Basic",       4,   2,          1,   .5,          1,    .4,          b2ShapeType.polygonShape, @(.5, 1), 0,           0,           0);
EnemyAddType(EnemyType_Pursuer, "Pursuer",     2,   1,          .5,  .5,          1,    .5,          b2ShapeType.capsuleShape, @(.3, .1), 0,           0,           0);
EnemyAddType(EnemyType_Charger, "Charger",     8,   2.5,        1,    1,          2,    .8,          ShapeType_Triangle,       @(.5, 0),   0,          0,           0);
EnemyAddType(EnemyType_Shooter, "Shooter",     5,   1.5,        1,   .5,          1,    .8,          b2ShapeType.polygonShape, @(.7, .7), Math.pi/4,   5,           1);

// == Weapon ============================================

0 => int WeaponType_None;
1 => int WeaponType_Chisel;
1 => int WeaponType_Chicken;

class Weapon {
    int weapon_type;
    string name;
    Texture@ sprite;

    UI_Float base_cd_sec;
    UI_Float base_dmg;
    UI_Float base_range;

    vec2 size;

    fun static void ui() {
        if (!UI.collapsingHeader("weapon types", 0)) return;

        for (1 => int i; i < weapon_type_list.size(); ++i) {
            weapon_type_list[i] @=> Weapon w;

            UI.pushID(w.name);

            UI.text(w.name);
            UI.slider("cd", w.base_cd_sec, .2, 5);
            UI.slider("dmg", w.base_dmg, 0, 10);
            UI.slider("range", w.base_range, 0, 10);

            UI.popID();
            UI.separator();
        }
    }
}

Weapon weapon_type_list[0];

fun void WeaponAddType(int weapon_type, string name, Texture@ sprite, float cd, float dmg, float range, vec2 size) {
    Weapon w;
    weapon_type => w.weapon_type;
    name => w.name;
    sprite @=> w.sprite;
    cd => w.base_cd_sec.val;
    dmg => w.base_dmg.val;
    range => w.base_range.val;
    size => w.size;

    weapon_type_list << w;
    T.assert(weapon_type_list[weapon_type] == w, "adding weapon types out of order");
}

//                                              sprite         cd      dmg      range     size(collider)
WeaponAddType(WeaponType_None,   "",            null,          0.0,    0,       0,        @(0,0) );
WeaponAddType(WeaponType_Chisel, "chisel",      chisel_sprite, 1.0,    3,       3,        @(.85,.33)   );

// == Projectile ============================================

0 => int ProjectileType_None;
1 => int ProjectileType_EnemyBasic;
2 => int ProjectileType_Arrow;

class Projectile {
    int projectile_type;
    string name;

    // float dmg; // dmg dealt via contact OR projectile
    // float dmg_scaling; // linearly scales with wave
    // UI_Float speed; // range of speed to spawn with @(min, max)

    // collider stuff
    int b2_shape_type;
    UI_Float2 size;          // for capsule: @(p1 -- p2, radius)
                             // for box:     @(w, h)
                             // for circle   @(radius, radius)
    vec2 rotation;

    UI_Float2 what;
    fun static void ui() {
        if (!UI.collapsingHeader("projectile types", 0)) return;

        for (1 => int i; i < projectile_type_list.size(); ++i) {
            projectile_type_list[i] @=> Projectile p;
            UI.pushID(p.name);

            UI.text(p.name);
            UI.drag("size", p.size, .005);

            UI.popID();
            UI.separator();
        }
    }
}


Projectile projectile_type_list[0];
fun void ProjectileAddType(int type, string name, int shape_type, vec2 size, float angle) {
    Projectile p;
    type => p.projectile_type;
    name => p.name;
    shape_type => p.b2_shape_type;
    new UI_Float2(size) @=> p.size;
    M.rot2vec(angle) => p.rotation;

    projectile_type_list << p;
    T.assert(projectile_type_list[type] == p, "adding projectile type out of order");
}

//                type,                        name,          shape_type,                size,      angle
ProjectileAddType(ProjectileType_None,         "",            0,                         @(0,0),    0    );
ProjectileAddType(ProjectileType_EnemyBasic,   "enemy_basic", b2ShapeType.circleShape,   @(.16,0),  0    );
ProjectileAddType(ProjectileType_Arrow,        "arrow",       b2ShapeType.segmentShape,  @(.5,0),   0    );

// == Pickup Pool ============================================
// @optimize: if there really are *a lot* of pickups, an store in flat
// float array and index fields via offsets to avoid object allocation

class Pickup {
    int _idx; // index in pool array
    vec2 pos;
    int target_body_id;

    static UI_Float visual_radius(.07);
    static UI_Float travel_speed(8); // for linear interp
    fun static void ui() {
        if (!UI.collapsingHeader("pickups", 0)) return;

        UI.slider("radius", visual_radius, 0.001, 1);
        UI.slider("travel speed", travel_speed, 0, 30);
    }
}

Pickup pickup_pool[0]; // NOTE: walk array backwards! to avoid skipping elements
int pickup_pool_count;

fun Pickup PickupAdd() {
    if (pickup_pool_count >= pickup_pool.size()) {
        pickup_pool << new Pickup;
        pickup_pool_count => pickup_pool[-1]._idx;
    }

    T.assert(pickup_pool[pickup_pool_count]._idx == pickup_pool_count, "pickup add");

    return pickup_pool[pickup_pool_count++];
}

fun void PickupRet(Pickup p) {
    T.assert(pickup_pool[p._idx] == p, "pickup ret idx mismatch");
    T.assert(p._idx < pickup_pool_count, "returning a pickup that was never checked out");

    // zero out game fields
    0 => p.target_body_id;

    // swap with last element
    pickup_pool[pickup_pool_count - 1] @=> Pickup last_p;
    T.assert(pickup_pool[last_p._idx] == last_p, "pickup ret idx mismatch last_p");

    // swap last
    last_p @=> pickup_pool[p._idx]; p._idx => last_p._idx;
    // swap p
    p @=> pickup_pool[pickup_pool_count - 1]; pickup_pool_count - 1 => p._idx; 

    pickup_pool_count--;
}


// == Spells and Voice Commands ============================================

0 => int SpellPhase_Init;
1 => int SpellPhase_Pre;
2 => int SpellPhase_Active;
3 => int SpellPhase_Post;
4 => int SpellPhase_Done; // mark spell as done to be removed from active_spells array

class Spell { // used for animating the spell
    int command_type; // for now just reusing CommandType_ enum
    int spell_phase;

    // timing (in secs)
    float total_time;
    float phase_time; // assuming here that spells use time elapsed to transition phase

    // aiming
    vec2 target; // can mean either position or direction depending on spell
    vec2 relative_target; // used by fireball
    vec2 start;

    // spell params
        // fireball
    static UI_Float fireball_cd(6.0);
    static UI_Float fireball_tell_time_sec(1.5);
    static UI_Float fireball_travel_time_sec(1.0);
    static UI_Float fireball_dmg(50);
    static UI_Float fireball_radius(0.5);
    static UI_Float fireball_explosion_radius(2);  // explosion radius
        // lightning
    static UI_Float lightning_cd(2.0);
    static UI_Float lightning_tell_time_sec(1.0);
    static UI_Float lightning_blast_radius(.3);
    static UI_Float lightning_dmg(20);

    fun void transition(int phase) {
        phase => this.spell_phase;
        0 => this.phase_time;
    }

    fun static void ui() {
        if (!UI.collapsingHeader("spell stats", 0)) return;
    }
}

Spell active_spells[0]; // @optimize: turn into pool if there are ever a lot of concurrent spells

fun void SpellCast(int type, vec2 pos) {
    Spell s;
    pos => s.target;
    type => s.command_type;
    SpellPhase_Init => s.spell_phase;
    active_spells << s;
}

0 => int CommandType_None;
1 => int CommandType_Stop;
2 => int CommandType_Roam;
3 => int CommandType_Fireball;
4 => int CommandType_Lightning;

class Command {
    int command_type;
    string name;
    string words[];

    fun static void ui() {
        if (!UI.collapsingHeader("voice commands", 0)) return;

        for (1 => int i; i < command_type_list.size(); ++i) {
            command_type_list[i] @=> Command c;

            if (UI.button(c.name)) CommandActivate(c.command_type);
            for (auto word : c.words) {
                UI.sameLine();
                UI.textColored(@(1, 1, 0, 1), word + ",");
            }
            
            UI.separator();
        }
    }
}

Command command_type_list[0];

fun void CommandAddType(int type, string name, string words[]) {
    Command c;
    type => c.command_type;
    name => c.name;
    words @=> c.words;
    command_type_list << c;
    T.assert(command_type_list[type] == c, "adding command out of order");

    voice.add(type, words);
}

fun void CommandActivate(int command_type) {
    command_type_list[command_type] @=> Command c;

    if (command_type == CommandType_Stop) {
        Cart_MoveState_Stop => cart_move_state;
    }
    else if (command_type == CommandType_Roam) {
        Cart_MoveState_Roam => cart_move_state;
    } 
    else if (command_type == CommandType_Fireball || command_type == CommandType_Lightning) {
        SpellCast(command_type, bard.pos() + RIGHT);
    } 
    else {
        T.err("Unsupported command " + command_type);
    }
}


CommandAddType(CommandType_None, "", null);
CommandAddType(CommandType_Stop, "Cart Stop", ["stop"]);
CommandAddType(CommandType_Roam, "Cart Roam", ["move", "start", "roam"]);
CommandAddType(CommandType_Fireball, "Spell Fireball", ["fireball"]);
CommandAddType(CommandType_Lightning, "Spell Lightning", ["lightning"]);

// == Entity ================================================

0 => int PoolState_Returned;
1 => int PoolState_CheckedOut;
2 => int PoolState_QueuedForReturn;

// @TODO extend PoolableItem, implement pool allocator, group enemies separately
class Entity {
    // internal
    int _pool_idx; // used by pool allocator, assumes each Entity only belongs to a single pool
    int _pool_state;

    // time
    float spawn_time_sec;

    // id
    int entity_type; // mask of EntityType_XXX
    int b2_body_id;
    int b2_shape_id;
    int b2_group_idx;

    // hp
    UI_Float hit_cd(0.0); // used for drawing hit flash. // @TODO: replace with gametime of last hit, use that for drawing instead
    // UI_Float invuln_cd(0.0); // used for player invuln on hit
    UI_Float hp_max;
    UI_Float hp_curr;

    // movement
    float speed; // @TODO: might not need this, can just look up speed from EntityType
    float last_dir_x;

    // player
    int player_type;
    int in_cart;
    int dead;
    float inspo_generation_cd;

    // sprite
    Texture@ sprite_tex;
    float sprite_time_since_last_frame;
    int sprite_current_frame;
    int sprite_num_frames;  // assuming a Nx1 sprite sheet

    // enemy
    int enemy_type;
    int enemy_state;
    int target_player_id; // used by pursuer to lock on player
    int touching_player_id; // set in contactEvents()
    int touching_player_count; 
    float enemy_contact_dmg_cd; // countdown timer for contact dmg

    // projectile
    int projectile_type;
    float projectile_dmg;

    // attack
    float attack_cd; // look up max CD from the EntityType class. This is a countdown cd. Used for enemies and player weapons

    // weapon
    int weapon_type;
    int player_id; // b2bodyid of player this weapon belongs to
    int weapon_state;

    // @feature: ability to zero classes in chuck
    fun void zero() {
        // DON'T ZERO POOL IDX
        PoolState_Returned => this._pool_state;

        T.assert(!b2Body.isValid(this.b2_body_id), "didn't destory body before return");
        T.assert(!b2Shape.isValid(this.b2_shape_id), "didn't destory shape before return");

        0 => this.spawn_time_sec;

        0 => this.entity_type; 
        0 => this.b2_body_id;
        0 => this.b2_shape_id;
        0 => this.b2_group_idx;

        0 => this.hit_cd.val; // used for drawing hit flash
        0 => this.hp_max.val;
        0 => this.hp_curr.val;

        0 => this.speed;

        0 => this.player_type;
        0 => this.in_cart;
        0 => this.dead;
        0 => this.inspo_generation_cd;

        0 => this.enemy_type;
        0 => this.enemy_state;
        0 => this.target_player_id;
        0 => this.touching_player_id;
        0 => this.touching_player_count;
        0 => this.enemy_contact_dmg_cd;

        0 => this.projectile_type;
        0 => this.projectile_dmg;
        0 => this.attack_cd;

        0 => this.weapon_type;
        0 => this.player_id;
        0 => this.weapon_state;
    }

    fun vec2 pos() { return b2Body.position(this.b2_body_id); }
    fun void pos(vec2 p) { b2Body.position(this.b2_body_id, p); }
    fun void rot(vec2 rot) { b2Body.rotation(this.b2_body_id, rot); }
    fun vec2 rot() { return b2Body.rotation(this.b2_body_id); }

    // rotate body to face p
    fun void lookAt(vec2 p) {
        p - this.pos() => vec2 dir;
        dir.normalize();
        this.rot(dir);
    }

    fun float angle() { return b2Body.angle(this.b2_body_id); }
    fun void vel(vec2 v) {  b2Body.linearVelocity(this.b2_body_id, v); }
    fun vec2 vel() {  return b2Body.linearVelocity(this.b2_body_id); }

    fun void angularVel(float w) { 
        if (b2Body.isFixedRotation(this.b2_body_id)) T.err("trying to set angular vel on body with fixed rotation");
        b2Body.angularVelocity(this.b2_body_id, w); 
    }
    fun float angularVel() {  return b2Body.angularVelocity(this.b2_body_id); }

    // move towards p with this.speed velocity
    fun void moveTo(vec2 p) { 
        T.assert(this.entity_type == EntityType_Enemy, "moveTO not impl for non-enemy types");
        p - this.pos() => vec2 dir; dir.normalize();
        enemy_type_list[this.enemy_type].speed.val() => float speed;
        b2Body.linearVelocity(this.b2_body_id, dir * speed);
    }

    // move in dir with this.speed velocity
    fun void moveDir(vec2 dir) {
        T.assert(this.entity_type == EntityType_Enemy, "moveDir not impl for non-enemy types");
        enemy_type_list[this.enemy_type].speed.val() => float speed;
        b2Body.linearVelocity(this.b2_body_id, dir * speed);
    }

    fun void die() {
        if (this.entity_type == EntityType_Enemy) {
            spork ~ FX.smokeCloud(this.pos(), Color.BLACK);
            EntityReturn(this);

            // drop pickup
            PickupAdd() @=> Pickup p;
            this.pos() => p.pos;

            // add money!
            // enemy_type_list[this.enemy_type].drop +=> currency;
        }
        else if (this.entity_type == EntityType_Player) {

        }
        else if (this.entity_type == EntityType_Player) {

        }
        else {
            T.err("in die(), unsupported entity type: " + this.entity_type);
        }

    }

    fun void heal(float amt) {
        T.assert(this.hp_max.val() > 0, "calling heal() on entity without hp");
        if (this.hp_curr.val() <= 0) return; // already dead, do nothing
        Math.min(this.hp_max.val(), this.hp_curr.val() + amt) => this.hp_curr.val;
    }

    fun void takeDamage(float amt) {
        T.assert(this.hp_max.val() > 0, "calling damage() on entity without hp");
        if (this.hp_curr.val() <= 0) return; // already dead, do nothing
        // if (this.invuln_cd.val() > 0) return; // invuln, do nothing

        t.easeOutQuad(this.hit_cd, 1.0, 0).over(.3);

        // apply player invuln
        // if (this.entity_type & EntityType_Player) {
        //     t.lerp(this.invuln_cd, 1.0, 0).over(1);
        // }

        Math.max(0.0, this.hp_curr.val() - amt) => this.hp_curr.val;

        // die
        if (this.hp_curr.val() <= 0) this.die();
    }

}

// == gamestate ================================================

UI_Float timescale(1.0);
UI_Float animation_secs_per_frame(.1); // walk speed (run speed should be 75ms)

// physics
b2WorldDef world_def;
int b2_world_id;
int begin_sensor_events[0];
int begin_touch_events[0];
int end_touch_events[0];

UI_Bool draw_b2_debug(true);
DebugDraw debug_draw;
debug_draw.layer(Layer_DebugDraw);
true => debug_draw.drawShapes;
true => debug_draw.outlines_only;

// playfield
UI_Float field_len(32.0);
UI_Float3 field_color(M.srgbToLinear(Color.hex(0x0f2a3f))); // @TODO background color not accurate
g.backgroundColor(field_color.val());

// player
UI_Float player_size(1);
Entity player_list[0];
int players_num_alive;

// weapon stuff
UI_Float weapon_speed(30.0);
UI_Float2 weapon_rest_pos(.8, 0.0); 
UI_Float weapon_size(0.85);

// spawning
// UI_Float spawn_period(-1); // seconds until next spawn
UI_Float spawn_inner_r(GG.camera().viewSize() * .4);
UI_Float spawn_outer_r(spawn_inner_r.val() * 2);
// float time_to_next_spawn;
// float time_since_last_spawn;

// entity stuff
Entity entity_pool[0]; // maybe it can all go in 1 entity list...
int entity_count; // # of entities currently checked out
HashMap b2body_to_entity_map;
Entity entities_to_return[0];
HashMap entity_count_by_type;

// camera
UI_Float camera_interp_rate(.02);
UI_Float camera_view_size(GG.camera().viewSize());

// bard
UI_Float bard_base_speed(.3);
UI_Float bard_inspo_max(100);
UI_Float bard_inspo_curr(0);
vec2 bard_target;
null @=> Entity@ bard;
Spell.fireball_cd.val() => float bard_fireball_cd;
Spell.lightning_cd.val() => float bard_lightning_cd;

// fog
PolygonGeometry polygon_geo;  
FlatMaterial fog_material(Color.BLACK);
true => fog_material.transparent;
.25 => fog_material.alpha;
UI_Float fog_alpha(fog_material.alpha());
GMesh fog_mesh(polygon_geo, fog_material);
Layer_Fog => fog_mesh.posZ;
UI_Bool show_fog(true);

// cart params
UI_Float cart_hp(100);
UI_Float2 cart_size(1.52, 1.52);
UI_Float cart_speed(.5);
UI_Float cart_speed_mult(1.0);
UI_Float cart_dodge(.0);
float cart_wheel_rot;
    // ambulance params
UI_Float cart_heal_base_radius(2 * cart_size.val().x);
UI_Float cart_heal_radius_mult(1);
UI_Float cart_heal_base_hp_per_sec(.2);
UI_Float cart_heal_amt_mult(1.0);
UI_Float cart_heal_period_sec(5.0);
cart_heal_period_sec.val() => float cart_heal_cd;
    // tower params
int cart_arrow_tower_count;
int cart_bomb_tower_count;
AttackParams arrow_tower_atk(5.0, 2 * cart_size.val().x, 1.0)[2];
AttackParams bomb_tower_atk(10.0, 4 * cart_size.val().x, 3.2);
    // movement
string cart_move_state;
vec2 cart_target;
null @=> Entity@ cart;

// resources
int currency;

// upgrades
// idea: bard AND cart share same pool of upgrades lots
// e.g. you can split 50/50 or go 100/0 into either
HashMap acquired_upgrades_to_level;  // map from [Upgrade : int level]
Upgrade shop_upgrades[0];

// == methods ================================================

fun void BardAddInspo(float amt, vec2 src_pos) {
    // @TODO: increment inspo *After* animation completes
    Math.clampf(bard_inspo_curr.val() + amt, 0, bard_inspo_max.val()) => bard_inspo_curr.val;

    // animate the filling
    spork ~ FX.particleTrail(src_pos, bard.pos(), Layer_Light);
}

fun void BardSpendInspo(float amt) {
    Math.clampf(bard_inspo_curr.val() - amt, 0, bard_inspo_max.val()) => bard_inspo_curr.val;
}

fun Entity EntityAdd(int entity_type, int b2_body_id, int b2_shape_id) {
    if (entity_count == entity_pool.size()) {
        entity_pool << new Entity;
        entity_count => entity_pool[-1]._pool_idx;
    }
    entity_pool[entity_count] @=> Entity entity;
    T.assert(entity._pool_state == PoolState_Returned, "entity should be in returned state here");
    T.assert(!b2Body.isValid(entity.b2_body_id), "entity should have no b2bodyid");
    T.assert(!b2body_to_entity_map.has(b2_body_id), "hashmap should not have registered body in addEntity()");

    b2_body_id => entity.b2_body_id;
    b2_shape_id => entity.b2_shape_id;
    now/second => entity.spawn_time_sec;

    if (b2Body.isValid(b2_body_id)) {
        b2body_to_entity_map.set(b2_body_id, entity);
    }
    T.assert(entity._pool_idx == entity_count, "invalid pool idx in addEntity()");
    ++entity_count;
    entity_type => entity.entity_type;
    PoolState_CheckedOut => entity._pool_state;
    return entity;
}

fun int EntityHas(int b2_body_id) {
    return b2body_to_entity_map.has(b2_body_id);
}

fun Entity EntityGet(int b2_body_id) {
    return b2body_to_entity_map.getObj(b2_body_id) $ Entity;
}

// CAREFUL: need to batch the returns because returning in the middle of a for-loop
// causes the loop to skip over elements.
fun void EntityReturn(Entity e) {
    T.assert(e._pool_state == PoolState_CheckedOut, "warning, trying to return an entity that's not checked out");

    if (e._pool_state == PoolState_CheckedOut) {
        entities_to_return << e;
        PoolState_QueuedForReturn => e._pool_state;
    }
}

fun Entity EntityFromShape(int id) {
    T.assert(b2Shape.isValid(id), "EntityFromShape invalid shape id");
    return b2body_to_entity_map.getObj(b2Shape.body(id)) $ Entity;
}

// call once per frame
fun void EntityProcessReturned() {
    for (auto e : entities_to_return) {
        // swap with last active
        T.assert(entity_pool[e._pool_idx] == e, "e pool idx incorrect");
        T.assert(entity_pool[entity_count - 1]._pool_idx == entity_count - 1, "e pool idx incorrect on end");
        --entity_count;
        e._pool_idx => int returned_pool_idx;

        // destroy the b2 body on return
        if (b2Body.isValid(e.b2_body_id)) {
            T.assert(b2body_to_entity_map.has(e.b2_body_id), "hashmap should have valid body id");
            b2body_to_entity_map.del(e.b2_body_id);
            b2.destroyBody(e.b2_body_id);
            0 => e.b2_body_id;
        }

        entity_pool[entity_count] @=> Entity last_spell;
        e._pool_idx => last_spell._pool_idx;
        last_spell @=> entity_pool[e._pool_idx];

        e @=> entity_pool[entity_count];
        entity_count => e._pool_idx;
        e.zero(); // reset all fields except _pool_idx
        
        T.assert(
            entity_pool[entity_count] == e &&
            entity_count == e._pool_idx, 
            "e pool idx incorrect after swap"
        );
        T.assert(
            entity_pool[returned_pool_idx] == last_spell &&
            returned_pool_idx == last_spell._pool_idx,
            "e pool idx incorrect on end after swap"
        );
    }
    entities_to_return.clear();
    T.assert(entities_to_return.size() == 0, "processed spells to return");
}

fun Entity PlayerAdd(int player_type, vec2 pos) {
    player_type_list[player_type] @=> Player pt;

    b2BodyDef player_body_def;
    pos => player_body_def.position;
    b2BodyType.dynamicBody => player_body_def.type;
    true => player_body_def.fixedRotation;

    // entity
    Entity player;
    -1 => player._pool_idx; // don't add players to the pool! they are stored separate
    EntityType_Player => player.entity_type;
    b2.createBody(b2_world_id, player_body_def) => player.b2_body_id;
    player_type => player.player_type;
    b2body_to_entity_map.set(player.b2_body_id, player);

    // filter
    b2Filter player_filter;
    EntityType_Player => player_filter.categoryBits;
    0xFFFFFFF ^ EntityType_Player => player_filter.maskBits; // disallow player-player collision
    -player_list.size() - 1 => player_filter.groupIndex; // give each player a group index to disallow player collision w/ own spells
    player_filter.groupIndex => player.b2_group_idx;

    // shape
    b2ShapeDef player_shape_def;
    player_filter @=> player_shape_def.filter;
    true => player_shape_def.enableSensorEvents;
    true => player_shape_def.enableContactEvents;
    1 => player_shape_def.density;

    // geo
    b2.makeBox(.5*player_size.val(), .5*player_size.val()) @=> b2Polygon player_geo;
    b2.createPolygonShape(player.b2_body_id, player_shape_def, player_geo) => player.b2_shape_id;

    player_list << player;
    players_num_alive++;

    // assign stats
    pt.base_hp => player.hp_max.val;
    pt.base_hp => player.hp_curr.val;
    pt.base_speed.val() => player.speed;
    Player.base_inspo_cd.val() => player.inspo_generation_cd;

    T.assert(EntityHas(player.b2_body_id), "player not in pool but still registered in b2body LUT");
    return player;
}

fun Entity CartAdd() {
    b2BodyDef body_def;
    b2BodyType.kinematicBody => body_def.type;
    true => body_def.fixedRotation;

    // entity
    Entity e;
    -1 => e._pool_idx; // don't add cart to the pool! stored separate
    EntityType_Cart => e.entity_type;
    b2.createBody(b2_world_id, body_def) => e.b2_body_id;
    b2body_to_entity_map.set(e.b2_body_id, e);

    // filter
    b2Filter filter;
    EntityType_Cart => filter.categoryBits;
    // 0xFFFFFFF ^ EntityType_Player => filter.maskBits; 
    // -player_list.size() - 1 => filter.groupIndex; 
    // filter.groupIndex => player.b2_group_idx;

    // shape
    b2ShapeDef shape_def;
    filter @=> shape_def.filter;
    true => shape_def.enableSensorEvents;
    true => shape_def.enableContactEvents;

    // geo
    b2.makeBox(cart_size.val().x, .55*cart_size.val().y) @=> b2Polygon geo;
    b2.createPolygonShape(e.b2_body_id, shape_def, geo) => int shape_id;

    // hp
    cart_hp.val() => e.hp_max.val;
    cart_hp.val() => e.hp_curr.val;

    T.assert(EntityHas(e.b2_body_id), "cart not registered in b2body LUT");
    return e;
}

// gets closest *living* player. null if none
fun Entity PlayerClosest(vec2 pos) {
    Math.FLOAT_MAX => float dist;
    null => Entity@ closest;
    for (auto p : player_list) {
        M.dist2(pos, p.pos()) => float d;
        if (d < dist) {
            d => dist;
            p @=> closest;
        }
    }
    
    // check cart too
    M.dist2(pos, cart.pos()) => float d;
    if (d < dist) {
        d => dist;
        cart @=> closest;
    }

    return closest;
}

// gets closest *living* enemy
b2QueryFilter EnemyClosest_filter;
0xFFFFFFFF => EnemyClosest_filter.categoryBits;
EntityType_Enemy => EnemyClosest_filter.maskBits;
fun Entity EnemyClosest(vec2 pos, float range) {
    Math.FLOAT_MAX => float dist;
    null => Entity@ closest;
    // @optimize: overlapCircleFast that takes in an array
    b2World.overlapCircle( b2_world_id, pos, range, EnemyClosest_filter) @=> int enemy_shape_ids[];
    for (int shape_id : enemy_shape_ids) {
        if (!b2Shape.isValid(shape_id)) continue;
        b2Shape.body(shape_id) => int body_id;
        EntityGet(body_id) @=> Entity@ e;
        if (e.hp_curr.val() <= 0) continue;

        M.dist2(pos, e.pos()) => float d;
        if (d < dist) {
            d => dist;
            e @=> closest;
        }
    }

    return closest;
}

b2QueryFilter EntityInRadius_filter;
fun int[] EntityInRadius(vec2 pos, float radius, int category_bits, int mask_bits) {
    mask_bits => EntityInRadius_filter.maskBits;
    category_bits => EntityInRadius_filter.categoryBits;
    b2World.overlapCircle( b2_world_id, pos, radius, EntityInRadius_filter) @=> int shape_ids[];
    // convert to body ids
    for (int i; i < shape_ids.size(); ++i) {
        b2Shape.body(shape_ids[i]) => shape_ids[i];
    }
    return shape_ids;
}

b2QueryFilter EntityClosest_filter;
fun Entity EntityClosest(vec2 pos, float radius, int category_bits, int mask_bits) {
    T.err("unimplemented");
    mask_bits => EntityClosest_filter.maskBits;
    category_bits => EntityClosest_filter.categoryBits;
    b2World.overlapCircle( b2_world_id, pos, radius, EntityInRadius_filter) @=> int shape_ids[];
    return null;
}


// gets random *living* player. null if none
fun Entity PlayerRandomLiving() {
    if (players_num_alive == 0) return null;
    return player_list[Math.random2(0, players_num_alive - 1)];
}

fun Entity EntityClosestFromBody(vec2 pos, int body_ids[]) {
    Math.FLOAT_MAX => float dist;
    null => Entity@ closest;
    for (auto id : body_ids) {
        EntityGet(id) @=> Entity e;
        M.dist(pos, e.pos()) => float d;
        if (d < dist) {
            d => dist;
            e @=> closest;
        }
    }
    return closest;
}

fun int CreateShape(int b2_shape_type, int body_id, vec2 size, b2ShapeDef@ def) {
    int shape_id;
    if (b2_shape_type == b2ShapeType.polygonShape) {
        // @optimize avoid creating a new b2Polygon object every time
        b2.makeBox(size.x, size.y) @=> b2Polygon enemy_geo;
        b2.createPolygonShape(body_id, def, enemy_geo) => shape_id;
    }
    else if (b2_shape_type == b2ShapeType.capsuleShape) {
        b2.createCapsuleShape(
            body_id, def, 
            @(-size.x/2, 0),  // center1
            @(size.x/2, 0),  // center1
            size.y // radius
        ) => shape_id;
    }
    else if (b2_shape_type == b2ShapeType.circleShape) {
        b2.createCircleShape(
            body_id, def, 
            @(0, 0),
            size.x // radius
        ) => shape_id;
    } 
    else if (b2_shape_type == b2ShapeType.segmentShape) {
        // for segments, `size` represents vector which is auto-centered
        b2.createSegmentShape(
            body_id, def, 
            -.5 * size,
            .5 * size
        ) => shape_id;
    }
    else if (b2_shape_type == ShapeType_Triangle) {
        // size is @(distance_from_center_to_vertex, 0)
        b2.makePolygon(
            [
                @(size.x, 0),
                @(-.5*size.x, size.x * Math.sqrt(3)/2),
                @(-.5*size.x, -size.x * Math.sqrt(3)/2),
            ], 
            0.1
        ) @=> b2Polygon geo;
        b2.createPolygonShape(body_id, def, geo) => shape_id;
    }
    else {
        T.err("unrecognized shape type in CreateShape(): " + b2_shape_type);
    }
    return shape_id;
}

fun Entity EnemyAdd(int enemy_type, vec2 pos, int wave) {
    // @TODO test different spawn modes
    // @optimize: reuse static b2 defs 
    enemy_type_list[enemy_type] @=> Enemy et;

    // clampSpawnToArena
    .5 * field_len.val() - 1.5 => float hw;
    Math.clampf(pos.x, -hw, hw) => pos.x;
    Math.clampf(pos.y, -hw, hw) => pos.y;

    // body
    b2BodyDef enemy_body_def;
    pos => enemy_body_def.position;
    b2BodyType.dynamicBody => enemy_body_def.type;

    if (enemy_type == EnemyType_Charger) false => enemy_body_def.fixedRotation;
    else                                 true => enemy_body_def.fixedRotation;

    et.rotation => enemy_body_def.rotation;
        // charger rotates 
    if (enemy_type == EnemyType_Charger)  Math.random2f(.5, 1.0) * (maybe ? -1 : 1) => enemy_body_def.angularVelocity;

    b2.createBody(b2_world_id, enemy_body_def) => int body_id;
    
    // filter
    b2Filter enemy_filter;
    EntityType_Enemy => enemy_filter.categoryBits;
    (0xFFFFFFFF ^ (EntityType_Enemy | EntityType_EnemyShadow)) => enemy_filter.maskBits; // what it accepts for collision
    
    // make enemy a sensor if we want player to be able to move through enemies
    // @TODO play brotato, vampire survivors, 20 minutes till dawn etc and see what they do
    // true => enemy_shape_def.isSensor;

    // shape
    b2ShapeDef enemy_shape_def;
    true => enemy_shape_def.enableSensorEvents;
    enemy_filter @=> enemy_shape_def.filter;

    // geo
    CreateShape(et.b2_shape_type, body_id, et.size.val(), enemy_shape_def) => int shape_id;
    T.assert(b2Shape.isValid(shape_id), "EnemyAdd invalid shape id");

    // shadow collision (add small collider where drop shadow would be)
    // Allows enemies to only partially stack
    {
        b2Filter filter;
        EntityType_EnemyShadow => filter.categoryBits;
        (EntityType_EnemyShadow) => filter.maskBits;

        b2ShapeDef shadow_shape_def;
        filter @=> shadow_shape_def.filter;

        -.5 * et.size.val().y => float py;
        b2.makeOffsetBox(
            .5 * et.size.val().x,
            .5 * BardG2D.shadow_height.val(), // drop shadow 
            @(0, py), 0
        ) @=> b2Polygon shadow_geo;
        b2.createPolygonShape(body_id, shadow_shape_def, shadow_geo);
    }

    // entity
    EntityAdd(
        EntityType_Enemy,
        body_id,
        shape_id
    ) @=> Entity enemy;

    enemy_type => enemy.enemy_type;

    et.base_hp + wave * et.hp_scaling => float max_hp;
    max_hp => enemy.hp_max.val;
    max_hp => enemy.hp_curr.val;
    et.speed.val() => enemy.speed;
    et.ability_cd.val() => enemy.attack_cd;
    Enemy.contact_dmg_cd.val() => enemy.enemy_contact_dmg_cd;

    return enemy;
}

// creates an *outline* of a static body using line segments
fun Entity StaticAdd(vec2 vertices[], vec2 pos) {
    // body def
    b2BodyDef static_body_def;
    b2BodyType.staticBody => static_body_def.type;
    pos => static_body_def.position;
    b2.createBody(b2_world_id, static_body_def) => int body_id;

    // filter
    b2Filter filter;
    EntityType_Static => filter.categoryBits;
    0xFFFFFFFF ^ EntityType_Enemy => filter.maskBits;

    // shape def
    b2ShapeDef shape_def;
    filter @=> shape_def.filter;
    true => shape_def.enableSensorEvents;

    // shape
    for (int i; i < vertices.size(); ++i)
        b2.createSegmentShape(body_id, shape_def, vertices[i-1], vertices[i]);

    EntityAdd(
        EntityType_Static,
        body_id,
        0
    ) @=> Entity e;

    return e;
}

// gives weapon to player
fun Entity WeaponAdd(int weapon_type, Entity@ player) {
    weapon_type_list[weapon_type] @=> Weapon@ wt;

    // body
    b2BodyDef body_def;
    b2BodyType.kinematicBody => body_def.type;
    UP => body_def.rotation;
    b2.createBody(b2_world_id, body_def) => int body_id;

    // filter
    b2Filter filter;
    EntityType_Weapon => filter.categoryBits;
    EntityType_Enemy | EntityType_Cart => filter.maskBits;

    // shape
    b2ShapeDef shape_def;
    false => shape_def.enableSensorEvents; // enabled during attack anim
    true => shape_def.isSensor;
    filter @=> shape_def.filter;

    // geo
    b2.makeBox(wt.size.x, wt.size.y) @=> b2Polygon geo;
    b2.createPolygonShape(body_id, shape_def, geo) => int shape_id;

    // entity
    EntityAdd(
        EntityType_Weapon,
        body_id, shape_id
    ) @=> Entity e;
    weapon_type => e.weapon_type;
    player.b2_body_id => e.player_id;

    return e;
}

fun void EnemyFire(int projectile_type, vec2 pos, vec2 dir, float speed, float dmg) {
    projectile_type_list[projectile_type] @=> Projectile pt;
    dir.normalize();

    // body
    b2BodyDef body_def;
    b2BodyType.kinematicBody => body_def.type;
    pos => body_def.position;
    dir => body_def.rotation;
    speed * dir => body_def.linearVelocity;

    b2.createBody(b2_world_id, body_def) => int body_id;

    // filter
    b2Filter filter;
    EntityType_EnemyProjectile => filter.categoryBits;
    EntityType_Player | EntityType_Static | EntityType_Cart => filter.maskBits;

    // shape
    b2ShapeDef shape_def;
    true => shape_def.enableSensorEvents;
    true => shape_def.isSensor;
    filter @=> shape_def.filter;

    // geo
    CreateShape(pt.b2_shape_type, body_id, pt.size.val(), shape_def) => int shape_id;
    T.assert(b2Shape.isValid(shape_id), "EnemyFire invalid shape id");

    // entity
    EntityAdd(
        EntityType_EnemyProjectile,
        body_id, shape_id
    ) @=> Entity e;
    dmg => e.projectile_dmg;
    projectile_type => e.projectile_type;

    return;
}

fun void Fire(AttackParams@ p, float dt, int projectile_type, vec2 pos, vec2 target) {
    projectile_type_list[projectile_type] @=> Projectile pt;

    // check if off cooldown first
    dt * p._atk_speed.val() -=> p.cd;
    if (p.cd > 0) return;
    p._cd_sec.val() +=> p.cd;

    <<< "firing", pos, target >>>;

    target - pos => vec2 dir;
    dir.normalize();

    // body
    b2BodyDef body_def;
    b2BodyType.kinematicBody => body_def.type;
    pos => body_def.position;
    dir => body_def.rotation;
    // @TODO projectile speed
    10 * dir => body_def.linearVelocity;

    b2.createBody(b2_world_id, body_def) => int body_id;

    // filter
    b2Filter filter;
    EntityType_PlayerProjectile => filter.categoryBits;
    EntityType_Enemy | EntityType_Static => filter.maskBits;

    // shape
    b2ShapeDef shape_def;
    true => shape_def.enableSensorEvents;
    true => shape_def.isSensor;
    filter @=> shape_def.filter;

    // geo
    CreateShape(pt.b2_shape_type, body_id, pt.size.val(), shape_def) => int shape_id;
    T.assert(b2Shape.isValid(shape_id), "Player Projectile invalid shape id");

    // entity
    EntityAdd(
        EntityType_PlayerProjectile,
        body_id, shape_id
    ) @=> Entity e;
    projectile_type => e.projectile_type;
    p.dmg() => e.projectile_dmg;

    return;
}

fun void UpdateFogGeometry(float radius) {
	64 => int circle_segments;
	vec2 circle_vertices[circle_segments];
	for (int i; i < circle_segments; i++) {
		Math.two_pi * i / circle_segments => float theta;
		radius * @( Math.cos(theta), Math.sin(theta)) => circle_vertices[i];
	}
    10000 => float l;
    polygon_geo.build(
        [
            @(-l, l),
            @(l, l),
            @(l, -l),
            @(-l, -l)
        ], // something really massive for the black sheet
        circle_vertices,
        [circle_vertices.size()]
    );
}

// gets next layer of accessible upgrades in upgrade tree
fun Upgrade[] UpgradeGetUnlockable() {
    // logic: if all upgrade slots are occupied, only show upgrades from that pool
    // this is like VS
    // else show all possible upgrades that are not already maxed out

    Upgrade unlockable[0];
    upgrade_type_list @=> Object keys[];

    T.assert(acquired_upgrades_to_level.size() <= MAX_ACTIVE_UPGRADES, "cannot have more than MAX_ACTIVE_UPGRADES");

    (acquired_upgrades_to_level.size() >= MAX_ACTIVE_UPGRADES) => int upgrade_slots_full;
    if (upgrade_slots_full) {
        acquired_upgrades_to_level.objKeys() @=> keys;
    } 

    // loop over and add any that aren't maxed
    for (Object u : keys) {
        // @TODO this is where to track requirements for evos
        if (acquired_upgrades_to_level.getInt(u) < (u $ Upgrade).max_level) {
            unlockable << (u $ Upgrade);
        }
    }

    return unlockable;
}

fun int UpgradeLevel(int upgrade_type) {
    return acquired_upgrades_to_level.getInt(upgrade_type_list[upgrade_type]);
}

fun int UpgradeHas(int upgrade_type, int level) {
    return acquired_upgrades_to_level.getInt(upgrade_type_list[upgrade_type]) >= level;
}

fun int UpgradeHas(int upgrade_type) {
    return UpgradeHas(upgrade_type, 1);
}

fun void UpgradeApply(Upgrade u, int upgrade) {
    int level;
    !upgrade => int downgrade;
    if (upgrade) {
        acquired_upgrades_to_level.getInt(u) + 1 => level;
        acquired_upgrades_to_level.set(u, level);
    } else {
        acquired_upgrades_to_level.getInt(u) => level;
        acquired_upgrades_to_level.set(u, level - 1);
    }

    T.assert(level <= u.max_level && level > 0, "illegal upgrade level " + level);

    if (u.upgrade_type == Upgrade_Cart_Ambulance) {
        if (level == 1) {} // do nothing, just activate
        else if (level == 2) {
            // increase radius by %
            if (upgrade) cart_heal_radius_mult.val() + .30 => cart_heal_radius_mult.val;
            else         cart_heal_radius_mult.val() - .30 => cart_heal_radius_mult.val;
        }
        else if (level == 3) {
            // increase healing amount
            // the amount should probably scale with some item/player stat
            if (upgrade) cart_heal_amt_mult.val() + .50 => cart_heal_amt_mult.val;
            else         cart_heal_amt_mult.val() - .50 => cart_heal_amt_mult.val;
        } 
        else {
            T.err("unimplemented upgrade level " + u.title + " " + level);
        }
    }
    else if (u.upgrade_type == Upgrade_Cart_Fortress) {
        if (level == 1) {
            // +1 arrow tower
            if (upgrade) 1 +=> cart_arrow_tower_count;
            else         1 -=> cart_arrow_tower_count;
        }
        else if (level == 2) {
            // +1 arrow tower
            if (upgrade) 1 +=> cart_arrow_tower_count;
            else         1 -=> cart_arrow_tower_count;
        }
        else if (level == 3) {
            // +1 bomb tower
            if (upgrade) 1 +=> cart_bomb_tower_count;
            else         1 -=> cart_bomb_tower_count;
        }
        else {
            T.err("unimplemented upgrade level " + u.title + " " + level);
        }
    }
    else if (u.upgrade_type == Upgrade_Cart_Speed) {
        if (upgrade) cart_speed_mult.val() + .30 => cart_speed_mult.val;
        else         cart_speed_mult.val() - .30 => cart_speed_mult.val;
    }
    else if (u.upgrade_type == Upgrade_Cart_Dodge) {
        if (upgrade) cart_dodge.val() + .10 => cart_dodge.val;
        else         cart_dodge.val() - .10 => cart_dodge.val;
    } 
    // bard upgrades
    else if (u.upgrade_type == Upgrade_Bard_Fireball) {

    }
    else if (u.upgrade_type == Upgrade_Bard_Lightning) {

    }
    else {
        T.err("UpgradeApply() unsupported upgrade " + u.title + " " + level);
    }


    // TODO change stats / gamestate etc
}

fun void RerollUpgradeShop() {
    UpgradeGetUnlockable() @=> Upgrade upgrades[];
    upgrades.shuffle();
    upgrades.erase(4, upgrades.size());
    upgrades @=> shop_upgrades;
}

// == wave spawn system ================================================

0 => int SpawnLocation_Random;
1 => int SpawnLocation_Group;

0 => int SpawnTiming_OneShot;
1 => int SpawnTiming_Loop;

class Spawn {
    int enemy_type; // the enemy type to spawn

    // location
    int location_type;
    float spawn_group_radius;
    int number; // number of enemies to spawn

    // probability
    // @TODO

    // timing
    int spawn_timing_type;
    float pre_delay_sec;
    float interval_sec;  // the times at which to spawn. for one-shots this is the exact time of spawn

    // struct initializers would be nice...
    fun Spawn(int enemy_type, int location_type, float spawn_group_radius, int number, int spawn_timing_type, float pre_delay_sec, float interval_sec) {
        enemy_type => this.enemy_type ;
        location_type => this.location_type ;
        spawn_group_radius => this.spawn_group_radius ;
        number => this.number ;
        spawn_timing_type => this.spawn_timing_type ;
        pre_delay_sec => this.pre_delay_sec ;
        interval_sec => this.interval_sec ;
    }
}

class SpawnManager {
    Wave@ wave;
    float prev_time_sec;
    float curr_time_sec;

    // switches to the given spawn list
    fun void register(Wave wave) {
        0.0 => this.prev_time_sec;
        wave @=> this.wave;
    }

    // inner_r is bard vision, outer_r is s max spawn distance
    fun void update(float dt, vec2 bard_pos, float inner_r, float outer_r) {
        if (wave == null) return;
        T.assert(outer_r > inner_r, "spawner invalid spawn ranges");

        this.curr_time_sec => this.prev_time_sec;
        dt +=> this.curr_time_sec;

        for (auto spawn : this.wave.spawn_list) {
            int do_spawn;
            if (spawn.spawn_timing_type == SpawnTiming_OneShot) {
                spawn.pre_delay_sec + spawn.interval_sec => float time_to_spawn;
                (prev_time_sec < time_to_spawn && curr_time_sec >= time_to_spawn) => do_spawn;
            }
            else if (spawn.spawn_timing_type == SpawnTiming_Loop) {
                (
                (this.prev_time_sec > spawn.pre_delay_sec) 
                &&
                Math.fmod(this.prev_time_sec - spawn.pre_delay_sec, spawn.interval_sec) >= .5 * spawn.interval_sec
                &&
                Math.fmod(this.curr_time_sec - spawn.pre_delay_sec, spawn.interval_sec) <= .5 * spawn.interval_sec
                ) => do_spawn;
            }
            else {
                T.err("unknown spawn timing" + spawn.spawn_timing_type);
            }

            if (!do_spawn) continue;

            // else spawn the boys
            <<< "spawning at t=", curr_time_sec >>>;
            if (spawn.location_type == SpawnLocation_Random) {
                T.assert(spawn.spawn_group_radius == 0, "mistake: random spawn location specified nonzero group radius");
                repeat (spawn.number) {
                    EnemyAdd(spawn.enemy_type, M.randomPointInCircle(bard_pos, inner_r, outer_r), this.wave.wave_number);
                }
            }
            else if (spawn.location_type == SpawnLocation_Group) {
                // adjust inner_r so that the radius of the spawn group doesn't intersect with bard circle
                M.randomPointInCircle(bard_pos, inner_r + spawn.spawn_group_radius, outer_r) => vec2 group_center;
                repeat (spawn.number) {
                    EnemyAdd(
                        spawn.enemy_type, 
                        M.randomPointInCircle(group_center, 0, spawn.spawn_group_radius), 
                        this.wave.wave_number
                    );
                }
            }
            else {
                T.err("unknown spawn location " + spawn.location_type);
            }
        }
    }

    UI_Int spawner_wave_num;
    fun void ui() {
        // wave and spawn UI
        UI.separatorText("wave");
        if (UI.listBox("wave", curr_wave, wave_names)) {
            spawn_manager.register(wave_list[curr_wave.val()]);
        }

        UI.inputInt("Manual Spawn Wave# ", this.spawner_wave_num);

        for (1 => int i; i < enemy_type_list.size(); ++i) {
            enemy_type_list[i] @=> Enemy et;
            UI.pushID(et.enemy_type);
            if (UI.button("Spawn " + et.name)) {
                EnemyAdd(et.enemy_type, M.randomPointInCircle(bard.pos(), spawn_inner_r.val(), spawn_outer_r.val()), this.spawner_wave_num.val());
            }

            UI.popID();
        }

    }
}

// == WAVES ======================================================

class Wave {
    int wave_number;
    Spawn spawn_list[];
}

UI_Int curr_wave(0);
Wave wave_list[0];
string wave_names[0];

fun void WaveAdd(Spawn spawn_list[])
{
    Wave w;
    wave_list.size() => w.wave_number;
    spawn_list @=> w.spawn_list;
    wave_list << w;
    wave_names << ("Wave " + w.wave_number);
}

//        enemy_type,      location_type,          group_radius,  number,   timing_type,       pre_delay, interval
WaveAdd([ // wave 1
new Spawn(EnemyType_Basic,  SpawnLocation_Random,  0,             1,        SpawnTiming_Loop,  0,         3)
]);
WaveAdd([ // wave 2
new Spawn(EnemyType_Basic,  SpawnLocation_Random,  0,             2,        SpawnTiming_Loop,  0,         3),
new Spawn(EnemyType_Pursuer,SpawnLocation_Group,   1,             4,        SpawnTiming_Loop,  5,         4)
]);
WaveAdd([ // wave 3
new Spawn(EnemyType_Basic,  SpawnLocation_Random,  0,             2,        SpawnTiming_Loop,  0,         2),
new Spawn(EnemyType_Pursuer,SpawnLocation_Group,   1,             4,        SpawnTiming_Loop,  3,         3),
new Spawn(EnemyType_Charger,SpawnLocation_Random,  0,             1,        SpawnTiming_Loop,  5,         4),
]);



// == rooms (maybe this was a bad idea) ================================================

class ArenaRoom extends Room {
"arena" => room_name;
fun void ui() {}
fun void enter() {
    fog_mesh --> GG.scene();
}
fun void leave() {
    fog_mesh.detachParent();
}

fun Room update(float dt) { 

curr_wave.val() => int wave_number;

// == handle voice commands ================================================
voice.activated() @=> int activated[];
for (int c : activated) {
    CommandActivate(c);
}

// == process collisions ================================================
b2World.sensorEvents(b2_world_id, begin_sensor_events, null);
for (int i; i < begin_sensor_events.size(); 2 +=> i) {
    begin_sensor_events[i] => int sensor_shape_id;
    begin_sensor_events[i+1] => int visitor_shape_id;

    // might be invalid because we destroy in prior iteration
    if (!b2Shape.isValid(sensor_shape_id) || !b2Shape.isValid(visitor_shape_id)) continue;

    EntityFromShape(sensor_shape_id) @=> Entity sensor;
    EntityFromShape(visitor_shape_id) @=> Entity visitor;


    // collide weapon
    if (sensor.entity_type == EntityType_Weapon) {
        if (visitor.entity_type == EntityType_Enemy)
            visitor.takeDamage(weapon_type_list[sensor.weapon_type].base_dmg.val());
        else if (visitor.entity_type == EntityType_Cart)
            visitor.heal(weapon_type_list[sensor.weapon_type].base_dmg.val());
    }
    // collide player projectile
    else if (sensor.entity_type == EntityType_PlayerProjectile) {
        <<< "projectile collision" >>>;

        spork ~ FX.bulletHitEffect(sensor.pos(), .3);
        EntityReturn(sensor);

        if (visitor.entity_type == EntityType_Static) { 
        }
        else if (visitor.entity_type == EntityType_Enemy) {
            visitor.takeDamage(sensor.projectile_dmg);
        } else {
            T.err("unhandled player projectile collision with entity type " + visitor.entity_type);
        }
    }
    else if (sensor.entity_type == EntityType_EnemyProjectile) {
        EntityReturn(sensor);
        if (visitor.entity_type & (EntityType_Player | EntityType_Cart)) {
            visitor.takeDamage(sensor.projectile_dmg);
        }
    } else {
        <<< "unhandled sensor event between", entity_type_names.getStr(sensor.entity_type), entity_type_names.getStr(visitor.entity_type) >>>;
    }

    // z.returnEntity(sensor);
}

b2World.contactEvents(b2_world_id, begin_touch_events, end_touch_events, null);
for (int i; i < begin_touch_events.size(); 2 +=> i) {
    begin_touch_events[i] => int touch_shape_a;
    begin_touch_events[i+1] => int touch_shape_b;
    if (!b2Shape.isValid(touch_shape_a) || !b2Shape.isValid(touch_shape_b)) continue;
    b2Shape.body(touch_shape_a) => int touch_body_id_a;
    b2Shape.body(touch_shape_b) => int touch_body_id_b;
    if (!b2Body.isValid(touch_body_id_a) || !b2Body.isValid(touch_body_id_b)) continue;
    EntityGet(touch_body_id_a) @=> Entity a;
    EntityGet(touch_body_id_b) @=> Entity b;

    a.entity_type & EntityType_Enemy => int a_is_enemy;
    b.entity_type & EntityType_Enemy => int b_is_enemy;

    <<< "contact event", entity_type_names.getStr(a.entity_type), entity_type_names.getStr(b.entity_type) >>>;

    // contact enemy
    T.assert(!a_is_enemy, "assuming enemy is always second in contact event pair");
    if (b_is_enemy) {
        b @=> Entity@ enemy;
        if (a.entity_type & (EntityType_Player | EntityType_Cart)) {
            T.assert(
                (enemy.touching_player_count == 0 && enemy.touching_player_id == 0)
                ||
                (enemy.touching_player_count > 0 && b2Body.isValid(enemy.touching_player_id)),
                "enemy contact semantics"
            );
            a.b2_body_id => enemy.touching_player_id;
            ++enemy.touching_player_count;
        }
        else {
            T.err("unhandled enemy contact with entity type " + a.entity_type);
        }
    }

    // a.entity_type & EntityType_Spell => int a_is_spell;
    // b.entity_type & EntityType_Spell => int b_is_spell;
    // a.entity_type & EntityType_Player => int a_is_player;
    // b.entity_type & EntityType_Player => int b_is_player;

    // T.assert(!(a_is_spell && b_is_spell), "spells should not collide with each other");
    // if (a_is_spell) Spell.collide(a, b);
    // if (b_is_spell) Spell.collide(b, a);
    // if (a_is_player) playerCollide(a, b);
    // if (b_is_player) playerCollide(b, a);
}

for (int i; i < end_touch_events.size(); 2 +=> i) {
    end_touch_events[i] => int touch_shape_a;
    end_touch_events[i+1] => int touch_shape_b;
    if (!b2Shape.isValid(touch_shape_a) || !b2Shape.isValid(touch_shape_b)) continue;
    b2Shape.body(touch_shape_a) => int touch_body_id_a;
    b2Shape.body(touch_shape_b) => int touch_body_id_b;
    if (!b2Body.isValid(touch_body_id_a) || !b2Body.isValid(touch_body_id_b)) continue;
    EntityGet(touch_body_id_a) @=> Entity a;
    EntityGet(touch_body_id_b) @=> Entity b;

    a.entity_type & EntityType_Enemy => int a_is_enemy;
    b.entity_type & EntityType_Enemy => int b_is_enemy;

    T.assert(!a_is_enemy, "assuming enemy is always second in end_contact event pair");
    if (b_is_enemy) {
        b @=> Entity@ enemy;
        if (a.entity_type & (EntityType_Player | EntityType_Cart)) {
            T.assert(enemy.touching_player_count > 0, "enemy player contact refcoount busted");
            --enemy.touching_player_count;
            if (enemy.touching_player_count == 0) {
                0 => enemy.touching_player_id;
            }
        }
        else {
            T.err("unhandled enemy contact with entity type " + a.entity_type);
        }
    }
}

// == input ================================================

for (int player_idx; player_idx < player_list.size(); ++player_idx) {
    player_list[player_idx] @=> Entity player;

    if (player.player_type == PlayerType_Bard) continue; // maybe the bard shouldn't be a player?
    if (player.dead) continue;

    player_type_list[player.player_type] @=> Player pt;

    if (0) { // WASD controls
        vec2 dir;
        if (GWindow.key(GWindow.KEY_A)) 1 -=> dir.x;
        if (GWindow.key(GWindow.KEY_D)) 1 +=> dir.x;
        if (GWindow.key(GWindow.KEY_S)) 1 -=> dir.y;
        if (GWindow.key(GWindow.KEY_W)) 1 +=> dir.y;
        dir.normalize();
        if (dir.x != 0) dir.x => player.last_dir_x;
        player.vel(dir * pt.base_speed.val());
    }

    if (1) { // arrow controls
        vec2 dir;
        player.pos() => vec2 start_pos;
        if (GWindow.key(GWindow.KEY_LEFT)) 1 -=> dir.x;
        if (GWindow.key(GWindow.KEY_RIGHT)) 1 +=> dir.x;
        if (GWindow.key(GWindow.KEY_DOWN)) 1 -=> dir.y;
        if (GWindow.key(GWindow.KEY_UP)) 1 +=> dir.y;
        dir.normalize();
        if (dir.x != 0) dir.x => player.last_dir_x;
        player.vel(dir * pt.base_speed.val());

        // abilities
        if (GWindow.keyDown(GWindow.KEY_Q)) {
            // blink!
            player.pos(player.pos() + 2.2 * dir);
            // draw trail
            spork ~ FX.dashTrail(start_pos, player.pos());
            // generate inspo
            BardAddInspo(1, player.pos());
        }
    }

    // clamp player position to screen bounds
    g.ndc2world(1, 1) => vec2 screen_max;
    g.ndc2world(-1, -1) => vec2 screen_min;
    M.clamp(player.pos(), screen_min, screen_max) => player.pos;
}

{ // verify alive players are contiguous and at start of array
for (int i; i < players_num_alive; ++i) T.assert(!player_list[i].dead, "dead player at front");
for (players_num_alive => int i; i < player_list.size(); ++i) T.assert(player_list[i].dead, "alive player at end");
}

// == update ================================================

{ // update and draw pickups
g.pushLayer(Layer_Pickup);
    Pickup.visual_radius.val() => float r;
    (Pickup.travel_speed.val() * dt) => float travel_dist;
    2 * r * r => float threshold_squared;
    for (pickup_pool_count - 1 => int i; i >= 0; i--) {
        pickup_pool[i] @=> Pickup p;

        // if already attracted to player, continue lerping to their position
        if (b2Body.isValid(p.target_body_id)) {
            b2Body.position(p.target_body_id) => vec2 target_pos;

            // linear interpolation
            M.dist2(target_pos, p.pos) => float d2;
            if (travel_dist * travel_dist >= d2) {
                currency++;
                // @TODO animation
                PickupRet(p);
                continue;
            } else {
                // move towards player
                (target_pos - p.pos) => vec2 dir; dir.normalize();
                travel_dist * dir +=> p.pos;
            }
        } else {
            // look for nearest player
            for (int player_idx; player_idx < player_list.size(); ++player_idx) {
                player_list[player_idx] @=> Entity player;

                Player.base_pickup_radius.val() * Player.base_pickup_radius.val() => float pickup_radius_squared;

                // begin attracting
                if (M.dist2(p.pos, player.pos()) <= pickup_radius_squared) {
                    player.b2_body_id => p.target_body_id;
                }
            }
        }

        // draw pickup
        // @TODO this doesn't look very visible
        g.circleFilled(p.pos, r, Color.YELLOW);
    }
g.popLayer();
}


{ // update cart
if (cart_move_state == Cart_MoveState_Roam ) 
{ 
    .5 * .75 * field_len.val() => float hw;
    // debug draw bounds
    g.pushLayer(Layer_DebugDraw);
    g.square(@(0,0), 0, 2*hw, Color.WHITE);
    g.popLayer();

    if (M.dist(cart.pos(), cart_target) < .05) {
        M.randomPointInCircle(cart.pos(), 1, spawn_inner_r.val()) => cart_target;

        // cart stays within inner 75% of the arena


        // bounce away from edges
        if (cart_target.x > hw) cart.pos().x - (cart_target.x - cart.pos().x) => cart_target.x;
        if (cart_target.x < -hw) cart.pos().x + (cart.pos().x - cart_target.x) => cart_target.x;
        if (cart_target.y > hw) cart.pos().y - (cart_target.y - cart.pos().y) => cart_target.y;
        if (cart_target.y < -hw) cart.pos().y + (cart.pos().y - cart_target.y) => cart_target.y;
        T.assert(M.inside(cart_target, M.aabb(@(0,0), hw, hw)), "cart target is not inside bounds");
        T.assert(M.dist(cart_target, cart.pos()) < spawn_inner_r.val(), "cart target is not inside spawn_inner_r");
    }
    cart_speed.val() * M.dir(cart.pos(), cart_target) => cart.vel;

    // wheel anim
    2 => float cart_wheel_speed;
    if (cart.pos().x < cart_target.x) cart_wheel_speed * dt -=> cart_wheel_rot;
    else                              cart_wheel_speed * dt +=> cart_wheel_rot;
} else {
    @(0, 0) => cart.vel;
}

// cart upgrades
if (UpgradeHas(Upgrade_Cart_Ambulance)) {
    dt -=> cart_heal_cd;
    if (cart_heal_cd <= 0) {
        cart_heal_base_radius.val() * cart_heal_radius_mult.val() => float heal_radius;
        heal_radius * heal_radius => float heal_radius_squared;
        cart_heal_period_sec.val() +=> cart_heal_cd;

        g.pushLayer(Layer_Background + 1);
        spork ~ FX.ripple(
            cart.pos(), 
            heal_radius,
            .2::second,
            Color.GREEN
        );
        g.popLayer();

        cart_heal_base_hp_per_sec.val() * cart_heal_amt_mult.val() * cart_heal_period_sec.val() => float heal_amt;

        // heal players in vicinity
        for (Entity player : player_list) {
            if (M.dist2(player.pos(), cart.pos()) < heal_radius_squared) {
                // @TODO entity heal anim
                Math.min(player.hp_curr.val() + heal_amt, player.hp_max.val()) => player.hp_curr.val;
            }
        }
    }
}

// tower upgrade
for (int i; i < cart_arrow_tower_count; i++) {
    EnemyClosest(cart.pos(), arrow_tower_atk[i].range()) @=> Entity target;
    if (target == null) continue;

    Fire(arrow_tower_atk[i], dt, ProjectileType_Arrow, cart.pos(), target.pos());
}

} // update cart

{ // update bard
if (bard.in_cart) {
    cart.pos() + @(0, .5 * cart_size.val().y) => bard.pos;
} else {
    if (M.dist(bard.pos(), bard_target) < .05) {
        M.randomPointInCircle(bard.pos(), .5, 1.5) => bard_target;
    }
    bard_base_speed.val() * M.dir(bard.pos(), bard_target) => bard.vel;
}

// bard spells 
{ // cast fireball 
    // for now, casting spells in spawn_outer_r 
    // @TODO probably want to add a bard range stat
    .5 * (spawn_outer_r.val() + spawn_inner_r.val()) => float spell_range;
    EntityInRadius(
        bard.pos(), spell_range, EntityType_Spell,
        EntityType_Enemy | EntityType_Player
    ) @=> int entities_in_range[];

    10 => float SPELL_COST;

    dt -=> bard_fireball_cd;
    UpgradeLevel(Upgrade_Bard_Fireball) => int fireball_level;
    if (fireball_level && bard_fireball_cd <= 0 && bard_inspo_curr.val() >= SPELL_COST) {
        Spell.fireball_cd.val() => bard_fireball_cd;
        BardSpendInspo(SPELL_COST);

        for (int i; i < fireball_level; ++i) {
            vec2 target;
            if (i < entities_in_range.size()) {
                // fire at random one
                b2Body.position(
                    entities_in_range[Math.random2(0, entities_in_range.size()-1)]
                ) => target;
                // random jitter
                M.randomPointInCircle(target, 0, .5 * Spell.fireball_explosion_radius.val()) => target;
            } else {
                // not enough entities, fire at random point
                M.randomPointInCircle(bard.pos(), 0, spell_range) => target;
            }
            // cast fireball
            SpellCast(CommandType_Fireball, target);
        }
    }

    dt -=> bard_lightning_cd;
    UpgradeLevel(Upgrade_Bard_Lightning) => int lightning_level;
    if (lightning_level && bard_lightning_cd <= 0 && bard_inspo_curr.val() >= SPELL_COST)  {
        Spell.lightning_cd.val() => bard_lightning_cd;
        BardSpendInspo(SPELL_COST);
        for (int i; i < lightning_level; ++i) {
            vec2 target;
            if (i < entities_in_range.size()) {
                // fire at random one
                b2Body.position(
                    entities_in_range[Math.random2(0, entities_in_range.size()-1)]
                ) => target;
            } else {
                // not enough entities, fire at random point
                M.randomPointInCircle(bard.pos(), 0, spell_range) => target;
            }
            repeat (3) { // fire 3 lightning per level
                // jitter
                M.randomPointInCircle(target, 0, 4 * Spell.lightning_blast_radius.val()) => target;
                // cast lightning
                SpellCast(CommandType_Lightning, target);
            }
        }
    }

}




fog_mesh.pos(bard.pos());
}

// update player
for (int player_idx; player_idx < player_list.size(); ++player_idx) {
    player_list[player_idx] @=> Entity e;

    if (e.sprite_tex != null) {
        M.mag(e.vel()) > 0 => int moving;
        if (moving | e == bard) {
            dt +=> e.sprite_time_since_last_frame;
            if (e.sprite_time_since_last_frame >= animation_secs_per_frame.val()) {
                // advance to next frame
                0 => e.sprite_time_since_last_frame;
                (e.sprite_current_frame + 1) % e.sprite_num_frames => e.sprite_current_frame;
            } 
        }
    }

    // generate inspo
    // Q: should bard be able to generate their own inspo?
    dt -=> e.inspo_generation_cd;
    if (e.inspo_generation_cd <= 0 && e != bard) {
        Player.base_inspo_cd.val() => e.inspo_generation_cd;
        BardAddInspo(1, e.pos());
    }
}

// update entity
for (int entity_idx; entity_idx < entity_count; ++entity_idx) {
    entity_pool[entity_idx] @=> Entity e;
    e.pos() => vec2 pos;

    // update sprite anim
    if (e.sprite_tex != null) {
        dt +=> e.sprite_time_since_last_frame;
        if (e.sprite_time_since_last_frame >= animation_secs_per_frame.val()) {
            // advance to next frame
            0 => e.sprite_time_since_last_frame;
            (e.sprite_current_frame + 1) % e.sprite_num_frames => e.sprite_current_frame;
        } 
    }

    // update enemy
    if (e.entity_type == EntityType_Enemy) {
        enemy_type_list[e.enemy_type] @=> Enemy et;

        // all enemies do contact dmg, process that first
        dt -=> e.enemy_contact_dmg_cd;
        if (e.touching_player_count > 0 && e.enemy_contact_dmg_cd <= 0) {
            Enemy.contact_dmg_cd.val() => e.enemy_contact_dmg_cd; // reset cd
            EntityGet(e.touching_player_id) @=> Entity player;
            player.takeDamage(et.dmg(wave_number));

            // lurch towards player as attack animation
            // M.dir(pos, player.pos()) => vec2 attack_dir;
            // e.pos(pos + Enemy.contact_attack_anim_amt.val() * attack_dir);
        }

        if (e.enemy_type == EnemyType_Basic) {
            // find closest player if not touching
            if (e.touching_player_count == 0) {
                PlayerClosest(e.pos()) @=> Entity@ closest_player;
                vec2 dir_to_closest_player;
                if (closest_player != null) M.dir(e.pos(), closest_player.pos()) => dir_to_closest_player;

                e.vel(dir_to_closest_player * e.speed);
            } else {
                // else stop moving and look towards target
                @(0,0) => e.vel;
            }
        } 
        else if (e.enemy_type == EnemyType_Pursuer) {
            if (e.touching_player_count == 0) {
                EntityGet(e.target_player_id) @=> Entity target_player;

                // select valid player target
                if (target_player == null || target_player.dead) PlayerRandomLiving() @=> target_player;

                if (target_player != null) {
                    target_player.b2_body_id => e.target_player_id;
                    M.dir(e.pos(), target_player.pos()) => vec2 dir;
                    e.vel(dir * e.speed);
                    e.rot(dir);
                }
            }
        } 
        else if (e.enemy_type == EnemyType_Charger) {
            // @TODO add some kind of charging logic...
            // for now just copying basic enemy logic
            if (e.enemy_state == EnemyState_Default) {
                PlayerClosest(e.pos()) @=> Entity@ closest_player;

                M.dist2(pos, closest_player.pos()) => float d2;

                // move towards player
                vec2 dir_to_closest_player;
                if (closest_player != null) M.dir(e.pos(), closest_player.pos()) => dir_to_closest_player;
                b2Body.linearVelocity(
                    e.b2_body_id, 
                    dir_to_closest_player * e.speed
                );
                
                // if close enough and attack off cd
                Enemy.charger_charge_range.val() * Enemy.charger_charge_range.val() => float charge_threshold;
                if (draw_b2_debug.val()) g.circleDotted(pos, Enemy.charger_charge_range.val(), 0, Color.WHITE);
                dt -=> e.attack_cd;
                if (e.attack_cd < 0 && d2 < charge_threshold) {
                    EnemyState_Pre => e.enemy_state;
                    Enemy.charger_tell_dur.val() => e.attack_cd; // reusing attack cd for tell cd
                    // stop moving and rotating, face target direction
                    @(0, 0) => e.vel;
                    0 => e.angularVel;
                    e.lookAt(closest_player.pos());
                }
            }
            else if (e.enemy_state == EnemyState_Pre) {
                // charger tell
                dt -=> e.attack_cd;
                if (e.attack_cd <= 0) {
                    Enemy.charger_charge_dur.val() => e.attack_cd;
                    EnemyState_Act => e.enemy_state;

                    // charge!
                    b2Body.impulse(e.b2_body_id, Enemy.charger_charge_impulse.val() * e.rot(), e.pos(), true);
                    b2Body.torque(e.b2_body_id, Enemy.charger_charge_torque.val(), true);
                }

                // draw lookat dir
                g.line(pos, pos + 2 * e.rot(), Color.RED);
            }
            else if (e.enemy_state == EnemyState_Act) {
                dt -=> e.attack_cd;
                if (e.attack_cd < 0) {
                    Enemy.charger_charge_cd.val() => e.attack_cd;
                    @(0, 0) => e.vel;
                    Math.random2f(.5, 1.0) * (maybe ? -1 : 1) => e.angularVel;
                    EnemyState_Default => e.enemy_state;
                }
            }
        }
        else if (e.enemy_type == EnemyType_Shooter) {
            M.normalize(e.vel()) => vec2 dir;
            M.perp(dir) => vec2 perp;
            // behavior idea:
            // stays around perimeter of inner circle, (fixed distance from bard) 
            // shoots bullets in 4 directions
            M.dir(e.pos(), bard.pos()) => vec2 dir_to_bard;
            M.dist(e.pos(), bard.pos()) => float d;

            if (d > spawn_inner_r.val()) {
                // move towards bard
                dir_to_bard +=> dir;
            } 
            if (d < spawn_inner_r.val() - 1) {
                // move way from bard
                -1*dir_to_bard +=> dir;
            } 
            // orbit bard
            M.perp(dir_to_bard) +=> dir;
            e.moveDir(dir);

            // fire on cd
            dt -=> e.attack_cd;
            if (e.attack_cd <= 0) {
                projectile_type_list[ProjectileType_EnemyBasic] @=> Projectile pt;
                et.dmg(wave_number) => float dmg;
                EnemyFire(ProjectileType_EnemyBasic, pos, UP, et.ability_speed.val(), dmg);
                EnemyFire(ProjectileType_EnemyBasic, pos, DOWN, et.ability_speed.val(), dmg);
                EnemyFire(ProjectileType_EnemyBasic, pos, LEFT, et.ability_speed.val(), dmg);
                EnemyFire(ProjectileType_EnemyBasic, pos, RIGHT, et.ability_speed.val(), dmg);
                et.ability_cd.val() +=> e.attack_cd;
            }
        } 
        else {
            T.err("unknown enemy type " + e.enemy_type);
        }
    }

    // update weapon
    if (e.entity_type == EntityType_Weapon) {
        // determine phase from the timing
        weapon_type_list[e.weapon_type] @=> Weapon w;
        
        // current idea: break entire cd into 4 equal-sized phases:
        // 1. lerp to attack target
        // 2. hold at attack target
        // 3. lerp to rest position
        // 4. rest position

        w.base_range.val() => float weapon_range;
        EntityGet(e.player_id) @=> Entity p; 
        T.assert(p != null, "weapon must belong to player");

        // @TODO rest_pos should be based on number of weapons held by player
        // p.pos() + @(0, -.5 * player_size.val()) => vec2 rest_pos;
        p.pos() + weapon_rest_pos.val() => vec2 rest_pos;
        dt -=> e.attack_cd;

        // check for enemies in range
        if (e.weapon_state == WeaponState_Rest) {
            T.assert(!b2Shape.areSensorEventsEnabled(e.b2_shape_id), "sensors enabled on weapon at rest");
            e.pos(rest_pos);

            // default rest angle
            e.rot(UP);

            // query for enemies in range
            // @optimize, when looping over enemies, calculate closest enemy to each weapon, don't need to query 
            // at all
            EntityInRadius(e.pos(), weapon_range, EntityType_Weapon, EntityType_Enemy) @=> int enemies_in_range[];

            // chisel can heal cart
            if (cart.hp_curr.val() > 0 && cart.hp_curr.val() < cart.hp_max.val()) enemies_in_range << cart.b2_body_id;

            if (enemies_in_range.size() > 0) {
                EntityClosestFromBody(e.pos(), enemies_in_range) @=> Entity target;
                e.lookAt(target.pos());

                // attack!
                if (e.attack_cd <= 0) {
                    WeaponState_Attack => e.weapon_state;
                    b2Shape.enableSensorEvents(e.b2_shape_id, true);
                    // weapon_speed.val() * M.dir(e.pos(), target.pos()) => e.vel;

                    // reset cd
                    w.base_cd_sec.val() => e.attack_cd;
                }
            }
        } 
        else if (e.weapon_state == WeaponState_Attack) {
            // don't zeno interp because there's no dt
            T.assert(b2Shape.areSensorEventsEnabled(e.b2_shape_id), "sensor not enabled on weapon");

            M.dist(e.pos(), rest_pos) => float dist_from_rest;
            dt * weapon_speed.val() => float dist_to_travel;

            // lerping to target in first half of cd
            if (e.attack_cd > .5 * w.base_cd_sec.val()) {
                Math.min(dist_from_rest + dist_to_travel, weapon_range) => float dist;
                rest_pos + dist * e.rot() => e.pos;
            } else { // lerping back to rest pos in second half of cd
                // made it back to rest pos, transition to rest state
                if (dist_from_rest < dist_to_travel) {
                    WeaponState_Rest => e.weapon_state;
                    rest_pos => e.pos;
                    b2Shape.enableSensorEvents(e.b2_shape_id, false);
                } else {
                    // lerp back to rest pos
                    rest_pos + Math.max(dist_from_rest - dist_to_travel, 0) * e.rot() => e.pos;
                }
            }
        } 
        else {
            T.err("unrecognized weapon state " + e.weapon_state);
        }
    }
}

// update camera
// @TODO scale with dt
GG.camera().pos() $ vec2 + camera_interp_rate.val() * (bard.pos() - GG.camera().pos() $ vec2) => GG.camera().pos;
if (draw_b2_debug.val()) {
    g.pushLayer(Layer_DebugDraw);
    g.circleFilled(GG.camera().pos() $ vec2, .01, Color.DARKGRAY);
    g.circle(GG.camera().pos() $ vec2, .03, Color.DARKGRAY);
    g.popLayer();
}
GG.camera().viewSize() + 4 * dt * GWindow.scrollY() => GG.camera().viewSize;

// == update and draw spells ================================================
for (active_spells.size()-1 => int i; i >= 0; --i) {
    active_spells[i] @=> Spell s;
    if (s.spell_phase == SpellPhase_Done) {
        active_spells.erase(i);
        continue;
    }

    dt +=> s.total_time;
    dt +=> s.phase_time;

    T.assert(
        s.spell_phase >= SpellPhase_Init && s.spell_phase < SpellPhase_Done,
         "unknown spell phase " + s.spell_phase + " for spell " + s.command_type);

    g.pushLayer(Layer_Projectile);

    if (s.command_type == CommandType_Fireball) {
        if (s.spell_phase == SpellPhase_Init) {
            // aim (target should already be set here)
            bard.pos() => s.start;
            s.target - s.start => s.relative_target;
            s.transition(SpellPhase_Pre);
        }
        else if (s.spell_phase == SpellPhase_Pre) {
            s.phase_time / Spell.fireball_tell_time_sec.val() => float t;
            s.relative_target + bard.pos() => vec2 target;

            // draw the tell (just charge up the ball)
            M.dir(bard.pos(), target) => vec2 aim;
            g.chevron(
                bard.pos() + aim, M.angle(aim), Math.pi/2, player_size.val(), .4, 
                Color.RED
            );
            g.circle(bard.pos(), t * t * Spell.fireball_radius.val(), 1.0, t * t * Color.RED);

            // draw the landing position
            g.circle(target, Spell.fireball_explosion_radius.val() * t * t, Color.RED);
            
            // transition
            if (s.phase_time >= s.fireball_tell_time_sec.val())  {
                s.transition(SpellPhase_Active);
                // lock in absolute target
                target => s.target;
            }
        }
        else if (s.spell_phase == SpellPhase_Active) {
            // assume the fireball always takes same time to reach target, no matter how far
            s.phase_time / s.fireball_travel_time_sec.val() => float t;

            // if reached destination
            if (t >= 1) {
                // do damage
                EntityInRadius(
                    s.target, Spell.fireball_explosion_radius.val(), EntityType_Spell,
                    EntityType_Enemy | EntityType_Player
                ) @=> int entities[];
                for (auto entity : entities) {
                    EntityGet(entity).takeDamage(Spell.fireball_dmg.val());
                }

                // draw an explosion circle
                spork ~ FX.circleFlash(
                    s.target, Spell.fireball_explosion_radius.val(), 
                    .1::second, .5*Color.WHITE);

                // mark for cleanup
                s.transition(SpellPhase_Done);
            } else {
                // travel to target
                M.lerp(t, s.start, s.target) => vec2 pos; 

                // drop shadow
                g.pushLayer(Layer_Shadow);
                g.pushBlend(g.BLEND_MULT);
                g.ellipseFilled(pos, @(Spell.fireball_radius.val(), .2), .1 * Color.WHITE);
                g.popBlend(); g.popLayer();

                // fireball
                M.parabola(t, 1) +=> pos.y;
                Spell.fireball_radius.val() * .9 +=> pos.y;
                g.circle(pos, Spell.fireball_radius.val(), 1.0, Color.RED);

                // draw tell
                g.circle(s.target, Spell.fireball_explosion_radius.val(), Color.RED);
            }
        }
        else if (s.spell_phase == SpellPhase_Post) {
            // cleanup, any "after-glow" fx
        } 
    }
    else if (s.command_type == CommandType_Lightning) {
        if (s.spell_phase == SpellPhase_Init) {
            s.transition(SpellPhase_Pre);
        }
        else if (s.spell_phase == SpellPhase_Pre) {
            s.phase_time / Spell.lightning_tell_time_sec.val() => float t;

            // draw the landing position
            g.attackTell(s.target, 2 * Spell.lightning_blast_radius.val(), Spell.lightning_blast_radius.val());
            
            // transition
            if (t > 1) s.transition(SpellPhase_Active);
        }
        else if (s.spell_phase == SpellPhase_Active) {
            spork ~ FX.lightning(s.target, 12, .5, 3);
            spork ~ FX.lightning(s.target, 12, .5, 3);
            // reached destination
            EntityInRadius(
                s.target, Spell.lightning_blast_radius.val(), EntityType_Spell,
                EntityType_Enemy | EntityType_Player
            ) @=> int entities[];
            for (auto entity : entities) {
                EntityGet(entity).takeDamage(Spell.lightning_dmg.val());
            }

            // mark for cleanup
            s.transition(SpellPhase_Done);

            // draw blast
            spork ~ FX.circleFlash(
                s.target, Spell.lightning_blast_radius.val(), 
                .1::second, .5*Color.WHITE);
        }
        else if (s.spell_phase == SpellPhase_Post) {
            // cleanup, any "after-glow" fx
        } 
    }
    else {
        T.err("unrecognized spell type: " + s.command_type);
    }

    g.popLayer();
}



// == draw ================================================
// draw playfield
g.pushLayer(Layer_Background);
g.pushPolygonRadius(.2);
// g.squareFilled(@(0,0), 0, field_len.val(), field_color.val());
g.sprite(ground_texture, @(0, 0), 1.02 * field_len.val(), 0);
g.popPolygonRadius();
g.popLayer();

// draw bard (sprite is drawn in player update loop)
{
    if (draw_b2_debug.val()) {
    g.pushLayer(Layer_DebugDraw); 
    g.circle(bard.pos(), spawn_inner_r.val(), Color.RED);
    g.circle(bard.pos(), spawn_outer_r.val(), Color.RED);
    if (!bard.in_cart) g.dashed(bard_target, bard.pos(), Color.WHITE, .1);
    g.popLayer(); 
    }
        // bard voice transcription
    g.pushTextMaxWidth(2);
    g.pushTextControlPoint(.5, 0);
    voice.transcription_raw => string bard_text;
    bard_text.replace("[BLANK_AUDIO]", "...");
    g.text(bard_text, bard.pos() + @(0, .3), .4);
    g.popTextControlPoint();
    g.popTextMaxWidth();

    // bard inspo bar
    g.ndc2world(0, .9) => vec2 inspo_bar_pos;
    g.progressBar(
        bard_inspo_curr.val() / bard_inspo_max.val(),  // %
        inspo_bar_pos,
        8,
        .5,
        Color.WHITE
    );
    
    g.pushColor(Color.DARKGRAY);
    g.text(bard_inspo_curr.val()$int + "/" + bard_inspo_max.val()$int, inspo_bar_pos, .5);
    g.popColor();
}

{ // draw cart
    g.pushLayer(Layer_Cart);
    M.lerp(cart.hit_cd.val(), Color.WHITE, Color.RED) => vec3 color;
    g.sprite(cart_sprite, cart.pos(), cart_size.val().x, 0, color);
    g.dashed(cart_target, cart.pos(), Color.BROWN, .1);

    // g.boxFilled(
    //     cart.pos(),
    //     0,
    //     cart_size.val().x,
    //     cart_size.val().y,
    //     Color.DARKBROWN
    // 	);
    // wheels
    .21 => float wheel_radius;
    cart.pos() + @(-cart_size.val().x * .25, -.3 * cart_size.val().y ) => vec2 wheel1;
    cart.pos() + @(cart_size.val().x * .25, -.3 * cart_size.val().y ) => vec2 wheel2;
    // g.circleFilled(wheel1, wheel_radius, Color.BLACK);
    // g.circleFilled(wheel2, wheel_radius, Color.BLACK);
    // g.sprite(wheel_sprite, wheel1, 2 * wheel_radius, cart_wheel_rot);
    // g.sprite(wheel_sprite, wheel2, 2 * wheel_radius, cart_wheel_rot);
    g.sprite(wheel_sprite, wheel1, 2 * wheel_radius, cart_wheel_rot);
    g.sprite(wheel_sprite, wheel2, 2 * wheel_radius, cart_wheel_rot);
    g.popLayer();

    if (draw_b2_debug.val()) {
        g.pushLayer(Layer_DebugDraw);
        g.circleDotted(cart.pos(), arrow_tower_atk[0].range(), 0, Color.BROWN);
        g.popLayer();
    }

    // health bar
    if (cart.hp_curr.val() < cart.hp_max.val()) {
        g.pushLayer(Layer_HP);
        g.progressBar(cart.hp_curr.val() / cart.hp_max.val(), 
            cart.pos() + @(0, -.5 * cart_size.val().x),
            cart_size.val().x, .2 * cart_size.val().x,
            Color.GREEN
        );
        g.popLayer();
    }
}


// g.(bard_target, bard.pos(), Color.WHITE, .1);

// draw player
// g.pushLayer(Layer_Player);
for (int player_idx; player_idx < player_list.size(); ++player_idx) {
    player_list[player_idx] @=> Entity player;
    T.assert(player.player_type, "player has invalid type");

    player.pos() => vec2 pos;

    // draw health bar
    if (player.hp_curr.val() < player.hp_max.val()) {
        g.progressBar(player.hp_curr.val() / player.hp_max.val(), 
            pos + @(0, .67 * player_size.val()),
            player_size.val(), .2 * player_size.val(),
            Color.GREEN
        );
    }

    M.lerp(player.hit_cd.val(), Color.WHITE, Color.RED) => vec3 color;
    // if (player.player_type == PlayerType_Carpenter) {
    //     // @TODO sprite should be field of Entity
    //     g.sprite(knight_sprite, pos, player_size.val(), 0, color);
    // }

    // draw player sprite
    if (player.sprite_tex != null) {
        player_size.val() * @(1,1) => vec2 sprite_sca;
        if (player.last_dir_x < 0) -1 *=> sprite_sca.x;
        g.sprite(
            player.sprite_tex, @(player.sprite_num_frames, 1), 
            @(player.sprite_current_frame $ float / player.sprite_num_frames, 0), 
            player.pos(), sprite_sca, 0, color
        );
    }
}
// g.popLayer();

// draw entity
for (int entity_idx; entity_idx < entity_count; ++entity_idx) {
    entity_pool[entity_idx] @=> Entity e;
    e.pos() => vec2 pos;

    if (e.entity_type == EntityType_Static) {
        // not drawing borders for now
    } else if (e.entity_type == EntityType_Player) {
        T.assert("player should be in player list, not entity pool!");
    } 
    // draw enemy
    else if (e.entity_type == EntityType_Enemy) {
        g.pushLayer(Layer_Enemy);
        enemy_type_list[e.enemy_type] @=> Enemy et;

        e.hit_cd.val() * Color.WHITE + Color.BLACK => vec3 color;

        if (e.enemy_type == EnemyType_Basic) {
            et.size.val().x => float w;
            et.size.val().y => float h;

            // animation
            now/second - e.spawn_time_sec => float t;
            Math.sin(3.7*t) * .08 + 1 => float xs;
            Math.cos(4*t) * .08 + 1 => float ys;

            // basic body
            g.capsuleFilled(e.pos(), xs * h/2, ys * h/4, Math.pi/2, color);

            // basic eye
            // e.rot() => vec2 dir;
            // M.perp(dir) => vec2 perp;
            // e.pos() + .4 * r * dir + .2 * r * perp => vec2 eye1;
            // e.pos() + .4 * r * dir - .2 * r * perp => vec2 eye2;
            // g.circleFilled(eye1, .08 * r, Color.YELLOW);
            // g.circleFilled(eye2, .08 * r, Color.YELLOW);
            e.pos() + .3 * h * UP + .2 * w * RIGHT => vec2 eye1;
            e.pos() + .3 * h * UP - .2 * w * RIGHT => vec2 eye2;
            g.circleFilled(eye1, .08 * w, Color.YELLOW);
            g.circleFilled(eye2, .08 * w, Color.YELLOW);

            // basic shadow
            g.shadow(e.pos() + h/2 * DOWN, 1.1 * w * ys);
        }
        else if (e.enemy_type == EnemyType_Pursuer) {
            et.size.val().x => float l;
            g.capsuleFilled(e.pos(), l, et.size.val().y, e.angle(), color);

            // pursuer eye
            g.circleFilled(e.pos() + .5 * l * e.rot(), .15 * l, Color.YELLOW);
        } 
        else if (e.enemy_type == EnemyType_Charger) {
            // Math.max(et.size.val().x, et.size.val().y) => float sca;
            // Math.sin(7 * (now/second)) * .1 + 1 => float sca;
            // Math.cos(7 * (now/second)) * .1 + 1 => float scay;
            // g.sprite(
            //     charger_sprite,
            //     25,  // spritesheet dimensions
            //     0,  // offset
            //     e.pos(),  // position
            //     @(1, sca),  // scale
            //     0,  // rotation
            //     Color.WHITE
            // );
            et.size.val().x => float w;
            et.size.val().y => float h;
            e.angle() => float rot;

            // draw charger eyes
            for (int eye; eye < 3; eye++) {
                rot + Math.two_pi / 3.0 * eye => float angle;
                g.circleFilled(
                    w * .8 * @(Math.cos(angle), Math.sin(angle)) + pos, 
                    .08, 
                    Color.YELLOW
                );
            }
            // draw charger body
            g.polygonFilled(e.pos(), e.angle(), 
                1,
                [
                @(w, 0),
                @(-.5*w, w * Math.sqrt(3)/2),
                @(-.5*w, -w * Math.sqrt(3)/2),
                ], 
            .1, color
            );
            // draw charger shadow
            g.pushLayer(Layer_Shadow);
            g.pushBlend(g.BLEND_MULT);
            g.ellipseFilled(pos - @(0, w), @(w, .1), .1 * Color.WHITE);
            g.popBlend();
            g.popLayer();
        }
        else if (e.enemy_type == EnemyType_Shooter) {
            et.size.val().x / 2 => float l;
            g.pushPolygonRadius(l/2);
            g.squareFilled(e.pos(), Math.pi/4, l, color);
            g.popPolygonRadius();

            g.pushLayer(Layer_Light);
            .14 * l => float r;
            g.circleFilled(e.pos() + .8 * l*@(1, 0), r, Color.YELLOW);
            g.circleFilled(e.pos() + .8 * l*@(-1, 0), r, Color.YELLOW);
            g.circleFilled(e.pos() + .8 * l*@(0, 1), r, Color.YELLOW);
            g.circleFilled(e.pos() + .8 * l*@(0, -1), r, Color.YELLOW);
            g.popLayer();
        }
        else {
            T.err("cannot draw enemy type " + e.enemy_type);
        }

        g.popLayer(); // pop Layer_Enemy
    }
    // draw weapon
    else if (e.entity_type == EntityType_Weapon) {
        g.pushLayer(Layer_Weapon);
        weapon_type_list[e.weapon_type] @=> Weapon w;
        // debug range
        if (draw_b2_debug.val()) {
            g.pushLayer(Layer_DebugDraw);
            g.circle(e.pos(), w.base_range.val(), Color.WHITE);
            g.popLayer();
        }

        g.sprite(chisel_sprite, pos, weapon_size.val(), e.angle());
        g.popLayer();
    }
    else if (e.entity_type == EntityType_PlayerProjectile) {
        projectile_type_list[e.projectile_type] @=> Projectile pt;
        if (e.projectile_type == ProjectileType_Arrow) {
            g.pushLayer(Layer_Projectile);
            // g.pushEmission(Color.WHITE);
            g.pushColor(2 * Color.WHITE);
            g.sprite(arrow_sprite, e.pos(), pt.size.val().x, e.angle());
            // g.sprite(arrow_sprite, e.pos(), 10, e.angle());
            g.popColor();
            // g.popEmission();
            g.popLayer();
        }
        else {
            T.err("cannot draw player projectile type " + e.projectile_type);

        }
    }
    else if (e.entity_type == EntityType_EnemyProjectile) {
        g.pushLayer(Layer_Projectile);

        projectile_type_list[e.projectile_type] @=> Projectile p;
        if (e.projectile_type == ProjectileType_EnemyBasic) {
            p.size.val().x => float r;
            g.circleFilled(pos, r, Color.RED);
            g.circleFilled(pos, .8 * r, Color.WHITE);
        }
        else {
            T.err("cannot draw projectile type " + e.projectile_type);
        }
        
        g.popLayer();
    }
    else {
        T.err("Cannot draw entity type " + e.entity_type);
    }
}

// spawner
spawn_manager.update(dt, bard.pos(), spawn_inner_r.val(), spawn_outer_r.val());

// cleanup / bookkeeping

return null;
} // update()
} // ArenaRoom

class UpgradeRoom extends Room {
"upgrade" => room_name;

UI_Float ui_margin(.2);

int select_idx;

fun void ui() {
    UI.slider("margin", ui_margin, 0, 1.0);
}

fun void enter() {
    RerollUpgradeShop();
    GG.camera().pos(@(0,0));
}

fun Room update(float dt) { 
    shop_upgrades.size() => int num_widgets;

    // input
    if (
        GWindow.keyDown(GWindow.KEY_RIGHT) || GWindow.keyDown(GWindow.KEY_DOWN)
        ||
        GWindow.keyDown(GWindow.KEY_D) || GWindow.keyDown(GWindow.KEY_S)
    ) ++this.select_idx;
    if (
        GWindow.keyDown(GWindow.KEY_LEFT) || GWindow.keyDown(GWindow.KEY_UP)
        ||
        GWindow.keyDown(GWindow.KEY_A) || GWindow.keyDown(GWindow.KEY_W)
    ) --this.select_idx;
    if (this.select_idx < 0) num_widgets - 1 => this.select_idx;
    if (this.select_idx >= num_widgets) num_widgets -=> this.select_idx;

    // apply the upgrade
    if (GWindow.keyDown(GWindow.KEY_SPACE) || GWindow.keyDown(GWindow.KEY_ENTER)) {
        UpgradeApply(shop_upgrades[this.select_idx], true);
        RerollUpgradeShop();
    }

    // update


    // draw
    g.ndc2world(0, ui_margin.val()).y => float margin_y_world;
    g.screenSize() => vec2 screen_size;

    (screen_size.y - 2*margin_y_world) / (num_widgets) => float widget_h;
    screen_size.x * .5 => float widget_w;
    widget_w / widget_h => float widget_aspect;

    @(0, .5 * screen_size.y - margin_y_world - .5 * widget_h) => vec2 cursor;
	g.pushTextMaxWidth(widget_w * .8);
    for (int i; i < shop_upgrades.size(); ++i) {
        shop_upgrades[i] @=> Upgrade u;
        g.box(cursor, widget_w, widget_h);
        if (i == this.select_idx) {
            Math.remap(
                Math.sin(now/second * 5),
                -1, 1,
                0,  widget_h * .33
            ) => float sca;
            g.box(cursor, widget_w - sca, widget_h - sca, Color.PURPLE);
        }

        acquired_upgrades_to_level.getInt(u) + 1 => int u_level;
        g.text(u.title + " " + u_level + "\n" + u.desc, cursor);


        widget_h -=> cursor.y;
    }
    g.popTextMaxWidth();

    return null;
}
}

class DebugRoom extends Room {
    "debug" => room_name;

    int body_id;
    fun void ui() {}
    fun void enter() {
        if (b2Body.isValid(body_id)) {
            b2.destroyBody(body_id);
            0 => body_id;
        }

        b2BodyDef body_def;
        b2BodyType.dynamicBody => body_def.type;
        b2.createBody(b2_world_id, body_def) => body_id;

        b2Filter filter;
        0 => filter.categoryBits;
        0 => filter.maskBits;

        b2ShapeDef shape_def;
        filter @=> shape_def.filter;
        CreateShape(b2ShapeType.polygonShape, body_id, @(1,1), shape_def);
    }
    fun Room update(float dt) {
        g.attackTell(@(0, 0), 1.0, .5);

        return null;
    }
} // DebugRoom


// == init ================================================
Room default_room; RoomAdd(default_room);
ArenaRoom arena_room; RoomAdd(arena_room);
UpgradeRoom upgrade_room; RoomAdd(upgrade_room);
DebugRoom debug_room; RoomAdd(debug_room);

@(0, 0) => world_def.gravity; // no top-down, no gravity
b2.createWorld(world_def) => b2_world_id;
b2.world(b2_world_id);

StaticAdd([
    .5 * @(-field_len.val(), field_len.val()),
    .5 * @(field_len.val(), field_len.val()), 
    .5 * @(field_len.val(), -field_len.val()), 
    .5 * @(-field_len.val(), -field_len.val()), 
], @(0,0));

CartAdd() @=> cart;
PlayerAdd(PlayerType_Bard, @(0,0)) @=> bard;
bard_sprite @=> bard.sprite_tex;
6 => bard.sprite_num_frames;

PlayerAdd(PlayerType_Carpenter, @(1,0)) @=> Entity player1;
WeaponAdd(WeaponType_Chisel, player1);
    // set up player sprite
prisoner_sprite @=> player1.sprite_tex;
8 => player1.sprite_num_frames;


// put bard in cart (need to disable collision)
true => bard.in_cart;
b2Body.disable(bard.b2_body_id);
// q: should players collide with enemies?
b2Shape.enableSensorEvents(bard.b2_shape_id, false);
b2Shape.enableContactEvents(bard.b2_shape_id, false);

UpdateFogGeometry(spawn_inner_r.val());

SpawnManager spawn_manager;

// == gameloop ================================================

RoomEnter(rooms[curr_room_idx.val()]);

while (1) {
GG.nextFrame() => now;
GG.dt() * timescale.val() => float dt;

{ // random test stuff
}


{ // ui
UI.setNextWindowBgAlpha(0.00);
UI.begin("", null, UI_WindowFlags.NoFocusOnAppearing);

if (UI.slider("timescale", timescale, 0.0, 2.0)) {
    timescale.val() => b2.rate;  // adjust physics rate.
}

if (UI.listBox("room", curr_room_idx, room_names)) {
    RoomEnter(rooms[curr_room_idx.val()]);
}

UI.text("currency: "+ currency);

UI.drag("field size", field_len);
if (UI.colorEdit("field color", field_color)) g.backgroundColor(field_color.val());
UI.checkbox("draw b2 colliders", draw_b2_debug);
UI.slider("camera zeno", camera_interp_rate, 0.0, 1.0);
if (UI.slider("camera viewsize", camera_view_size, 0.1, 100.0)) GG.camera().viewSize(camera_view_size.val());

UI.separatorText("fog");
if (UI.checkbox("fog", show_fog)) {
    if (show_fog.val())  fog_mesh --> GG.scene();
    else                 fog_mesh.detachParent();
}
if (UI.slider("alpha", fog_alpha, 0.0, 1.0)) fog_material.alpha(fog_alpha.val());

UI.separatorText("spawn zone");
if (UI.slider("spawn inner radius", spawn_inner_r, 0, 10)) UpdateFogGeometry(spawn_inner_r.val());
UI.slider("spawn outer radius", spawn_outer_r, 1, 10);

UI.separatorText("weapons");
UI.slider("weapon speed", weapon_speed, 0, 50);
UI.drag("weapon rest pos", weapon_rest_pos, .01);
UI.slider("weapon size", weapon_size, 0, 2);

voice.ui();
Command.ui();
Player.ui();
Weapon.ui();
Projectile.ui();
Enemy.ui();
Pickup.ui();
spawn_manager.ui();

UI.separatorText("Entity Stats");
UI.text("#Entities: " + (player_list.size() + entity_count));
UI.text("#Players: " + player_list.size());
UI.text("#ActiveSpells: " + active_spells.size());


UI.separatorText("cart");
UI.slider("speed", cart_speed, 0.0, 1.0);
UI.slider("speed mult", cart_speed_mult, 0.0, 2.0);
UI.slider("cart max hp", cart.hp_max, 0.0, 50.0);
UI.slider("cart curr hp", cart.hp_curr, 0.0, cart.hp_max.val());

UI.separatorText("upgrades");
for (1 => int i; i < upgrade_type_list.size(); i++) {
    upgrade_type_list[i] @=> Upgrade u;
    acquired_upgrades_to_level.getInt(u) => int level;

    UI.text(u.title + " " + level);
    UI.sameLine();
    if (UI.arrowButton(u.title + "##left", UI_Direction.Left)) UpgradeApply(u, false);
    UI.sameLine();
    if (UI.arrowButton(u.title + "##right", UI_Direction.Right)) UpgradeApply(u, true);
}

UI.separatorText("Player Stats");
for (int player_idx; player_idx < player_list.size(); ++player_idx) {
    player_list[player_idx] @=> Entity player;

    player_type_list[player.player_type] @=> Player p_t;
    UI.pushID(player_idx);

    UI.text(p_t.name);
    UI.slider("max hp", player.hp_max, 0, p_t.base_hp);
    UI.slider("curr hp", player.hp_curr, 0, player.hp_max.val());
    UI.separator();

    UI.popID();
}



UI.end();
} // end ui

rooms[curr_room_idx.val()].update(dt) @=> Room@ next_room;
if (next_room != null) RoomEnter(next_room);

// == cleanup / bookkeeping ================================================
    // physics debug draw
if (draw_b2_debug.val()) b2World.draw(b2_world_id, debug_draw);
debug_draw.update();
t.update(dt);
EntityProcessReturned();
}