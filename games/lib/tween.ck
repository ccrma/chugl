@import "HashMap.chug"

// by default, all tweens are unique to the UI_Float object ptr
public class Tween {
    // @optimize: combine with tween_pool to avoid reallocations
    HashMap tween_map;

    // easing type enums
    0 => static int Tween_Lerp;
    1 => static int Tween_EaseOutQuad;
    2 => static int Tween_Toggle;

    fun TweenItem lerp(UI_Float v, float from, float to) {
        _add(v, Tween_Lerp) @=> TweenItem item;
        from => item.start_val;
        to => item.end_val;
        from => item.v.val;
        return item;
    }

    fun TweenItem easeOutQuad(UI_Float v, float from, float to) {
        _add(v, Tween_EaseOutQuad) @=> TweenItem item;
        from => item.start_val;
        to => item.end_val;
        from => item.v.val;
        return item;
    }

    fun void update(float dt) {
        // Note: we iterate backwards so we can remove items from the array without skipping a value on the 
        // next iteration. Since the order of the items doesn't matter, to remove an item we just replace it 
        // with the last item and decrement the total item count.
        tween_map.objKeys() @=> Object keys[];
        for (int i; i < keys.size(); ++i) {
            tween_map.getObj(keys[i]) $ TweenItem @=> TweenItem it;
            if (it.d == 0) <<< "warning, Tween has duration of 0, did you forget to set with .over()?" >>>;

            dt +=> it.t;
            it.t / it.d => float t; // % progress

            // remove item if it has completed
            if (t >= 1.0) {
                it.end_val => it.v.val;
                _remove(it);
                continue;
            }

            // update progress
            if (it.type == Tween_Lerp) {
                it.start_val + (it.end_val - it.start_val) * t => it.v.val;
            } else if (it.type == Tween_EaseOutQuad) {
                it.start_val + (it.end_val - it.start_val) * easeOutQuad(t) => it.v.val;
            } else if (it.type == Tween_Toggle) {

            } else <<< "unknkown tween type: ", it.type >>>;
        }
    }

// internal =============================
    fun TweenItem _add(UI_Float v, int type) {
        tween_map.getObj(v) $ TweenItem @=> TweenItem t;
        if (t == null) {
            new TweenItem @=> t;
            tween_map.set(v, t);
        }
        t.zero();

        // initialize
        type => t.type;
        v @=> t.v;

        return t;
    }

    fun void _remove(TweenItem it) {
        tween_map.del(it.v);
        null @=> it.v;
    }


    // =====================================================================
    // Tweens (most take a function that outputs from 0-1 over an input 0-1
    // =====================================================================
    fun static float easeOutQuad(float x) {
        return 1 - (1 - x) * (1 - x);
    }

    fun static float easeInOutCubic(float x) {
        if (x < 0.5) 
            return 4 * x * x * x;
        else 
            return 1 - Math.pow(-2 * x + 2, 3) / 2;
    }

    fun static float lerp(float x, float a, float b) {
        return (1.0 - x) * a + x * b;
    }

    fun static vec3 lerp(float x, vec3 a, vec3 b) {
        return (1.0 - x) * a + x * b;
    }

    fun static vec2 lerp(float x, vec2 a, vec2 b) {
        return (1.0 - x) * a + x * b;
    }
}

// t.lerp(f, 0, 1).over(5::second);

class TweenItem {
    int type;
    UI_Float v;

    float start_val;
    float end_val;

    // timing
    float   t;     // time since start
    float   d;         // how long to tween for (sec)
    float   ival;      // used by toggle (sec)

    // TweenItem@ next;
    fun void zero() {
        0 => type;
        null @=> v;
        0 => start_val;
        0 => end_val;
        0 => t;
        0 => d;
        0 => ival;
    }

    fun void over(dur d) {
        d / second => this.d;
    }

    fun void over(float t_sec) {
        t_sec => this.d;
    }
}
