GSuzanne suz --> GG.scene();

UI_Float4 viewport_normalized(0., 0., 1., 1.);
UI_Float4 viewport_absolute(0., 0., 200., 200.);
UI_Float4 scissor_normalized(0., 0., 1., 1.);
UI_Float4 scissor_absolute(0., 0., 200., 200.);

while (1) {
    GG.nextFrame() => now;
    if (UI.begin("splitscreen example")) {
        if (UI.drag("viewport (normalized)", viewport_normalized, .01)) {
            GG.scenePass().viewportNormalized(viewport_normalized.val());
        }
        if (UI.drag("viewport (in pixels)", viewport_absolute)) {
            GG.scenePass().viewport(viewport_absolute.val());
        }

        if (UI.drag("scissor (normalized)", scissor_normalized, .01)) {
            GG.scenePass().scissorNormalized(scissor_normalized.val());
        }
        if (UI.drag("scissor (in pixels)", scissor_absolute)) {
            GG.scenePass().scissor(scissor_absolute.val());
        }
    }
    UI.end();
}