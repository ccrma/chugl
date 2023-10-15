// Test garbage collection on mesh, mat, and geo
// TODO add texture garbage collection

fun void Spawn() {
    GCube c;
}

while (true) {
    spork ~ Spawn();
    GG.nextFrame() => now;
}