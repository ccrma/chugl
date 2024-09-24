//-----------------------------------------------------------------------------
// name: orbits.ck
// desc: solar system example demoing scenegraph + local vs global transforms
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------
GG.scene().backgroundColor( @(0,0,0) );

// scene setup
GScene scene;
GGen sunSystem, earthSystem, moonSystem;
GSphere sun, earth, moon;

// set to wireframe
for( auto x : [ sun, earth, moon ] )
    x.mat().polygonMode( Material.POLYGON_LINE );

sun.mat().color( @(1,1,.25) );
earth.mat().color( @(.25,.25,1) );
moon.mat().color( @(.5,.5,.5) );

earthSystem.pos(@(2.2, 0.0, 0.0));
moonSystem.pos(@(.55, 0.0, 0.0));

sun.sca(@(2.0, 2.0, 2.0));
earth.sca(@(0.4, 0.4, 0.4));
moon.sca(@(0.12, 0.12, 0.12));

// construct scenegraph
moonSystem --> earthSystem --> sunSystem --> scene;
sun --> sunSystem;
earth --> earthSystem;
moon --> moonSystem;

// position camera
GG.camera().pos(@(0, 5, 7)); 
GG.camera().lookAt(@(0, 0, 0));

while (true) {
	GG.dt() => float dt;  // get delta time

	// rotate systems
	sunSystem.rotateY(.5 * dt);
	earthSystem.rotateY(.7 * dt);

	// rotate planets
	sun.rotateY(-1 * dt);
	earth.rotateY(.4 * dt);
	moon.rotateY(.9 * dt);

	GG.nextFrame() => now;
}


