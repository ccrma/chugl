GOrbitCamera orbit_camera --> GG.scene();
GG.scene().camera(orbit_camera);
orbit_camera.posZ(99.84);

SuzanneGeometry geo;
// PhongMaterial mat;
FlatMaterial mat;


100 => int NUM_ROWS;
100 => int NUM_COLS;
GMesh monkeys[NUM_ROWS][NUM_COLS];

float rotation_speeds[NUM_ROWS][NUM_COLS];

for (int y; y < NUM_ROWS; y++) {
    for (int x; x < NUM_COLS; x++) { 
        monkeys[y][x] --> GG.scene();
        monkeys[y][x].sca(.3);
        monkeys[y][x].pos(x - NUM_ROWS / 2.0, y - NUM_COLS / 2.0);
        monkeys[y][x].mesh(geo, mat);

        Math.random2f(-15, 15) => rotation_speeds[x][y];
    }
}

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    for (int y; y < NUM_ROWS; y++) {
        for (int x; x < NUM_COLS; x++) { 
            monkeys[y][x].rotateY(dt * rotation_speeds[x][y]);
        }
    }

    <<< orbit_camera.posZ() >>>;
}