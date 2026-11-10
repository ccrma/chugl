//--------------------------------------------------------------------
// name: hud.ck
// desc: boilerplate for creating an in-game UI / hud by chaining
//       renderpasses. The hud will always be drawn on top of the 
//       underlying 3D scene, regardless of camera position.
// requires: ChuGL 0.3.0 (alpha) + chuck-1.5.5.6 or higher
// 
// author: Andrew Zhu Aday
//   date: Fall 2026
//--------------------------------------------------------------------

// init a scene for our hud / UI
GScene hud;
null => hud.skybox; // disable skybox
hud.camera().orthographic(); // make hud camera orthographic (for 2d UI)
hud.camera().viewSize(10);   // give hud camera vertical viewsize of 10 units

// update the rendergraph to render our 2D hud on top of the default 3D scene
GG.scenePass() --> ScenePass hudpass(hud) --> GG.outputPass();

// configure the hud pass
hudpass.colorOutput(GG.scenePass().colorOutput());  // draw to same render texture
hudpass.clear(false);                               // *dont* clear prior contents
hudpass.msaa(false);                                // disable anti aliasing (requires additional render texture)

// try a different tonemapping function! (not required)
GG.outputPass().tonemap(OutputPass.ToneMap_Cineon);

// connect objects to our 3D scene
GSuzanne suz --> GG.scene();

// connect UI to our 2D hudpass
GText ui_text --> hud; ui_text.text("UI GOES HERE");
ui_text.size(1.5);

while (1) {
    GG.nextFrame() => now;
    now/second => float t;

    // spin monkey
    suz.rotateY(GG.dt());

    // move 3D scene camera (does *NOT* affect hud camera)
    GG.scene().camera().posZ(3 + 3*Math.cos(t));

    // swap text colors periodically
    t - (t)$int => float second_fract;
    if (second_fract < .5) ui_text.color(Color.GREEN);
    else ui_text.color(Color.ORANGE);
}