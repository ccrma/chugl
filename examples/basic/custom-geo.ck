//-----------------------------------------------------------------------------
// name: custom-geo.ck
// desc: creating custom meshes via passing vertex data directly in chuck
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// Let's build a square out of 2 triangles!

// Construct the vertex data ==================================================

CustomGeometry customGeometry;

// pass in 3D positions for each vertex
customGeometry.positions(
    // vertex positions for a plane
    [
        // left triangle
        @(-1.0,  1.0,  0.0),
        @(-1.0, -1.0,  0.0),
         @(1.0, -1.0,  0.0),

        // right triangle
        @(-1.0,  1.0,  0.0),
         @(1.0, -1.0,  0.0),
         @(1.0,  1.0,  0.0)
    ]
);

// pass in the normals (used in lighting calculations), make sure they
// are normalized (i.e. have magnitude = 1)
customGeometry.normals(
    // vertex normals for a plane ( all of them point out along +z axis )
    [
        // left triangle
        @(0.0,  0.0,  1.0),
        @(0.0,  0.0,  1.0),
        @(0.0,  0.0,  1.0),

        // right triangle
        @(0.0,  0.0,  1.0),
        @(0.0,  0.0,  1.0),
        @(0.0,  0.0,  1.0),
    ]
);

// pass in the texture coordinates (used to map textures onto the mesh)
// each field be clamped between 0 and 1
customGeometry.uvs(
    // vertex uvs for a plane
    [
        // left triangle
        @(0.0,  1.0),
        @(0.0,  0.0),
        @(1.0,  0.0),

        // right triangle
        @(0.0,  1.0),
        @(1.0,  0.0),
        @(1.0,  1.0),
    ]
);

// pass in the indices. optional
// if set, every 3 indices will be used to construct a triangle
customGeometry.indices(
    [
        0, 1, 2,  // bottom left triangle
        3, 4, 5   // top right triangle
    ]
);

// Scene setup ================================================================

GScene scene;
MangoUVMaterial mangoMat;
GMesh mesh;

// set geo and material
mesh.set(customGeometry, mangoMat);
// set position
mesh.position(@(0.0, 0.0, -5.0));
// add to scene
mesh --> scene;

// Game loop ==================================================================
while (true) {
    <<< GG.fps() >>>;
    GG.dt() => mesh.rotY;  // rotate on Y axis
    GG.nextFrame() => now; 
}