fun void alwaysShouldGC() {
    // GCube cube;
    BoxGeometry box;
}

alwaysShouldGC();

while (true) {
    // alwaysShouldGC();
    // spork ~ alwaysShouldGC();
    // spork ~ sporkShouldGC();
    GG.nextFrame() => now;
}