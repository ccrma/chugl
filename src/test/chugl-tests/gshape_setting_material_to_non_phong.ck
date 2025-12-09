/*
Original issue: https://github.com/ccrma/chugl/issues/8
Thanks Nathan for reporting this.

Problem: setting GSphere/GCube/etc material to a non-PhongMaterial would result in a crash
if you then tried to use one of the phong material setter fns, e.g. GSphere.color().
This happens because of shader bindgroup misalignment between PhongMaterial and whatever you
the other material is.
Because we can't predict if the uniform at that slot in the new shader is expecting the same
data type (e.g. PhongMaterial has expects a vec4f in @binding(1)), we just no-op and print a 
warning.

A better solution would involve a wgsl parser/reflection so that we can determine the material
bindgroup layout from the wgsl shader string.
*/

GSphere s --> GG.scene();
s.mat(new FlatMaterial);
s.color(Color.BLACK);

GG.nextFrame() => now;