//-----------------------------------------------------------------------------
// name: ggen-primitives.ck
// desc: drawing a few primitives with basic GGens
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// uncomment to run in fullscreen mode
GG.fullscreen();

// shorthand for our scene root
GG.scene() @=> GScene @ scene;

// connect different GGens to the scene root
// 3D primitives
GCube cube --> scene;
GSphere sphere --> scene;
GTorus torus --> scene; torus.mat().shine(0);
// 2D primitives
GPlane plane --> scene;
GCircle circle --> scene;
// points primitive
GPoints points --> scene; points.mat().pointSize(55.0);

// put into an array of GMesh (super class)
[ cube,
  sphere,
  circle,
  plane,
  points,
  torus
] @=> GMesh ggens[];

0 => int pos;
// loop over our array
for( GMesh obj : ggens )
{
    // set position
    -ggens.size() + 2 * pos++ => obj.posX;

    // randomize color
    Math.random2f(0.0, 1.0) => float r;
    Math.random2f(0.0, 1.0) => float g;
    Math.random2f(0.0, 1.0) => float b;
    // set color on the material for each GGen
    @(r, g, b) => obj.mat().color;
}

// put camera on a dolly
GG.camera() --> GGen dolly --> scene;
// position
GG.camera().posZ( 10 );

// our local update
fun void update()
{
    // rotate camera by rotating the dolly
    dolly.rotY( GG.dt() );
    // look at
    GG.camera().lookAt( GG.scene().pos() );
    // raise the cube to a constant height
    2 => cube.posY;
    // oscillate as a function of time
    Math.sin(now/second) => cube.posX;
}

// infinite time loop
while( true )
{
    // manually call local update
    update();
    // IMPORTANT: synch with next graphics frame
    GG.nextFrame() => now;
}
