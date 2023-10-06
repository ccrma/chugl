/*
ideal solar system example
tests scenegraph parent/child relations + local vs global transforms + rotation
*/

class Body extends GGen {
	child --> GGSphere body --> parent;  // child and parent are keywords like inlet and outlet

	1.0 => float rotSpeed;

	fun void update(float dt) {
		rotSpeed * dt => body.rotY;
	}
}

NormMat planetMat;
GGSphere sun, earth, moon;
[sun, earth, moon] @=> GGSphere @ planets[];

GGGroup sunSystem, earthSystem, moonSystem;

moonSystem --> earthSystem --> sunSystem --> GG.root;
sun --> sunSystem;
earth --> earthSystem;
moon --> moonSystem;

// set wireframe
planetMat.mode(GG.LINES);
for (auto planet : planets)
	planet.mat(planetMat);


@(2.2, 0.0, 0.0) => earthSystem.pos;
@(.55, 0.0, 0.0) => moonSystem.pos;

@(2.0, 2.0, 2.0) => sun.scale;
@(0.4, 0.4, 0.4) => earth.scale;
@(0.12, 0.12, 0.12) => moon.scale;


fun void Update(time t, dur dt) 
{
	t / second => float ftime;

	sunSystem.rotation(@(0.0, .5 * ftime, 0.0));
	earthSystem.rotation(@(0.0, .7 * ftime, 0.0));

	sun.rotation(@(0.0, .1 * ftime, 0.0));
	earth.rotation(@(0.0, .4 * ftime, 0.0));
	moon.rotation(@(0.0, .9 * ftime, 0.0));
}

now => time lastTime;
while (true) {
	// compute timing
	now - lastTime => dur deltaTime;
	now => lastTime;

	// Update logic
	Update(now, deltaTime);

	GG.nextFrame() => now;
}
