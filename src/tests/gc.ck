// Test garbage collection on mesh, mat, and geo
// TODO add texture garbage collection

fun void Spawn() {
    GCube c;
    c --> GG.scene();
    GG.nextFrame() => now;
    c --< GG.scene();
}

while (true) {
    spork ~ Spawn();
    // Spawn();  // TODO: this leaks right now until we figure out a better way to refcount GGens
    GG.nextFrame() => now;
}