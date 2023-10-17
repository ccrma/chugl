// "circles, in numbers, are cool" -- Ge

// fullscreen
GG.fullscreen();
// position
GG.camera().posZ( 3 );
// set bg color
GG.scene().backgroundColor( @(0,0,0) );

// how many circles?
512 => int NUM_CIRCLES;

// creating a custom GGen class for a 2D circle
class Circle extends GGen
{
    // many vertices (more == smoother)
    720 => int N;
    // normalized radius
    1 => float RADIUS;
    // a cirle of lines
    GLines circle --> this;
    // incremental angle from 0 to 2pi in N steps
    2*pi / N => float theta;    
    // randomize rate
    Math.random2f(2,3) => float rate;
    // initialize
    init();

    // initialize an circle
    fun void init()
    {
        vec3 pos[N];
        // previous, init to 1 zero
        @(RADIUS,0) => vec3 prev;
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
        // color
        color( @(.5, 1, .5) );
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
for( auto ggen : circles )
{
    // connect it
    ggen --> GG.scene();
    // randomize location in XY
    @( Math.random2f(-1.5,1.5), Math.random2f(-1,1), Math.random2f(-1,1) ) => ggen.position;
}

vec3 color;
fun void cycleColors( float dt )
{
    // iterate over circles array
    for( auto ggen : circles )
    {
        // cycle RGB
        @( (1+Math.sin(now/second*.7))/2, (1+Math.sin(now/second*.8))/2, (1+Math.sin(now/second*.9))/2 ) => ggen.color;
    }
}

// hook up camera
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