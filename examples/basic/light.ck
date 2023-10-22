// Note: current max# of supported lights is:
// 12 point lights
// 12 directional lights

// lightbulb class that renders pointlight with a colored sphere to visualize its position
class LightBulb extends GGen {
    // GGen network. a light + sphere at the same position
    FlatMaterial mat;
    GPointLight light --> GSphere bulb --> this;

    // set up sphere to be a flat color
    bulb.mat(mat);
    mat.color(@(1, 1, 1));
    @(0.1, 0.1, 0.1) => bulb.sca;

    // set light falloff
    light.falloff(0.14, 0.7);  // falloff chart: https://wiki.ogre3d.org/tiki-index.php?page=-Point+Light+Attenuation

    vec3 lightCol;
    Math.random2f(0.5, 1.5) => float pulseRate;  // randomize pulse rate for fading in/out

    fun void color(float r, float g, float b) {
        @(r, g, b) => lightCol;  // save the set color
        mat.color(@(r, g, b));   // set material color
        light.diffuse(@(r, g, b));  // set light diffuse color
    }

    // this is called automatically every frame but ChuGL
    // IF the GGen or one of its parents is connected to GG.scene()
    fun void update(float dt) {
        // fluctuate intensity
        0.5 + 0.5 * Math.sin((now/second) * pulseRate) => light.intensity;  // range [0, 1]
        // fluctuate material color
        light.intensity() * lightCol => mat.color;
    }
}

// camera angle
GG.camera() @=> GCamera @ cam;
@(0, 10, 10) => cam.pos;
cam.lookAt(@(0, 0, 0));

// scene setup
GG.scene() @=> GScene @ scene;
scene.backgroundColor(@(0, 0, 0)); // black background
scene.light().intensity(0);  // disable default directional light

// ground for lights to cast on
GPlane ground --> GG.scene();
@(10, 10, 1) => ground.sca;
@(-Math.PI/2, 0, 0) => ground.rot;  // make the plane lay flat

// instantiate lightbulbs
GGen lightGroup --> scene;
LightBulb redLight--> lightGroup;
LightBulb greenLight--> lightGroup;
LightBulb blueLight--> lightGroup;
LightBulb whiteLight--> lightGroup;
1 => lightGroup.posY;  // lift all lights 1 unit off the ground

// set light colors
2 => redLight.posX;
redLight.color(1, 0, 0);
2 => greenLight.posZ;
greenLight.color(0, 1, 0);
-2 => blueLight.posX;
blueLight.color(0, 0, 1);
-2 => whiteLight.posZ;
whiteLight.color(1, 1, 1);

// Gameloop ==================================
while (true) {
    // rotate lights
    GG.dt() => lightGroup.rotateY;
    // nextFrame
    GG.nextFrame() => now;
}
