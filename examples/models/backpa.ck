//-----------------------------------------------------------------------------
// name: backpack.ck
// desc: multi-format asset loader, backpack model
//
// download model:
//   https://chuck.stanford.edu/chugl/examples/assets/backpack.zip
//
// requires: ChuGL + chuck-1.5.1.8 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// fullscreen
// GG.fullscreen();
// windowed
// GG.windowed( 3200, 1800);

// set window title
GG.windowTitle( "ChuGL asset loading + rendering demo" );

// load asset
AssLoader.load( me.dir() + "../assets/backpack/backpack.obj" ) @=> GGen backpack;

// connect to scene
backpack --> GG.scene();
// move title
backpack.posZ(-2);

// time loop
while (true)
{
    <<< GG.fps() >>>;
    // rotate 
    GG.dt() => backpack.rotateY;
    // next frame
    GG.nextFrame() => now; 
}
