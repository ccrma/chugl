// window size
1024 => int WINDOW_SIZE;
// 1024 => int WATERFALL_DEPTH;
512 => int WATERFALL_DEPTH;
// accumulate samples from mic
adc => Flip accum => blackhole;
// take the FFT
adc => PoleZero dcbloke => FFT fft => blackhole;
// set DC blocker
.95 => dcbloke.blockZero;
// set size of flip
WINDOW_SIZE => accum.size;
// set window type and size
Windowing.hann(WINDOW_SIZE) => fft.window;
// set FFT size (will automatically zero pad)
WINDOW_SIZE*2 => fft.size;
// get a reference for our window for visual tapering of the waveform
Windowing.hann(WINDOW_SIZE) @=> float window[];
// sample array
float samples[0];
// FFT response
complex response[0];
// mapped FFT response
float spectrum[WINDOW_SIZE];

// audio loop (runs every 1/2 window size)
fun void doAudio()
{
    while( true )
    {
        // upchuck to process accum
        accum.upchuck();
        // get the last window size samples (waveform)
        accum.output( samples );
        // upchuck to take FFT, get magnitude response
        fft.upchuck();
        // get spectrum (as complex values)
        fft.spectrum( response );
        // jump by samples
        WINDOW_SIZE::samp/2 => now;
    }
}
spork ~ doAudio();

// map FFT output to scalar values
fun void map2spectrum( complex in[], float out[] )
{
    if( in.size() != out.size() )
    {
        <<< "size mismatch in map2spectrum()", "" >>>;
        return;
    }
    
    // mapping to scalar value
    for (int i; i < in.size(); i++)
    {
        // map frequency bin magnitude
        25 * Math.sqrt( (in[i]$polar).mag ) => out[i];
    }
}

// Initialize our spectrum height map texture
TextureDesc spectrum_texture_desc;
WINDOW_SIZE => spectrum_texture_desc.width;
WATERFALL_DEPTH => spectrum_texture_desc.height;
Texture.Format_R32Float => spectrum_texture_desc.format; // single channel float (to hold spectrum data)
1 => spectrum_texture_desc.mips; // no mipmaps

Texture spectrum_texture(spectrum_texture_desc);

 // initialize TextureWrite params for writing spectrum data to texture
TextureWriteDesc write_desc;
WINDOW_SIZE => write_desc.width;

// Create our custom audio shader
ShaderDesc shader_desc;
me.dir() + "/audio_terrain.wgsl" => shader_desc.vertexFilepath;
me.dir() + "/audio_terrain.wgsl" => shader_desc.fragmentFilepath;
Shader terrain_shader(shader_desc); // create shader from shader_desc

// Apply the shader to a material
Material terrain_material;
terrain_shader => terrain_material.shader;
// assign the spectrum texture to the material
terrain_material.texture(0, spectrum_texture);
terrain_material.uniformInt(1, 0); // initialize playhead to 0
terrain_material.uniformVec3(2, Color.WHITE);
terrain_material.topology(Material.Topology_LineList);

// create our terrain mesh
// PlaneGeometry plane_geo(
//     10,  // width
//     10,  // height
//     spectrum_texture.width(), // width segments
//     spectrum_texture.height()// height segments
// );
TorusGeometry torus_geo(
    5,  // radius
    2,  // tube radius
    spectrum_texture.height(), // radial segments
    spectrum_texture.width(), // tubular segments
    Math.PI * 2 // arc length
);

// GMesh terrain_mesh(plane_geo, terrain_material) --> GG.scene();
// terrain_mesh.rotateX(-Math.PI/2);
GMesh terrain_mesh(torus_geo, terrain_material) --> GG.scene();

// camera
// GWindow.mouseMode(GWindow.MouseMode_Disabled);
// GFlyCamera cam --> GG.scene();
GOrbitCamera cam --> GG.scene();
GG.scene().camera(cam);
cam.posZ(10);
cam.posY(10);

// game loop (runs at frame rate) ============================================

UI_Float3 terrain_color(Color.WHITE);
while (true) {
    GG.nextFrame() => now;
    // map FFT response to scalar values
    map2spectrum( response, spectrum );
    // write to texture
    {
        spectrum_texture.write( spectrum, write_desc );

        // update the playhead
        terrain_material.uniformInt(1, write_desc.y);

        // bump the row we write to next frame
        (write_desc.y + 1) % WATERFALL_DEPTH => write_desc.y;
    }

    if (UI.begin("Audio Terrain")) {
        UI.scenegraph(GG.scene());

        if (UI.colorEdit("Terrain Color", terrain_color, 0)) {
            terrain_material.uniformVec3(2, terrain_color.val());
        }
    }
    UI.end();
}