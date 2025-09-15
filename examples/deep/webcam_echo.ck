//-----------------------------------------------------------------------------
// name: webcam_echo.ck
// desc: create an echo/waterfall video effect using webcam, suing
// multiple textures and texture copying
//
// authors: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//    date: Fall 2024
//-----------------------------------------------------------------------------

// create a ChuGL webcam
Webcam webcam;
// set camera to orthographic projection
GG.camera().orthographic();

// geometry
PlaneGeometry geo;
// material
FlatMaterial plane_mats[9];

// function to place a row of delayed webcam textures
fun void positionPlanes(float y_off, int flip_x) {
    for (int i; i < plane_mats.size(); i++) {
        // add plane to scene
        GMesh plane(geo, plane_mats[i]) --> GG.scene();
        // position plane
        plane.pos(@(flip_x * (i - 4), y_off, 0));
        // set material
        plane.mat(plane_mats[i]);
        // set geometry
        plane.geo(geo);
    }
}

// position planes, one call for each row
// positionPlanes(-3, -1);
positionPlanes(-2, 1);
positionPlanes(-1, -1);
positionPlanes(0, 1);
positionPlanes(1, -1);
positionPlanes(2, 1);
// positionPlanes(3, -1);

// initialize our texture "delay line" to buffer webcam frames
TextureDesc desc;
// do NOT generate mips, otherwise will be invisible because we are only copying to mip level 0
false => desc.mips;
// webcame dimensions
webcam.width() => desc.width;
webcam.height() => desc.height;
// array of textures
Texture echo_textures(desc)[30 * plane_mats.size()];
// index
0 => int curr_texture;

// game/render loop
while (true)
{
    // copy every 2 frames (webcam fps is typically 30 not 60)
    GG.nextFrame() => now; GG.nextFrame() => now;

    // update delayed webcam textures
    if (curr_texture >= echo_textures.size()) 0 => curr_texture;
    for (int i; i < plane_mats.size(); i++) {
        echo_textures[(curr_texture + 30 * (i + 1)) % echo_textures.size()] => plane_mats[i].colorMap;
    } 

    // copy newest frame from webcam
    Texture.copy(echo_textures[curr_texture], webcam.texture());
    // increment index
    curr_texture++;
}
 