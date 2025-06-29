GG.rootPass() --> ScenePass sp(GG.scene());

FlatMaterial mat;
PlaneGeometry geo;

TextureLoadDesc tex_load_desc;
false => tex_load_desc.gen_mips; 
Texture.load(me.dir() + "./chugl-tests/flower_petals.png", tex_load_desc) @=> Texture@ sprite_tex;
mat.colorMap(sprite_tex);

GMesh mesh(geo, mat) --> GG.scene();

while (1) {
    GG.nextFrame() => now;
}