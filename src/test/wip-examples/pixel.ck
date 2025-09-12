ShaderDesc desc;
me.dir() + "./pixel_shader.wgsl" => desc.vertexPath;
me.dir() + "./pixel_shader.wgsl" => desc.fragmentPath;
null => desc.vertexLayout;

Shader shader(desc);
shader.name("screen shader");

Texture pixel;
pixel.write([0.1, .1, .1, 1]);

// render graph
GG.rootPass() --> ScreenPass screen_pass(shader);

UI_Float4 viewport_normalized(0., 0., 1., 1.);
UI_Float4 viewport_absolute(0., 0., 200., 200.);
UI_Float4 scissor_normalized(0., 0., 1., 1.);
UI_Float4 scissor_absolute(0., 0., 200., 200.);

// UI elements
UI_Float viewport_height_pixels(160);
UI_Float2 viewport_center_pixels(160, 160);
UI_Bool grid(false);
UI_Float4 brush_color(@(1.0, 0.0, 0.0, 1.0));

// set defaults
TextureSampler.nearest() @=> TextureSampler sampler;
// TextureSampler.WRAP_CLAMP => sampler.wrapU;
// TextureSampler.WRAP_CLAMP => sampler.wrapV;
screen_pass.material().texture(0, pixel);
screen_pass.material().texture(0, Texture.load("/Users/Andrew/Downloads/septembit.png"));
screen_pass.material().sampler(1, sampler);
screen_pass.material().uniformInt(2, viewport_height_pixels.val() $ int);
screen_pass.material().uniformInt2(3, viewport_center_pixels.val().x $ int, viewport_center_pixels.val().y $ int);
screen_pass.material().uniformInt(4, grid.val()); // grid;
screen_pass.material().uniformFloat4(5, brush_color.val());

fun vec2 mouseCell() {
    GWindow.framebufferSize() => vec2 resolution;
    resolution.x / resolution.y => float aspect;
    aspect * viewport_height_pixels.val() => float viewport_width_cells;

    GWindow.mousePos() => vec2 mouse;
    GWindow.contentScale().x *=> mouse.x;
    GWindow.contentScale().y *=> mouse.y;
    
    resolution.y $ float / viewport_height_pixels.val() => float pixels_per_cell;
    @(
        // Math.floor((mouse.x + 0.5) / pixels_per_cell),
        // Math.floor((mouse.y + 0.5) / pixels_per_cell)
        ((mouse.x + 0.5) / pixels_per_cell),
        ((mouse.y + 0.5) / pixels_per_cell)
    ) => vec2 cell;

    // compute the texture coordinate of the upper left cell 
    viewport_center_pixels.val() - @(.5 * viewport_width_cells - 0.0, .5 * viewport_height_pixels.val() - 0.0) => vec2 upper_left;
    upper_left +=> cell;

    <<< resolution, mouse, pixels_per_cell, cell >>>;

    return cell;
}

fun void writePixel(vec4 color) {
    mouseCell() => vec2 cell;
    screen_pass.material().texture(0) @=> Texture tex;
    TextureWriteDesc desc;
    1 => desc.width;
    1 => desc.height;
    cell.x $ int => desc.x;
    cell.y $ int => desc.y;
    tex.write([color.r, color.g, color.b, color.a], desc);
}

GWindow.files() @=> string files[];
while (1) {
    GG.nextFrame() => now;


    if (GWindow.files() != files) {
        GWindow.files() @=> files;

        screen_pass.material().texture(0, Texture.load(files[0]));
    }

    // input
    if (GWindow.key(GWindow.KEY_LEFTCONTROL) || GWindow.key(GWindow.KEY_LEFTSUPER)) {
    } 
    if (GWindow.mouseRight()) { // drag
        viewport_center_pixels.val() - 0.25 * GWindow.mouseDeltaPos() => viewport_center_pixels.val;
    }
    if (GWindow.keyDown(GWindow.KEY_G)) !grid.val() => grid.val;
    if (GWindow.mouseLeftDown()) writePixel(brush_color.val());
    viewport_height_pixels.val() - GWindow.scrollY() => viewport_height_pixels.val;


    UI.setNextWindowBgAlpha(0.1);
    if (UI.begin("")) {
        (UI.drag("Viewport Height (pixels)", viewport_height_pixels));
        screen_pass.material().uniformInt(2, viewport_height_pixels.val() $ int);

        if (UI.drag("Viewport Center (pixels)", viewport_center_pixels));
        screen_pass.material().uniformInt2(3, viewport_center_pixels.val().x $ int, viewport_center_pixels.val().y $ int);

        (UI.checkbox("Grid (G to toggle)", grid));
        screen_pass.material().uniformInt(4, grid.val());

        if (UI.colorEdit("Color", brush_color)) screen_pass.material().uniformFloat4(5, brush_color.val());

        if (UI.button("save")) {
            screen_pass.material().texture(0).save("/Users/Andrew/Downloads/pixel_saved.png");
        }



        // if (UI.drag("viewport (normalized)", viewport_normalized, .01)) {
        //     screen_pass.viewportNormalized(viewport_normalized.val());
        // }
        // if (UI.drag("viewport (in pixels)", viewport_absolute)) {
        //     screen_pass.viewport(viewport_absolute.val());
        // }
        // if (UI.drag("scissor (normalized)", scissor_normalized, .01)) {
        //     screen_pass.scissorNormalized(scissor_normalized.val());
        // }
        // if (UI.drag("scissor (in pixels)", scissor_absolute)) {
        //     screen_pass.scissor(scissor_absolute.val());
        // }
    }
    UI.end();
}
