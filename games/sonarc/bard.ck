@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "../lib/whisper/Whisper.chug"
@import "../lib/tween.ck"
@import "../lib/b2/b2DebugDraw.ck"
@import "HashMap.chug"

Tween t;

// GWindow.windowed(1920, 1080);
GWindow.center();
// GWindow.fullscreen();
BardG2D g;
g.sortDepthByY(true);
GText.defaultFont(me.dir() + "./assets/m5x7.ttf");

GG.camera().viewSize(16);

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

    fun static void ripple(vec2 pos, float end_radius, dur effect_dur, vec3 color) {
        dur elapsed_time;
        while (elapsed_time < effect_dur) {
            GG.nextFrame() => now;
            GG.dt()::second +=> elapsed_time;
            M.easeOutQuad(elapsed_time / effect_dur) => float t;

            g.circle(pos, end_radius * t, .1 * (1 - t), color);
        }
    }
}

// == constants ================================================

6 => int MAX_ACTIVE_UPGRADES;

// == enums ================================================

0 =>        int EntityType_None;
(1 << 0) => int EntityType_Static;
(1 << 1) => int EntityType_Player;
(1 << 2) => int EntityType_Cart;
(1 << 3) => int EntityType_Enemy;
(1 << 4) => int EntityType_Pickup;
(1 << 5) => int EntityType_Weapon;
(1 << 6) => int EntityType_PlayerProjectile;

0 =>        int PlayerType_None;
1 =>        int PlayerType_Bard;
2 =>        int PlayerType_Carpenter;

0 =>        int EnemyType_None;
1 =>        int EnemyType_Basic;
2 =>        int EnemyType_Pursuer;

-1 =>        int Layer_Background;
5  =>        int Layer_Player;
6  =>        int Layer_Cart;
7  =>        int Layer_Fog; // drawing b2 physics colliders
8  =>        int Layer_Light; // drawing b2 physics colliders
9  =>        int Layer_DebugDraw; // drawing b2 physics colliders

// @design: is this better or the "smear" melee attack (like in nuclear throne)
0 => int WeaponState_Rest;
1 => int WeaponState_Attack;  // going out to target
2 => int WeaponState_Return;  // (for melee weapons) coming back after reaching target


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
10 =>  int Upgrade_Count;

// == ui ================================================

// == attack params ===========================================
// experimenting here... thinking all attacks share the same
// base of range / targetting / cooldown / 
// each instance of a weapon would need its own attack params
// because multiple players can have the same weapon with different
// stats...

// and maybe bullets are their own class? or they can just 
// be an Entity...
// or should this be class Weapon?
class AttackParams { 
    UI_Float _dmg;
    UI_Float _dmg_mult(1.0);
    UI_Float _range;
    UI_Float _range_mult(1.0);
    UI_Float _cd_sec;
    UI_Float _atk_speed(1.0); // need to invert in attack speed calc

    fun AttackParams(float dmg, float range, float cd) {
        dmg => this._dmg;
        range => this._range;
        cd => this._cd_sec;
    }

    fun float dmg() { return this._dmg.val() * this._dmg_mult.val(); }
    fun float range() { return this._range.val() * this._range_mult.val(); }
    fun float cd() { return this._cd_sec.val() * (1.0 / this._atk_speed.val()); } 
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

// == player type ================================================


class Player {
    int player_type;
    string name;

    float base_hp;
    float base_speed;
}

Player player_type_list[0];
fun void PlayerAddType(
    int player_type, float base_hp, float base_speed, string name
) {
    Player p;

    player_type => p.player_type;
    name => p.name;
    base_hp => p.base_hp;
    base_speed => p.base_speed;

    player_type_list << p;
    T.assert(player_type_list[player_type] == p, "adding player type out of order");
}

//                                 hp   hp_scaling  dmg  dmg_scaling  drop  speed_range  shape                     size
//            player_type           hp     speed    name
PlayerAddType(PlayerType_None,      0,     0,       "");
PlayerAddType(PlayerType_Bard,      8,     .3,      "Bard");
PlayerAddType(PlayerType_Carpenter, 10,    .6,      "Carpenter");

// == enemy type ================================================

class Enemy {
    int enemy_type;
    float base_hp;
    float hp_scaling;

    float drop; // how much exp/$ it gives on death

    float dmg; // dmg dealt via contact OR projectile
    float dmg_scaling; // linearly scales with wave
    UI_Float2 speed; // range of speed to spawn with @(min, max)

    // collider stuff
    int b2_shape_type;
    UI_Float2 size;          // for capsule: @(p1 -- p2, radius)
                             // for box:     @(w, h)
                             // for circle   @(radius, radius)
}

Enemy enemy_type_list[0];
fun void EnemyAddType(
    int enemy_type, float base_hp, float hp_scaling, float dmg, float dmg_scaling, float drop, vec2 speed,
    int b2_shape_type, vec2 size
) {
    Enemy e;
    enemy_type => e.enemy_type;
    base_hp => e.base_hp;
    hp_scaling => e.hp_scaling;
    drop => e.drop;
    dmg => e.dmg;
    dmg_scaling => e.dmg_scaling;
    new UI_Float2(speed) @=> e.speed; 
    b2_shape_type => e.b2_shape_type;
    new UI_Float2(size) @=> e.size;
    enemy_type_list << e;

    T.assert(enemy_type_list[enemy_type] == e, "adding enemy type out of order");
}

// enemy types
//                                 hp   hp_scaling  dmg  dmg_scaling  drop  speed_range  shape                     size
EnemyAddType(EnemyType_None, 0, 0, 0, 0, 0, @(0,0), 0, @(0,0));
EnemyAddType(EnemyType_Basic,      4,   2,          2,   1,           1,    @(.3, .4),   b2ShapeType.circleShape,  @(.5, .5));
EnemyAddType(EnemyType_Pursuer,    2,   1,          1,   .5,          1,    @(.5, .5),   b2ShapeType.capsuleShape, @(.3, .1));

// == Entity ================================================

0 => int PoolState_Returned;
1 => int PoolState_CheckedOut;
2 => int PoolState_QueuedForReturn;

// @TODO extend PoolableItem, implement pool allocator, group enemies separately
class Entity {
    // internal
    int _pool_idx; // used by pool allocator, assumes each Entity only belongs to a single pool
    int _pool_state;

    // id
    int entity_type; // mask of EntityType_XXX
    int b2_body_id;
    int b2_shape_id;
    int b2_group_idx;

    // hp
    UI_Float hit_cd(0.0); // used for drawing hit flash
    UI_Float hp_max;
    UI_Float hp_curr;

    // movement
    float speed;

    // player
    int player_type;
    int in_cart;
    int dead;

    // enemy
    int enemy_type;
    int target_player_id;

    // weapon
    int weapon_type;
    int player_id; // b2bodyid of player this weapon belongs to
    int weapon_state;
        // for attack targeting
        vec2 weapon_attack_start_pos;
        vec2 weapon_attack_dir;
        float weapon_attack_dist;

    // @feature: ability to zero classes in chuck
    fun void zero() {
        // DON'T ZERO POOL IDX
        PoolState_Returned => this._pool_state;

        0 => this.entity_type; 
        0 => this.b2_body_id;
        0 => this.b2_group_idx;
        0 => this.hp_max.val;
        0 => this.hp_curr.val;
        0 => this.player_type;
        0 => this.dead;
        0 => this.enemy_type;

        0 => this.weapon_type;
        0 => this.player_id;
        0 => this.weapon_state;

        @(0,0) => this.weapon_attack_start_pos;
        @(0,0) => this.weapon_attack_dir;
        0 => this.weapon_attack_dist;
    }

    fun vec2 pos() { return b2Body.position(this.b2_body_id); }
    fun void pos(vec2 p) { b2Body.position(this.b2_body_id, p); }
    fun void rot(vec2 rot) { b2Body.rotation(this.b2_body_id, rot); }
    fun void vel(vec2 v) {  b2Body.linearVelocity(this.b2_body_id, v); }
    fun vec2 vel() {  return b2Body.linearVelocity(this.b2_body_id); }

    fun void takeDamage(float amt) {
        T.assert(this.hp_max.val() > 0, "calling damage() on entity without hp");
        if (this.hp_curr.val() <= 0) return; // already dead, do nothing

        t.easeOutQuad(this.hit_cd, 1.0, 0).over(.3);

        this.hp_curr.val() - amt => this.hp_curr.val;

        // die
        if (this.hp_curr.val() <= 0) { 
            if (this.entity_type == EntityType_Enemy) {
                spork ~ FX.smokeCloud(this.pos(), Color.BLACK);
                EntityReturn(this);

                // add money!
                enemy_type_list[this.enemy_type].drop +=> currency;
            } else {
                T.err("calling damage on unsupported entity type: " + this.entity_type);
            }
        }
    }

}

// == gamestate ================================================
// physics
b2WorldDef world_def;
int b2_world_id;
int begin_sensor_events[0];
int begin_touch_events[0];

UI_Bool draw_b2_debug(true);
DebugDraw debug_draw;
debug_draw.layer(Layer_DebugDraw);
true => debug_draw.drawShapes;
true => debug_draw.outlines_only;

// playfield
UI_Float field_len(32.0);
UI_Float3 field_color(18/255.0, 39/255.0, 8/255.0);

// player
UI_Float player_size(1);
Entity player_list[0];
int players_num_alive;

// weapon stuff
UI_Float weapon_speed(8.0);
UI_Float weapon_zeno(.4);

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
vec2 bard_target;
null @=> Entity@ bard;
PolygonGeometry polygon_geo;  // for shadow fog
FlatMaterial fog_material(Color.BLACK);
true => fog_material.transparent;
.25 => fog_material.alpha;
UI_Float fog_alpha(fog_material.alpha());
GMesh fog_mesh(polygon_geo, fog_material);
Layer_Fog => fog_mesh.posZ;

UI_Bool show_fog(true);

// cart
UI_Float2 cart_size(1.67, 1.0);
UI_Float cart_speed(.2);
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
AttackParams arrow_tower_atk(5.0, 2 * cart_size.val().x, 1.0);
AttackParams bomb_tower_atk(10.0, 4 * cart_size.val().x, 3.2);


vec2 cart_target;
null @=> Entity@ cart;

// resources
float currency;

// upgrades
// idea: bard AND cart share same pool of upgrades lots
// e.g. you can split 50/50 or go 100/0 into either
HashMap acquired_upgrades_to_level;  // map from [Upgrade : int level]
Upgrade shop_upgrades[0];

// == methods ================================================

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

    // geo
    b2.makeBox(.5*player_size.val(), .5*player_size.val()) @=> b2Polygon player_geo;
    b2.createPolygonShape(player.b2_body_id, player_shape_def, player_geo) => player.b2_shape_id;

    player_list << player;
    players_num_alive++;

    // assign stats
    pt.base_hp => player.hp_max.val;
    pt.base_hp => player.hp_curr.val;
    pt.base_speed => player.speed;

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
    b2.makeBox(cart_size.val().x, cart_size.val().y) @=> b2Polygon geo;
    b2.createPolygonShape(e.b2_body_id, shape_def, geo) => int shape_id;

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

    return closest
}

// gets random *living* player. null if none
fun Entity PlayerRandomLiving() {
    if (players_num_alive == 0) return null;
    return player_list[Math.random2(0, players_num_alive - 1)];
}

fun Entity EntityClosestFromShapes(vec2 pos, int shape_ids[]) {
    Math.FLOAT_MAX => float dist;
    null => Entity@ closest;
    for (auto id : shape_ids) {
        EntityGet(b2Shape.body(id)) @=> Entity e;
        M.dist(pos, e.pos()) => float d;
        if (d < dist) {
            d => dist;
            e @=> closest;
        }
    }
    return closest;
}

fun Entity EnemyAdd(int enemy_type, vec2 pos) {
    // @TODO test different spawn modes
    // @optimize: reuse static b2 defs 

    // body
    b2BodyDef enemy_body_def;
    pos => enemy_body_def.position;
    b2BodyType.dynamicBody => enemy_body_def.type;
    true => enemy_body_def.fixedRotation;
    b2.createBody(b2_world_id, enemy_body_def) => int body_id;
    
    // filter
    b2Filter enemy_filter;
    EntityType_Enemy => enemy_filter.categoryBits;
    // @TODO: we might want a separate collision shape at feet of enemy that only
    // collides with other enemies. Allows enemies to partially stack

    // make enemy a sensor if we want player to be able to move through enemies
    // @TODO play brotato, vampire survivors, 20 minutes till dawn etc and see what they do
    // true => enemy_shape_def.isSensor;

    // shape
    b2ShapeDef enemy_shape_def;
    true => enemy_shape_def.enableSensorEvents;
    enemy_filter @=> enemy_shape_def.filter;

    // geo
    int shape_id;
    enemy_type_list[enemy_type] @=> Enemy et;
    if (et.b2_shape_type == b2ShapeType.polygonShape) {
        b2.makeBox(et.size.val().x, et.size.val().y) @=> b2Polygon enemy_geo;
        b2.createPolygonShape(body_id, enemy_shape_def, enemy_geo) => shape_id;
    }
    else if (et.b2_shape_type == b2ShapeType.capsuleShape) {
        b2.createCapsuleShape(
            body_id, enemy_shape_def, 
            @(-et.size.val().x/2, 0),  // center1
            @(et.size.val().x/2, 0),  // center1
            et.size.val().y // radius
        ) => shape_id;
    }
    else if (et.b2_shape_type == b2ShapeType.circleShape) {
        b2.createCircleShape(
            body_id, enemy_shape_def, 
            @(0, 0),
            et.size.val().x // radius
        ) => shape_id;
    }
    else {
        T.err("unknown enemy shape type: " + et.b2_shape_type);
    }

    // entity
    EntityAdd(
        EntityType_Enemy,
        body_id,
        shape_id
    ) @=> Entity enemy;

    enemy_type => enemy.enemy_type;
    et.base_hp => enemy.hp_max.val;
    et.base_hp => enemy.hp_curr.val;
    Math.random2f(et.speed.val().x, et.speed.val().y) => enemy.speed;

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
    EntityType_Player => filter.maskBits;

    // shape def
    b2ShapeDef shape_def;
    filter @=> shape_def.filter;

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
fun Entity WeaponAdd(Entity@ player) {
    // body
    b2BodyDef body_def;
    b2BodyType.kinematicBody => body_def.type;
    b2.createBody(b2_world_id, body_def) => int body_id;

    // filter
    b2Filter filter;
    EntityType_Weapon => filter.categoryBits;
    EntityType_Enemy  => filter.maskBits;

    // shape
    b2ShapeDef shape_def;
    false => shape_def.enableSensorEvents; // enabled during attack anim
    true => shape_def.isSensor;
    filter @=> shape_def.filter;

    // geo
    b2.makeBox(.3 * player_size.val(), .3 * player_size.val()) @=> b2Polygon geo;
    b2.createPolygonShape(body_id, shape_def, geo) => int shape_id;

    // entity
    EntityAdd(
        EntityType_Weapon,
        body_id, shape_id
    ) @=> Entity e;
    // weapon_type => enemy.weapon_type; // @TODO
    player.b2_body_id => e.player_id;

    return e;
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
    } else {
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
    Spawn spawn_list[];
    float prev_time_sec;
    float curr_time_sec;

    // switches to the given spawn list
    fun void register(Spawn spawn_list[]) {
        0.0 => this.prev_time_sec;
        spawn_list @=> this.spawn_list;
    }

    // inner_r is bard vision, outer_r is s max spawn distance
    fun void update(float dt, vec2 bard_pos, float inner_r, float outer_r) {
        T.assert(outer_r > inner_r, "spawner invalid spawn ranges");

        this.curr_time_sec => this.prev_time_sec;
        dt +=> this.curr_time_sec;

        for (auto spawn : spawn_list) {
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
                    EnemyAdd(spawn.enemy_type, M.randomPointInCircle(bard_pos, inner_r, outer_r));
                }
            }
            else if (spawn.location_type == SpawnLocation_Group) {
                // adjust inner_r so that the radius of the spawn group doesn't intersect with bard circle
                M.randomPointInCircle(bard_pos, inner_r + spawn.spawn_group_radius, outer_r) => vec2 group_center;
                repeat (spawn.number) {
                    EnemyAdd(spawn.enemy_type, M.randomPointInCircle(group_center, 0, spawn.spawn_group_radius));
                }
            }
            else {
                T.err("unknown spawn location " + spawn.location_type);
            }
        }
    }
}

// == WAVES ======================================================

class Wave {
    int wave_number;
    Spawn spawn_list[];
}

UI_Int curr_wave(-1);
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
WaveAdd([
new Spawn(EnemyType_Basic,  SpawnLocation_Random,  0,             1,        SpawnTiming_Loop,  0,         3)
]);
WaveAdd([
new Spawn(EnemyType_Basic,  SpawnLocation_Random,  0,             2,        SpawnTiming_Loop,  0,         3),
new Spawn(EnemyType_Pursuer,SpawnLocation_Group,   1,             4,        SpawnTiming_Loop,  5,         4)
]);


// == load assets ================================================
TextureLoadDesc tex_load_desc;
true => tex_load_desc.flip_y;
false => tex_load_desc.gen_mips;
Texture.load(me.dir() + "./assets/bard.bmp", tex_load_desc) @=> Texture bard_sprite;
Texture.load(me.dir() + "./assets/knight.bmp", tex_load_desc) @=> Texture knight_sprite;
Texture.load(me.dir() + "./assets/ground_8x8.png", tex_load_desc) @=> Texture ground_texture;
Texture.load(me.dir() + "./assets/cart.png", tex_load_desc) @=> Texture cart_sprite;
Texture.load(me.dir() + "./assets/wheel.png", tex_load_desc) @=> Texture wheel_sprite;


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

// == process collisions ================================================
b2World.sensorEvents(b2_world_id, begin_sensor_events, null);
for (int i; i < begin_sensor_events.size(); 2 +=> i) {
    begin_sensor_events[i] => int sensor_shape_id;
    begin_sensor_events[i+1] => int visitor_shape_id;

    // might be invalid because we destroy in prior iteration
    if (!b2Shape.isValid(sensor_shape_id) || !b2Shape.isValid(visitor_shape_id)) continue;

    EntityFromShape(sensor_shape_id) @=> Entity sensor;
    EntityFromShape(visitor_shape_id) @=> Entity visitor;

    // weapon collide
    if (sensor.entity_type == EntityType_Weapon) {
        T.assert(visitor.entity_type == EntityType_Enemy, "weapon collide with non-enemy");
        visitor.takeDamage(1.0);
    }

    // z.returnEntity(sensor);
}

b2World.contactEvents(b2_world_id, begin_touch_events, null, null);
for (int i; i < begin_touch_events.size(); 2 +=> i) {
    begin_touch_events[i] => int touch_shape_a;
    begin_touch_events[i+1] => int touch_shape_b;
    if (!b2Shape.isValid(touch_shape_a) || !b2Shape.isValid(touch_shape_b)) continue;
    b2Shape.body(begin_touch_events[i]) => int touch_body_id_a;
    b2Shape.body(begin_touch_events[i+1]) => int touch_body_id_b;
    if (!b2Body.isValid(touch_body_id_a) || !b2Body.isValid(touch_body_id_b)) continue;
    EntityGet(touch_body_id_a) @=> Entity a;
    EntityGet(touch_body_id_b) @=> Entity b;

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

// == enemy spawn ================================================
// if (spawn_period.val() > 0) {
//     dt +=> time_since_last_spawn;
//     if (time_to_next_spawn <= 0) M.poisson(spawn_period.val()) => time_to_next_spawn;

//     while (time_since_last_spawn > time_to_next_spawn) {
//         // spawnPickup(M.randomPointInArea(@(0, 0), .8 * dungeon_hw, .8 * dungeon_hw), ZSensorType_SpawnZone) @=> ZEntity e;
//         // 4.0 => e.time_to_spawn_secs;

//         // @TODO replace center w/ bard pos
//         EnemyAdd(EnemyType_Basic, M.randomPointInCircle(bard.pos(), spawn_inner_r.val(), spawn_outer_r.val()));

//         time_to_next_spawn -=> time_since_last_spawn;
//         M.poisson(spawn_period.val()) => time_to_next_spawn;
//     }
// }

// == input ================================================

for (int player_idx; player_idx < player_list.size(); ++player_idx) {
    player_list[player_idx] @=> Entity player;

    if (player.player_type == PlayerType_Bard) continue; // maybe the bard shouldn't be a player?
    if (player.dead) continue;

    vec2 dir;
    if (GWindow.key(GWindow.KEY_A)) 1 -=> dir.x;
    if (GWindow.key(GWindow.KEY_D)) 1 +=> dir.x;
    if (GWindow.key(GWindow.KEY_S)) 1 -=> dir.y;
    if (GWindow.key(GWindow.KEY_W)) 1 +=> dir.y;
    dir.normalize();
    // if (dir.x != 0) dir.x => player.last_dir_x;
    b2Body.linearVelocity(player.b2_body_id, dir * 3); // @TODO: speed should be a stat
}

{ // verify alive players are contiguous and at start of array
for (int i; i < players_num_alive; ++i) T.assert(!player_list[i].dead, "dead player at front");
for (players_num_alive => int i; i < player_list.size(); ++i) T.assert(player_list[i].dead, "alive player at end");
}

// == update ================================================

{ // update cart
if (M.dist(cart.pos(), cart_target) < .05) {
    M.randomPointInCircle(cart.pos(), 1, spawn_inner_r.val()) => cart_target;
    // clamp to arena bounds
    .5 * field_len.val() - cart_size.val().x => float hw;

    Math.clampf(cart_target.x, -hw, hw) => cart_target.x;
    Math.clampf(cart_target.y, -hw, hw) => cart_target.y;
    T.assert(M.inside(cart_target, M.aabb(@(0,0), hw, hw)), "cart target is not inside bounds");
}
cart_speed.val() * M.dir(cart.pos(), cart_target) => cart.vel;

// wheel anim
2 => float cart_wheel_speed;
if (cart.pos().x < cart_target.x) cart_wheel_speed * dt -=> cart_wheel_rot;
else                              cart_wheel_speed * dt +=> cart_wheel_rot;

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
    EnemyClosest(cart.pos(), arrow_tower_atk.range()) @=> Enemy target;
    if (target == null) continue;

    // FireProjectile

}

} // update cart

{ // update bard
if (bard.in_cart) {
    cart.pos() + @(0, .67 * cart_size.val().y) => bard.pos;
} else {
    if (M.dist(bard.pos(), bard_target) < .05) {
        M.randomPointInCircle(bard.pos(), .5, 1.5) => bard_target;
    }
    bard_base_speed.val() * M.dir(bard.pos(), bard_target) => bard.vel;
}

fog_mesh.pos(bard.pos());
}

// update player

// update entity
for (int entity_idx; entity_idx < entity_count; ++entity_idx) {
    entity_pool[entity_idx] @=> Entity e;

    // update enemy
    if (e.entity_type == EntityType_Enemy) {
        enemy_type_list[e.enemy_type] @=> Enemy enemy_t;

        if (e.enemy_type == EnemyType_Basic) {
            // find closest player
            PlayerClosest(e.pos()) @=> Entity@ closest_player;
            vec2 dir_to_closest_player;
            if (closest_player != null) M.dir(e.pos(), closest_player.pos()) => dir_to_closest_player;
            b2Body.linearVelocity(
                e.b2_body_id, 
                dir_to_closest_player * e.speed
            );
        } 
        else if (e.enemy_type == EnemyType_Pursuer) {
            EntityGet(e.target_player_id) @=> Entity target_player;

            // select valid player target
            if (target_player == null || target_player.dead) PlayerRandomLiving() @=> target_player;

            if (target_player != null) {
                target_player.b2_body_id => e.target_player_id;
                M.dir(e.pos(), target_player.pos()) => vec2 dir;
                e.vel(dir * e.speed);
                e.rot(dir);
            }
        } else {
            T.err("unknown enemy type " + e.enemy_type);
        }
    }

    // update weapon
    if (e.entity_type == EntityType_Weapon) {
        1.0 => float weapon_range; // @TODO
        EntityGet(e.player_id) @=> Entity p;
        p.pos() + @(.5 * player_size.val(), 0) => vec2 rest_pos;

        // check for enemies in range
        if (e.weapon_state == WeaponState_Rest) {
            T.assert(!b2Shape.areSensorEventsEnabled(e.b2_shape_id), "sensors enabled on weapon at rest");

            e.pos(p.pos() + @(.5 * player_size.val(), 0));

            // query for enemies in range
            // @optimize pass in int[], don't reallocate
            // @optimize, when looping over enemies, calculate closest enemy to each weapon, don't need to query 
            // at all
            b2QueryFilter filter;
            EntityType_Weapon => filter.categoryBits;
            EntityType_Enemy => filter.maskBits;
            b2World.overlapCircle( b2_world_id, e.pos(), weapon_range, filter) @=> int enemies_in_range[];
            if (enemies_in_range.size() > 0) {
                EntityClosestFromShapes(e.pos(), enemies_in_range) @=> Entity target;

                // attack!
                WeaponState_Attack => e.weapon_state;
                b2Shape.enableSensorEvents(e.b2_shape_id, true);
                e.pos() => e.weapon_attack_start_pos;
                weapon_speed.val() * M.dir(e.pos(), target.pos()) => e.vel;
                weapon_range => e.weapon_attack_dist;
            }
        } else if (e.weapon_state == WeaponState_Attack) {
            // don't zeno interp because there's no dt
            T.assert(b2Shape.areSensorEventsEnabled(e.b2_shape_id), "sensor not enabled on weapon");

            // reached target, return
            if (M.dist(e.pos(), e.weapon_attack_start_pos) > weapon_range) {
                WeaponState_Return => e.weapon_state;
                @(0,0) => e.vel;
            }
        } else if (e.weapon_state == WeaponState_Return) {
            // zeno interp back @TODO add BSC GingerBill math video dt modification
            e.pos() + weapon_zeno.val() * (rest_pos - e.pos()) => e.pos;

            if (M.dist(e.pos(), rest_pos) < .01) {
                rest_pos => e.pos;
                WeaponState_Rest => e.weapon_state;
                b2Shape.enableSensorEvents(e.b2_shape_id, false);
            }
        } else {
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




// == draw ================================================
// draw playfield
g.pushLayer(Layer_Background);
g.pushPolygonRadius(.2);
// g.squareFilled(@(0,0), 0, field_len.val(), field_color.val());
g.sprite(ground_texture, @(0, 0), 1.1 * field_len.val(), 0);
g.popPolygonRadius();
g.popLayer();

// draw bard (sprite is drawn in player update loop)
if (draw_b2_debug.val()) {
g.pushLayer(Layer_DebugDraw); 
g.circle(bard.pos(), spawn_inner_r.val(), Color.RED);
g.circle(bard.pos(), spawn_outer_r.val(), Color.RED);
if (!bard.in_cart) g.dashed(bard_target, bard.pos(), Color.WHITE, .1);
g.popLayer(); 
}

// draw cart
g.pushLayer(Layer_Cart);
g.sprite(cart_sprite, cart.pos(), cart_size.val().x, 0);
g.dashed(cart_target, cart.pos(), Color.BROWN, .1);

// g.boxFilled(
//     cart.pos(),
//     0,
//     cart_size.val().x,
//     cart_size.val().y,
//     Color.DARKBROWN
// 	);
// wheels
cart_size.val().y / 4 => float wheel_radius;
cart.pos() + @(-cart_size.val().x * .25, -.5 * cart_size.val().y ) => vec2 wheel1;
cart.pos() + @(cart_size.val().x * .25, -.5 * cart_size.val().y ) => vec2 wheel2;
// g.circleFilled(wheel1, wheel_radius, Color.BLACK);
// g.circleFilled(wheel2, wheel_radius, Color.BLACK);
g.sprite(wheel_sprite, wheel1, 2 * wheel_radius, cart_wheel_rot);
g.sprite(wheel_sprite, wheel2, 2 * wheel_radius, cart_wheel_rot);
g.popLayer();

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

    if (player.player_type == PlayerType_Carpenter) {
        // @TODO sprite should be field of Entity
        g.sprite(knight_sprite, pos, player_size.val(), 0);
    }
    else if (player.player_type == PlayerType_Bard) {
        g.sprite(bard_sprite, pos, player_size.val(), 0);
        // <<< "bard pos", player.pos() >>>;
    }
}
// g.popLayer();

// draw entity
for (int entity_idx; entity_idx < entity_count; ++entity_idx) {
    entity_pool[entity_idx] @=> Entity e;

    if (e.entity_type == EntityType_Static) {
        // not drawing borders for now
    } else if (e.entity_type == EntityType_Player) {
        T.assert("player should be in player list, not entity pool!");
    } 
    // draw enemy
    else if (e.entity_type == EntityType_Enemy) {
        enemy_type_list[e.enemy_type] @=> Enemy et;

        M.normalize(e.vel()) => vec2 dir;
        M.perp(dir) => vec2 perp;

        if (e.enemy_type == EnemyType_Basic) {
            et.size.val().x => float r;

            e.pos() + .4 * r * dir + .2 * r * perp => vec2 eye1;
            e.pos() + .4 * r * dir - .2 * r * perp => vec2 eye2;
            g.circleFilled(e.pos(), r, e.hit_cd.val() * Color.WHITE + Color.BLACK);
            g.pushLayer(Layer_Light);
            g.circleFilled(eye1, .08 * r, Color.YELLOW);
            g.circleFilled(eye2, .08 * r, Color.YELLOW);
            g.popLayer();
        }
        else if (e.enemy_type == EnemyType_Pursuer) {

        } else {
            T.err("cannot draw enemy type " + e.enemy_type);
        }

    }
    else if (e.entity_type == EntityType_Weapon) {
        // debug range
        g.pushLayer(Layer_DebugDraw);
        g.circle(e.pos(), 1.0, Color.WHITE);
        g.popLayer();
    }
    else {
        T.err("Cannot draw entity type " + e.entity_type);
    }
}

// physics debug draw
if (draw_b2_debug.val()) b2World.draw(b2_world_id, debug_draw);

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


// == init ================================================
Room default_room; RoomAdd(default_room);
ArenaRoom arena_room; RoomAdd(arena_room);
UpgradeRoom upgrade_room; RoomAdd(upgrade_room);

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
PlayerAdd(PlayerType_Carpenter, @(1,0)) @=> Entity player1;
WeaponAdd(player1);

// put bard in cart
true => bard.in_cart;
b2Shape.enableSensorEvents(bard.b2_shape_id, false);
b2Shape.enableContactEvents(bard.b2_shape_id, false);

EnemyAdd(EnemyType_Basic, @(0, 1));

UpdateFogGeometry(spawn_inner_r.val());

SpawnManager spawn_manager;

// == gameloop ================================================

RoomEnter(rooms[curr_room_idx.val()]);

while (1) {
GG.nextFrame() => now;
GG.dt() => float dt;

{ // ui
UI.setNextWindowBgAlpha(0.00);
UI.begin("");

if (UI.listBox("room", curr_room_idx, room_names)) {
    RoomEnter(rooms[curr_room_idx.val()]);
}

UI.text("currency: "+ currency $ int);

UI.drag("field size", field_len);
UI.colorEdit("field color", field_color);
UI.checkbox("draw b2 colliders", draw_b2_debug);
UI.slider("camera zeno", camera_interp_rate, 0.0, 1.0);
if (UI.slider("camera viewsize", camera_view_size, 0.1, 100.0)) GG.camera().viewSize(camera_view_size.val());

UI.separatorText("fog");
if (UI.checkbox("fog", show_fog)) {
    if (show_fog.val())  fog_mesh --> GG.scene();
    else                 fog_mesh.detachParent();
}
if (UI.slider("alpha", fog_alpha, 0.0, 1.0)) fog_material.alpha(fog_alpha.val());

UI.separatorText("enemy");
// UI.slider("spawn period (sec)", spawn_period, .1, 5);
if (UI.slider("spawn inner radius", spawn_inner_r, 0, 10)) UpdateFogGeometry(spawn_inner_r.val());
UI.slider("spawn outer radius", spawn_outer_r, 1, 10);

UI.separatorText("weapons");
UI.slider("weapon speed", weapon_speed, 0, 10);
UI.slider("weapon zeno", weapon_zeno, 0, 1);

UI.separatorText("cart");
UI.slider("speed", cart_speed, 0.0, 1.0);
UI.slider("speed mult", cart_speed_mult, 0.0, 2.0);

UI.separatorText("wave");
if (UI.listBox("wave", curr_wave, wave_names)) {
    spawn_manager.register(wave_list[curr_wave.val()].spawn_list);
}



UI.separatorText("Entity Stats");
UI.text("#Entities: " + (player_list.size() + entity_count));
UI.text("#Players: " + player_list.size());


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
if (draw_b2_debug.val()) debug_draw.update();
t.update(dt);
EntityProcessReturned();
}