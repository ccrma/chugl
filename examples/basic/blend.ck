//--------------------------------------------------------------------
// name: blend.ck
// desc: additive blending example
// requires: ChuGL 0.2.8 (alpha) + chuck-1.5.5.6 or higher
//
// author: Ge Wang and Andrew Zhu Aday
//   date: Fall 2025
//--------------------------------------------------------------------

// assuming a compatible path (change as needed)
Texture.load(me.dir() + "../data/textures/artful-design/flare-tng-1.png") @=> Texture flare1_tex;
Texture.load(me.dir() + "../data/textures/artful-design/flare-tng-5.png") @=> Texture flare2_tex;

// flare enum
0 => int FLARE_1;
1 => int FLARE_2;

// all flares are rendered on quads, can share the same plane geometry
PlaneGeometry geo;

// create a new flare as a GMesh GGen
fun GMesh addFlare( Texture tex, vec3 color)
{
    // material
    FlatMaterial mat;
    // make a mesh GGen using mat and the shared geo
    GMesh flare(geo, mat);

    // apply texture to material
    mat.colorMap(tex);
    // set base color
    mat.color(color);
    // set material blend mode to additive blending
    mat.blend(Material.BLEND_MODE_ADD);

    return flare;
}

// initialize flares as 3 GGens
addFlare( flare2_tex, Color.RED ) @=> GMesh flare1;
addFlare( flare2_tex, Color.GREEN ) @=> GMesh flare2;
addFlare( flare2_tex, Color.BLUE ) @=> GMesh flare3;

// gruck the GGens into the scene graph
flare1 --> GG.scene();
flare2 --> GG.scene();
flare3 --> GG.scene();

// render loop
while( true )
{
    // synch audio and graphics
    GG.nextFrame() => now;

    // move two of the flares
    Math.sin(now/second) => flare1.posX;
    -Math.sin(now/second) => flare3.posX;
}
