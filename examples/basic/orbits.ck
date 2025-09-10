//-----------------------------------------------------------------------------
// name: orbits.ck
// desc: solar system example demoing scenegraph + local vs global transforms
// requires: ChuGL + chuck-1.5.5.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------
// scene setup
GGen sunSystem, earthSystem, moonSystem;
GSphere sun, earth, moon;

// orbit camera
GOrbitCamera cam => GG.scene().camera;

// set to wireframe
for( auto x : [ sun, earth, moon ] )
    x.mat().wireframe(true);

// up the ambient light
GG.scene().ambient(@(.5,.5,.5));

// color
sun.color( Color.YELLOW );
earth.color( (Color.SKYBLUE + Color.BLUE) / 2 );
moon.color( Color.GRAY );

// position
earthSystem.pos(@(2.2, 0.0, 0.0));
moonSystem.pos(@(.55, 0.0, 0.0));

// scale
sun.sca(@(2.0, 2.0, 2.0));
earth.sca(@(0.4, 0.4, 0.4));
moon.sca(@(0.12, 0.12, 0.12));

// construct scenegraph
moonSystem --> earthSystem --> sunSystem --> GG.scene();
// add sun earth moon to respective systems
sun --> sunSystem;
earth --> earthSystem;
moon --> moonSystem;

// position camera
cam.pos( @(0, 5, 7) ); 
cam.lookAt( @(0, 0, 0) );

// render loop
while (true)
{
    // render loop
	GG.nextFrame() => now;

	// rotate systems
	sunSystem.rotateY(.5 * GG.dt());
	earthSystem.rotateY(.7 * GG.dt());

	// rotate planets
	sun.rotateY(-1 * GG.dt());
	earth.rotateY(.4 * GG.dt());
	moon.rotateY(.9 * GG.dt());
}
