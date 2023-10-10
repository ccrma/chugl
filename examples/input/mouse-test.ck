// Mouse test
// need to run Mouse.ck before this (or run go.ck)

Mouse mouse;
// open mouse device 0
spork ~ mouse.start(0);

// keep it going
while( true )
{
    <<< mouse.deltas() >>>;
    
    // every so often!
    50::ms => now;
}
