//-----------------------------------------------------------------------------
// name: bloom.ck
// desc: built-in post-processing BLOOM effect
// 
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//   date: Fall 2024
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// currently, there are two methods to enable/set up Bloom
// 1) directly modify the rendergraph and connect; this is the
//    more explicit and customization way, and could accommodate
//    various render graph configurations
//-----------------------------------------------------------------------------
// GG.renderPass() --> BloomPass bloom_pass --> GG.outputPass();
// bloom_pass.input(GG.renderPass().colorOutput());
// GG.outputPass().input(bloom_pass.colorOutput());
//
//-----------------------------------------------------------------------------
// alternately, as of ChuGL-v0.2.7 / ChucK-1.5.5.5
// 2) do the above with one line; this assumes a default rendergraph
//    and is meant as a quick way to access common Bloom usage
//-----------------------------------------------------------------------------
GG.bloom(true);
GG.bloomPass() @=> BloomPass bloom_pass; // pre-allocated 
//-----------------------------------------------------------------------------
// FYI method #2 uses a pre-allocated GG.bloomPass(), which can be
// used to control bloom parameters
//-----------------------------------------------------------------------------


// geometry
PlaneGeometry plane_geo;
SphereGeometry sphere_geo;
// materials
FlatMaterial mat1, mat2, mat3;

2 => float intensity;
mat1.color( @(intensity, 0, 0));
mat2.color( @(0, 0, intensity));
mat3.color( @(0, intensity, 0));

// add meshes to scene
GMesh plane_l(plane_geo, mat1) --> GG.scene();
GMesh plane_r(plane_geo, mat2) --> GG.scene();
GMesh sphere(sphere_geo, mat3) --> GG.scene();

// position
@(-1.5, 0, 0) => plane_l.translate;
@(0, 0, 0) => sphere.translate;
@(1.5, 0, 0) => plane_r.translate;

// uncomment to bypass bloom
// GG.renderPass() --> GG.outputPass();

// UI variables
UI_Float bloom_intensity(bloom_pass.intensity());
UI_Float radius(bloom_pass.radius());

[ 
    "NONE",
    "LINEAR",
    "REINHARD",
    "CINEON",
    "ACES",
    "UNCHARTED",
] @=> string builtin_tonemap_algorithms[];

// UI variables
UI_Int tonemap(GG.outputPass().tonemap());
UI_Int levels(bloom_pass.levels());
UI_Float exposure(GG.outputPass().exposure());
UI_Float color_intensity(intensity);
UI_Float threshold(bloom_pass.threshold());

// render loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;

    // begin UI
    UI.setNextWindowSize(@(400, 600), UI_Cond.Once);
    if (UI.begin("Bloom Example", null, 0))
    {
        // slider for color intensity
        if (UI.slider("color intensity", color_intensity, 0.0, 5.0))
        {
            mat1.color( @(color_intensity.val(), 0, 0));
            mat2.color( @(0, 0, color_intensity.val()));
            mat3.color( @(0, color_intensity.val(), 0));
        }
        // slider for threshold
        if (UI.slider("threshold", threshold, 0.0, 4.0))
        { bloom_pass.threshold(threshold.val());}
        // slider for intensity
        if (UI.slider("intensity", bloom_intensity, 0.0, 1.0))
        { bloom_pass.intensity(bloom_intensity.val()); }
        // slider for radius
        if (UI.slider("radius", radius, 0.0, 1.0))
        { bloom_pass.radius(radius.val()); }
        // slider for levels
        if (UI.slider("levels", levels, 0, 10))
        { bloom_pass.levels(levels.val()); }
        
        // separator
        UI.separator();
        
        // listbox for tonemap function
        if (UI.listBox("tonemap function", tonemap, builtin_tonemap_algorithms, -1))
        { GG.outputPass().tonemap(tonemap.val()); }
        // slider for exposure
        if (UI.slider("exposure", exposure , 0, 4))
        { GG.outputPass().exposure(exposure.val()); }
    }
    // end UI
    UI.end();
}
