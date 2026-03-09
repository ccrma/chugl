@import "../../lib/g2d/ChuGL.chug"
@import "../../lib/g2d/g2d.ck"
@import "../../lib/spring.ck"
@import "../../lib/M.ck"
@import "../../lib/T.ck"

G2D g;

GText.defaultFont("chugl:proggy-tiny");

TextureLoadDesc load_desc;
false => load_desc.gen_mips;
true => load_desc.flip_y;
Texture.load(me.dir() + "./assets/cooking-icon.png", load_desc) @=> Texture cooking_icon_tex;
Texture.load(me.dir() + "./assets/friend-icon.png", load_desc) @=> Texture friend_icon_tex;
Texture.load(me.dir() + "./assets/grass-icon.png", load_desc) @=> Texture grass_icon_tex;
Texture.load(me.dir() + "./assets/hike-icon.png", load_desc) @=> Texture hike_icon_tex;
Texture.load(me.dir() + "./assets/meditation-icon.png", load_desc) @=> Texture meditation_icon_tex;

[
    null,
    grass_icon_tex,
    friend_icon_tex,
    hike_icon_tex,
    cooking_icon_tex,
    meditation_icon_tex,
] @=> Texture resource_textures[];

UI_Float font_pixel_sz(.0625); // worldspace size of 1 pixel of font
UI_Int font_h_pixels(10); // height in font-space pixels of 1 glyph
UI_Int font_w_pixels(5);  // width  "
UI_Int font_inner_spacing_pixels(1);  // number of pixels between each glyph of a word
UI_Float font_size(2.0);

UI_Int sprite_size_pixels(10);

UI_Float2 pos;
UI_Float2 cp(.5, .5);

// max pixel height is 10px
// .625 / 10 = .0625 worldspace units per font pixel (at fontsize = 1)
// pixel width is 5 px
// 5 * .0625 = .3125
// when sequencing letters, there is 1 pixel gap within same word.
// given N letter string, total pixel width is (len(N) * pixels_per_char_w) + (len(N) - 1) * pixels_per_pad
// in the case of proggy font, this simplifies to (6 * len(N) - 1) * font_pixel_width

UI_String text_input("A");

int cost[6];

fun vec2 textBBox(string text) {
    font_pixel_sz.val() * font_h_pixels.val() => float glyph_h;
    font_pixel_sz.val() * font_inner_spacing_pixels.val() => float glyph_spacing_w;
    font_pixel_sz.val() * font_w_pixels.val() => float glyph_w;
    font_size.val() * glyph_h => float h;
    text_input.val().length() => float len;

    font_size.val() * ((len * glyph_w) + (len - 1) * glyph_spacing_w)
        => float w;

    return @(w, h);
}

fun void costButton(int cost[], vec2 pos, vec2 cp) {
    vec2 bb;
    font_size.val() * font_pixel_sz.val() * font_h_pixels.val() => bb.y;

    font_size.val() * font_pixel_sz.val() * font_inner_spacing_pixels.val() => float glyph_spacing_w;
    font_size.val() * font_pixel_sz.val() * font_w_pixels.val() => float glyph_w;

    font_size.val() * sprite_size_pixels.val() * font_pixel_sz.val() => float sprite_sz;

    int num_resource_types;
    for (auto c : cost) {
        if (c <= 0) continue;
        (c + "").length() => int N;
        ((N * glyph_w) + (N - 1) * glyph_spacing_w) +=> bb.x;
        glyph_spacing_w + 2 * glyph_w +=> bb.x;
        ++ num_resource_types;
    }
    // add padding between each resource type
    Math.max(0, (num_resource_types - 1)) * glyph_w +=> bb.x;

    // adjust pos for cp
    // pos is the *center* position of the entire display string
    (2 * cp.x - 1) * .5 * bb.x -=> pos.x;
    (2 * cp.y - 1) * .5 * bb.y -=> pos.y;

    // draw background button
    .1 => float pad; // TODO switch to global pad setting
    g.boxFilled(pos, bb.x + 2 * pad, bb.y + 2 * pad, Color.GRAY);

    // draw starting from left edge
    @(
        pos.x - .5 * bb.x,
        pos.y
    ) => vec2 cursor;
    // add tiny margin to text to match icon art padding
    // .3 * font_size.val() * font_pixel_sz.val() +=> cursor.x;

    // now actually draw it
    // (assumes centered at @(0,0)
    g.pushTextControlPoint(0, .5);
	g.pushFontSize(font_size.val());
    g.pushLayer(g.layer() + .01);

    int sprites_drawn;
    for (int i; i < cost.size(); ++i) {
        cost[i] => int c;
        if (c <= 0) continue;
        c + "" => string cost_str;
        cost_str.length() => int N;
        g.text(cost_str, cursor);

        ((N * glyph_w) + (N - 1) * glyph_spacing_w) +=> cursor.x;
        glyph_spacing_w + glyph_w +=> cursor.x;
        g.sprite(resource_textures[i], cursor, sprite_sz);
        glyph_w +=> cursor.x;

        // add spacing if not last
        if (sprites_drawn < num_resource_types)
            glyph_w +=> cursor.x;
    }

    g.popLayer();
    g.popFontSize();
    g.popTextControlPoint();
}


while (1) {
    GG.nextFrame() => now;

    // UI.inputText("input", text_input);

    UI.slider("cost", cost, 0, 100);
    UI.drag("pos", pos, .01);
    UI.drag("cp", cp, .01, 0, 1, "%.1f", 0);
    UI.slider("font pixel sz", font_pixel_sz, 0, .1);
    UI.slider("font size", font_size, 0, 10);

    g.pushLayer(2);
    g.circleFilled(pos.val(), .1, Color.RED);
    g.popLayer();

    costButton(cost, pos.val(), cp.val());

    // font_size.val() * font_pixel_sz.val() * font_h_pixels.val() => float glyph_h;
    // font_size.val() * font_pixel_sz.val() * font_inner_spacing_pixels.val() => float glyph_spacing_w;
    // font_size.val() * font_pixel_sz.val() * font_w_pixels.val() => float glyph_w;
    // glyph_h => float h;
    // text_input.val().length() => float len;
    // ((len * glyph_w) + (len - 1) * glyph_spacing_w) => float w;
    // font_size.val() * sprite_size_pixels.val() * font_pixel_sz.val() => float sprite_sz;

    // g.pushTextControlPoint(cp.val());
    // g.text(text_input.val(), pos.val(), font_size.val());
    // textBBox(text_input.val()) => vec2 bb;
    // g.box(
    //     pos.val(), 
    //     bb.x, bb.y,
    //     Color.WHITE
    // );

}