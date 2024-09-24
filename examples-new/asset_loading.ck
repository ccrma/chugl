AssLoader ass_loader;

// ass_loader.loadObj(me.dir() + "../assets/obj/suzanne.obj") @=> GGen@ model;
ass_loader.loadObj(me.dir() + "../assets/obj/backpack/backpack.obj") @=> GGen@ model;

model --> GG.scene();

// GG.scene().light().intensity(0);

while (true) {
    GG.nextFrame() => now;
    if (UI.begin("Asset Loading")) {
        UI.scenegraph(GG.scene());
    }
    UI.end();
    // GG.dt() => model.rotateY;
}