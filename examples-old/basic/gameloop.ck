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
// GG.fullscreen();

// some variables for printing time; not needed for game loop
0 => int fc;
now => time lastTime;

// infinite time loop
while( true )
{
    // compute our own delta time for printing
    now - lastTime => dur dt;
    // remember now as last time
    now => lastTime;
    // print
    <<< "fc:", fc++ , "now:", now, "dt:", dt, "fps:", GG.fps() >>>;

    // IMPORTANT: synchronization point with next frame to render
    GG.nextFrame() => now;
}
