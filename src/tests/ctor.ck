// GGen ggen;
// GMesh mesh;
// PointLight light;
GCube cube;

cube --> GG.scene();
-5.0 => cube.posZ;

fun void update(float dt) {
    dt => cube.rotX;
}

while (true) {
    update(GG.dt());
    GG.nextFrame() => now;
}