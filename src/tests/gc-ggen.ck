// Test garbage collection on mesh, mat, and geo

// face camera away to avoid flashing cube
GG.camera().lookAt(@(0, 0, 10));

// Global GGen reference to test case where the shred that
// created the GGen is destroyed, but the GGen is still
// referenced by another shred.

GGen @ out_of_shred_ref;

fun void spawnAndGC() {
    // GGen `a` is never even attached to scenegraph
    // GGen `b` is sent to external reference, should be cleaned up when that reference gets replaced
    // GGen `c` is attacheed but then immediately detached from scenegraph
    // ALL should be cleaned up by GC
    GSphere a, b, c;
    b @=> out_of_shred_ref;
    c --> GG.scene();
    GG.nextFrame() => now;
    c --< GG.scene();
}

fun void spawnAndGCifSporked() {
    // upon shred exit, GGen `a` should be GC'd because the shred
    // will disconnect it from the scenegraph on shred exit
    GCircle a --> GG.scene();
    GPlane c --> GTorus d;
    GG.nextFrame() => now; // give chance for this to be propagated to render thread

    // also test GC on GGens that are never attached to render-thread scenegraph
    GCylinder b --> GG.scene();
}

fun void spawnNoGC() {
    // GGen is attached to the scenegraph. when function exits and `a` falls out of scope,
    // GGen should still be refcounted by scenegraph and NOT be GC'd
    GCube a --> GG.scene();
    // place in front of camera so we can see
    a.pos(GG.camera().pos() + 3*GG.camera().forward());
}
spawnNoGC();

while (true) {
    spork ~ spawnAndGC();  // test GC happens when shred exits
    spawnAndGC();          // test GC happens when shred does not exit
    spork ~ spawnAndGCifSporked(); // test GC happens when shred exits

    GG.nextFrame() => now;
}