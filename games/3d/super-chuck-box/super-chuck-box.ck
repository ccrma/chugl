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


Enemy types
- bouncer (like DVD logo)
- stationary turret guy
- look at following games for inspo:
    - devil daggers
    - scb
    - tboi
    - mewgenics
    - halo
    - vampire surivvors
    - brotato
        -   and other survival bullet heaven


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
*/

@import "../../lib/g2d/ChuGL-debug.chug"
@import "../../lib/M.ck"

// == load assets ========================================
Texture.load(me.dir() + "./assets/floor.png") @=> Texture floor_tex;

// GG.scene().camera(GOrbitCamera orbit_cam);
// orbit_cam.posY(5);

GWindow.mouseMode(GWindow.MOUSE_DISABLED);
int debug_console;

GPlane ground --> GG.scene();
ground.rotX(M.PI/2);
ground.colorMap(floor_tex);
ground.sca(100);
ground.uvScale(@(100, 100));
ground.specular(Color.BLACK);
// ground.scale(10);

GCube cube --> GG.scene();
cube.posY(.5);
cube.posZ(-2);
cube.colorMap(floor_tex);
cube.color(5 * Color.YELLOW);

GMesh crosshair(new CircleGeometry, FlatMaterial crosshair_mat) --> GG.camera();
crosshair.sca(.001);
crosshair.posZ(-.11);

// player params
@(0, 1, 3) => vec3 player_pos;
vec3 velocity;
UI_Float speed(125);
UI_Float friction(10);

UI_Float mouse_sens(.001);
UI_Bool use_mouse_avg(true);
int first_mouse_delta;

vec2 mouse_deltas[2];
int mouse_deltas_idx;
vec2 mouse_deltas_avg;


while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

// == input ===============================
    GWindow.mousePos() => vec2 mouse_pos;
    GWindow.windowSize() => vec2 window_size;
    2 * @(M.clamp01(mouse_pos.x / window_size.x), -M.clamp01(mouse_pos.y / window_size.y)) - @(1, -1) => vec2 mouse_pos_norm; // [-1, 1]

    GWindow.mouseDeltaPos() => vec2 mouse_delta;
    // on first frame its not 0, zero out to avoid large jump
    if (!first_mouse_delta) {
        (M.mag(mouse_delta) > 0) => first_mouse_delta;
        if (first_mouse_delta) @(0, 0) => mouse_delta;
    }

    // store mouse deltas history
    mouse_delta => mouse_deltas[mouse_deltas_idx++];
    if (mouse_deltas_idx >= mouse_deltas.size()) 0 => mouse_deltas_idx;

    // compute mouse deltas avg
    @(0,0) => mouse_deltas_avg;
    for (auto v : mouse_deltas) v +=> mouse_deltas_avg;
    (1.0 / mouse_deltas.size()) *=> mouse_deltas_avg;

    // TODO add key remapping
    GWindow.key(GWindow.KEY_W) => int key_u;
    GWindow.key(GWindow.KEY_S) => int key_d;
    GWindow.key(GWindow.KEY_D) => int key_r;
    GWindow.key(GWindow.KEY_A) => int key_l;


// == ui ==================================
    if (GWindow.keyDown(GWindow.KEY_GRAVEACCENT)) {
        !debug_console => debug_console;
        if (debug_console) GWindow.mouseMode(GWindow.MOUSE_NORMAL);
        else GWindow.mouseMode(GWindow.MOUSE_DISABLED);
    }

    if (debug_console) {
        UI.slider("speed", speed, 0, 100);
        UI.slider("friction", friction, 0, 100);
        UI.slider("mouse sens", mouse_sens, 0, .01);
        UI.checkbox("avg mouse", use_mouse_avg);
    }

// == update ==================================

    if (!debug_console) { // player controls

        if (use_mouse_avg.val()) {
            GG.camera().rotateOnLocalAxis(@(1, 0, 0), -mouse_deltas_avg.y * mouse_sens.val()); 
            GG.camera().rotateOnWorldAxis(@(0, 1, 0), -mouse_deltas_avg.x * mouse_sens.val()); 
        } else {
            GG.camera().rotateOnLocalAxis(@(1, 0, 0), -mouse_delta.y * mouse_sens.val()); 
            GG.camera().rotateOnWorldAxis(@(0, 1, 0), -mouse_delta.x * mouse_sens.val()); 
        }

        GG.camera().forward() => vec3 forward; 0 => forward.y; forward.normalize();
        GG.camera().right() => vec3 right; 0 => right.y; right.normalize();

        // movement
        true => int on_ground;
        speed.val() * (on_ground ? 1.0 : 0.3) * (
            (key_r - key_l) * right + (key_u - key_d) * forward
		) => vec3 acceleration;

		// Integrate acceleration & friction into velocity
		Math.min(friction.val() * dt, 1) => float friction_force;
		(
            acceleration * dt -
			friction_force * @(velocity.x, 0, velocity.z) // TODO this is weird, why scale friction force by velocity?
        ) => vec3 delta_v; 
        delta_v +=> velocity;

        velocity * dt +=> player_pos;

        <<< mouse_delta, mouse_deltas_avg >>>;

		// Divide the physics integration into 16 unit steps; otherwise fast
		// projectiles may just move through walls.
		// let 
		// 	original_step_height = this._step_height,
		// 	move_dist = vec3_mulf(this.v, game_tick),
		// 	steps = Math.ceil(vec3_length(move_dist) / 16),
		// 	move_step = vec3_mulf(move_dist, 1/steps);

        GG.camera().pos(player_pos);
    }




}