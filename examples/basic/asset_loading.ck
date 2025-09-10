//--------------------------------------------------------------------
// name: asset_loading.ck
// desc: example of loading assets (e.g., 3D models) from file
//       and rendering as a GGen
//--------------------------------------------------------------------

// using asset loader, load from file, returns a GGen as the root of a model graph
AssLoader.loadObj( me.dir() + "../data/models/suzanne.obj" ) @=> GGen model;

// connect model to scene
model --> GG.scene();

// for a more extensive model, download this asset:
//   https://chuck.stanford.edu/chugl/examples/data/models/backpack.zip
// AssLoader.loadObj( me.dir() + "backpack/backpack.obj" ) @=> GGen model2;
// connect model to scene
// model2 --> GG.scene();

// render loop
while( true )
{
    // sync with next graphics frame
    GG.nextFrame() => now;
    // rotate
    -GG.dt() => model.rotateY;
}
