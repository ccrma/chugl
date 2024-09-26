//-----------------------------------------------------------------------------
// name: gameloop.ck
// desc: basic ChuGL rendering/game loop
// requires: ChuGL + chuck-1.5.1.5 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// uncomment to run in fullscreen

// some variables for printing time; not needed for game loop
0 => int fc;
now => time lastTime;

fun void gameloop()
{
    while (true) {
        GG.nextFrame() => now;
    }
} spork ~ gameloop();


// infinite time loop
while( true )
{
    // IMPORTANT: synchronization point with next frame to render
    // must be called at the beginning of every frame,
    // so the chuck VM can properly mark this shred as a graphics shred
    GG.nextFrame() => now;
}
