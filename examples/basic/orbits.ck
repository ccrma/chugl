//-----------------------------------------------------------------------------
// name: orbits.ck
// desc: solar system example demoing scenegraph + local vs global transforms
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------
NormalsMaterial normMat;
normMat.worldNormals();

// scene setup
GScene scene;
GGen sunSystem, earthSystem, moonSystem;
GCube sun, earth, moon;

sun.mat(normMat); 
earth.mat(normMat);
moon.mat(normMat);

earthSystem.position(@(2.2, 0.0, 0.0));
moonSystem.position(@(.55, 0.0, 0.0));

sun.scale(@(2.0, 2.0, 2.0));
earth.scale(@(0.4, 0.4, 0.4));
moon.scale(@(0.12, 0.12, 0.12));

// construct scenegraph
moonSystem --> earthSystem --> sunSystem --> scene;
sun --> sunSystem;
earth --> earthSystem;
moon --> moonSystem;

// position camera
GG.camera().position(@(0, 5, 7)); 
GG.camera().lookAt(@(0, 0, 0));

while (true) {
	GG.dt() => float dt;  // get delta time

	// rotate systems
	sunSystem.rotY(.5 * dt);
	earthSystem.rotY(.7 * dt);

	// rotate planets
	sun.rotY(.1 * dt);
	earth.rotY(.4 * dt);
	moon.rotY(.9 * dt);

	GG.nextFrame() => now;
}


