GOrbitCamera cam --> GG.scene();
GG.scene().camera(cam);

// SphereGeometry geo;
// PhongMaterial mat;
// GMesh instanced_spheres(geo, mat)[512]; 
// GSuzanne instanced_spheres[512]; 

// for (auto s : instanced_spheres)
//     s --> GG.scene();

50 => int ROWS;
50 => int COLS;
GSuzanne suz[ROWS][COLS];

for (int i; i < ROWS; i++) {
    for (int j; j < COLS; j++) {
        suz[i][j] --> GG.scene();
        suz[i][j].pos(i - 15, j - 15);
    }
}


while (1) GG.nextFrame() => now;