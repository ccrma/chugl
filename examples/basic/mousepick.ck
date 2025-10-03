//--------------------------------------------------------------------
// name: mousepick.ck
// desc: mouse picking with different projections
// 
// author: Andrew Zhu Aday
//   date: Fall 2024
//--------------------------------------------------------------------
// geometry and material
SphereGeometry sphere_geo;
FlatMaterial material;

// text for UI
[ "perspective", "orthographic" ] @=> string camera_modes[];
// UI variable
UI_Int camera_mode(0);

// UI in its own render loop here
fun void ui()
{
    while (true)
    {
        // synchronize
        GG.nextFrame() => now;
        
        // list box
        if (UI.listBox("camera mode", camera_mode, camera_modes, -1))
        {
            if (camera_mode.val() == 0) {
                GG.camera().perspective();
            } else {
                GG.camera().orthographic();
            }
        }
    }
} spork ~ ui();

// render loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;

    // camera movement
    if (GWindow.key(GWindow.KEY_LEFT)) GG.camera().rotateY(GG.dt());
    if (GWindow.key(GWindow.KEY_RIGHT)) GG.camera().rotateY(-GG.dt());
    
    // mouse handling
    if (GWindow.mouseLeftDown())
    {
        // add a new sphere to the scene
        GMesh sphere(sphere_geo, material) --> GG.scene();
        // scale the sphere
        sphere.sca(.1);
        // position the sphere
        sphere.pos(GG.camera().screenCoordToWorldPos(GWindow.mousePos(), 10));
    }
}
