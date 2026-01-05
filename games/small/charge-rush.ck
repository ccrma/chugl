@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "../bytepath/topograph.ck"
@import "sound.ck"

// Charge Rush
// https://junongx.github.io/crips-game-lib-collection/?chargerushre
// https://abagames.sakura.ne.jp/html5/cr/

/*
IDEAS
- display difficulty as current level
    - change enemy and ebullet color based on level + speed? 
- play vlambeer airplane game
- lives
    - player has N lives, regens life after M kills
- how does this compare to jblows space invaders? 
    - whats more fun
    - what has been minified/concentrated?
    - what is missing?
NEXT UP
- the mechanic of "marking" a group of enemies and then detonating all at once is super satisfying
    - remniscent of demoman sticky grenades from TF2
        - maybe combine with a vlambeer-style side-scroll schmup?
    - maybe can make a game around this mechanic, only the detonation is voluntary by the player
    and not decided by the music
*/

// text
GText.defaultFont("chugl:proggy-tiny");
"charge rush" => string title;
"[mouse] move" => string description;

// sound
CKFXR sfx => Gain sfx_gain(.2) => dac;
Topograph topograph; 
true => topograph.mode_henri;

120.0 => float BPM;
(60.0 / BPM)::second => dur qt_note;
qt_note / 8.0 => dur step; // 1 drum machine step = 32nd note

NRev rev => dac; .01 => rev.mix;
SndBuf kick => rev;
SndBuf snare => rev;
SndBuf hat => rev;
me.dir() + "../../assets/samples/punchy-kick.wav" => kick.read;
me.dir() + "../../assets/samples/snare.wav" => snare.read;
me.dir() + "../../assets/samples/trap-hihat.wav" => hat.read;
0 => kick.rate => snare.rate => hat.rate;
[kick, snare, hat] @=> SndBuf drum_rack[];

int kick_triggered;
int snare_triggered;
int hat_triggered;
int drum_machine_steps;
true => int mute_bgm;

fun void bgm() {
    int count;
    @(topograph.x, topograph.y) => vec2 curr;
    curr => vec2 target;
    64 => int steps_to_change;
    while (1) {
        drum_machine_steps++;
        if (mute_bgm) {
            step => now;
            continue;
        }

        topograph.step() @=> int velocities[];

        // play instruments
        (velocities[topograph.Inst_Kick] > 0) => kick_triggered;
        (velocities[topograph.Inst_Snare] > 0) => snare_triggered;
        (velocities[topograph.Inst_Hat] > 0) => hat_triggered;

        for (int i; i < velocities.size(); i++) {
            if (velocities[i] > 0) {
                0 => drum_rack[i].pos;
                1.0 => drum_rack[i].rate;
                velocities[i] / 255.0 => drum_rack[i].gain;
                // <<< drum_rack[i].gain() >>>;
            }
        }
        step => now;
        // lerp towards target
        1.0 * count / steps_to_change => float t;
        curr + t * (target - curr) => topograph.pos;
        if (count++ == steps_to_change) {
            // random teleport
            // topograph.pos(Math.random2f(0, 1), Math.random2f(0, 1));

            // browning motion
            // topograph drum map is 5x5, so 1 step is @(.2, .2)
            .2 * @(Math.random2f(-1, 1), Math.random2f(-1, 1)) +=> target;
            if (target.x < 0) -1 *=> target.x;
            if (target.y < 0) -1 *=> target.y;
            <<< "topology target", 4 * target, "curr", topograph.x, topograph.y >>>;
            0 => count;
        }
    }
} spork ~ bgm();

// appearance
.67 => float aspect;
GWindow.sizeLimits(0, 0, 0, 0, @(aspect, 1));
GWindow.center();
GWindow.mouseMode(GWindow.MouseMode_Disabled);

G2D g;

// utility (TODO maybe move this into g2d)
-.5 * GG.camera().viewSize() => float screen_min_y;
.5 * GG.camera().viewSize()  => float screen_max_y;
4.5 => float MARGIN;

class Enemy {
    vec2 pos;
    int hit;
    float speed;

    fun Enemy(float x, float y, float spd) {
        @(x, y) => this.pos;
        spd => this.speed;
    }
}

// gamestate
float gametime;
    // score
int highscore;
int score;
    // room
0 => int Room_Start;
1 => int Room_Play;
Room_Start => int room;

    // player 
vec2 player_pos;
UI_Float player_fire_rate(1.0 / 12);
float player_fire_cd;
UI_Float player_bullet_speed(18.0);
UI_Float player_bullet_radius(0.16);
vec2 player_bullets[0];
int player_fire_left;
UI_Float player_size(0.4);
    // enemy
Enemy enemies[0];
UI_Float enemy_spacing_y(1.0);
UI_Float enemy_fire_period(1.5);
UI_Float enemy_bullet_speed(4.0);
UI_Float enemy_bullet_radius(.2);
float enemy_bullet_cd;
float enemy_speed;
vec2 enemy_bullet_pos[0];
vec2 enemy_bullet_dir[0];

fun void init() {
    0 => gametime;
    0 => score;
    player_bullets.clear();
    enemies.clear();
    enemy_bullet_pos.clear();
    enemy_bullet_dir.clear();
    @(0, -4) => player_pos;
}

@(0, -1) => vec2 title_pos;
fun void start() {
    // update
    now/second => float tsec;
    if (g.anyInputDown()) {
        init();
        g.screenFlash(qt_note);
        Room_Play => room;
    }

    // draw
    -.5 * M.dir(title_pos, g.mousePos()) + @(0, -1) => vec2 target_pos;
    .05 * (target_pos - title_pos) +=> title_pos;
    g.text(title, @(0, 1));
    g.text(description, title_pos);
}


"
  ww  
  ww  
ccwwcc
ccwwcc
ccwwcc
cc  cc
" => string character;


"
r     r
r     r
rr o rr
rrrorrr
r  o  r
r     r
r     r
" => string enemy_sprite;

fun void die(vec2 pos) {
    g.explode(pos, 5, 3::second, Color.WHITE, 0, Math.two_pi, ExplodeEffect.Shape_Squares);
    // g.screenFlash(qt_note);
    sfx.explosion(1::second);
    Math.max(score, highscore) => highscore;
    Room_Start => room;
}

fun void enemyDie(int idx) {
    true => enemies[idx].hit;
    // g.explode(enemies[idx].pos, 2, 1::second, Color.RED, 0, Math.two_pi, ExplodeEffect.Shape_Squares);
    // enemies.erase(idx);
}


// build starfield
vec3 stars[0]; // xy pos, z radius
fun vec3 randomStar() {
    return screen_max_y * @(
        Math.random2f(-1, 1) * aspect,
        Math.random2f(-1, 1),
        .002 + .006 * Math.randomf()  
    );
}
repeat(70) { stars << randomStar(); }

// gameloop
init();
while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
    if (room == Room_Play) dt +=> gametime;

    // only play music in play mode
    (room == Room_Start) => mute_bgm; 

    // difficulty scaling
    // difficulty incr every 30 seconds instead of crisp-game-lib's typical 60
    (1 + (gametime / 30)) $ int => int difficulty; 

    topograph.density(2, .85 + .05 * difficulty); // hat density
    topograph.density(1, .15 + .05 * difficulty); // snare density
    topograph.density(0, .05 + .05 * difficulty);                     // kick density

    // ui
    // UI.text("player bullets: " + player_bullets.size());
    // UI.text("#enemies: " + enemies.size());
    // UI.text("gametime: " + gametime);
    // UI.text("difficulty: " + difficulty);
    // UI.text("kick density: " + topograph._density[0]);
    // UI.text("sna density: " + topograph._density[1]);
    // UI.text("hat density: " + topograph._density[2]);
    // for (auto e : enemies) UI.text("    pos: " + (e.pos.x) + (",") + e.pos.y + " hit: " + e.hit);
    

    { // score
        g.pushTextControlPoint(@(1, 1));
        g.text("HI " + highscore, @(MARGIN * aspect, MARGIN), .8);
        g.popTextControlPoint();

        g.pushTextControlPoint(@(0, 1));
        g.text(score + "", @(-MARGIN * aspect, MARGIN), .8);
        g.popTextControlPoint();
    }

    { // update and draw stars
        g.pushLayer(-1);
        for (int i; i < stars.size(); i++) {
            20 * difficulty * dt * stars[i].z -=> stars[i].y;
            if (stars[i].y < screen_min_y - stars[i].z) {
                randomStar() + @(0, 10, 0) => stars[i];
            }
            // draw
            g.circleFilled(stars[i] $ vec2, stars[i].z, 
                Color.WHITE * .75
            );
        }
        g.popLayer();
    }

    if (room == Room_Start) start();

    if (room == Room_Play) {
        // TODO: switch to mouseDelta so player doesn't get stuck
        g.clampScreen(player_pos + g.mouseDeltaWorld()) => player_pos;
        g.char(character, player_pos, player_size.val(), .8, Color.BLACK);
        // bbox
        // g.square(player_pos - @(0, .075), 0, .75 * player_size.val(), Color.WHITE);

        // fire bullets (Doesn't support >1 per frame)
        dt -=> player_fire_cd;
        // if (player_fire_cd < 0) {
        if (hat_triggered) {
            player_fire_rate.val() => player_fire_cd;
            (player_size.val() / 3) * (player_fire_left * 2 - 1) => float offset;

            player_pos + @(-offset, 0) => vec2 bullet_pos;
            player_bullets << bullet_pos;

            !player_fire_left => player_fire_left;

            // muzzle flash
            bullet_pos + @(0, .08) => vec2 muzzle_pos;
            g.pushLayer(1);
            g.circleFilled(muzzle_pos, 1 * player_bullet_radius.val(), Color.WHITE);
            g.popLayer();
        }
    }

    // spawn enemies
    int num_unhit_enemies;
    for (auto e : enemies) if (!e.hit) num_unhit_enemies++;

    // if (enemies.size() == 0) {
    if (num_unhit_enemies == 0) {
        Math.random2f(2,3) + .5 * difficulty => enemy_speed;
        enemy_fire_period.val() => enemy_bullet_cd;

        Math.random2(5, 8) + difficulty => int n;
        for (int i; i < n; i++) {
            screen_max_y + i*enemy_spacing_y.val() => float y;
            .8 * aspect * Math.random2f(-1, 1) * screen_max_y => float x;
            enemies << new Enemy(x, y, enemy_speed);
        }
    }

    // update and draw bullets
    for (player_bullets.size() - 1 => int i; i >= 0; i--) {
        dt * player_bullet_speed.val() * @(0, 1) +=> player_bullets[i];
        player_bullets[i] + .25 * player_bullet_radius.val() * @(0,1) => vec2 bullet_center;
        // collision with enemies
        int collided;
        for (int j; j < enemies.size(); j++) {
            enemies[j].pos => vec2 enemy_pos;
            if (
                M.aabbIsect(
                    bullet_center, @(player_bullet_radius.val(), player_bullet_radius.val()),
                    enemy_pos, player_bullet_radius.val() * @(.5,.75)
                )
            ) {
                true => collided;
                enemyDie(j);
                break;
            }
        }

        // remove if offscreen
        if (g.offscreen(player_bullets[i], 1.1) || collided) {
            player_bullets.erase(i);
            continue;
        }

        // draw bullet
        g.capsuleFilled(
            player_bullets[i], 
            player_bullets[i] + .5 * player_bullet_radius.val() * @(0,1),
            .5 * player_bullet_radius.val(),
            Color.YELLOW
        );
        // bbox
        // g.box(bullet_center, player_bullet_radius.val(), 1.5*player_bullet_radius.val());
    }

    // update enemies
    if (room == Room_Play) dt -=> enemy_bullet_cd;
    int fire;
    if (enemy_bullet_cd < 0) {
        true => fire;
        enemy_fire_period.val() => enemy_bullet_cd;
    }
    for (enemies.size() - 1 => int i; i >= 0; i--) {
        if (enemies[i].pos.y < screen_min_y - player_size.val()) {
            // enemies.erase(i);
            enemyDie(i);
            continue;
        }

        if (!enemies[i].hit) 
            dt * enemy_speed * @(0, -1) +=> enemies[i].pos;

        // draw enemy
        Color.BLACK => vec3 e_color;
        if (enemies[i].hit && drum_machine_steps % 2) Color.YELLOW => e_color;
        g.char(
            enemy_sprite, enemies[i].pos, player_size.val() * 1.16, .75, e_color
        );

        // bbox
        // g.square(enemies[i], 0, player_size.val(), Color.WHITE);

        // fire enemy bullet
        // if (fire) {
        if (snare_triggered && room == Room_Play && !enemies[i].hit) {
            enemy_bullet_pos << enemies[i].pos;
            enemy_bullet_dir << M.dir(enemies[i].pos, player_pos);
        }

        // enemy-player collision
        if (room == Room_Play && 
            M.aabbIsect(
                player_pos - @(0,.075), .75 * player_size.val() * @(.5, .5), // maintani copy with enemy_bullet-player collision
                enemies[i].pos, player_bullet_radius.val() * @(.5,.75)
            )
        ) {
            enemyDie(i);
            die(player_pos);
            break;
        }
    }

    // update and draw bullets
    for (enemy_bullet_pos.size() - 1 => int i; i >= 0; i--) {
        // update pos
        dt * (enemy_bullet_speed.val() + .5 * difficulty) * enemy_bullet_dir[i] +=> enemy_bullet_pos[i];

        if (g.offscreen(enemy_bullet_pos[i], 1.1)) {
            enemy_bullet_pos.erase(i);
            enemy_bullet_dir.erase(i);
            continue;
        }

        // enemy_bullet-player collision
        if (
            room == Room_Play &&
            M.aabbIsect(
                player_pos - @(0,.075), .75 * player_size.val() * @(.5, .5),
                enemy_bullet_pos[i], enemy_bullet_radius.val() * @(.5, .5)
            )
        ) {
            die(player_pos);
            break;
        }

        // draw
        // g.square(enemy_bullet_pos[i], 0, enemy_bullet_radius.val(),  Color.WHITE);
        M.angle(enemy_bullet_dir[i]) => float forward;
        g.chevron(enemy_bullet_pos[i] - enemy_bullet_dir[i]*.15, forward, Math.pi/2, enemy_bullet_radius.val(), enemy_bullet_radius.val(), Color.RED);
    }

    // cleanup
    if (kick_triggered || room == Room_Start) {
        int num_onscreen;
        for (enemies.size() - 1 => int i; i >= 0; i--) {
            if (enemies[i].hit) {
                <<< "removing enemy", i >>>;
                enemies[i].pos.y > screen_min_y - player_size.val() => int onscreen;
                if (onscreen){
                    g.explode(enemies[i].pos, 2, 1::second, Color.RED, 0, Math.two_pi, ExplodeEffect.Shape_Squares);
                    // add score based on enemy speed
                    if (Room_Play == room) {
                        ((difficulty + num_onscreen) * enemies[i].speed) $ int => int points;
                        // (enemies[i].speed * 10) $ int => int points;
                        g.score("+" + points, enemies[i].pos, .5::second, .5,  0.6);
                        points +=> score;
                        num_onscreen++;
                    }
                }
                enemies.erase(i);
            }
        }
    }

    false => kick_triggered;
    false => snare_triggered;
    false => hat_triggered;
}
