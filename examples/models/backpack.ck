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

// load asset
AssLoader.load( me.dir() + "../assets/backpack/backpack.obj" ) @=> GGen backpack;

// connect to scene
backpack --> GG.scene();

// time loop
while (true)
{
    // rotate 
    GG.dt() => backpack.rotateY;
    // next frame
    GG.nextFrame() => now; 
}
