//--------------------------------------------------------------------
// name: thx-visual.ck
// desc: real-time visualizes the THX Deep Note using ChucK/ChuGL,
//       coloring each voice in frequency and gain;
//       based on thx.ck code (see below)
//
// author: Kunwoo Kim (kunwoo@ccrma.stanford.edu)
//   date: Fall 2024
//--------------------------------------------------------------------
// name: thx.ck
// desc: emulation of the original THX Deep Note
//       (by Dr. James Andy Moorer)
//
// authors: Perry R. Cook (https://www.cs.princeton.edu/~prc/)
//          Ge Wang (https://ccrma.stanford.edu/~ge/)
//--------------------------------------------------------------------

// ------------------ THX SOUND PARAMETERS ------------------- //
// 30 target frequencies, corresponding to pitches in a big chord:
// D1,  D2, D3,  D4,  D5,  A5,  D6,   F#6,  A6
[ 37.5, 75, 150, 300, 600, 900, 1200, 1500, 1800,
  37.5, 75, 150, 300, 600, 900, 1200, 1500, 1800,
  37.5, 75, 150, 300, 600, 900, 1200,       1800,
            150, 300,      900, 1200  
] @=> float targets[];

// initial frequencies
float initials[30];
// for the initial "wavering" in the steady state
float initialsBase[30];
float randomRates[30];

// parameters (play with these to control timing)
12.5::second => dur initialHold; // initial steady segment
6.0::second => dur sweepTime; // duration over which to change freq
5.5::second => dur targetHold; // duration to hold target chord
6.0::second => dur decayTime; // duration for reverb tail to decay to 0

// sound objects
SawOsc saw[30]; // sawtooth waveform (30 of them)
Gain gainL[30]; // left gain (volume)
Gain gainR[30]; // right gain (volume)
// connect stereo reverberators to output
NRev reverbL => dac.left;
NRev reverbR => dac.right;
// set the amount of reverb
0.075 => reverbL.mix => reverbR.mix;

// ------------------ VISUALIZE THE SOUND ------------------- //
// parent object for all the lines
GGen line_group --> GG.scene();
// number of segment of each line
3000 => int numPoints;
// number of lines (same as number of SawOscs)
GLines lines[30];
// vec2 for storing points
vec2 positions[30][numPoints];
// vec3 for stroing granular color
vec3 colors[30][numPoints];
// x position increment length (x = time)
0.01 => float xIncrement;
// counter for x position progress
0 => int xCounter;

// Color Map
vec3 newColorMap[6];
[@(1,0,1), @(0,0,1), @(0,1,0), @(1,1,0), @(1,0.5,0), @(1,0,0)] @=> newColorMap;
// Map colors based on frequency
2200 => float freqMax;
0 => float freqMin;
0.5 => float freqResolution;
vec3 colorMapTotal[((freqMax - freqMin) / freqResolution) $ int];

// Interpolate between colors based on the color map.
colorMapTotal.size() / 6 => int numInterval;
for (int i; i < 5; i++)
{  
    for (int j; j < numInterval; j++)
    {
        newColorMap[i].x + (newColorMap[i+1].x - newColorMap[i].x) * j / numInterval => colorMapTotal[i * numInterval + j].x;
        newColorMap[i].y + (newColorMap[i+1].y - newColorMap[i].y) * j / numInterval => colorMapTotal[i * numInterval + j].y;
        newColorMap[i].z + (newColorMap[i+1].z - newColorMap[i].z) * j / numInterval => colorMapTotal[i * numInterval + j].z;
    }
}

// Camera
GCamera orbit_cam --> GG.scene();
GG.scene().camera(orbit_cam);
GWindow.fullscreen();
@(2, 0, 8) => vec3 camInitialPos;
@(14, 7, 28) => vec3 camTargetPos;

// Instantiate Lines
for (int i; i < 30; i++)
{
    lines[i] --> line_group; lines[i].width(0.01);
}

// Initial positions
line_group.posX(-3);
line_group.posY(-3);

// BLOOM
GG.renderPass() --> BloomPass bloom_pass --> GG.outputPass();
bloom_pass.input(GG.renderPass().colorOutput());
GG.outputPass().input(bloom_pass.colorOutput());

bloom_pass.intensity(0.8);
bloom_pass.radius(0.68);
bloom_pass.levels(9);

// run ChuGL
fun void runChuGL()
{
    while (true)
    {
        GG.nextFrame() => now;
    }
}
spork ~ runChuGL();

// visualize Lines
fun void visualize_lines()
{
    while (true)
    {
        if (xCounter < numPoints)
        {
            for (int i; i < 30; i++)
            {
                // move x position that represents time
                xIncrement * xCounter => positions[i][xCounter].x;
                // assign y position based on frequency
                saw[i].freq() * 0.01 => positions[i][xCounter].y;
                // set positions for lines
                lines[i].positions(positions[i]);
                
                // gain determines the intensity of the colors
                Math.pow(saw[i].gain() * 15.0, 1.0) * colorMapTotal[(saw[i].freq() / freqResolution)$int] * 4 => colors[i][xCounter];
                // set colors
                colors[i] => lines[i].colors;
            }
        }
        xCounter++;
        10::ms => now;
    }
}
spork ~ visualize_lines();

// lerp camera
fun void lerp_camera()
{
    while (true)
    {
        if (xCounter < numPoints * 0.65)
        {
            camInitialPos.x + (camTargetPos.x - camInitialPos.x) * xCounter/(numPoints * 0.65) => orbit_cam.posX;
            camInitialPos.y + (camTargetPos.y - camInitialPos.y) * xCounter/(numPoints * 0.65) => orbit_cam.posY;
            camInitialPos.z + (camTargetPos.z - camInitialPos.z) * xCounter/(numPoints * 0.65) => orbit_cam.posZ;
        }
        10::ms => now;
    }
}
spork ~ lerp_camera();

// for each sawtooth: connect, compute frequency trajectory
for( 0 => int i; i < 30; i++ )
{
    // connect sound objects (left channel)
    saw[i] => gainL[i] => reverbL;
    // connect sound objects (right channel)
    saw[i] => gainR[i] => reverbR;
    // randomize initial frequencies
    Math.random2f( 160.0, 360.0 ) => initials[i] 
               => initialsBase[i] => saw[i].freq;
    // initial gain for each sawtooth generator
    0.1 => saw[i].gain;
    // randomize gain (volume)
    Math.random2f( 0.0, 1.0 ) => gainL[i].gain;
    // right.gain is 1-left.gain -- effectively panning in stereo
    1.0 - gainL[i].gain() => gainR[i].gain;
    // rate at which to waver the initial voices
    Math.random2f(.1,1) => randomRates[i];
}

// hold steady cluster (initial chaotic random frequencies)
now + initialHold => time end;
// fade in from silence
while( now < end )
{
    // percentage (should go from 0 to 1)
    1 - (end-now) / initialHold => float progress;
    // for each sawtooth
    for( 0 => int i; i < 30; i++ ) {
        // set gradually decaying values to volume
        0.1 * Math.pow(progress,3) => saw[i].gain;
        // waver the voices
        initialsBase[i] + (1.25-progress)*.5*initialsBase[i]*Math.sin(now/second*randomRates[i])
             => initials[i] => saw[i].freq;
    }
    // advance time
    10::ms => now;
}

// when to stop
now + sweepTime => end;
// sweep freqs towards target freqs
while( now < end )
{
    // percentage (should go from 0 to 1)
    1 - (end-now)/sweepTime => float progress;
    // for each sawtooth
    for( 0 => int i; i < 30; i++ ) {
        // update frequency by delta, towards target
        initials[i] + (targets[i]-initials[i])*progress => saw[i].freq;
    }
    // advance time
    10::ms => now;
}

// at this point: reached target freqs; briefly hold
targetHold => now;

// when to stop
now + decayTime => end;
// chord decay (fade to silence)
while( now < end )
{
    // percentage (should go from 1 to 0)
    (end-now) / decayTime => float progress;
    // for each sawtooth
    for( 0 => int i; i < 30; i++ ) {
        // set gradually decaying values to volume
        0.1 * progress => saw[i].gain;
    }
    // advance time
    10::ms => now;
}

// wait for reverb tail before ending
8::second => now;
