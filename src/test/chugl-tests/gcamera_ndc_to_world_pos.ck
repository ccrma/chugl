/*
Tests the GCamera mousepick and aspect methods.

Original bug: GCamera aspect was auto-determined from the aspect ratio of the GWindow.
This caused incorrect behaviour in the mousepick methods when the camera was *actually*
rendering to a color target that happened to have a different aspect ratio than the GWindow.

As a workaround, add the ability to specify a fixed aspect ratio to the camera,
so the programmer can set the aspect ratio explicitly when needed.

Tried to think of more clever ways to keep this automatic but couldn't come up with anything
because under the current SceneGraph model, a single camera can render to multiple different 
color targets, each with their own aspect and resolution. So there's basically no way to 
automatically decide on a single aspect, and use that for the mousepicking. Decided it's 
better to just let the user be explicit.
*/

// setup to render to a fixed 100x100 texture
GG.camera().orthographic();
GG.camera().viewSize(10); // Y ranges from [-5,5]

TextureDesc texture_desc;
false => texture_desc.mips;
false => texture_desc.resizable;
200 => texture_desc.width;
100 => texture_desc.height;
Texture color_target(texture_desc);


fun void testCameraRaycastMethods(float correct_aspect) {
    T.assert(T.feq(GG.camera().aspect(), correct_aspect), "aspect incorrect");

    GG.camera().NDCToWorldPos(@(-1, -1, 0)) $ vec2 => vec2 world_pos;
    T.assert(T.eq(world_pos, @(correct_aspect * -5, -5)), "NDCToWorldPos " + T.str(world_pos));
    GG.camera().worldPosToNDC(@(correct_aspect * -5, -5, 0)) $ vec2 => vec2 ndc;
    T.assert(T.eq(ndc, @(-1, -1)), "WorldPosToNDC " + T.str(ndc));

    GG.camera().screenCoordToWorldPos(@(0, 0), 1) $ vec2 => world_pos;
    T.assert(T.eq(world_pos, @(correct_aspect * -5, 5)), "screenCordToWorldPos " + T.str(world_pos));
    GG.camera().worldPosToScreenCoord(@(correct_aspect * 5, -5, 1)) $ vec2 => vec2 screen_pos;
    T.assert(T.eq(screen_pos, GWindow.windowSize()), "WorldPosToNDC " + T.str(ndc));
}

// pass frame to start GWindow
GG.nextFrame() => now;

T.assert(GG.camera().autoUpdateAspect(), "default auto update aspect");
T.println("Default rendergraph");
testCameraRaycastMethods(GWindow.windowSize().x / GWindow.windowSize().y);

T.println("Fixed resolution render texture with fixed camera aspect");
color_target.width() / color_target.height() => float color_target_aspect;
GG.scenePass().colorOutput(color_target);
GG.camera().aspect(color_target_aspect);
T.assert(!GG.camera().autoUpdateAspect(), "disable auto update aspect when explicitly setting an aspect");
testCameraRaycastMethods(color_target_aspect);
