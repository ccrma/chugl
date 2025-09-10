//-----------------------------------------------------------------------------
// name: video.ck
// desc: video playback; currently only supports the MPEG1 video and 
//       MP2 audio
// requires: ChuGL + chuck-1.5.5.5 or higher
//
// (DATA) download and place this music video in the same directory: 
//   https://chuck.stanford.edu/chugl/examples/data/video/bjork.mpg
//
// find more mpeg samples here:
//   https://filesamples.com/formats/mpeg
//
// authors: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//    date: Fall 2024
//-----------------------------------------------------------------------------

// fullscreen
GG.fullscreen();

// Video is a UGen
Video video( me.dir() + "bjork.mpg" ) => dac; 

// print file infomation
<<< "VM sample rate: ", 1::second / 1::samp >>>;
<<< "framerate: ", video.framerate() >>>;
<<< "samplerate: ", video.samplerate() >>>;
<<< "duration: ", video.duration() >>>;
<<< "loop: ", video.loop() >>>;
<<< "rate: ", video.rate() >>>;

// video texture
video.texture() @=> Texture texture;
(video.width() $ float) / video.height() => float video_aspect;

// array of geometries
[
    new PlaneGeometry,
    new SuzanneGeometry,
    new SphereGeometry,
    new CubeGeometry,
    new CircleGeometry,
    new TorusGeometry,
    new CylinderGeometry,
    new KnotGeometry,
] @=> Geometry geometries[];

// UI variable and list
UI_Int geometry_index(3);
[
    "PlaneGeometry",
    "SuzanneGeometry",
    "SphereGeometry",
    "CubeGeometry",
    "CircleGeometry",
    "TorusGeometry",
    "CylinderGeometry",
    "KnotGeometry",
] @=> string builtin_geometries[];

// material
FlatMaterial video_mat;
// negative: flip the y-axis
video_mat.scale(@(5, -5));

// connect video mesh to scene
GMesh video_mesh(geometries[geometry_index.val()], video_mat) --> GG.scene();

// set orbit camera as main camera
GOrbitCamera camera => GG.scene().camera;
camera.clip(.01, 1000);

// set mesh scaling
video_mesh.scaX(3 * video_aspect);
video_mesh.scaY(3);
video_mesh.scaZ(3);

// set color map for video texture
video_mat.colorMap( texture );

// UI variables
UI_Float rate(1.0);
UI_Bool loop(video.loop());
UI_Float2 copies(video_mat.scale());
UI_Float3 scale(video_mesh.sca());

// render loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;

    // check for key input
    if (GWindow.keyDown(GWindow.KEY_LEFT)) {
        video.seek(video.timestamp() - 10::second);
    } else if (GWindow.keyDown(GWindow.KEY_RIGHT)) {
        video.seek(video.timestamp() + 10::second);
    }

    // begin UI
    if (UI.begin(""))
    {
        // text
        UI.textWrapped("Use the arrow keys to seek 10 seconds back or forward.");
        // separator
        UI.separator();
        // playback rate
        if (UI.slider("Rate", rate, -2.0, 2.0)) {
            rate.val() => video.rate;
        }
        // listbox for builtin geometries
        if (UI.listBox("builtin geometries", geometry_index, builtin_geometries)) {
            // set selected geometry into mesh
            video_mesh.geometry(geometries[geometry_index.val()]);
        }
        // how many copies to draw
        if (UI.drag("copies", copies)) {
            copies.val() => video_mat.scale;
        }
        // scale of geometry
        if (UI.drag("scale", scale)) {
            scale.val() => video_mesh.sca;
        }
        // whether to loop
        if (UI.checkbox("loop", loop)) {
            loop.val() => video.loop;
        }
    }
    // end UI
    UI.end();
}
