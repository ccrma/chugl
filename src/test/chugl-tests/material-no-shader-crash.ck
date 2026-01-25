/*
Bug: rendering a GMesh that has a Material with no shader crashes (null ptr segfault)
Fix: in _R_RenderScene(), skip materials that have no shader and log a warning
*/

Material no_shader_mat;
new GMesh(new PlaneGeometry, no_shader_mat) --> GG.scene();
no_shader_mat.name("no shader mat");

GG.nextFrame() => now;