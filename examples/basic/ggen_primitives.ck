//-----------------------------------------------------------------------------
// name: ggen-primitives.ck
// desc: drawing a few primitives with basic GGens
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2024
//-----------------------------------------------------------------------------

// uncomment to run in fullscreen mode
GG.fullscreen();

// empty group to hold all our primitives
GGen group --> GG.scene();

// connect different GGens to the scene root
// 3D primitives
GCube cube --> group;
GSphere sphere --> group;
GTorus torus --> group; torus.sca(.7);
GCylinder cylinder --> group;
GKnot knot --> group; knot.sca(.5);
GSuzanne suzanne --> group; suzanne.sca(.5);

// platonic solids
GPolyhedron tetrahedron(PolyhedronGeometry.TETRAHEDRON) --> group; tetrahedron.posY(-2.).posX(-2);
GPolyhedron octahedron(PolyhedronGeometry.OCTAHEDRON) --> group; octahedron.posY(-2).posX(-6);
GPolyhedron dodecahedron(PolyhedronGeometry.DODECAHEDRON) --> group; dodecahedron.posY(-2).posX(2);
GPolyhedron icosahedron(PolyhedronGeometry.ICOSAHEDRON) --> group; icosahedron.posY(-2).posX(6);

// randomize polyhedron colors
Color.random() => tetrahedron.color;
Color.random() => octahedron.color;
Color.random() => dodecahedron.color;
Color.random() => icosahedron.color;

// 2D primitives
GPlane plane --> group;
GCircle circle --> group;

// put into an array of GMesh (super class)
[ cube,
  sphere,
  circle,
  plane,
  torus,
  cylinder,
  knot,
  suzanne,
] @=> GMesh ggens[];

0 => int pos;
// loop over our array
for( GMesh obj : ggens )
{
    // set position
    -ggens.size() + 2 * pos++ => obj.posX;
    
    // randomize color
    Color.random() => (obj.mat() $ PhongMaterial).color;
}

// position
GG.camera().posZ( 10 );

// our local update
fun void update()
{
    // raise the cube to a constant height
    2 => cube.posY;
    // oscillate as a function of time
    Math.sin(now/second) => cube.posX;
}

// infinite time loop
while( true )
{
    // IMPORTANT: sync with next graphics frame
    GG.nextFrame() => now;
    // manually call local update
    update();
}