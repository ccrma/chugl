/*

Andrew and Ge's average sunday night chugling-------

This tests the following:
1. The auto-update invoker is correctly getting the origin_shred from the s_GGen2OriginShred map
This handles if the shred that created a GGen exits, but that shred lives on via an external reference
( see ulib_cgl.h for more context )
2. Tests that the s_GGen2OriginShred map is correctly being erased by GGen destructor

*/

GCube @ g;

fun void foo() {
    if (g != null) {
        g --< GG.scene();
    }
    new GCube @=> g;   // by default won't be connected to GG.scene();
    g --> GG.scene();
    GG.nextFrame() => now;
}

while( true )
{
    spork ~ foo(); 
    me.yield(); // yield to let shred run
    <<< "next frame" >>>;
    GG.nextFrame() => now;
}