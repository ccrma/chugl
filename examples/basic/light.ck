//--------------------------------------------------------------------
// name: light.ck
// desc: moving point lights (with UI control)
//       (there is no prescribed limit on max # of lights)
// requires: ChuGL + chuck-1.5.3.0 or higher
// 
// author: Andrew Zhu Aday
//   date: Fall 2024
//--------------------------------------------------------------------

// window title
GG.windowTitle( "ChuGL light demo" );

// a custom GGen:
// lightbulb class that renders pointlight with a colored sphere to visualize its position
class LightBulb extends GGen
{
    // GGen network. a light + sphere at the same position
    GPointLight light --> GSphere bulb --> this;

    // set up sphere to be a flat color
    FlatMaterial mat( Color.WHITE );
    // set the material
    bulb.mat(mat);
    // scale the bulb
    0.1 => bulb.sca;

    // set light radius (how far the light reaches)
    light.radius(2);
    // light color
    vec3 m_color;
    // randomize pulse rate for fading in/out
    Math.random2f(0.5, 1.5) => float pulseRate;

    // method for setting color
    fun void color(float r, float g, float b) {
        @(r, g, b) => m_color;  // save the set color
        mat.color(@(r, g, b));   // set material color
        light.color(@(r, g, b));  // set light diffuse color
    }

    // this is called automatically every frame by ChuGL...
    // IF the GGen or one of its parents is connected to GG.scene()
    fun void update(float dt) {
        // fluctuate intensity
        0.5 + 0.5 * Math.sin((now/second) * pulseRate) => light.intensity; // range [0, 1]
        // fluctuate material color
        light.intensity() * m_color => mat.color;
    }
}

// select new orbit camera as main camera
GG.scene().camera( new GOrbitCamera );

// disable default directional light
GG.scene().light().intensity(0);
// disable default ambient light
GG.scene().ambient(@(0,0,0));

// camera angle
@(0, 10, 10) => GG.scene().camera().pos;

// ground for lights to cast on
GPlane ground --> GG.scene();
// scale ground plane
@(10, 10, 1) => ground.sca;
// make the plane lay flat
ground.rotateX(Math.PI/2);

// instantiate a group of bulbs as a GGen
GGen lightGroup --> GG.scene();
// instantiate 4 bulbs, each a child of the group
LightBulb redLight--> lightGroup;
LightBulb greenLight--> lightGroup;
LightBulb blueLight--> lightGroup;
LightBulb whiteLight--> lightGroup;
// lift all lights 1 unit off the ground
1 => lightGroup.posY;

// set light colors
2 => redLight.posX;
redLight.color(1, 0, 0);
2 => greenLight.posZ;
greenLight.color(0, 1, 0);
-2 => blueLight.posX;
blueLight.color(0, 0, 1);
-2 => whiteLight.posZ;
whiteLight.color(1, 1, 1);

// gameloop ==================================

// UI variables
UI_Float light_radius(2);
UI_Float light_falloff(2);

// render loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;
    // rotate lights
    GG.dt() => lightGroup.rotateY;

    // begin UI
    if (UI.begin("Point light demo"))
    {
        // light radius
        if (UI.slider("Light radius", light_radius, 0, 10)) {
            redLight.light.radius(light_radius.val());
            greenLight.light.radius(light_radius.val());
            blueLight.light.radius(light_radius.val());
            whiteLight.light.radius(light_radius.val());
        }
        // light falloff
        if (UI.slider("Light falloff", light_falloff, 0, 10)) {
            redLight.light.falloff(light_falloff.val());
            greenLight.light.falloff(light_falloff.val());
            blueLight.light.falloff(light_falloff.val());
            whiteLight.light.falloff(light_falloff.val());
        }
    }
    // end UI
    UI.end();
}
