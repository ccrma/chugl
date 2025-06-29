UVMaterial mat;
PlaneGeometry geo;

GMesh mesh(geo, mat) --> GG.scene();

while (1) {
    GG.nextFrame() => now;
}