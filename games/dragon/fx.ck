@import "../lib/g2d/g2d.ck"

public class FX {
    static G2D@ g;
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
    fun static void lightning(vec2 start, vec2 end, int n_segments, float rand_displacement_max, float fade_secs) {
        vec2 vertices[0];
        vertices << start;

        (end - start) / n_segments => vec2 delta;
        M.perp(M.dir(start, end)) => vec2 perp;

        for (int i; i < n_segments - 1; i++) {
            start + (i + Math.random2f(0, 1)) * delta => vec2 v;
            // displace along normal
            v + Math.random2f(-rand_displacement_max, rand_displacement_max) * perp => v;
            vertices << v << v;
        }

        // final endpoint
        vertices << end;    

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

    fun static void screenFlashEffect(dur duration) {
        float elapsed;
        duration/second => float total_time;
        while (elapsed < total_time) {
            GG.nextFrame() => now;
            GG.dt() +=> elapsed;
            elapsed / total_time => float t;

            g.pushBlend(g.BLEND_ADD);
            g.pushLayer(GG.camera().posZ() - 1);
            g.squareFilled(
                GG.camera().pos() $ vec2,
                0, 
	            GG.camera().viewSize() * 1,
                Color.WHITE * M.expImpulse(t, 9)
                // Color.WHITE
            );
            // g.popLayer();
            g.popBlend();
        }
    }

    static int camera_shake_generation;
    fun static void cameraShakeEffect(float amplitude, dur shake_dur, float hz) {
        ++camera_shake_generation => int gen;
        dur elapsed_time;

        // generate shake params
        shake_dur / 1::second => float shake_dur_secs;
        vec2 camera_deltas[(hz * shake_dur_secs + 1) $ int];
        for (int i; i < camera_deltas.size(); i++) {
            @(Math.random2f(-amplitude, amplitude),
            Math.random2f(-amplitude, amplitude)) => camera_deltas[i];
        }
        @(0,0) => camera_deltas[0]; // start from original pos
        @(0,0) => camera_deltas[-2]; // return to original pos
        @(0,0) => camera_deltas[-1]; // return to original pos (yes need this one too)

        (1.0 / hz)::second => dur camera_delta_period; // time for 1 cycle of shake

        while (true) {
            GG.nextFrame() => now;
            // another shake triggred, stop this one
            if (elapsed_time > shake_dur || gen != camera_shake_generation) break;
            // update elapsed time
            GG.dt()::second +=> elapsed_time;

            // compute fraction shake progress
            elapsed_time / shake_dur => float progress;
            elapsed_time / camera_delta_period => float elapsed_periods;
            elapsed_periods $ int => int floor;
            elapsed_periods - floor => float fract;

            // clamp to end of camera_deltas
            if (floor + 1 >= camera_deltas.size()) {
                camera_deltas.size() - 2 => floor;
                1.0 => fract;
            }

            // interpolate the progress
            camera_deltas[floor] * (1.0 - fract) + camera_deltas[floor + 1] * fract => vec2 delta;
            // update camera pos with linear decay based on progress
            (1.0 - progress) * delta => GG.camera().pos;
        }
    }
}