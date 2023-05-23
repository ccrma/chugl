// -------------------------------------------------------------
// name: gg-brainstorm.ck
// desc: code we might like to write for ChuGL using GG
//       Graphics Generator (GG)
//
// author: Andrew Aday Zhu
//         Ge Wang
// date: spring 2023 | 2023.5.22
// -------------------------------------------------------------

// finally using the -> operator...the 'GG' operator!
// note: GGs cannot have more than one parent
//       -> to a second parent GG will "deparent"
// note: the un-GG (-<) opeartor explicitly deparent a GG
// ---
// option 1 (maybe this one)
// 'groot' == graphics root
// moon -> sun -> earth -> groot;
// ---
// option 1a alternate (maybe confusing)
// groot <- earth <- sun <- moon;
// ---
// option 2
// groot -> sun -> earth -> moon;

// instantiate cube graphics generators
GGCube cubes[50][50];
// option: a group is an empty GG
// GGGroup groupOfGGs;

// load texture; need constructors!!!
GGTexture.load("chuck-logo.png") @=> GGTexture @ texture;

// loop over cube GGs
for( int i; i < 50; i++ )
{
    for( int j; j < 50; j++ )
    {
        // connect each cube to the graphics root
        cubes[i][j] -> groot;
        // set location
        i => cubes[i][j].pos.x;
        j => cubes[i][j].pos.y;

        // set color
        // cubes[i][j].ambientColor( @(1,1,1) );
        // cubes[i][j].diffuseColor( @(1,1,1) );
        // cubes[i][j].specularColor( @(1,1,1) );

        // set material
        // cubes[i][j].material( GGMaterial.Phong );
        // loading a texture
        cubes[i][j].material.diffuseMap( texture );

        // set position
        cubes[i][j].pos.set( i, j, 0 );

        // option 1: spawn one animating shred per cube at this point
        spork ~ cubeAnimator( cubes[i] );
        // or option 2: can do in a lump later
    }
}

SinOsc foo => NRev r => dac;

// create a light
GGLight.createDirectional( eye, color ) @=> GGLight @ light1;
GGLight.createPositional( at, color ) @=> GGLight @ light2;
// add light to scene graph
light1 -> groot;
light2 -> groot;
// set position of light2
light2.pos.set( 10, 10, 10 );

// option 1: one animator per GG
fun void cubeAnimator( GGCube @ cube )
{
    // frame is an Event
    while( frame => now )
    {
        // rotate about Y axis
        now / second => cube.rot.y;
        // option 2: dur CGL.delta() for throttling animation rates
        // question: should we go with ChuGL
        CGL.delta() * 5 => cube.rot.y;
    }
}

// option 2: animate as a lump
// basically an infinite time loop that updates each cube at frame => now

// audio: exactly same as before, but now naturally interleaved with graphicss
while( true )
{
    Math.random2f(30,1000) => foo.freq;
    100::ms => now;
}
