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

// UI elements
UI_Float viewport_height_pixels(160);
UI_Float2 viewport_center_pixels(160, 160);
UI_Bool grid(false);
UI_Int brush_size(1);

// globals
GWindow.files() @=> string files[];
string save_path;
[
    new UI_Float4(@(52, 101, 213, 255) / 255.0),
    new UI_Float4(@(193, 235, 243, 255) / 255.0),
] @=> UI_Float4 color_palette[];
int palette_idx;

UI_Float4 brush_color(color_palette[0].val());
screen_pass.material().uniformFloat4(5, brush_color.val());

// set defaults
loadTexture("/Users/Andrew/Downloads/septembit.png");
screen_pass.material().sampler(1, TextureSampler.nearest());
screen_pass.material().uniformInt(2, viewport_height_pixels.val() $ int);
screen_pass.material().uniformInt2(3, viewport_center_pixels.val().x $ int, viewport_center_pixels.val().y $ int);
screen_pass.material().uniformInt(4, grid.val()); // grid;

// safe-save on exit
// TODO API for this of this can be better...see what Raylib does
GWindow.closeable( false ); 
fun void closeCallback() {
    while (true) {
        // listening for event closing event
        GWindow.closeEvent() => now;

        if (save_path != null && save_path.length() > 0) {
            <<< "saving to:", save_path >>>;
            screen_pass.material().texture(0).save(save_path) @=> TextureSaveEvent e;
            e => now;
            if (e.error) <<< "Could not save to", save_path >>>;
            else {
                <<< "saved" >>>;
                GWindow.close();
            }
        }
    }
}
// spork the window closing
spork ~ closeCallback();

fun void loadTexture(string path) {
    screen_pass.material().texture(0, Texture.load(path));
    if (save_path == null || save_path.length() == 0) {
        path => save_path;
    }
}

fun vec2 mouseCell() {
    GWindow.framebufferSize() => vec2 resolution;
    resolution.x / resolution.y => float aspect;

    GWindow.mousePos() => vec2 mouse;
    GWindow.contentScale().x *=> mouse.x;
    GWindow.contentScale().y *=> mouse.y;
    @(mouse.x / resolution.x, mouse.y / resolution.y) => mouse; // normalize to [0,1]

    @(0.5, 0.5) -=> mouse;    // scale to [-0.5, 0.5]
    aspect *=> mouse.x;       // aspect adjust
    Math.floor(viewport_height_pixels.val()) *=> mouse; // scale to pixels
    @(
        Math.floor(viewport_center_pixels.val().x),
        Math.floor(viewport_center_pixels.val().y)
    ) +=> mouse;

    return mouse;
}

fun void writePixel(vec4 color) {
    mouseCell() => vec2 cell;
    screen_pass.material().texture(0) @=> Texture tex;
    TextureWriteDesc desc;
    brush_size.val() => desc.width;
    brush_size.val() => desc.height;
    cell.x $ int => desc.x;
    cell.y $ int => desc.y;
    float cols[0];
    repeat (brush_size.val() * brush_size.val()) {
        cols << color.r << color.g << color.b << color.a;
    }
    tex.write(cols, desc);
}

while (1) {
    GG.nextFrame() => now;

    if (GWindow.files() != files) {
        GWindow.files() @=> files;
        loadTexture(files[0]);
    }

    // input
    GWindow.key(GWindow.KEY_LEFTCONTROL) || GWindow.key(GWindow.KEY_LEFTSUPER) => int mod_key;

    if (GWindow.keyDown(GWindow.KEY_TAB)) {
        (palette_idx + 1) % color_palette.size() => palette_idx;
        color_palette[palette_idx].val() => brush_color.val;
        screen_pass.material().uniformFloat4(5, brush_color.val());
    }

    if ((mod_key && GWindow.mouseLeft()) || GWindow.mouseRight()) { // drag
        viewport_center_pixels.val() - 0.25 * GWindow.mouseDeltaPos() => viewport_center_pixels.val;
    }
    if (GWindow.keyDown(GWindow.KEY_G)) !grid.val() => grid.val;
    if (GWindow.keyDown(GWindow.KEY_EQUAL)) brush_size.val() + 1 =>  brush_size.val;
    if (GWindow.keyDown(GWindow.KEY_MINUS)) Math.max(brush_size.val() - 1, 1) =>  brush_size.val;

    if (!mod_key && GWindow.mouseLeft()) writePixel(brush_color.val());

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

        UI.separatorText("Palette");
        for (int i; i < color_palette.size(); ++i) {
            UI.pushID(i);
            UI.colorEdit("Color", color_palette[i]);
            UI.popID();
        }
        if (UI.button("+")) {
            color_palette << new UI_Float4;
        }
        UI.sameLine();
        if (UI.button("-")) {
            color_palette.popBack();
        }

        UI.inputInt("brush size", brush_size);


        UI.text("Current save path: " + save_path);
        if (UI.button("save")) {
            screen_pass.material().texture(0).save(save_path);
        }

        if (UI.button("save as")) {
            GG.saveFileDialog(screen_pass.material().texture(0).name()) => string save_fp;
            if (save_fp != null) { // TODO Textures.save(null) should default to timestamp in cwd
                save_fp => save_path;
                screen_pass.material().texture(0).save(save_fp);
            }
        }
    }
    UI.end();
}
