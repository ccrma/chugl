/*
Mr. driller is awesome
So is Motherlode

- press on rythm to get faster resources.
    - I actually hate rhythm stuff because of variable latency with different kboards
    - but maybe we can quantize the mining to music, REZ-style

- ideas:
    - destroying blocks harvests their resource
    - have a "shop" block that very randomly spawns, allow spending resources to buy upgrades
        - high hp, you need to try to path towards it
        - shop items:
            - steel shoes: don't take damage from falling on spikes
            - spring: enables jumping
            - delts: mine faster sideways
            - +long arms: mining hits 2 blocks in same direction
    - level up from mining blocks?
        - upgrades:
            - dmg per hit
            - hit cooldown
    - other block types
        - mine blocks that explode
        - spike traps that kill on contact
            - spawn logic: increase chance if there are more empty blocks above
        - invincible blocks?
        - coin blocks (mine to get coin to spend at shop)
        - obsidian: can't destroy
        - straw: touching makes it break
    - goal: survive XX minutes
    - difficulty curve
        - block hp increases
        - camera moves down faster?
        - camera livable "band" narrows and moves, i.e. top and bottom visible edges
    - contraints
        - no enemies
        - no upward movement. this might change, follow gameplay needs
        - no mouse. kb only


collision:
    - only need to check the 9grid squares immediately around player

following risk-reward model, there needs to be things that encourage players to be in the riskiest areas
    - at very top, near off-screen death zone
    - at very bottom, with minimal visibility of what's further down

Up to flap? like old birb controls

Optimizaitons (if needed)
- only draw a tile if onscreen

Stretch:
- technical flex: on death, lerp camera back to starting pos, 
drawing all previous tile history as it moves back
    - can maybe fake this by drawing random shit, camera might move fast enough to not be able to tell
    - rapidly decrement the depth(m) counter while moving up
    - synchronize a sliding upward sfx 
    - actually think we need this to juice the death sequence

TODO:
- JUICE THE MINING!
    - 
- exp + lvl up system
- obsidian tile
- straw tile
- coin should only spawn every N rows
    - change tile generation to be at tilemap level, not individual tile.
- test flying code
*/

@import "../lib/g2d/ChuGL.chug"
// @import "../lib/g2d/ChuGL-debug.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "../bytepath/topograph.ck"
@import "../lib/b2/b2DebugDraw.ck"
@import "sound.ck"
@import "HashMap.chug"
@import "spring.ck"

// game params
UI_Int seconds_per_level(30);

// tile params
UI_Float tile_pull_force(.1);
UI_Float egg_probability(.002);

// map params
UI_Int MINE_W(7);
UI_Int MINE_H(20);

// camera params
UI_Float camera_speed(.5); // TWEAK
UI_Float screen_zeno(.05); // TODO TWEAK
UI_Float player_target_pos(-2.0); // how far the player should be above camera center. TWEAK

// player params
UI_Float player_speed(4.0);
UI_Float player_base_size(.6);
UI_Float player_size(player_base_size.val());

G2D g;
g.antialias(true);
GText.defaultFont("chugl:proggy-tiny");

1.2 => float aspect;
GWindow.sizeLimits(0, 0, 0, 0, @(aspect, 1));
GWindow.center();

GG.camera().viewSize(10);

// == load assets ================================================
TextureLoadDesc tex_load_desc;
true => tex_load_desc.flip_y;
false => tex_load_desc.gen_mips;
Texture.load(me.dir() + "./assets/coin.png", tex_load_desc) @=> Texture coin_single_sprite;
Texture.load(me.dir() + "./assets/coin_anim.png", tex_load_desc) @=> Texture coin_sprite; // 100::ms per frame
Texture.load(me.dir() + "./assets/chicken.png", tex_load_desc) @=> Texture chicken_sprite; // 50::ms per frame

Texture.load(me.dir() + "./assets/dirt.png", tex_load_desc) @=> Texture dirt_sprite_0;
Texture.load(me.dir() + "./assets/dirt1.png", tex_load_desc) @=> Texture dirt_sprite_1;
Texture.load(me.dir() + "./assets/dirt2.png", tex_load_desc) @=> Texture dirt_sprite_2;
Texture.load(me.dir() + "./assets/dirt3.png", tex_load_desc) @=> Texture dirt_sprite_3;
Texture.load(me.dir() + "./assets/dirt4.png", tex_load_desc) @=> Texture dirt_sprite_4;

Texture.load(me.dir() + "./assets/stone.png", tex_load_desc) @=> Texture stone_sprite_0;
Texture.load(me.dir() + "./assets/stone1.png", tex_load_desc) @=> Texture stone_sprite_1;
Texture.load(me.dir() + "./assets/stone2.png", tex_load_desc) @=> Texture stone_sprite_2;
Texture.load(me.dir() + "./assets/stone3.png", tex_load_desc) @=> Texture stone_sprite_3;
Texture.load(me.dir() + "./assets/stone4.png", tex_load_desc) @=> Texture stone_sprite_4;

Texture.load(me.dir() + "./assets/wood.png", tex_load_desc) @=> Texture wood_sprite_0;
Texture.load(me.dir() + "./assets/wood1.png", tex_load_desc) @=> Texture wood_sprite_1;
Texture.load(me.dir() + "./assets/wood2.png", tex_load_desc) @=> Texture wood_sprite_2;
Texture.load(me.dir() + "./assets/wood3.png", tex_load_desc) @=> Texture wood_sprite_3;
Texture.load(me.dir() + "./assets/wood4.png", tex_load_desc) @=> Texture wood_sprite_4;

Texture.load(me.dir() + "./assets/axe.png", tex_load_desc) @=> Texture axe_sprite; // 50::ms per frame
Texture.load(me.dir() + "./assets/pickaxe.png", tex_load_desc) @=> Texture pickaxe_sprite; // 50::ms per frame
Texture.load(me.dir() + "./assets/shovel.png", tex_load_desc) @=> Texture shovel_sprite; // 50::ms per frame

Texture.load(me.dir() + "./assets/smear.png", tex_load_desc) @=> Texture smear_sprite; // 4 frames, 100::ms per frame
Texture.load(me.dir() + "./assets/spike.png", tex_load_desc) @=> Texture spike_sprite; // 11 frames, 75::ms per frame

Texture.load(me.dir() + "./assets/egg_break.png", tex_load_desc) @=> Texture egg_sprite; // 5 frames
Texture.load(me.dir() + "./assets/egg_unlock.png", tex_load_desc) @=> Texture egg_lock_sprite; // 2 frames

// == sound ================================
CKFXR sfx => dac;

// == graphics helpers ================================

vec2 tri_vertices[3];
fun void tri(vec2 pos, float base, float height) {
    @(base/2, 0) => tri_vertices[0];
    @(0, height) => tri_vertices[1];
    @(-base/2, 0) => tri_vertices[2];
    g.polygonFilled(pos, 0, tri_vertices, 0);
}

class AnimationEffect extends Effect {
    int nframes;
    .1 => float secs_per_frame;
    Texture@ sprite;
    vec2 pos;
    vec2 sca;
    float rot;


    // private
    int curr_frame;
	
	fun @construct(vec2 pos, vec2 sca, float rot, int nframes, Texture@ sprite, float secs_per_frame) { 
        pos => this.pos;
        sca => this.sca;
        rot => this.rot;
        nframes => this.nframes;
        sprite @=> this.sprite;
        secs_per_frame => this.secs_per_frame;
    }

	fun int update(G2D g, float dt) {
        (uptime / secs_per_frame) $ int => int frame;
        if (frame >= nframes) return END;

        g.pushLayer(1);
        g.sprite(
            this.sprite, nframes, frame,
            pos, sca, rot, Color.WHITE
        );
        g.popLayer();

		return STILL_GOING;
	}
}

class LevelUpEffect extends Effect {
    string text;
	Texture@ s;
	vec2 pos;
	float max_dur;
	float dy;
	float size;

	fun @construct(string text, Texture@ s, vec2 pos, float max_dur, float dy, float size) {
        text => this.text;
		s @=> this.s;
		pos => this.pos;
		max_dur => this.max_dur;
		dy => this.dy;
		size => this.size;
	}

	fun int update(G2D g, float dt) {
		if (uptime > max_dur) return END;
		uptime / max_dur => float t;

		// quadratic ease
		1 - (1 - t) * (1 - t) => t;
		g.pushLayer(1);
        g.pushColor(Color.WHITE);
		g.text(text, pos + @(-.1, t * dy), .8 * size);
		g.sprite(s, pos + @(.1, t * dy), .4 * size, 0);
        g.popColor();
		g.popLayer();

		return STILL_GOING;
	}
}


fun void smear(vec2 pos, float rot) {
    g.add(new AnimationEffect(pos, .75 * @(1, 1), rot, 4, smear_sprite, .06));
}

fun void levelUpEffect(string text, Texture@ t, vec2 pos, float dy, float sz) {
    g.add(new LevelUpEffect(text, t, pos, .75, dy, sz));
}

fun void progressBar(
    float percentage, vec2 pos, float width, float height
) {
    width * .5 => float hw;
    height * .5 => float hh;

    g.box(pos - @(0, hh), 2 * hw, 2 * hh, Color.WHITE);

    (percentage * 2 * hh) => float end_y;
    g.boxFilled(
        pos - @(hw, 0), // bot left
        pos + @(hw, -end_y),   // top right
        Color.WHITE
    );
}


// == physics ================================
class Physics {
    b2WorldDef world_def;
    int b2_world_id;
    int begin_sensor_events[0];
    int begin_touch_events[0];
    int end_touch_events[0];

    UI_Bool draw_b2_debug(true);
    DebugDraw debug_draw;
    debug_draw.layer(10);
    true => debug_draw.drawShapes;
    true => debug_draw.outlines_only;

    @(0, -9.81) => world_def.gravity;
    b2.createWorld(world_def) => b2_world_id;
    b2.world(b2_world_id);

    fun int createBody(vec2 pos, int body_type, int category, float sz, int is_sensor, b2Polygon@ geo) {
        b2BodyDef player_body_def;
        pos => player_body_def.position;
        body_type => player_body_def.type;
        true => player_body_def.fixedRotation;
        false => player_body_def.enableSleep;

        // entity
        b2.createBody(b2_world_id, player_body_def) => int b2_body_id;

        // filter
        b2Filter player_filter;
        category => player_filter.categoryBits;

        // shape
        b2ShapeDef player_shape_def;
        player_filter @=> player_shape_def.filter;
        if (
            category == EntityType_Player || is_sensor
        ) true => player_shape_def.enableSensorEvents;
        true => player_shape_def.enableContactEvents;
        1 => player_shape_def.density;
        is_sensor => player_shape_def.isSensor;

        // geo
        if (geo == null) b2.makeRoundedBox(.9 * sz, .9 * sz, .05 * sz) @=> geo; // use rounded corners to prevent ghost collisions
        b2.createPolygonShape(b2_body_id, player_shape_def, geo) => int b2_shape_id;

        return b2_body_id;
    }
}
Physics p;

0 => int EntityType_None;
1 => int EntityType_Player;
2 => int EntityType_Tile;

0 => int TileType_None; // empty space
1 => int TileType_Dirt; // shovel
2 => int TileType_Wood; // axe
3 => int TileType_Stone; // pickaxe
4 => int TileType_Coin;
5 => int TileType_Spike;
6 => int TileType_Egg;
7 => int TileType_Count; 

[
    Color.MAGENTA,
    Color.DARKGREEN,
    Color.BROWN,
    Color.GRAY,
] @=> vec3 tile_colors[];

[
    null,
    [
        dirt_sprite_0,
        dirt_sprite_1,
        dirt_sprite_2,
        dirt_sprite_3,
        dirt_sprite_4,
    ],
    [
        wood_sprite_0,
        wood_sprite_1,
        wood_sprite_2,
        wood_sprite_3,
        wood_sprite_4,
    ],
    [
        stone_sprite_0,
        stone_sprite_1,
        stone_sprite_2,
        stone_sprite_3,
        stone_sprite_4,
    ],
    [coin_sprite],
    null,
] @=> Texture tile_textures[][];

[
    null,
    shovel_sprite,
    axe_sprite,
    pickaxe_sprite,
    coin_sprite,
    null,
] @=> Texture tile_tools[];

/*
Egg mechanic: need X coins/keys to open lock. After openning, need to break the egg to get the power
- will become clear what type of egg it is after opening 
*/
0 => int EggType_Spoiled; // dud. does nothing
1 => int EggType_Juggernaut; // become larger. move slower. do more dmg
2 => int EggType_Connection; // damaging a tile damages all tiles of the same time that are connected
3 => int EggType_Count; // become larger. move slower. do more dmg

[
    "spoiled",
    "jeggernaut",
    "connegg",
] @=> string egg_names[];

class Player {
    int b2_body_id;

    TileType_Dirt => int tool;
    int tool_level[TileType_Count];
    int tool_exp[TileType_Count];

    // tiles touching
    Tile@ tile_left;
    Tile@ tile_right;
    Tile@ tile_down;

    // animation
    float animation_time_secs;
    1 => int facing;
    Spring tool_scale_spring(0, 500, 20);

    vec2 prev_vel;

    // eggs
    int eggs[EggType_Count];


    // preconstructor
    p.createBody(@(0,1), b2BodyType.dynamicBody, EntityType_Player, player_size.val(), false, null) => b2_body_id;
    b2Body.disable(b2_body_id);

    fun void remakeCollider() {
        pos() => vec2 old_pos;
        b2.destroyBody(b2_body_id);
        p.createBody(old_pos, b2BodyType.dynamicBody, EntityType_Player, player_size.val(), false, null) => b2_body_id;
    }

    fun int expToLevel(int tool_type) {
        (tool_level[tool_type] + 3) => int x;
        return x*x;
    }

    fun vec2 pos() { return b2Body.position(this.b2_body_id); }
    fun void pos(vec2 p) { b2Body.position(this.b2_body_id, p); }
    fun void posX(float x) { b2Body.position(this.b2_body_id, @(x, this.pos().y)); }

    fun void vel(vec2 v) {  b2Body.linearVelocity(this.b2_body_id, v); }
    fun vec2 vel() {  return b2Body.linearVelocity(this.b2_body_id); }
}

class Tile {
    int b2_body_id;

    // position in tilemap. currently only used in allConnected().
    // NOT set by default
    int row;
    int col;

    int type;
    int max_hp;
    int hp;

    // egg params
    int egg_type;
    int cost_to_unlock; // init to cost, decremented to 0 on purchase

    Spring translation_spring(0, 4200, 20);
    Spring rotation_spring(0, 1000, 10);
    Spring egg_price_spring(0, 500, 20);

    static HashMap b2body_to_tile_map;
    fun static Tile get(int b2_body_id) {
        return b2body_to_tile_map.getObj(b2_body_id) $ Tile;
    }

    fun void _destroyBody() {
        if (b2_body_id) {
            T.assert(b2Body.isValid(b2_body_id), "Tile empty() b2body not valid");
            b2body_to_tile_map.del(b2_body_id);
            b2.destroyBody(b2_body_id);
            0 => b2_body_id;
        }
    }

    fun void become(int type) {
        // TODO impl

    }

    fun void empty() {
        TileType_None => type;
        0 => max_hp;
        0 => hp;
        _destroyBody(); 
    }

    fun void coin(vec2 pos) {
        // TODO: should coins also need to be "mined" ?
        TileType_Coin => type;
        0 => max_hp;
        0 => hp;
        _destroyBody();
        p.createBody(pos, b2BodyType.staticBody, TileType_Coin, .68, true, null) => b2_body_id;
        b2body_to_tile_map.set(b2_body_id, this);
    }

    fun void egg(vec2 pos, int egg_type) {
        T.assert(egg_type < EggType_Count, "invalid egg type");
        <<< "egg type", egg_names[egg_type] >>>;
        // TODO: should egg HP be displayed??
        // do we need a cracking egg animation?
        egg_type => this.egg_type;
        TileType_Egg => type;
        difficulty * 15 => max_hp; // tweak egg hp
        max_hp => hp;
        5 => cost_to_unlock;
        _destroyBody();
        p.createBody(pos, b2BodyType.staticBody, EntityType_Tile, 1.0, false, null) => b2_body_id;
        b2body_to_tile_map.set(b2_body_id, this);
    }

    fun void spike(vec2 pos) {
        TileType_Spike => type;
        100 => max_hp;
        max_hp => hp;
        _destroyBody();
        // hit box is half the tile
        b2.makeOffsetBox(
            .35, // hw
            .25, // hh 
            @(0, -.25),    // center (local space)
            0 // rotation radians
        ) @=> b2Polygon geo;
        p.createBody(pos, b2BodyType.staticBody, TileType_Spike, 0.0, false, geo) => b2_body_id;
        b2body_to_tile_map.set(b2_body_id, this);
    }
    
    fun void randomize(vec2 pos) {
        // first: have a fixed n% chance of spawning a coin
        // TODO: spawn egg every rnd(10,30) rows
        if (Math.randomf() < .01) {
            coin(pos);
            return;
        }

        // TODO only spawn egg if we have enough coins
        if (Math.randomf() < egg_probability.val()) {
            egg(pos, Math.random2(0, EggType_Count - 1));
            return;
        }

        Math.random2(0, TileType_Stone) => type;
        Math.random2(0, HP_MAX + difficulty) => max_hp;
        if (type == TileType_None || max_hp == 0) {
            empty();
            return;
        }
        max_hp => hp;

        _destroyBody();
        p.createBody(pos, b2BodyType.staticBody, EntityType_Tile, 1.0, false, null) => b2_body_id;
        b2body_to_tile_map.set(b2_body_id, this);
    }

    fun vec2 pos() { return b2Body.position(this.b2_body_id); }
    fun void pos(vec2 p) { b2Body.position(this.b2_body_id, p); }
}


Tile tilemap[MINE_H.val()][MINE_W.val()];
int base_row; // what #row is tilemap[0]? (increments with every shift)
1.0 => float spawn_dist; // camera distance to spawn next row of blocks

fun vec2 tilepos(int r, int c) {
    base_row +=> r;
    return @(
        -((MINE_W.val()) * .5) + .5 + c,
        // (MINE_H.val() * .5) - .5 - r
        -.5 - r
    );
}

fun vec2 gridpos(vec2 p) {
    vec2 grid;
    -(p.y + base_row) $ int => grid.x;
    (p.x + MINE_W.val() / 2.0) $ int => grid.y;
    return grid;
}

5 => int HP_MAX;

// player params
Player player; // TODO physics body
5 => int n_coins;

int resources[TileType_Count];

int score;
int highscore;
float start_depth;

0 => int Room_Start;
1 => int Room_Play;
int room;

float gametime;
int difficulty;

Spring camera_shake_spring(0, 4200, 20);

0 => int Dir_None;
1 => int Dir_Left;
2 => int Dir_Right;
3 => int Dir_Down;
4 => int Dir_Up;


HashMap visited_tiles;
// optimize: don't recreate array every frame
fun Tile[] allConnected(int row, int col) {
    tilemap[row][col] @=> Tile tile;
    row => tile.row; col => tile.col;
    Tile tiles[0];
    if (tile.hp == 0) return tiles;

    visited_tiles.clear();
    tile.type => int type;
    [tile] @=> Tile bfs_queue[];

    while (bfs_queue.size()) {
        bfs_queue[-1] @=> Tile t;
        bfs_queue.popBack();
        T.assert(t.type == type && t.hp > 0, "invalid bfs tile");

        if (visited_tiles.has(t)) continue;

        tiles << t;
        visited_tiles.set(t, true);

        // add adjacent
        t.row => int r; t.col => int c;
        if (r+1 < MINE_H.val()) { // below
            tilemap[r+1][c] @=> Tile n;
            if (n.type == type && n.hp > 0) {
                r+1 => n.row; c => n.col;
                bfs_queue << n;
            }
        }
        if (r-1 >= 0) { // above
            tilemap[r-1][c] @=> Tile n;
            if (n.type == type && n.hp > 0) {
                r-1 => n.row; c => n.col;
                bfs_queue << n;
            }
        }
        if (c+1 < MINE_W.val()) { // right
            tilemap[r][c+1] @=> Tile n;
            if (n.type == type && n.hp > 0) {
                r => n.row; c+1 => n.col;
                bfs_queue << n;
            }
        }
        if (c-1 >= 0) { // left
            tilemap[r][c-1] @=> Tile n;
            if (n.type == type && n.hp > 0) {
                r => n.row; c-1 => n.col;
                bfs_queue << n;
            }
        }
    }

    T.assert(tiles.size() == visited_tiles.size(), "potential duplicates in allConnected " + tiles.size() + ", " + visited_tiles.size());

    return tiles;
}

fun void mine(Tile tile, int row, int col, int dir) { // returns true if tile was originally empty
    if (tile.hp <= 0) {
        T.err("mining an empty tile");
        return;
    }

    if (tile.type == TileType_Egg && tile.cost_to_unlock > 0) {
        <<< "paying for egg", n_coins, egg_names[tile.egg_type] >>>;
        if (n_coins == 0) {
            // u a broke boi
            // TODO sound effect
            // TODO flavor text: "u r a poor chicken"
            return;
        }

        n_coins--;
        tile.cost_to_unlock--;
        // TODO sound effect

        // TODO unlock juice
        levelUpEffect("-", coin_single_sprite, player.pos() + .5*g.UP, .3, .6);
        tile.egg_price_spring.pull(.3);

        // unlcokde the egg!
        // if (tile.cost_to_unlock == 0) {
        //     // TODO unlock sfx
        //     g.add(new AnimationEffect(tile.pos(), 1.0 * @(1, 1), 0, 4, smear_sprite, .06));
        // }

        return;
    }

    // attack anim
    player.pos() => vec2 smear_pos;
    float smear_rot;
    if (dir == Dir_Right) {
        .5*player_size.val() +=> smear_pos.x;
    }
    if (dir == Dir_Left) {
        .5*player_size.val() -=> smear_pos.x;
        Math.pi => smear_rot;
    }
    if (dir == Dir_Down) {
        .5*player_size.val() -=> smear_pos.y;
        -Math.pi/2 => smear_rot;
    }
    // smear(smear_pos, smear_rot);
    g.explode(smear_pos, .3, .3::second, Color.WHITE, smear_rot + Math.pi, Math.pi, ExplodeEffect.Shape_Squares);

    // only do dmg if tool matches
    if (player.tool == tile.type || tile.type == TileType_Egg) {
        if (tile.type == TileType_Egg) {
            T.assert(tile.cost_to_unlock == 0, "egg should only be damaged if unlocked");
            <<< "mining egg", egg_names[tile.egg_type] >>>;
        }

        1.0 => float dmg_modifier;
        player.tool_level[tile.type] => int base_dmg;
        if (player.eggs[EggType_Juggernaut]) 2.0 *=> dmg_modifier;
        (base_dmg * dmg_modifier) $ int => int dmg;

        [tile] @=> Tile connected_tiles[];
        if (player.eggs[EggType_Connection]) allConnected(row, col) @=> connected_tiles;

        for (auto tile : connected_tiles) {
            Math.max(0, tile.hp - dmg) => tile.hp;
            (tile.hp == 0) => int destroyed;

            // juice
            if (destroyed) {
                tile.max_hp +=> resources[tile.type];
                g.score("+" + tile.max_hp, tile.pos(), .5::second, .5,  0.6);
                g.explode(tile.pos(), 1, 1::second, Color.WHITE, 0, Math.two_pi, ExplodeEffect.Shape_Squares);

                // add exp to tool
                tile.max_hp +=> player.tool_exp[tile.type];
                if (player.tool_exp[tile.type] >= player.expToLevel(tile.type)) {
                    player.expToLevel(tile.type) -=> player.tool_exp[tile.type];
                    ++player.tool_level[tile.type];

                    sfx.powerup();
                    levelUpEffect("+", tile_tools[player.tool], player.pos() + .5*g.UP, .3, .6);
                }

                // acquire egg
                if (tile.type == TileType_Egg) {
                    <<< "acquiring egg", egg_names[tile.egg_type] >>>;
                    true => player.eggs[tile.egg_type];

                    // TODO add egg acquire juice

                    // on acquire egg logic
                    if (tile.egg_type == EggType_Juggernaut) {
                        .9 => player_size.val;
                        player.remakeCollider();
                    }
                }
                tile.empty();
            } else {
                g.hitFlash((1/62.0)::second, 1.0, tile.pos(), Color.WHITE);
                tile.rotation_spring.pull(tile_pull_force.val());
                tile.translation_spring.pull(tile_pull_force.val());
            }
        }
    }
}

fun void shift() {
    // shift everything up (can easily optimize)
    tilemap[0] @=> Tile bottom_row[];
    for (int row; row < MINE_H.val() - 1; row++) {
        tilemap[row+1] @=> tilemap[row];
    }
    // shift bottom to top
    bottom_row @=> tilemap[-1];
    base_row++;

    // randomize new row
    for (int col; col < MINE_W.val(); col++) {
        bottom_row[col] @=> Tile tile;
        tilepos(tilemap.size() - 1, col) => vec2 pos;

        (tilemap[tilemap.size() - 2][col].hp == 0) => int tile_above_is_empty;
        // 10% chance to spawn a spike when tile above is empty
        if (tile_above_is_empty && Math.randomf() < .1) {
            tile.spike(pos);
            continue;
        }
        tile.randomize(tilepos(tilemap.size() - 1, col));
    }
}

fun void die() {
    g.explode(player.pos(), 5, 3::second, Color.WHITE, 0, Math.two_pi, ExplodeEffect.Shape_Squares);
    Room_Start => room;
    Math.max(score, highscore) => highscore;
    b2Body.disable(player.b2_body_id);
    GG.camera().posY(0);
}

fun void init() {
    0 => gametime;
    0 => score;


    0 => base_row;
    1.0 => spawn_dist;
    // init tiles
    for (int row; row < MINE_H.val(); row++) {
        for (int col; col < MINE_W.val(); col++) {
            // first 5 rows empty
            if (row < 5) tilemap[row][col].empty();
            else tilemap[row][col].randomize(tilepos(row, col));
        }
    }

    // init camera
    -GG.camera().viewSize() * .5 => GG.camera().posY;
    GG.camera().posY() => start_depth;
    // 0 => GG.camera().posY;
    // 0 => start_depth;

    // init player
    player_base_size.val() => player_size.val;
    player.remakeCollider();
    TileType_Dirt => player.tool;
    b2Body.enable(player.b2_body_id);
    @(0, 1) => player.pos;
    0 => player.animation_time_secs;
    for (int i; i < player.tool_level.size(); ++i) 1 => player.tool_level[i];
    player.tool_exp.zero();
    player.eggs.zero();
    50 => n_coins;
}

int draw_all;
int do_ui;

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    if (room == Room_Play) dt +=> gametime;

    if (GWindow.keyDown(GWindow.KEY_GRAVEACCENT)) !do_ui => do_ui;

    if (do_ui) { // ui
        UI.slider("screen zeno", screen_zeno, 0, 1);
        UI.slider("zeno pos", player_target_pos, -10, 10);

        UI.slider("player speed", player_speed, 0, 10);
        UI.checkbox("draw b2 debug", p.draw_b2_debug);
    }

    // difficulty scaling
    // difficulty incr every N seconds
    (1 + (gametime / seconds_per_level.val())) $ int => difficulty; 

    if (room == Room_Start) {
        g.text("[arrow keys] move/mine", @(0, 1), .5);
        g.text("[tab] change tool", @(0, -1), .5);
        if (g.anyInputDown()) {
            init();
            Room_Play => room;
        }
        continue;
    }

    // score
    {
        // g.n2w(.9, 1-(.1*aspect)) => vec2 pos;
        // g.pushTextControlPoint(@(1, 1));
        // g.text("HI " + highscore, pos, .5);
        // g.popTextControlPoint();

        g.n2w(-.9, 1-(.1*aspect)) => vec2 pos;
        g.pushTextControlPoint(@(0, 1));
        g.text(Std.ftoa(start_depth - GG.camera().posY(), 0) + "m", pos - @(.25, 0), .5);
        .5 -=> pos.y;
        g.text("L" + difficulty, pos - @(.25, 0), .5);
        g.popTextControlPoint();

        .2 -=> pos.x;
        1 -=> pos.y;
        (gametime / .1)$int % 6 => int curr_frame; 
        g.sprite(
            coin_sprite, 6, curr_frame,
            pos, .5 * @(1, 1), 0, Color.WHITE
        );
        g.text(n_coins + "", pos + @(.4, 0), .45);


        // tool exp progress
        progressBar(
            1.0 *  player.tool_exp[player.tool] / player.expToLevel(player.tool),
            pos + @(.8, .25),
            .3,
            2.0
        );

        // tool levels
        .5 -=> pos.y;

        .4 + .025 * Math.sin(5 * (now/second)) => float active_tool_sz;

        g.sprite( shovel_sprite, pos, .3, 0 );
        g.text("" + player.tool_level[TileType_Dirt], pos + @(.4, 0), .45);
        if (player.tool == TileType_Dirt) g.square(pos, 0, active_tool_sz, Color.WHITE);
        .5 -=> pos.y;
        g.sprite( axe_sprite, pos, .3, 0 );
        g.text("" + player.tool_level[TileType_Wood], pos + @(.4, 0), .45);
        if (player.tool == TileType_Wood) g.square(pos, 0, active_tool_sz, Color.WHITE);
        .5 -=> pos.y;
        g.sprite( pickaxe_sprite, pos, .3, 0 );
        g.text("" + player.tool_level[TileType_Stone], pos + @(.4, 0), .45);
        if (player.tool == TileType_Stone) g.square(pos, 0, active_tool_sz, Color.WHITE);


    }

    { // egg list
        g.n2w(.9, 1-(.1*aspect)) => vec2 pos;
        g.pushTextControlPoint(1, .5);

        for (int egg_type; egg_type < EggType_Count; ++egg_type) {
            if (player.eggs[egg_type]) {
                g.text(egg_names[egg_type], pos, .45);
                .5 -=> pos.y;
            }
        }
        g.popTextControlPoint();
    }

    // collision
    player.pos() => vec2 player_pos;
    b2World.contactEvents(p.b2_world_id, p.begin_touch_events, p.end_touch_events, null);
    for (int i; i < p.begin_touch_events.size(); 2 +=> i) {
        p.begin_touch_events[i] => int touch_shape_a;
        p.begin_touch_events[i+1] => int touch_shape_b;
        if (!b2Shape.isValid(touch_shape_a) || !b2Shape.isValid(touch_shape_b)) continue;
        b2Shape.body(touch_shape_a) => int touch_body_id_a;
        b2Shape.body(touch_shape_b) => int touch_body_id_b;
        if (!b2Body.isValid(touch_body_id_a) || !b2Body.isValid(touch_body_id_b)) continue;

        T.assert(
            touch_body_id_a == player.b2_body_id
            ||
            touch_body_id_b == player.b2_body_id,
            "non-player collision"
        ); 

        touch_body_id_b => int contact_body_id;
        if (touch_body_id_b == player.b2_body_id) touch_body_id_a => contact_body_id;

        Tile.get(contact_body_id) @=> Tile tile;
        tile.pos() => vec2 tile_pos;

        if (tile.type == TileType_Spike) {
            die();
            continue;
        }

        // check for if we fell a large distance
        // gridpos(tile_pos) => vec2 tile_grid_pos;
        // gridpos(player.pos()) => vec2 player_grid_pos;
        // ((tile_grid_pos.y $ int) > (player_grid_pos.y $ int)) => int player_fell;
        // if (player_fell) {
        //     <<< player.vel(), tile_grid_pos, player_grid_pos >>>;
        //     camera_shake_spring.pull(player.vel().y);
        // }
    }

    b2World.sensorEvents(p.b2_world_id, p.begin_sensor_events, null);
    for (int i; i < p.begin_sensor_events.size(); 2 +=> i) {
        p.begin_sensor_events[i] => int sensor_shape_id;
        p.begin_sensor_events[i+1] => int visitor_shape_id;
        if (!b2Shape.isValid(sensor_shape_id) || !b2Shape.isValid(visitor_shape_id)) continue;
        b2Shape.body(sensor_shape_id) => int sensor_body_id;
        b2Shape.body(visitor_shape_id) => int visitor_body_id;
        if (!b2Body.isValid(sensor_body_id) || !b2Body.isValid(visitor_body_id)) continue;
        T.assert(visitor_body_id == player.b2_body_id, "sensor event with non-player");
        Tile.get(sensor_body_id) @=> Tile tile;

        if (tile.type == TileType_Coin) {
            // TODO: b2 functions should print ck error if passed an invalid body id
            g.score("+" + 1, tile.pos(), .5::second, .5,  0.6);
            n_coins++;
            sfx.coin(72, 76);
            tile.empty();
        }
    }

    if (GWindow.keyDown(GWindow.Key_1)) !draw_all => draw_all;

    // controls
    { 
        if (GWindow.keyDown(GWindow.KEY_UP) || GWindow.keyDown(GWindow.KEY_TAB)) {
            // cycle tools
            if (player.tool == TileType_Stone) TileType_Dirt => player.tool;
            else ++player.tool;

            player.tool_scale_spring.pull(.3);
        }

        // if (GWindow.keyDown(GWindow.KEY_RIGHT) && player_col < MINE_W.val() - 1) {
        // if (GWindow.keyDown(GWindow.KEY_DOWN) && player_row < MINE_H.val() - 1) {
            // if (mine(player_row + 1, player_col)) {
            //     player_row++;
            //     // shift();
            // }
        // }

        if (GWindow.key(GWindow.KEY_RIGHT) || GWindow.key(GWindow.KEY_LEFT)) dt +=> player.animation_time_secs;
        if (GWindow.keyDown(GWindow.KEY_RIGHT)) 1 => player.facing;
        if (GWindow.keyDown(GWindow.KEY_LEFT)) -1 => player.facing;

        1.0 => float speed_modifier;
        if (player.eggs[EggType_Juggernaut]) .5 *=> speed_modifier;
        @(
            speed_modifier * player_speed.val() * (
                GWindow.key(GWindow.KEY_RIGHT) - GWindow.key(GWindow.KEY_LEFT)
            ),
            player.vel().y
        ) => player.vel;

        // clamp position to screen
        Math.clampf(player.pos().x, -.5 * MINE_W.val(), .5 * MINE_W.val()) => player.posX;

        // determine grid pos of player
        gridpos(player.pos()) => vec2 gridpos;
        tilepos(gridpos.x $ int, gridpos.y $ int) => vec2 player_tile_pos;

        if (p.draw_b2_debug.val()) g.square(player_tile_pos, 0, 1.0, Color.GREEN);

        // calculate touching tiles
        gridpos.x $ int => int row;
        gridpos.y $ int => int col;
        // <<< row, col >>>;


        if (GWindow.keyDown(GWindow.Key_Down) && col < MINE_W.val() && row < MINE_H.val()) {
            tilemap[row+1][col] @=> Tile tile_below;
            (1 + M.fract(player.pos().y) < (.5 * player_size.val())) => int player_on_ground;
            if (
                (tile_below.hp > 0) && player_on_ground
            ) {
                mine(tile_below, row+1, col, Dir_Down);
            }
        }

        if (GWindow.keyDown(GWindow.Key_Right) && col < MINE_W.val() - 1 && row > 0) {
            (player.pos().x - player_tile_pos.x) > (.5 - .5*player_size.val() - .01) => int touching_side;
            tilemap[row][col+1] @=> Tile tile_right;
            if ( (tile_right.hp > 0) && touching_side) {
                mine(tile_right, row, col+1, Dir_Right);
            }
        }

        if (GWindow.keyDown(GWindow.Key_Left) && col > 0) {
            (player.pos().x - player_tile_pos.x) < (-.5  + .5 * player_size.val() + .01) => int touching_side;
            tilemap[row][col-1] @=> Tile tile_left;
            if ((tile_left.hp > 0) && touching_side) {
                mine(tile_left, row, col-1, Dir_Left);
            }
        }

        // shake on falling a large distance (TODO maybe remove this)
        if (1) {
            player.vel().y - player.prev_vel.y => float delta;
            if (delta > 8) {
                <<< delta >>>;
                camera_shake_spring.pull(delta * .05);
            }
        }
    }

    // update camera
    dt * camera_speed.val() => float scroll_dist;
    player_target_pos.val() + GG.camera().posY() - player.pos().y => float distance_from_threshold; 
    Math.max(0, distance_from_threshold) * screen_zeno.val() +=> scroll_dist;
    GG.camera().translateY(-scroll_dist);

    // death condition
    if (gametime > 3 && player.pos().y > GG.camera().posY() + 5) {
        die();
    }

    // spawn more blocks 
    scroll_dist -=> spawn_dist;
    while (spawn_dist < 0) {
        shift();
        1.0 +=> spawn_dist;
    }


    g.pushLayer(1); { // draw player
        (player.animation_time_secs / .05) $ int % 4 => int curr_frame;
        g.sprite(
            chicken_sprite, 4, curr_frame,
            player.pos(), player_size.val() * @(player.facing, 1), 0, Color.WHITE
        );
        // g.circleFilled(player.pos(), .25, tile_colors[player.tool]);
    
        // debug draw neighbording
        if (p.draw_b2_debug.val()) {
            if (player.tile_left != null) g.circle(player.tile_left.pos(), .1, Color.WHITE);
            if (player.tile_right != null) g.circle(player.tile_right.pos(), .1, Color.WHITE);
            if (player.tile_down != null) g.circle(player.tile_down.pos(), .1, Color.WHITE);
        }

        // draw tool
        player.tool_scale_spring.x + 1 => float tool_sca;
        g.sprite( tile_tools[player.tool], player.pos() + @(0,.0), tool_sca * .5 * @(-player.facing, 1), 0, Color.WHITE);

    } g.popLayer();

    // update and draw tiles
    for (int row; row < MINE_H.val(); row++) {
        for (int col; col < MINE_W.val(); col++) {
            tilemap[row][col] @=> Tile tile;
            tilepos(row, col) => vec2 pos;

            // update springs
            tile.rotation_spring.update(dt);
            tile.translation_spring.update(dt);
            tile.egg_price_spring.update(dt);

            // g.square(pos, 0, 1.0, Color.WHITE);
            if (tile.type == TileType_Coin) {
                (gametime / .1)$int % 6 => int curr_frame; 
                g.sprite(
                    coin_sprite, 6, curr_frame,
                    tile.pos(), .9 * @(1, 1), 0, Color.WHITE
                );
            }
            else if (tile.type == TileType_Spike) {
                // old spike programmer art
                // pos - @(.5, .5) => vec2 p;
                // .33 / 2 +=> p.x;
                // repeat (3) {
                //     tri(p, .33, .5);
                //     .33 +=> p.x;
                // }

                (gametime / .075) $ int % 12 => int curr_frame; 
                g.sprite(
                    spike_sprite, 12, curr_frame,
                    tile.pos(), @(1,1), 0, Color.WHITE
                );
            }
            else if (tile.type == TileType_Egg) {
                // .05 * M.rot2vec(gametime * 3) => vec2 delta;
                @(0,0) => vec2 delta;
                tile.pos() + delta => vec2 pos;
                if (tile.cost_to_unlock > 0) { // still not bought
                    g.sprite(
                        egg_lock_sprite, 2, 0,
                        pos, 1.0 * @(1,1), 0, Color.WHITE
                    );

                    .5 * tile.egg_price_spring.x => float sca;

                    (gametime / .1)$int % 6 => int curr_frame; 
                    g.sprite(
                        coin_sprite, 6, curr_frame,
                        pos - @(-.08, .1), (.16 + 0) * @(1, 1), 0, Color.WHITE
                    );
                    g.pushColor(Color.WHITE);
                    g.text(tile.cost_to_unlock + "", pos - @(.08, .1), (.3 - sca));
                    g.popColor();
                } 
                else if (tile.cost_to_unlock == 0 && tile.hp == tile.max_hp) {
                    g.sprite(
                        egg_lock_sprite, 2, 1,
                        pos, 1.0 * @(1,1), 0, Color.WHITE
                    );
                } else {
                    1.0 * tile.hp / tile.max_hp => float perc_health;
                    ((1 - perc_health) * 4) $ int => int frame;

                    0.8 * tile.rotation_spring.x => float rot;
                    @(0, 1.5 * tile.translation_spring.x) + tile.pos() => vec2 pos;
                    g.sprite(
                        egg_sprite, 5, frame,
                        pos, (15.0/16) * @(1,1), rot, Color.WHITE
                    );
                }
            }
            else if (tilemap[row][col].hp != 0 || draw_all) {
                // display hp
                // g.pushColor(Color.WHITE);
                // g.text("" + tilemap[row][col].hp, pos, .5);
                // g.popColor();

                0.8 * tile.rotation_spring.x => float rot;
                1.5 * tile.translation_spring.x +=> pos.y;

                1.0 * tile.hp / tile.max_hp => float perc_health;

                ((1 - perc_health) * tile_textures[tile.type].size()) $ int => int tex_idx;

                // g.sprite( tile_textures[tile.type][0], pos, 16/16.0, rot );
                g.sprite( tile_textures[tile.type][tex_idx], pos, 15.0/16.0, rot );
            //    g.square( pos, 0, 1.0, Color.WHITE);
            //    g.squareFilled( pos, 0, 1.0, Color.WHITE);
            }

        }
    }

    { // cleanup
        // physics debug draw
        if (p.draw_b2_debug.val()) b2World.draw(p.b2_world_id, p.debug_draw);
        p.debug_draw.update();

        // springs
        camera_shake_spring.update(dt);
        camera_shake_spring.x => GG.camera().posX;

        // player
        player.vel() => player.prev_vel;
        player.tool_scale_spring.update(dt);
    }
}

