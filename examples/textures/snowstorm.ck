//-----------------------------------------------------------------------------
// name: snowstorm.ck
// desc: Snowstorm example demoing the use of point sprites, transparency, 
//       and UI controls.
//
// source: Inspired and adapted from the following three.js example
//         https://github.com/mrdoob/three.js/blob/master/examples/webgl_points_sprites.html
//
// requires: ChuGL + chuck-1.5.1.7 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
// date: Fall 2023
//-----------------------------------------------------------------------------

// setup scene ====================================================
GScene scene;
GCamera camera;
scene.backgroundColor(@(0, 0, 0));
camera.clip(1.5, 1000);
camera.posZ(16);

// setup textures ======================================
FileTexture sprites[6];
for (int i; i < 6; i++) {
    sprites[i].path(me.dir() + "data/snowflake" + (i+1) + ".png");
}

// materials ============================================
PointMaterial snowflakeMats[6];

// default material values
85.0 => float DEFAULT_POINT_SIZE;
0.75 => float DEFAULT_ALPHA;
@(.741, .894, 1.0) => vec3 DEFAULT_COLOR;

// initialize materials
for (int i; i < snowflakeMats.size(); i++) {
    snowflakeMats[i] @=> PointMaterial @ mat;
    mat.pointSize(DEFAULT_POINT_SIZE);
    mat.pointSprite(sprites[i]);
    mat.attenuatePoints(true);
    mat.transparent(true);
    mat.alpha(DEFAULT_ALPHA);
    mat.color(DEFAULT_COLOR);
}

// geometry =============================================
10000 => int NUM_SNOWFLAKES;
14 => int SNOWFLAKE_SPREAD;
vec3 snowflakePos[NUM_SNOWFLAKES];

// initialize snowflake positions
for (0 => int i; i < snowflakePos.size(); i++) {
    @(
        Math.random2f(-SNOWFLAKE_SPREAD, SNOWFLAKE_SPREAD),  // x
        Math.random2f(-SNOWFLAKE_SPREAD, SNOWFLAKE_SPREAD),  // y
        Math.random2f(-SNOWFLAKE_SPREAD, SNOWFLAKE_SPREAD)   // z
    ) => snowflakePos[i];
}

CustomGeometry snowflakeGeo;
snowflakeGeo.positions(snowflakePos);

// meshes  ==============================================
GGen snowstorm --> scene;
GMesh snowflakes[6];

// initialize snowflake meshes
for (int i; i < 6; i++) {
    snowflakes[i] @=> GMesh @ snowflake;
    snowflakes[i].set(snowflakeGeo, snowflakeMats[i]);
    snowflakes[i] --> snowstorm;

    // randomize rotation
    .15 => float s;
    Math.random2f(0, s*Math.TWO_PI) => snowflake.rotX;
    Math.random2f(0, s*Math.TWO_PI) => snowflake.rotY;
    Math.random2f(0, s*Math.TWO_PI) => snowflake.rotZ;
}

// UI Controls ==========================================
UI_Window window;
window.text("Snowstorm Controls");

// pointsize
UI_SliderFloat pointSizeSlider;
pointSizeSlider.text("Snowflake Size");
pointSizeSlider.range(0.1, 200.0);
pointSizeSlider.val(DEFAULT_POINT_SIZE);  // default value

// alpha
UI_SliderFloat alphaSlider;
alphaSlider.text("Snowflake Opacity");
alphaSlider.range(0.0, 1.0);
alphaSlider.val(DEFAULT_ALPHA);  // default value

// color
UI_Color3 snowflakeColor;
snowflakeColor.text("Snowflake Color");
snowflakeColor.val(DEFAULT_COLOR);  // default value

// Camera Z Pos
UI_SliderFloat cameraZSlider;
cameraZSlider.text("Camera Z Position");
cameraZSlider.range(0.0, 50.0);
cameraZSlider.val(camera.posZ());  // default value

// Wind Speed
UI_SliderFloat windSpeedSlider;
windSpeedSlider.text("Wind Speed");
windSpeedSlider.range(0.0, 1.0);
windSpeedSlider.val(0.05);  // default value

// add controls to window
window.add(pointSizeSlider);
window.add(alphaSlider);
window.add(snowflakeColor);
window.add(cameraZSlider);
window.add(windSpeedSlider);

// UI Event Handlers ====================================
fun void PointSizeListener() {
    while (true) {
        pointSizeSlider => now;
        pointSizeSlider.val() => float ps;
        for (auto mat : snowflakeMats)
            ps => mat.pointSize;
    }
} spork ~ PointSizeListener();

fun void AlphaListener() {
    while (true) {
        alphaSlider => now;
        alphaSlider.val() => float a;
        for (auto mat : snowflakeMats)
            a => mat.alpha;
    }
} spork ~ AlphaListener();

fun void ColorListener() {
    while (true) {
        snowflakeColor => now;
        snowflakeColor.val() => vec3 c;
        for (auto mat : snowflakeMats)
            c => mat.color;
    }
} spork ~ ColorListener();

fun void CameraZListener() {
    while (true) {
        cameraZSlider => now;
        cameraZSlider.val() => camera.posZ;
    }
} spork ~ CameraZListener();

windSpeedSlider.val() => float rotationRate;
fun void WindSpeedListener() {
    while (true) {
        windSpeedSlider => now;
        windSpeedSlider.val() => rotationRate;
    }
} spork ~ WindSpeedListener();


// Game loop ============================================
0.1 => float camSpeed;
3.5 => float camMovement;
while (true) {
    // camera controls (inverted so it looks like you're moving the snowflakes!)
    -camMovement * GG.mouseX() / GG.windowWidth() => float mouseX;
    camMovement * GG.mouseY() / GG.windowHeight() => float mouseY;
    camSpeed * (mouseX - camera.posX()) + camera.posX() => camera.posX;
    camSpeed * (mouseY - camera.posY()) + camera.posY() => camera.posY;
    camera.lookAt( scene.pos() );

    // snowflake update
    for (int i; i < snowflakes.size(); i++) {
        Math.map(i, 0, snowflakes.size(), 1, 2.5) * rotationRate => float snowflakeRotRate;
        snowflakeRotRate * GG.dt() => snowflakes[i].rotateY;
    }

    // next frame
    GG.nextFrame() => now;
}