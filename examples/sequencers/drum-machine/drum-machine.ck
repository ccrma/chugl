// TODO add mouseclick event to chugl
Mouse mouse;
spork ~ mouse.start(0);  // start listening for mouse events
spork ~ mouse.selfUpdate();

// Scene setup ================================================================
// GG.resolution(1280, 720);
GG.scene() @=> GScene @ scene;
GG.camera() @=> GCamera @ cam;
cam.orthographic();  // 2D scene


GG.nextFrame() => now;  // bug: have to wait 1 frame for renderer to propagate correct frame dimensions


class GPad extends GGen {
    GPlane pad --> this;
    FlatMaterial mat;
    pad.mat(mat);

    Mouse @ mouse;

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
        // Color.WHITE,  // NONE
        Color.BLACK,  // NONE
        Color.RED,    // HOVERED
        Color.YELLOW,   // ACTIVE
        Color.GREEN   // PLAYING
    ] @=> vec3 colorMap[];

    // constructor
    fun void init(Mouse @ m) {
        if (mouse != null) return;
        m @=> this.mouse;
        spork ~ this.clickListener();
    }

    fun void setBaseCol(vec3 baseCol) {
        baseCol => colorMap[NONE];
    }

    fun int active() {
        return state == ACTIVE;
    }

    fun void color(vec3 c) {
        pad.mat().color(c);
    }

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

    fun void pollHover() {
        if (isHovered()) {
            onHoverEvent.broadcast();
            handleInput(MOUSE_HOVER);
        } else {
            if (state == HOVERED) handleInput(MOUSE_EXIT);
        }
    }

    fun void clickListener() {
        now => time lastClick;
        while (true) {
            mouse.mouseDownEvents[Mouse.LEFT_CLICK] => now;
            if (isHovered()) {
                handleInput(MOUSE_CLICK);
            }
            100::ms => now; // cooldown
        }
    }

    fun void play() {
        handleInput(NOTE_ON);
    }

    fun void stop() {
        handleInput(NOTE_OFF);
    }

    fun void activate() {
        enter(ACTIVE);
    }

    0 => int lastState;

    fun void enter(int s) {
        state => lastState;
        s => state;
    }

    // state machine for handling input
    fun void handleInput(int input) {
        if (input == NOTE_ON) {
            enter(PLAYING);
            return;
        }

        if (input == NOTE_OFF) {
            enter(lastState);
            return;
        }

        if (state == NONE) {  // can enter any state from NONE
            if (input == MOUSE_HOVER)      enter(HOVERED);
            else if (input == MOUSE_CLICK) enter(ACTIVE);
        } else if (state == HOVERED) {
            if (input == MOUSE_EXIT)       enter(NONE);
            else if (input == MOUSE_CLICK) enter(ACTIVE);
        } else if (state == ACTIVE) {
            if (input == MOUSE_CLICK)      enter(NONE);
            // else if (input == NOTE_ON)     enter(PLAYING);
            // else if (input == MOUSE_HOVER) state = HOVERED;
        } else if (state == PLAYING) {
            if (input == MOUSE_CLICK)      enter(NONE);
            if (input == NOTE_OFF)         enter(ACTIVE);
        }
    }

    fun void update(float dt) {
        pollHover();
        this.color(colorMap[state]);
    }
}


// Instruments ==================================================================
class Instrument extends Chugraph {
    fun void play() {}
    fun void play(int note) {}
}
class AcidBass extends Chugraph {
    SawOsc saw1, saw2;
    ADSR env;
    LPF filter;

    TriOsc freqLFO => blackhole;
    TriOsc qLFO => blackhole;
    saw1 => env => filter => Gain g => outlet;
    saw2 => env => filter;

    freqLFO.period(4::second);
    qLFO.period(5::second);
    env.set(50::ms, 10::ms, .6, 100::ms);
    filter.freq(1500);
    filter.Q(10);

    fun void modulate() {
        while (true) {
            (freqLFO.last() + 1.0) / 2.0 => float freqFactor;  // remap [-1, 1] --> [0, 1]
            (qLFO.last() + 1.0) / 2.0 => float qFactor;
            freqFactor * 1500.0 + 100 => filter.freq;
            qFactor * 7.0 + 1 => filter.Q;
            
            // TODO map env to filter Q

            10::ms => now;
        }
    } spork ~ modulate();

    fun void setGain(float g) { g => filter.gain; }

    fun void map() {

    }

    fun void play(int note) {
        Std.mtof(note) => float freq;
        saw1.freq(freq);
        saw2.freq(2 * freq * 1.01); // slight detune for more harmonic content
        env.keyOn();
        env.attackTime() + env.decayTime() => now;
        env.keyOff();
        env.releaseTime() => now;
    } 

}

class Kick extends Instrument {  // thanks Tristan
    inlet => Noise n => LPF f => ADSR e => outlet;
    110 => f.freq;
    40 => f.gain;
    e.set(5::ms, 50::ms, 0.1, 100::ms);

    fun void play() {
        e.keyOn();
        50::ms => now;
        e.keyOff();
        e.releaseTime() => now;
    }
}

class Snare extends Instrument {  // thanks Tristan
    inlet => Noise n => BPF f => ADSR e => outlet;
    440 => f.freq;
    15. => f.Q;
    15 => f.gain;
    e.set(5::ms, 50::ms, 0.1, 50::ms);

    fun void play() {
        e.keyOn();
        50::ms => now;
        e.keyOff();
        e.releaseTime() => now;
    }

}

class Hat extends Instrument {  // thanks Tristan
    inlet => Noise n => HPF f => ADSR e => outlet;
    2500 => f.freq;
    0.05 => f.gain;
    e.set(5::ms, 50::ms, 0.1, 100::ms);

    fun void play() {
        e.keyOn();
        50::ms => now;
        e.keyOff();
        e.releaseTime() => now;
    }
}

// Params ======================================================================

120 => int BPM;
(1.0/BPM)::minute / 2.0 => dur STEP;

// Sequencer ===================================================================
Gain main => JCRev rev => dac;
.1 => main.gain;
0.1 => rev.mix;

16 => int NUM_STEPS;

[
    -5, -2, 0, 3, 5, 7, 10, 12, 15
] @=> int SCALE[];

AcidBass acidBasses[SCALE.size()];
for (auto bass : acidBasses) {
    bass => main;
    bass.setGain(2.0 / acidBasses.size());
}
GPad acidBassPads[NUM_STEPS][SCALE.size()];
for (int i; i < acidBassPads.size(); i++) {
    for (int j; j < acidBassPads[i].size(); j++)
        acidBassPads[i][j].setBaseCol(Color.BLACK);
}

Kick kick => main;
Snare snare => main;
Hat openHat => main;
Hat closedHat => main;

GPad kickPads[NUM_STEPS];
GPad snarePads[NUM_STEPS];
GPad openHatPads[NUM_STEPS];
GPad closedHatPads[NUM_STEPS];

// randomize pattern
for (int i; i < NUM_STEPS; i++) {
    if (Math.randomf() < .35) kickPads[i].activate();
    if (Math.randomf() < .45) snarePads[i].activate();
    if (Math.randomf() < .4) {
        if (Math.randomf() < .5) {
            openHatPads[i].activate();
        } else {
            closedHatPads[i].activate();
        }
    }
}


GGen kickPadGroup --> GG.scene();
GGen snarePadGroup --> GG.scene();
GGen openHatPadGroup --> GG.scene();
GGen closedHatPadGroup --> GG.scene();
GGen acidBassGroups[NUM_STEPS];
for (auto group : acidBassGroups) group --> GG.scene();

spork ~ sequenceBeat(kick, kickPads, true);
spork ~ sequenceBeat(snare, snarePads, false);
spork ~ sequenceBeat(openHat, openHatPads, false);
spork ~ sequenceBeat(closedHat, closedHatPads, true);
spork ~ sequenceLead(acidBasses, acidBassPads, SCALE, 60 - 2 * 12);


fun void placePads() {
    // recalculate aspect
    (GG.frameWidth() * 1.0) / (GG.frameHeight() * 1.0) => float aspect;
    // calculate ratio between old and new height/width
    cam.viewSize() => float frustrumHeight;
    frustrumHeight * aspect => float frustrumWidth;
    frustrumWidth / NUM_STEPS => float padSpacing;

    // resize pads
    placePadsHorizontal(
        kickPads, kickPadGroup,
        frustrumWidth,
        - frustrumHeight / 2.0 + padSpacing / 2.0
    );

    placePadsHorizontal(
        snarePads, snarePadGroup,
        frustrumWidth,
        frustrumHeight / 2.0 - padSpacing / 2.0
    );

    // place relative to first kick pad
    kickPads[0].posWorld().x => float openHatX;
    (snarePads[0].posWorld() - kickPads[0].posWorld()).y - padSpacing => float hatHeight;
    placePadsVertical(
        openHatPads, openHatPadGroup,
        hatHeight,
        openHatX
    );

    kickPads[kickPads.size()-1].posWorld().x => float closedHatX;
    placePadsVertical(
        closedHatPads, closedHatPadGroup,
        hatHeight,
        closedHatX
    );

    // place lead pads
    (snarePads[snarePads.size()-2].posWorld() - snarePads[1].posWorld()).x => float leadWidth;
    for (0 => int i; i < NUM_STEPS; i++) {
        placePadsVertical(
            acidBassPads[i], acidBassGroups[i],
            hatHeight,
            snarePads[1].posWorld().x + (i * leadWidth / (acidBassPads.size() - 1.0))
        );
    }
}

fun void resizeListener() {
    placePads();
    WindowResizeEvent e;
    while (true) {
        e => now;
        placePads();
    }
} spork ~ resizeListener();

fun void placePadsHorizontal(GPad pads[], GGen @ parent, float width, float y) {
    width / pads.size() => float padSpacing;
    for (0 => int i; i < pads.size(); i++) {
        pads[i] @=> GPad pad;

        // initialize pad
        pad.init(mouse);

        // connect to scene
        pad --> parent;

        // set transform
        pad.sca(padSpacing * .7);
        pad.posX(padSpacing * i - width / 2.0 + padSpacing / 2.0);
    }
    parent.posY(y);
}

// places along vertical axis
fun void placePadsVertical(GPad pads[], GGen @ parent, float height, float x)
{
    // scale height down a smidge
    // .95 *=> height;
    height / pads.size() => float padSpacing;
    for (0 => int i; i < pads.size(); i++) {
        pads[i] @=> GPad pad;

        // initialize pad
        pad.init(mouse);

        // connect to scene
        pad --> parent;

        // set transform
        pad.sca(padSpacing * .7);
        pad.posY(padSpacing * i - height / 2.0 + padSpacing / 2.0);
    }
    parent.posX(x);
}


fun void sequenceBeat(Instrument @ instrument, GPad pads[], int rev) {
    0 => int i;
    if (rev) pads.size() - 1 => i;
    while (true) {
        if (pads[i].active()) {
            spork ~ instrument.play();
        }
        pads[i].play();  // must happen after .active() check
        STEP => now;
        pads[i].stop();

        // bump index
        if (rev) {
            i--;
            if (i < 0) pads.size() - 1 => i;
        } else {
            i++;
            if (i >= pads.size()) 0 => i;
        }
    }
} 

fun void sequenceLead(AcidBass leads[], GPad pads[][], int scale[], int root) {
    while (true) {
        for (0 => int i; i < pads.size(); i++) {
            pads[i] @=> GPad col[];
            for (0 => int j; j < col.size(); j++) {
                if (col[j].active()) {
                    col[j].play();
                    spork ~ leads[j].play(root + scale[j]);
                }
            }
            STEP / 2.0 => now;
            for (0 => int j; j < col.size(); j++) {
                col[j].stop();
            }
        }
    }
}

// Game loop ==================================================================

while (true) { GG.nextFrame() => now; }

