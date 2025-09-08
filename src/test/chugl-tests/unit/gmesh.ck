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


GMesh shadow_mesh_a --> GGen shadow_parent;
GMesh shadow_mesh_b --> shadow_parent;
T.assert(!shadow_mesh_a.shadowed(), "shadowed default false");
shadow_mesh_a.shadowed(true);
T.assert(shadow_mesh_a.shadowed(), "GMesh.shadowed(true)");
shadow_mesh_a.shadowed(false);
T.assert(!shadow_mesh_a.shadowed(), "GMesh.shadowed(false)");
shadow_parent.shadowed(true, true);
T.assert(shadow_mesh_a.shadowed() && shadow_mesh_b.shadowed(), "GGen.shadowed(true, true)");
shadow_parent.shadowed(false, true);
T.assert(!shadow_mesh_a.shadowed() && !shadow_mesh_b.shadowed(), "GGen.shadowed(false, true)");
shadow_parent.shadowed(true, false);
T.assert(!shadow_mesh_a.shadowed() && !shadow_mesh_b.shadowed(), "GGen.shadowed(true, false)");


// while (true) {
//     GG.nextFrame() => now;
// }

// =========================================
// GModel
// =========================================

GModel default_model;

T.assert(default_model.geometries.size() == 0, "GModel default geometries empty");
T.assert(default_model.materials.size() == 0, "GModel default materials empty");
T.assert(default_model.meshes.size() == 0, "GModel default meshes empty");
T.assert(Type.of(default_model).parent().name() == "GGen", "GModel parent is GGen");

T.assert(Machine.refcount(default_model.geometries) == 1, "GModel.geometries refcount");
T.assert(Machine.refcount(default_model.materials) == 1, "GModel.materials refcount");
T.assert(Machine.refcount(default_model.meshes) == 1, "GModel.meshes refcount");