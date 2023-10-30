// Test garbage collection on Material
// Correct behavior is seeing a single blue square

GCube cube --> GG.scene();

Material @ out_of_shred_ref;

// Cases where material should GC if the function is either
// sporked OR called normally
fun void alwaysShouldGC() {
    /*
    A is never assigned, should be GC'd immediately
    B is assigned to cube, but then immediately replaced
    C is assigned to a local GGen that is never used
    */
    GCube temp;
    PhongMaterial A, B, C;
    cube.mat() @=> Material @ prevMat;

    // B
    cube.mat(B);
    cube.mat(prevMat);

    // C
    temp.mat(C);

    // pass frame once to propagate changes to render thread
    GG.nextFrame() => now;
}

fun void sporkShouldGC() {
    GCube temp --> GG.scene();
    PhongMaterial A;
    temp.mat(A);
    // shred exits, triggering temp and A to be GC'd
}

fun void shouldNotGC() {
    NormalsMaterial A;
    A @=> out_of_shred_ref;
}

shouldNotGC();
cube.mat(out_of_shred_ref);

while (true) {
    alwaysShouldGC();
    spork ~ alwaysShouldGC();
    spork ~ sporkShouldGC();
    GG.nextFrame() => now;
}