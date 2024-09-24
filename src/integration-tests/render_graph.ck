PlaneGeometry plane_geo;
FlatMaterial flat_material;
GMesh mesh(plane_geo, flat_material) --> GG.scene();

(GG.rootPass().next() $ RenderPass) @=> RenderPass @ render_pass;
(render_pass.next() $ OutputPass) @=> OutputPass @ output_pass;

// TODO add render_texture class that takes these defaults
Texture render_texture(Texture.Usage_RenderAttachment | Texture.Usage_TextureBinding, Texture.Dimension_2D, Texture.Format_RGBA16Float);
render_pass.target(render_texture);
output_pass.inputTexture(render_texture);


while (true) {
    GG.nextFrame() => now;

    UI.showDemoWindow(null);

}