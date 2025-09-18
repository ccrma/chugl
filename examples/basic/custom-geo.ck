//-----------------------------------------------------------------------------
// name: custom-geo.ck
// desc: creating custom meshes via passing vertex data directly
//       (useful for creating procedural geometry or making your own asset importers)
// requires: ChuGL + chuck-1.5.3.0 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// let's build a square out of 2 triangles!

// construct the vertex data ==================================================
Geometry geo;

// pass in 3D positions for each vertex
// alternate: use geo.positions(...)
geo.vertexAttribute(
    Geometry.ATTRIBUTE_POSITION,
    3,
    // vertex positions for a plane
    [
        -0.5, 0.5, 0,  // top left
        0.5, 0.5, 0,   // top right
        -0.5, -0.5, 0, // bottom left
        0.5, -0.5, 0,  // bottom right
    ]
);

// pass in the normals (used in lighting calculations), make sure they
// are normalized (i.e. have magnitude = 1)
// alternate: use geo.normals(...)
geo.vertexAttribute(
    // vertex normals for a plane ( all of them point out along +z axis )
    Geometry.ATTRIBUTE_NORMAL,
    3,
    [
        0.0, 0, 1, 
        0, 0, 1, 
        0, 0, 1, 
        0, 0, 1,
    ]
);

// pass in the texture coordinates (used to map textures onto the mesh)
// each field be clamped between 0 and 1
// alternate: use geo.uvs(...)
geo.vertexAttribute(
    // vertex uvs for a plane
    Geometry.ATTRIBUTE_UV,
    2,
    [
        0.0, 1, 
        1, 1, 
        0, 0, 
        1, 0, 
    ]
);

// pass in the indices. optional
// if set, every 3 indices will be used to construct a triangle
geo.indices(
    [
        0, 2, 1, // bottom left triangle
        2, 3, 1, // top right triangle
    ]
);


// Scene setup ================================================================
GMesh mesh(geo, new PBRMaterial) --> GG.scene();


// Game loop ==================================================================
while (true) {
    GG.nextFrame() => now; 
    GG.dt() => mesh.rotateY;  // rotate on Y axis
}
