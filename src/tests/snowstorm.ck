// Inspired and adapted from the following three.js example
// https://github.com/mrdoob/three.js/blob/master/examples/webgl_points_sprites.html

// UI Params =============================================
/*
pointsize
alpha
color
camera Z
snowflake rotation

*/

// ====================================================
GScene scene;
GCamera camera;
scene.backgroundColor(@(0, 0, 0));
camera.clip(1.5, 1000);
camera.posZ(18);

// setup textures ======================================
FileTexture sprites[6];
for (int i; i < 6; i++) {
    sprites[i].path(me.dir() + "textures/snowflake" + (i+1) + ".png");
}

// materials ============================================
PointMaterial snowflakeMats[6]; 
for (int i; i < snowflakeMats.size(); i++) {
    snowflakeMats[i] @=> PointMaterial @ mat;
    mat.pointSize(50.0);
    mat.pointSprite(sprites[i]);
    mat.attenuatePoints(true);
    mat.transparent(true);
    mat.alpha(0.5);
}

// geometry =============================================
10000 => int NUM_SNOWFLAKES;
14 => int SNOWFLAKE_SPREAD;
vec3 snowflakePos[NUM_SNOWFLAKES];


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

// FPS printer ==========================================
fun void printFPS( dur howOften )
{
    while( true )
    {
        <<< "fps:", GG.fps() >>>;
        howOften => now;
    }
} spork ~ printFPS(.5::second);


// Game loop ============================================
0.1 => float rotationRate;
0.1 => float camSpeed;
while (true) {
    // camera controls
    GG.mouseX() / GG.windowWidth() => float mouseX;
    GG.mouseY() / GG.windowHeight() => float mouseY;
    camSpeed * (mouseX - camera.posX()) + camera.posX() => camera.posX;
    camSpeed * (mouseY - camera.posY()) + camera.posY() => camera.posY;

    camera.lookAt( scene.pos() );

    // snowflake update
    for (int i; i < snowflakes.size(); i++) {
        Math.map(i, 0, snowflakes.size(), 1, 2.5) * rotationRate => float snowflakeRotRate;
        snowflakeRotRate * GG.dt() => snowflakes[i].rotateY;
        // ( (snowflakes.size() / 2.0) - i ) * rotationRate * GG.dt() => snowflakes[i].rotateY;
    }
    GG.nextFrame() => now;
}