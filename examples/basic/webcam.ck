//-----------------------------------------------------------------------------
// name: webcam.ck
// desc: rendering the webcam in scene!
//
// authors: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//    date: Fall 2024
//-----------------------------------------------------------------------------

// create a webcam object by deviceID, width, height, fps
Webcam webcam( 0, 1280, 720, 60 );
<<< "webcam width: ", webcam.width() >>>;
<<< "webcam height: ", webcam.height() >>>;
<<< "webcam fps: ", webcam.fps() >>>;
<<< "webcam name: ", webcam.deviceName() >>>;

// material
FlatMaterial plane_mat;
// map webcam input as color map
plane_mat.colorMap(webcam.texture());
// geometry
PlaneGeometry plane_geo;
// connect mesh GGen to scene
GMesh plane(plane_geo, plane_mat) --> GG.scene();

// scale X and Y
plane.scaX(3 * webcam.aspect()); plane.scaY(3);

// UI variable
UI_Bool capture( webcam.capture() );

// render loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;

    // begin UI
    if (UI.begin("Webcam Example")) {
        if (UI.checkbox("Capture", capture)) {
            webcam.capture(capture.val());
        }
    }
    // end UI
    UI.end();
}
