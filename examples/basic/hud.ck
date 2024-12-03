//-----------------------------------------------------------------------------
// name: hud.ck
// desc: Using ChuGL hud to add 2D overlay to a 3D scene
// 
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//   date: Fall 2024
//-----------------------------------------------------------------------------

GFlyCamera fly_cam --> GG.scene();
GG.scene().camera(fly_cam);
fly_cam.posZ(10);

GSuzanne suz --> GG.scene();

GText hud_text --> GG.hud();
hud_text.text("HUD Display");

while (true) {
    GG.nextFrame() => now;
}