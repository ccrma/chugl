
Material materials[0];
for (int i; i < 10; i++) {
    for (int j; j < 3; j++) {
        FlatMaterial mat_c;
        mat_c.topology(j);
        materials << mat_c;
        PhongMaterial mat_b;
        mat_b.topology(j);
        materials << mat_b;
    }
}

SphereGeometry geo_1;
PlaneGeometry geo_2;

[ geo_1, geo_2 ] @=> Geometry geometries[];

// make a bunch of random meshes
for (int i; i < 200; i++) {
    new GMesh(
        geometries[Math.random2(0, geometries.size()-1)], 
        materials[Math.random2(0, materials.size()-1)]
    ) --> GG.scene();
}


while (true) {
    GG.nextFrame() => now;
}