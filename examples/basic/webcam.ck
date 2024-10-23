Webcam webcam;
<<< "webcam width: ", webcam.width() >>>;
<<< "webcam height: ", webcam.height() >>>;
<<< "webcam fps: ", webcam.fps() >>>;

FlatMaterial plane_mat;
plane_mat.colorMap(webcam.texture());
PlaneGeometry plane_geo;
GMesh plane(plane_geo, plane_mat) --> GG.scene();

plane.scaX(3 * webcam.aspect());
plane.scaY(-3); // flipping to match webcam orientation

UI_Bool capture(webcam.capture());
UI_Bool freeze(webcam.freeze());

while (true) {
    GG.nextFrame() => now;

    if (UI.begin("Webcam Example")) {
        if (UI.checkbox("Capture", capture)) {
            webcam.capture(capture.val());
        }

        if (UI.checkbox("Freeze", freeze)) {
            webcam.freeze(freeze.val());
        }   
    }
    UI.end();
}