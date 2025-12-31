@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "../lib/tween.ck"
@import "../lib/b2/b2DebugDraw.ck"
@import "fx.ck"


0 => int EntityType_None;
1 => int EntityType_Rock;
2 => int EntityType_Arrow;

class Entity {
    vec2 pos;

    // obstacle
    int type;
}

class Pool {
    Entity objects[0]; // remember to traverse in reverse order
    int count;

    // override these
    fun Entity spawn() { return new Entity; }
    fun void zero(Entity e) {
        @(0, 0) => e.pos;
        0 => e.type;
    }

    fun Entity get(int type) {
        if (count >= objects.size()) objects << spawn();
        objects[count++] @=> Entity e;
        type => e.type;
        return e;
    }

    fun void retire(Entity o, int idx) {
        if (objects[idx] != o) {
            <<< "cannot return object at incorrect idx", idx >>>;
            return;
        }

        // swap with last
        objects[count-1] @=> objects[idx];
        o @=> objects[count-1]; 
        count--;
        <<< "reetire count", count >>>;
        zero(o);
    }
}

Pool obstacles;
UI_Float obstacle_speed(6);

Entity player;
int collided;
UI_Float player_speed(12);
UI_Float player_amp(3.8);
UI_Float player_freq(.35);
UI_Bool mouse_controls(false);
    UI_Float player_zeno(.38); // for mouse controls

// size params
UI_Float player_size(0.5);
UI_Float obstacle_size(0.5);


// init
TextureLoadDesc tex_load_desc;
true => tex_load_desc.flip_y;
false => tex_load_desc.gen_mips;
Texture.load(me.dir() + "./earth.png", tex_load_desc) @=> Texture earth_sprite;
Texture.load(me.dir() + "./earth_clouds.png", tex_load_desc) @=> Texture earth_cloud_sprite;
Texture.load(me.dir() + "./arrow.png", tex_load_desc) @=> Texture arrow_sprite;


3 => int aspect;
600 => int resolution;
GWindow.sizeLimits(0, 0, 0, 0, @(aspect, 1));
GWindow.center();
G2D g;
g.resolution(resolution, (resolution/ aspect));
g.antialias(false);
g @=> FX.g;

fun void ui() {
    UI.checkbox("mouse controls", mouse_controls);
    if (!mouse_controls.val()) {
        UI.slider("speed", player_speed,0,10);
        UI.slider("amp", player_amp,0,10);
        UI.slider("freq", player_freq,0,10);
    }

    UI.separatorText("obstacles");
    UI.pushID("obstacles");
    UI.slider("speed", obstacle_speed,0,10);

    UI.popID(); // obstacles
}

// gameloop
while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    ui();

    { // input
        vec2 delta;
        if (mouse_controls.val()) {
            player_zeno.val() * (g.mousePos() - player.pos).x => delta.x;
        } else {
            if (GWindow.key(GWindow.KEY_LEFT) || GWindow.key(GWindow.KEY_A)) {
                player_speed.val() * dt -=> delta.x;
            }
            if (GWindow.key(GWindow.KEY_RIGHT) || GWindow.key(GWindow.KEY_D)) {
                player_speed.val() * dt +=> delta.x;
            }
        }

        if (g.mouseLeftDown() || GWindow.keyDown(GWindow.KEY_1)) {
            // spawn a thing
            obstacles.get(EntityType_Rock) @=> Entity e;
            g.mousePos() => e.pos;
        }

        delta +=> player.pos;
    }

    { // update
        false => collided;
        player_amp.val() * Math.sin(now/second * player_freq.val() * Math.two_pi) => player.pos.y;

        // update obstacles
        for (obstacles.count - 1 => int i; i >= 0; i--) {
            obstacles.objects[i] @=> Entity e;
            dt * obstacle_speed.val() -=> e.pos.x;
            // remove if off screen
            if (g.offscreen(e.pos, 1.2)) {
                obstacles.retire(e, i);
                <<< "returning" >>>;
                continue;
            }

            if (e.type == EntityType_Rock) {
                // check collision
                if (M.circlesOverlap(player.pos, player_size.val(), e.pos, obstacle_size.val())) {
                    true => collided;

                    // collide asteroid
                } 
            }
            else {
                T.err("in update: unrecognized entity type: " + e.type);
            }
        }
    }

    { // draw
        // draw player
        if (collided) g.pushColor(Color.RED);
        else g.pushColor(Color.WHITE);
        g.circle(player.pos, player_size.val());
        g.popColor();

        // draw obstacles
        for (obstacles.count - 1 => int i; i >= 0; i--) {
            obstacles.objects[i] @=> Entity e;
            // g.square(e.pos, 0, obstacle_size.val(), Color.RED);
            if (e.type == EntityType_Rock) {
                g.circle(e.pos, obstacle_size.val(), Color.RED);
            } else {
                T.err("unrecognized entity type: " + e.type);
            }
        }

        // draw earth
        g.pushLayer(-1);
        g.sprite(earth_sprite, @(0, -5), 4.0, (now/second * .1), Color.WHITE);  
        g.popLayer();
    }
}
