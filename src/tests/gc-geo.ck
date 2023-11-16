// Test garbage collection on Geometry Material
// Correct behavior is seeing a single white circle

GCube cube --> GG.scene();

Geometry @ out_of_shred_ref;

// Cases where material should GC if the function is either
// sporked OR called normally
fun void alwaysShouldGC() {
    /*
    A is never assigned, should be GC'd immediately
    B is assigned to cube, but then immediately replaced
    C is assigned to a local GGen that is never used
    */
    GCube temp;
    BoxGeometry A, B, C;
    cube.geo() @=> Geometry @ prevGeo;

    // B
    cube.geo(B);
    cube.geo(prevGeo);

    // C
    temp.geo(C);

    // pass frame once to propagate changes to render thread
    GG.nextFrame() => now;
}

fun void sporkShouldGC() {
    GCube temp --> GG.scene();
    SphereGeometry A;
    temp.geo(A);
    // shred exits, triggering temp and A to be GC'd
}

fun void shouldNotGC() {
    CircleGeometry A;
    A @=> out_of_shred_ref;
}

shouldNotGC();
cube.geo(out_of_shred_ref);

while (true) {
    <<< "gc geo " >>>;
    alwaysShouldGC();
    spork ~ alwaysShouldGC();
    spork ~ sporkShouldGC();
    GG.nextFrame() => now;
}
