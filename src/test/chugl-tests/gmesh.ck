GMesh A;
PlaneGeometry geo;
FlatMaterial mat;

T.assert(A.material() == null, "GMesh default material == null");
T.assert(A.geometry() == null, "GMesh default geometry == null");

A.mesh(geo, mat);
T.assert(A.material() == mat, "GMesh material set");
T.assert(A.geometry() == geo, "GMesh geometry set");

A.mesh(null, null);
T.assert(A.material() == null, "GMesh material unset");
T.assert(A.geometry() == null, "GMesh geometry unset");

A.material(mat);
T.assert(A.material() == mat, "GMesh material set");
A.material(null);
T.assert(A.material() == null, "GMesh material unset");

A.geometry(geo);
T.assert(A.geometry() == geo, "GMesh geometry set");
A.geometry(null);
T.assert(A.geometry() == null, "GMesh geometry unset");




// while (true) {
//     GG.nextFrame() => now;
// }