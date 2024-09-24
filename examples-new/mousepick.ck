FlatMaterial material;
SphereGeometry sphere_geo;
PlaneGeometry plane_geo;

GMesh plane(plane_geo, material) --> GG.scene();

GCamera camera;
camera --> GGen dolly --> GG.scene();
camera.pos(@(1, 2, 3)); 
GG.scene().camera(camera);
plane.posWorld(camera.posLocalToWorld(@(0, 0, -1)));
<<< plane.posWorld() >>>;

[
    "perspective",
    "orthographic",
] @=> string camera_modes[];
UI_Int camera_mode(0);

UI_Float plane_ndc_x(0.0);
UI_Float plane_ndc_y(0.0);
UI_Float plane_ndc_z(0.0);

fun void ui()
{
    while (true) {
        GG.nextFrame() => now;
        if (UI.listBox("Camera mode", camera_mode, camera_modes, -1)) {
            if (camera_mode.val() == 0) {
                camera.perspective();
            } else {
                camera.orthographic();
            }
            plane.posWorld(camera.NDCToWorldPos(@(plane_ndc_x.val(), plane_ndc_y.val(), plane_ndc_z.val())));
        }

        if (
            UI.slider("ndc x", plane_ndc_x, -1.0, 1.0)
            ||
            UI.slider("ndc y", plane_ndc_y, -1.0, 1.0)
            ||
            UI.slider("ndc z", plane_ndc_z, 0.0, 1.0)
        ) {
            plane.posWorld(camera.NDCToWorldPos(@(plane_ndc_x.val(), plane_ndc_y.val(), plane_ndc_z.val())));
            <<< "plane ndc pos: ", camera.worldPosToNDC(plane.posWorld()) >>>;
        }
    }
} spork ~ ui();

while (true) {
    GG.nextFrame() => now;
    if (GWindow.mouseLeftDown()) {
        GMesh sphere(sphere_geo, material) --> GG.scene();
        sphere.sca(.1);
        sphere.pos(camera.screenCoordToWorldPos(GWindow.mousePos(), 10));
        <<< "clicked" >>>;
    }
    if (GWindow.scrollX() > 0 || GWindow.scrollY() > 0) {
        <<< GWindow.scroll() >>>;
    }
}