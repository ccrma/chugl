// texture mapping example
<<< "initializing Texture..." >>>;
FileTexture tex;

// load textures
tex.path(me.dir() + "data/chuck-logo.png");
<<< "loading:", tex.path() >>>;

// make a scen
GScene scene;
// make a mesh GGen
GMesh mesh;
// make a geometry
BoxGeometry boxGeo;
// make a material
PhongMaterial phong;
// attach texture to material
phong.diffuseMap( tex );

// attach geometry and material
mesh.set( boxGeo, phong );
// connect to scene
mesh --> scene;
// move mesh 
mesh.posZ( -3 );

// time loop
while( true )
{
    // rotate the thing
    mesh.rotateY( GG.dt() );
    // wait until next frame
    GG.nextFrame() => now;
}
