// "circles, in numbers, are cool" -- Ge
//
// this example demonstrates:
//   1) creating a custom GGen class
//   2) plotting our circle with #math
//   3) many instances: local independence + global organization
//      -- design principle from artful design chapter 3

// fullscreen
GG.fullscreen();
// position
GG.camera().posZ( 3 );
// set bg color
GG.scene().backgroundColor( @(0,0,0) );

// how many circles?
512 => int NUM_CIRCLES;
// how many vertices per circle? (more == smoother)
720 => int N;
// normalized radius
1 => float RADIUS;

// creating a custom GGen class for a 2D circle
class Circle extends GGen
{
    // for drawing our circle 
    GLines circle --> this;
    // randomize rate
    Math.random2f(2,3) => float rate;
    // default color
    color( @(.5, 1, .5) );

    // initialize a circle
    fun void init( int resolution, float radius )
    {
        // incremental angle from 0 to 2pi in N steps
        2*pi / resolution => float theta;    
        // positions of our circle
        vec3 pos[resolution];
        // previous, init to 1 zero
        @(radius,0) => vec3 prev;
        // loop over vertices
        for( int i; i < pos.size(); i++ )
        {
            // rotate our vector to plot a circle
            // https://en.wikipedia.org/wiki/Rotation_matrix
            Math.cos(theta)*prev.x - Math.sin(theta)*prev.y => pos[i].x;
            Math.sin(theta)*prev.x + Math.cos(theta)*prev.y => pos[i].y;
            // just XY here, 0 for Z
            0 => pos[i].z;
            // remember v as the new previous
            pos[i] => prev;
        }
        
        // set positions
        circle.geo().positions( pos );
    }
    
    fun void color( vec3 c )
    {
        circle.mat().color( c );
    }
    
    fun void update( float dt )
    {
        .35 + .25*Math.sin(now/second*rate) => float s;
        circle.scale( @(s,s,s ) );
        // uncomment for xtra weirdness
        // circle.rotY(dt*rate/3);
    }
}

// an array of our custom circles
Circle circles[NUM_CIRCLES];
// iterate over circles array
for( auto circ : circles )
{
    // initialize each 
    circ.init( N, RADIUS );
    // connect it
    circ --> GG.scene();
    // randomize location in XY
    @( Math.random2f(-1.5,1.5),
       Math.random2f(-1,1),
       Math.random2f(-1,1) ) => circ.position;
}

// function to cycle colors
fun void cycleColors( float dt )
{
    // iterate over circles array
    for( auto circ : circles )
    {
        // cycle RGB
        @( (1+Math.sin(now/second*.7))/2,
           (1+Math.sin(now/second*.8))/2,
           (1+Math.sin(now/second*.9))/2 ) => circ.color;
    }
}

// connect camera to a dolly
// (in case we want to easily rotate camera around scene)
GG.camera() --> GGen dolly --> GG.scene();

// time loop
while( true )
{
    // cycle colors
    cycleColors( GG.dt() );
    // uncomment to rotate camera around scene, for reasons unknown
    // dolly.rotY(GG.dt()*.5);
    // next frame
    GG.nextFrame() => now;
}