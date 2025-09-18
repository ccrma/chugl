//-----------------------------------------------------------------------------
// name: panning.ck
// desc: very simple panning demonstration (audiovisual)
// 
// author: Kunwoo Kim (https://kunwookim.com)
//   date: Fall 2024
//-----------------------------------------------------------------------------

// Random SqrOsc sounds
SqrOsc foo => LPF lpf => ADSR e => NRev r => Pan2 p => dac;

// Parameter setup
// -low pass filter gain
0.3 => lpf.gain;
// -reverb
0.2 => r.mix;
// -duration of each note
50::ms => dur d;
// -gain
.5 => foo.gain;
// -pentatonic scale
[0, 2, 4, 5, 7, 9, 11] @=> int notes[];

// make some sounds
fun void makeSound()
{
    while (true)
    {
        // 50% of the time duration is twice longer.
        if (Math.random2f(0, 1) > 0.5)
        {
            d * 2 => now;
        }
        
        // set A, D, S, and R
        e.set( Math.random2(10, 70)::ms, 8::ms, .5, d );
            
        // random low pass filter
        Math.random2f(1000, 5000) => lpf.freq;
        
        // assign random frequency
        Std.mtof(notes[Math.random2(0, notes.size() - 1)] + Math.random2(0, 2) * 12 + 36) => foo.freq;

        // play!
        e.keyOn();
        d => now;
        e.keyOff();
        e.releaseTime() => now;
        d => now;
    }
}
spork ~ makeSound();

// convert from mouse position to world space
1.0 => float distance_from_camera;
GWindow.fullscreen();

// spawn something at that location
GCircle circle --> GG.scene();
GG.camera().screenCoordToWorldPos(GWindow.mousePos(), distance_from_camera) => vec3 pos;
circle.pos(pos);
circle.sca(0.05);
circle.color(Color.WHITE);

// mouse Range
0.56 => float mouseRangeX;

// make some graphics
fun void makeGraphics()
{
    GText text_directions --> GG.scene(); text_directions.posX(0); text_directions.posY(-1.8); text_directions.sca(0.1);
    "Press and hold mouse left button. Move around the space horizontally." => text_directions.text;

    while (true)
    {
        GG.nextFrame() => now;
        
        // when left mouse button is pressed
        if (GWindow.mouseLeft()) {        
            GG.camera().screenCoordToWorldPos(GWindow.mousePos(), distance_from_camera) => pos;
            circle.pos(pos);
            
            // mouse left means pan left, mouse right means pan right (-1: left, 1: right)
            Math.remap(pos.x, -mouseRangeX, mouseRangeX, -1, 1) => float pan;
            if (pan < -1)
                -1 => pan;
            else if (pan > 1)
                1 => pan;
            
            pan => p.pan;
        }
    }
}
spork ~ makeGraphics();

// keep going
while( true ) 1::day => now;
