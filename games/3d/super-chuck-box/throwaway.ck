/*
Ideas

A void-like + super crate box (SCB) arena survival shooter.
Single room, like SCB. Gain score by collecting crate/chuck boxes.

For now, assume room is a single quad, like Devil Dagger.

Vertical Slice:
- 1 weapon type (hitscan, like quake rail gun)
- 1 enemy type (box that moves around randomly, OR moves towards you)
- basic WASD movement
    - maybe allow diagonal strafe for speedup 
    - no jump

DON'T HAVE
- no ammo
    - remove reload animation, clip sizes, ammo types etc.... just shoot. 
- no healing / health / powerups
    - just a shield that regens out of combat, halo3/lufthauser style

Influences
- super crate box
- devil dagger
- (ultrakill?)
    TO STUDY: watch creator hikata's youtube, see if they have other design vids

Art ideas
- low res ps1
- bright colors/neon
- highly geometrical, maybe like a 3D version of snkrx / ae327x's aesthetic
- also see PhoboLab's games (q1k3, underrun, void call...)
- decals!!


Enemy types
- bouncer (like DVD logo)
- stationary turret guy
- enemy types that incentivize you to NOT go straight for the box
    - egg
    - spawner
    - splitter (big guy that dies and spawns N smaller guys)
- charger
- splitter
- bouncer (charges in straight line and bounces off map edges, like DVD logo)
- enemy with a "weak spot" (like Devil Daggers) so the player has to move/rotate around enemy to dmg it
- look at following games for inspo:
    - devil daggers
    - scb
    - tboi
    - mewgenics
    - halo
    - vampire surivvors
    - brotato
        -   and other survival bullet heaven

Spawn mechanisms:
- emerge from/around the chuck box itself (really nice risk/reward setup here)
- fall from overhead
- emerge from underground

Optimizations:
- custom shader (phongmat is expensive)
- render to fixed-size, lower res rendertarget. NN sampler
- backface culling on all geo!!!

Weapons
- rocket (aoe explode, self-dmg)
- katana (melee, cleave/splash can hit multiple if in range)
- rail gun (long cd, hitscan, PIERCING)
- disc gun (bounces off walls, self-dmg)
- quake zapper (low dmg, hitscan, requires tracking, maybe can chain lightning)
- sentinal beam
- shotgun
- basic pistol (hit scan, low dmg)
- grenade launcher (like Overwatch junkrat. fun, can bounce and hit around corners)
    - maybe this only makes sense if 
- flame thrower
- what if apply Tom Francis stats file format to weapon impl??
    - don't need to define weapon properties in code, can do it in file and go crazy with permutations

Physics:
- use all AABB colliders for now
- can eventually refactor into custom chuck 3d physics collision detection library
    - maybe look at cutephysics (the 2d physics lib that Mewgenics uses) as ref


Code references:
- q1k3
- dungeon of quake (odin + raylib)


Boxes / Upgrades
- chuck box for new weapon
- upchuck box for new movement type??? 


*/

// @import "../../lib/g2d/ChuGL-debug.chug"
@import "../../lib/g2d/ChuGL.chug"
@import "../../lib/M.ck"
@import "../../lib/T.ck"
@import "./constants.ck"
@import "./lines3d.ck"
@import "./util.ck"

// == chugl config ========================================
GWindow.mouseMode(GWindow.MOUSE_DISABLED);
GG.autoUpdate(false);
GG.outputPass().tonemap(OutputPass.ToneMap_None);
GG.outputPass().gamma(true);

Constants c;

// == load assets ========================================
Texture.load(me.dir() + "./assets/floor.png") @=> Texture floor_tex;
Texture.load(me.dir() + "./assets/shotgun.png") @=> Texture shotgun_tex;
Texture.load(me.dir() + "./assets/chuck_logo_web2.jpg") @=> Texture chuck_tex;
Texture.load(me.dir() + "./assets/chuck-logo2023w.png") @=> Texture chuck_tex_transparent;
(new FlatMaterial).colorMap() @=> Texture white_pixel;
// Texture.load(me.dir() + "./assets/chuck-logo2023w.png") @=> Texture chuck_tex;

class Sound {

    SndBuf shotgun(me.dir() + "./assets/sfx/Qtestguncock.wav") => dac;
    shotgun.rate(0);

    fun void shoot() {
        shotgun.pos(0);
        Math.random2f(.9, 1.2) => shotgun.rate;
    }
} Sound s;

// GG.scene().camera(GOrbitCamera orbit_cam);
// orbit_cam.posY(5);

// global gamestate
class GS {
    float gametime;

    // input
    int key_u;
    int key_d;
    int key_r;
    int key_l;
    int key_jump;

    vec2 mouse_delta;
    vec2 mouse_deltas[2];
    int mouse_deltas_idx;
    vec2 mouse_deltas_avg;

    // entities
    Entity ground;
    Entity player;
    EntityPool entity_pool;
    Entity@ enemy_entities[0]; // reset every frame

    // misc
    vec4 trails[0]; // .w is time until expiration

} GS gs;

int debug_console;


Lines3D l3d --> GG.scene(); // debug widget

GPlane ground --> GG.scene();
ground.rotX(M.PI/2);
ground.colorMap(floor_tex);
ground.sca(100);
ground.uvScale(@(100, 100));
ground.specular(Color.BLACK);
// ground.scale(10);

GCube chuckbox --> GG.scene();
chuckbox.posY(.5);
chuckbox.posZ(-2);
chuckbox.colorMap(chuck_tex);
chuckbox.material().cullMode(Material.Cull_Back);
chuckbox.emission(Color.WHITE);
// chuckbox.emissiveMap((chuckbox.material() $ PhongMaterial).normalMap());
// chuckbox.emissiveMap(white_pixel);
chuckbox.emissiveMap(chuck_tex);

// player cam
GCamera player_camera --> GG.scene();
player_camera => GG.scene().camera;
player_camera.fov(90 * M.DEG2RAD); // 90 deg fov
GMesh crosshair(new CircleGeometry, FlatMaterial crosshair_mat) --> player_camera;
crosshair.sca(.001);
crosshair.posZ(-.11);

UI_Float mouse_sens(.001);
UI_Bool use_mouse_avg(true);
int first_mouse_delta;

CylinderGeometry cylinder_geo(
    .2, .2,  // radius top and bot
    1,       // length
    6,       // radial segments
    1,       // length semgnets
    false,   // open ended?
    0,       // theta start
    Math.two_pi // theta len
);
FlatMaterial weapon_mat;  weapon_mat.colorMap(shotgun_tex);
GMesh tmp(cylinder_geo, weapon_mat) --> GGen tmp_container --> player_camera;
tmp.rotX(-Math.pi/2);
tmp_container.posZ(c.weapon_z_off.val());
tmp_container.posY(c.weapon_y_off.val());
// tmp.sca(.2);
// tmp.scaZ(.8);

class Collision {
    Entity@ e;
    float t;
    vec3 p;

    fun @construct(Entity@ e, float t, vec3 pos) {
        e @=> this.e;
        t => this.t;
        pos => this.p;
    }
}

// entity fat struct
class Entity {
    static CubeGeometry cube_geo;
    static FlatMaterial hitbox_mat; true => hitbox_mat.wireframe;

    0 => static int Type_Map;
    1 => static int Type_Player;
    2 => static int Type_Bullet;
    3 => static int Type_Particle;
    4 => static int Type_Enemy;
    5 => static int Type_Debug;


    // alive?
    int dead;
    float hp;

    // spawn stats
    float spawn_time;
    float lifetime; // if nonzero, will despawn after lifetime secs

    // type tags
    int type;
    int debug_type;

    // physics
    vec3 acc, vel, pos;
    vec3 aabb; // axis aligned bounding box, half-width, half-height, etc
    float gravity, friction;
    int check_against_enemies;

    // orientation rotation
    float pitch;
    float yaw;
    
    // for now enforce each entity only has 1 mesh
    PhongMaterial phong_mat;
    GMesh mesh; 
    GMesh hitbox(cube_geo, hitbox_mat);

    // weapon stuff
    float weapon_rotZ_predelay; // in secs
    float weapon_rotZ; // applied over time
    float weapon_bob;

    // enemy stuff
    float enemy_hit_cd; 

    fun void zero() {
        false => dead;
        0 => hp;

        @(0, 0, 0) => mesh.rot;
        @(1, 1, 1) => mesh.sca;

        0 => spawn_time => lifetime;

        0 => type => debug_type;

        @(0, 0, 0) => acc => vel => pos => aabb;
        0 => gravity => friction;
        false => check_against_enemies;

        0 => pitch => yaw;

        // disconnect from scenegraph
        mesh.detachParent();
        hitbox.detachParent();

        0 => weapon_rotZ_predelay => weapon_rotZ => weapon_bob;
    }


    fun void init(int type) {
        type => this.type;
        if (type == Type_Map) {

        }
        else if (type == Type_Player) {
            // player params
            c.player_gravity.val() => gravity;
            c.player_friction.val() => friction;
            @(.1, 1, .1) => aabb;
            aabb.y => pos.y;
        }

        else if (type == Type_Enemy) {
            // stats
            10 => hp;

            // physics
            .5 * @(1, 2, 1) => aabb;
            aabb.y => pos.y;
            true => check_against_enemies;

            // enemy mesh
            mesh.mesh(cube_geo, phong_mat);

            // init particle model
            mesh.mesh(cube_geo, phong_mat);

            // xform
            mesh --> GG.scene();
            2 * aabb => mesh.sca;
            <<< "init enemy sca", mesh.scaWorld() >>>;
        }

        // hitbox visualize
        if (type != Type_Player) {
            aabb * 2.01 => hitbox.sca;
            hitbox --> GG.scene();
        }
    }

    fun damage(float amt) {
        if (dead) return;

        if (type == Type_Enemy) {
            .5 => enemy_hit_cd;
            amt -=> hp;
            if (hp <= 0) {
                true => dead;
                spawnParticles(pos, 10, .15, 2);
            }
        }
    }

    fun collideWith(Entity@ e, vec3 collision_pos) {
        T.assert(e != null, "cannot collide with null entity");
        if (type == Type_Bullet) {
            true => dead;
            // <<< "bullet collided with", T.str(e.mesh) >>>;
            // TODO floor and stuff can be static entities, remove null check here
            if (e.type == Type_Enemy) e.damage(0);
            else if (e.type == Type_Map) {
                // collided with floor
                // trace ray back to y=0 for accurate hit location
                T.assert(collision_pos.y - aabb.y < 0, "floor collision should have y < 0");
                collision_pos.y / vel.y => float step_size;
                vel * step_size -=> collision_pos;
                collision_pos => pos;
            }

            spawnParticles(pos, 8, .03, 1);
        } 
    }

    // currently only checks against *enemy* collisions
    fun int collides(vec3 p) {
        p - aabb => vec3 min;
        p + aabb => vec3 max;

        for (Entity@ e : gs.enemy_entities) {
            if (e == this) continue;

            e.pos - e.aabb => vec3 e_min;
            e.pos + e.aabb => vec3 e_max;

            // <<< "type ", type, p, "checking against enemy", T.str(e.mesh)>>>;
            // <<< "min", min, "max", max, "e_min", e_min, "emax", e_max >>>;
            // <<< (min.x <= e_max.x && max.x >= e_min.x),
            //     (min.y <= e_max.y && max.y >= e_min.y),
            //     (min.z <= e_max.z && max.z >= e_min.z)
            // >>>;

            if ( 
                (min.x <= e_max.x && max.x >= e_min.x) &&
                (min.y <= e_max.y && max.y >= e_min.y) &&
                (min.z <= e_max.z && max.z >= e_min.z)
            ) {
				collideWith(e, p);
                return true;
            }
        }

        return false;
    }

    // returns closest hit entity. NULL if none are hit
    // NOTE: only checks enemies and ground
    fun Collision hitscan(vec3 o, vec3 d) {
        -1 => float t; // isection t
        Entity@ hit_enemy;
        
        for (Entity@ e : gs.enemy_entities) {
            if (e.dead) continue;

            // @optimize cache bbox min and max
            e.pos - e.aabb => vec3 e_min;
            e.pos + e.aabb => vec3 e_max;

            M.rayAABBIsect(e_min, e_max, o, d) => vec2 ts;
            (ts.x <= ts.y) => int penetrate;
            (ts.x >= 0) => int in_front;
            if (penetrate && in_front) {
                // hit something! check if closest
                if (hit_enemy == null || ts.x < t) {
                    ts.x => t;
                    e @=> hit_enemy;
                }
            }
        }

        if (hit_enemy != null) <<< "hit enemy!" >>>;

        // ground collision
        M.rayPlaneIsect(o, d, @(0,0,0), @(0,1,0)) => float t_ground;
        if (t_ground > 0) {
            if (hit_enemy == null || t_ground < t) {
                t_ground => t;
                gs.ground @=> hit_enemy;
            }
        }

        if (hit_enemy != null) return new Collision(hit_enemy, t, o + t * d);
        else return null;
    }

    fun void updatePhysics(float dt) {
		// Apply Gravity
		-20 * gravity => acc.y;

		// Integrate acceleration & friction into velocity
		Math.min(friction * dt, 1) => float friction_force;
		(
            acc * dt -
			friction_force * @(vel.x, 0, vel.z) // TODO this is weird, why scale friction force by velocity?
        ) +=> vel;

        /*
        Tunneling and Substeps

        Assuming AABB, for fast projectiles the minstep required to prevent tunneling is 
        equal to the smallest length of its AABB + the smallest length of the collision object's AABB.
        Assuming the other AABB is a singular point, this simplifies to the shortest axis of its own AABB.

        E.g. if a bullet is 1x1x1 unit, it cannot be integrated over 1 unit at a time without risk of 
        missing a collision.

        Heuristic: #substeps =  ceil(move_dist / min_aabb_axis)
        */
        Math.min(Math.min(aabb.x, aabb.y), aabb.z) => float min_aabb_axis;
        Math.max(min_aabb_axis, .01) => min_aabb_axis; // force nonzero
        vel * dt => vec3 delta_pos;

        1 => int steps;
        // only substep where it matters -- for enemy hit detection
        if (check_against_enemies) {
            Math.min(16, Math.ceil(delta_pos.magnitude() / min_aabb_axis)) $ int => int steps; // clamped 1-16
            // <<< steps, "num steps", "min_aabb_axis", min_aabb_axis >>>;
        }
        delta_pos / steps => vec3 move_step;

        for (int s; s < steps; s++) {
            move_step => vec3 delta_pos;

            if (check_against_enemies) {

                // YZ plane collision
                if (collides(pos + @(move_step.x, 0, 0))) { 
                    0 => delta_pos.x;
                    steps => s; // stop for loop
                }

                // XZ plane collision
                if (collides(pos + @(0, move_step.y, 0))) { 
                    0 => delta_pos.y;
                    steps => s; // stop for loop
                }

                // XY plane collision
                if (collides(pos + @(0, 0, move_step.z))) { 
                    0 => delta_pos.z;
                    steps => s; // stop for loop
                }
            }

            // ground collision (checks along y axis only)
            if ((move_step + pos).y - aabb.y < 0) {
                if (type == Type_Player) {
                    aabb.y => pos.y;
                }

                if (vel.y < 0) 0 => vel.y; // 0 so player doesn't fall through floor
                0 => delta_pos.y;

                collideWith(gs.ground, move_step + pos);
                steps => s; // stop for loop
            }

            // no collision, move forward
            delta_pos +=> pos;
        }

        pos => hitbox.posWorld;
    }

    fun void update(float dt) {
        if (lifetime > 0 && (gs.gametime > spawn_time + lifetime)) true => dead;

        if (dead) return;

        if (type == Type_Player) {
            if (use_mouse_avg.val()) {
                Math.clampf(pitch - gs.mouse_deltas_avg.y * mouse_sens.val(), -1.5, 1.5) => pitch;
                yaw - gs.mouse_deltas_avg.x * mouse_sens.val() => yaw;
            } else {
                Math.clampf(pitch - gs.mouse_delta.y * mouse_sens.val(), -1.5, 1.5) => pitch;
                yaw - gs.mouse_delta.x * mouse_sens.val() => yaw;
            }
            player_camera.rot(@(pitch, yaw, 0));

            player_camera.forward() => vec3 forward => vec3 forward_no_y;
            0 => forward_no_y.y; forward_no_y.normalize();
            player_camera.right() => vec3 right => vec3 right_no_y;
            0 => right_no_y.y; right_no_y.normalize();

            // movement
            (vel.y <= 0 && (pos.y - aabb.y <= 1e-4)) => int on_ground;
            c.player_speed.val() * (on_ground ? 1.0 : c.player_air_vel_ratio.val()) * (
                (gs.key_r - gs.key_l) * right_no_y + (gs.key_u - gs.key_d) * forward_no_y
            ) => acc;


            if (gs.key_jump && on_ground) {
                // jumpings sets velocity directly instead of acc, bc vertical acceleration is always gravity
                c.player_jump_vel.val() => vel.y;
                false => on_ground;
            }

            // physics and collision
            updatePhysics(dt);

            // update camera 
            pos + @(0, aabb.y, 0) => player_camera.pos;

            // update weapon xform
            if (weapon_rotZ_predelay > 0) {
                dt -=> weapon_rotZ_predelay;
            } else {
                if (weapon_rotZ > 0) {
                    Math.min(weapon_rotZ, 10 * dt) => float rot_amt;
                    tmp_container.rotateZ(rot_amt);
                    rot_amt -=> weapon_rotZ;
                }
            }

            @(acc.x, 0, acc.z).magnitude() * c.weapon_bob_freq.val() +=> weapon_bob; // TODO wrap between [0, 2pi]
            c.weapon_y_off.val() + c.weapon_bob_mag.val() * Math.sin(weapon_bob) => tmp_container.posY;

            // 12 + clamp(scale(shoot_wait, 0, weapon._reload, 5, 0), 0, 5),

            // 12 + clamp(scale(shoot_wait, 0, weapon._reload, 5, 0), 0, 5),

            // fire shoot weapon
            if (GWindow.mouseLeftDown()) {
                // sound effect!
                s.shoot();

                // rotate the barrel
                Math.two_pi/6 => weapon_rotZ;
                .25 => weapon_rotZ_predelay;

                // hitscan
                6 => int NUM_BULLETS;
                repeat (NUM_BULLETS) {
                    // fire from camera
                    player_camera.posWorld() => vec3 bullet_pos;
                    // OR fire from gun
                    // pos => vec3 bullet_pos;

                    .15 => float recoil_jitter; // angle displacement on shotgun blast

                    @(0, 0, -1) => vec3 dir;
                    M.rotateX(dir, pitch + recoil_jitter * Math.random2f(-1, 1)) => dir;
                    M.rotateY(dir, yaw + recoil_jitter * Math.random2f(-1, 1)) => dir;

                    // hitscan(bullet_pos, forward) @=> Collision collision;
                    hitscan(bullet_pos, dir) @=> Collision collision;

                    // trail renderer
                    if (collision != null) {
                        // this could prob be moved into a Weapon.hit() fn
                        collision.e.damage(2);
                        spawnParticles(collision.p, 8, .03, 1);


                        // @lang would be nice to have these vec constructors
                        gs.trails
                            << @(bullet_pos.x, bullet_pos.y, bullet_pos.z, 1.0)
                            << @(
                                collision.p.x,
                                collision.p.y,
                                collision.p.z,
                                1.0
                            );
                    }
                }

                // spawn non-hitscan projectile
                if (0) {
                    gs.entity_pool.spawn() @=> Entity bullet;
                    Entity.Type_Bullet => bullet.type;

                    // init bullet lifetime
                    3 => bullet.lifetime;

                    // init bullet model
                    bullet.mesh.mesh(cube_geo, phong_mat);

                    // bullet xform
                    bullet.mesh --> GG.scene();
                    .1 => float s;
                    s => bullet.mesh.sca;
                    bullet.mesh.rot(player_camera.rot());

                    // bullet physics
                    .5 * s * @(1, 1, 1) => bullet.aabb;
                    0 => bullet.gravity => bullet.friction;
                    true => bullet.check_against_enemies;
                    c.bullet_speed.val() * forward => bullet.vel;
                    pos + .5 * forward => bullet.pos;
                }
            }

            // debug visuals
            // pos + 10*forward => vec3 target;
            // l3d.line(pos - player_camera.right(), target);
            // l3d.line(pos + player_camera.right(), target);
        }

        else if (type == Type_Bullet) {
            updatePhysics(dt);
            pos => mesh.pos;
        }

        else if (type == Type_Particle) {
            updatePhysics(dt);
            // random spin
            mesh.rotateY(20*dt * vel.y);
            mesh.rotateX(20*dt * vel.x);
            mesh.rotateZ(20*dt * vel.z);
            pos => mesh.pos;
        }

        else if (type == Type_Enemy) {
            // just move towards player
            M.dir(pos, gs.player.pos) => vec3 dir;
            0 => dir.y; // cannot go upwards
            dir * c.enemy_speed.val() => vel;

            updatePhysics(dt);

            
            // hit color
            Math.max(0, enemy_hit_cd - dt) => enemy_hit_cd;
            M.lerp(enemy_hit_cd * enemy_hit_cd, Color.WHITE, 5*Color.RED) => vec3 color;
            color => phong_mat.color;

            pos => mesh.pos;
        }
        else if (type == Type_Map) {
        }
        else {
            T.err("update not impl for entity type " + type);
        }
    }
}

class EntityPool {
    Entity@ items[0];
    int len;
    
    fun Entity spawn() {
        if (len == items.size()) items << new Entity;
        items[len++] @=> Entity e;
        gs.gametime => e.spawn_time;
        return e;
    }

    fun void prune() {
        // prune dead stuff
        for (len - 1 => int i; i >= 0; i--) {
            items[i] @=> Entity@ e;
            if (e.dead) {
                e.zero();

                // swap with end
                items[len - 1] @=> items[i];
                e @=> items[len - 1];
                --len;
            }
        }
    }
}

fun void spawnEnemy(vec3 pos) {
    gs.entity_pool.spawn() @=> Entity@ e;
    e.init(Entity.Type_Enemy);
    // ignore y for now
    pos.x => e.pos.x;
    pos.z => e.pos.z;
}

fun void spawnParticles(vec3 pos, int amount, float sca, float spd) {
    spd * c.particle_speed.val() => float speed;
    .5 => float lifetime;

    for (int i; i < amount; i++) {
        gs.entity_pool.spawn() @=> Entity@ p;

        Math.random2f(.5, 2) * lifetime => p.lifetime;

        // init particle model
        p.mesh.mesh(Entity.cube_geo, p.phong_mat);

        // particle xform
        p.mesh --> GG.scene();
        Math.random2f(.6, 1.4) * sca => float s;
        s => p.mesh.sca;

        // give the particle a random rotation
        Math.two_pi * @(
            Math.random2f(-1, 1),
            Math.random2f(-1, 1),
            Math.random2f(-1, 1)
        ) => p.mesh.rot;


        // physics
        c.particle_gravity.val() => p.gravity;
        c.particle_friction.val() => p.friction; // TODO try different friction values
        pos => p.pos;
        .5 * s * p.mesh.sca() => p.aabb;

        @(
            Math.random2f(-1, 1) * speed,
            Math.random2f(.2, .8) * speed, // improvement: make blast radius normal to plane of contact
            Math.random2f(-1, 1) * speed
        ) => p.vel;

        // init and gruck hitbox visualizer
        p.init(Entity.Type_Particle);
    }
}



// init
gs.player.init(Entity.Type_Player);
gs.ground.init(Entity.Type_Map);
spawnEnemy(@(0,0,-4));
spawnEnemy(@(2,0,-4));

CD enemy_spawn_cd(2.0);

while (1) {
    GG.nextFrame() => now;
    Math.min(.033, GG.dt()) => float dt;
    dt +=> gs.gametime;


// == ui ==================================
    if (GWindow.keyDown(GWindow.KEY_GRAVEACCENT)) {
        !debug_console => debug_console;
        if (debug_console) GWindow.mouseMode(GWindow.MOUSE_NORMAL);
        else GWindow.mouseMode(GWindow.MOUSE_DISABLED);
    }

    if (debug_console) {

        UI.separatorText("player params");
        UI.slider("speed", c.player_speed, 0, 100);
        UI.slider("friction", c.player_friction, 0, 100);
        if (UI.slider("player gravity", c.player_gravity, 0, 10)) c.player_gravity.val() => gs.player.gravity;
        UI.slider("player jump vel", c.player_jump_vel, 0, 100);
        UI.slider("player air speed", c.player_air_vel_ratio, 0, 1);
        UI.slider("mouse sens", mouse_sens, 0, .01);
        UI.checkbox("avg mouse", use_mouse_avg);

        UI.separatorText("particle params");
        UI.slider("particle speed", c.particle_speed, 0, 100);
        UI.slider("particle friction", c.particle_friction, 0, 100);
        UI.slider("particle gravity", c.particle_gravity, 0, 100);


        UI.separatorText("weapon params");
        if (UI.slider("weapon_z_off", c.weapon_z_off, -1, 1)) c.weapon_z_off.val() => tmp_container.posZ;
        UI.slider("weapon_y_off", c.weapon_y_off, -1, 1);
        UI.slider("weapon_bob_mag", c.weapon_bob_mag, 0, 1);
        UI.slider("weapon_bob_freq", c.weapon_bob_freq, 0, .01);


        UI.text("Entities: " + gs.entity_pool.len);
    }

// == input ===============================
    if (!debug_console) {
        GWindow.mouseDeltaPos() => gs.mouse_delta;
        // on first frame its not 0, zero out to avoid large jump
        if (!first_mouse_delta) {
            (gs.mouse_delta.magnitude() > 0) => first_mouse_delta;
            if (first_mouse_delta) @(0, 0) => gs.mouse_delta;
        }

        // store mouse deltas history
        gs.mouse_delta => gs.mouse_deltas[gs.mouse_deltas_idx++];
        if (gs.mouse_deltas_idx >= gs.mouse_deltas.size()) 0 => gs.mouse_deltas_idx;

        // compute mouse deltas avg
        @(0,0) => gs.mouse_deltas_avg;
        for (auto v : gs.mouse_deltas) v +=> gs.mouse_deltas_avg;
        (1.0 / gs.mouse_deltas.size()) *=> gs.mouse_deltas_avg;

        // TODO add key remapping
        GWindow.key(GWindow.KEY_W) => gs.key_u;
        GWindow.key(GWindow.KEY_S) => gs.key_d;
        GWindow.key(GWindow.KEY_D) => gs.key_r;
        GWindow.key(GWindow.KEY_A) => gs.key_l;
        GWindow.key(GWindow.KEY_SPACE) => gs.key_jump;
    }

// == prep GS ==================================
    gs.entity_pool.prune();
    gs.enemy_entities.clear();
    for (int i; i < gs.entity_pool.len; i++) {
        gs.entity_pool.items[i] @=> Entity e;
        T.assert(!e.dead, "dead entity should have been pruned");

        if (e.type == Entity.Type_Enemy) gs.enemy_entities << e;
    }


// == update ==================================
    // update player
    gs.player.update(dt);

    { // update spawner
        if (enemy_spawn_cd.update(dt)) spawnEnemy(@(0, 0, 0));
    }

    // update entities (currently only bullets)
    for (auto e : gs.entity_pool.items) {
        e.update(dt);
    }

    { // chuckbox animation
        1.5 + .5 * Math.sin(2 * gs.gametime) => chuckbox.posY;
        1 + .2 * Math.cos(1.7 * gs.gametime) => chuckbox.sca;
        chuckbox.rotateX(dt);
        chuckbox.rotateY(1.2 * dt);
        chuckbox.rotateZ(.7 * dt);

        .8 * BezierColor.getPalette(.1 * gs.gametime, @(0.2, 0.5, .7), @(.9, 0.4, 0.1), @(1., 1.2, .5), @(1., -0.4, -.0)) => chuckbox.emission;
    }

    // trail renderers
    <<< "outside", gs.trails.size() >>>;
    for (gs.trails.size() - 2 => int i; i >= 0; 2 -=> i) {
        <<< "inside", i, gs.trails.size() >>>;
        dt -=> gs.trails[i].w;
        l3d.line(gs.trails[i] $ vec3, gs.trails[i+1] $ vec3, gs.trails[i].w * Color.WHITE);

        if (gs.trails[i].w < 0) {
            gs.trails.erase(i, i+2);
        }
    }

    // update widget drawers
    l3d.update();
}