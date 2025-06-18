GG.rootPass() --> ScenePass scene_pass;
scene_pass.scene(GG.scene());

GCube cube --> GG.scene();

while (1) {
    GG.nextFrame() => now;
}