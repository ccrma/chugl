TextureLoadDesc tex_load_desc;
true => tex_load_desc.flip_y;
false => tex_load_desc.gen_mips;
Texture.load(me.dir() + "./assets/bard.bmp", tex_load_desc) @=> Texture bard_sprite;
while (1) {
    GG.nextFrame() => now;
}