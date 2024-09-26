// Visuals ====================================================================
// Visually, the entire sequencer is made up of "GPads" which are meant to read 
// as the MIDI pads you'd see on a drum machine. 
// Each pad is a GPlane + mouse listener + state machine for handling input 
// and transitioning between various modes e.g. hovered, active, playing, etc.
// ============================================================================ 
public class GPad extends GGen {
    // initialize mesh
    GPlane pad --> this;
    FlatMaterial mat;
    pad.mat(mat);

    // reference to a mouse
    Mouse @ mouse;

    // events
    Event onHoverEvent, onClickEvent;  // onExit, onRelease
    
    // states
    0 => static int NONE;  // not hovered or active
    1 => static int HOVERED;  // hovered
    2 => static int ACTIVE;   // clicked
    3 => static int PLAYING;  // makine sound!
    0 => int state; // current state

    // input types
    0 => static int MOUSE_HOVER;
    1 => static int MOUSE_EXIT;
    2 => static int MOUSE_CLICK;
    3 => static int NOTE_ON;
    4 => static int NOTE_OFF;

    // color map
    [
        Color.BLACK,    // NONE
        Color.RED,      // HOVERED
        Color.YELLOW,   // ACTIVE
        Color.GREEN     // PLAYING
    ] @=> vec3 colorMap[];

    // constructor
    fun void init(Mouse @ m) {
        if (mouse != null) return;
        m @=> this.mouse;
        spork ~ this.clickListener();
    }

    // check if state is active (i.e. should play sound)
    fun int active() {
        return state == ACTIVE;
    }

    // set color
    fun void color(vec3 c) {
        mat.color(c);
    }

    // returns true if mouse is hovering over pad
    fun int isHovered() {
        pad.scaWorld() => vec3 worldScale;  // get dimensions
        worldScale.x / 2.0 => float halfWidth;
        worldScale.y / 2.0 => float halfHeight;
        pad.posWorld() => vec3 worldPos;   // get position

        if (mouse.worldPos.x > worldPos.x - halfWidth && mouse.worldPos.x < worldPos.x + halfWidth &&
            mouse.worldPos.y > worldPos.y - halfHeight && mouse.worldPos.y < worldPos.y + halfHeight) {
            return true;
        }
        return false;
    }

    // poll for hover events
    fun void pollHover() {
        if (isHovered()) {
            onHoverEvent.broadcast();
            handleInput(MOUSE_HOVER);
        } else {
            if (state == HOVERED) handleInput(MOUSE_EXIT);
        }
    }

    // handle mouse clicks
    fun void clickListener() {
        now => time lastClick;
        while (true) {
            GG.nextFrame() => now;
            if (GWindow.mouseLeftDown() && isHovered()) {
                onClickEvent.broadcast();
                handleInput(MOUSE_CLICK);
            }
        }
    }

    // animation when playing
    // set juice = true to animate
    fun void play(int juice) {
        handleInput(NOTE_ON);
        if (juice) {
            pad.sca(1.4);
            pad.rotZ(Math.random2f(-.5, .2));
        }
    }

    // stop play animation (called by sequencer on note off)
    fun void stop() {
        handleInput(NOTE_OFF);
    }

    // activate pad, meaning it should be played when the sequencer hits it
    fun void activate() {
        enter(ACTIVE);
    }

    0 => int lastState;
    // enter state, remember last state
    fun void enter(int s) {
        state => lastState;
        s => state;
        // uncomment to randomize color when playing
        // if (state == PLAYING) Color.random() => colorMap[PLAYING];
    }

    // basic state machine for handling input
    fun void handleInput(int input) {
        if (input == NOTE_ON) {
            enter(PLAYING);
            return;
        }

        if (input == NOTE_OFF) {
            enter(lastState);
            return;
        }

        if (state == NONE) {
            if (input == MOUSE_HOVER)      enter(HOVERED);
            else if (input == MOUSE_CLICK) enter(ACTIVE);
        } else if (state == HOVERED) {
            if (input == MOUSE_EXIT)       enter(NONE);
            else if (input == MOUSE_CLICK) enter(ACTIVE);
        } else if (state == ACTIVE) {
            if (input == MOUSE_CLICK)      enter(NONE);
        } else if (state == PLAYING) {
            if (input == MOUSE_CLICK)      enter(NONE);
            if (input == NOTE_OFF)         enter(ACTIVE);
        }
    }

    // override ggen update
    fun void update(float dt) {
        // check if hovered
        pollHover();

        // update state
        this.color(colorMap[state]);

        // interpolate back towards uniform scale (handles animation)

        // this is cursed
        // pad.scaX()  - .03 * Math.pow(Math.fabs((1.0 - pad.scaX())), .3) => pad.sca;
        
        // much less cursed
        pad.scaX()  + .05 * (1.0 - pad.scaX()) => pad.sca;
        pad.rot().z  + .06 * (0.0 - pad.rot().z) => pad.rotZ;

    }
}