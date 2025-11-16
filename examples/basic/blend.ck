//--------------------------------------------------------------------
// name: blend.ck
// desc: 
// requires: ChuGL 0.2.8 (alpha) + chuck-1.5.5.6 or higher
//
// author: Ge Wang and Andrew Zhu Aday
//   date: Fall 2025
//--------------------------------------------------------------------

Texture.load(me.dir() + "../data/textures/artful-design/flare-tng-1.png") @=> Texture flare1_tex;
Texture.load(me.dir() + "../data/textures/artful-design/flare-tng-5.png") @=> Texture flare2_tex;

// flare enum
0 => int FLARE_1;
1 => int FLARE_2;

PlaneGeometry geo; // all flares are rendered on quads, can share the same plane geometry
fun GMesh addFlare(int which, vec3 color) {
    FlatMaterial mat;
    GMesh flare(geo, mat);

    // apply texture to material
    if (which == FLARE_1) mat.colorMap(flare1_tex);
    if (which == FLARE_2) mat.colorMap(flare2_tex);

    // set base color
    mat.color(color);

    // set material blend mode to additive blending
    mat.blend(Material.BLEND_MODE_ADD);

    return flare;
}

// initialize flares
addFlare(FLARE_2, Color.RED) @=> GMesh flare1;
addFlare(FLARE_2, Color.GREEN) @=> GMesh flare2;
addFlare(FLARE_2, Color.BLUE) @=> GMesh flare3;

flare1 --> GG.scene(); 
flare2 --> GG.scene(); 
flare3 --> GG.scene(); 

// render loop
while (true) {
    GG.nextFrame() => now;
    Math.sin(now/second) => flare1.posX;
    -Math.sin(now/second) => flare3.posX;
}