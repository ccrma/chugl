@import "../lib/g2d/ChuGL.chug"

// inspo: https://www.shadertoy.com/view/4tGfDW

/*
https://graphics.cs.cmu.edu/nsp/course/15-464/Fall09/papers/StamFluidforGames.pdf
- simulating two grids: density and velocity
- sources and forces

density solver: add force --> diffuse --> update/move


*/


/*
Cool Parameters
- linear <--> nearest filtering (on nearest filtering looks like glitchy manhattan)
- the decay rate of the color texture
- ink
    - color
    - spawn position
    - intensity
- viscocity
- background color aka fade color (go from fading to white to fading to black)

Liu Shui
- in the harmonics at end of 72 gun fu, go from white background to black,
    - the ink on paper become stars in night sky (celestial)
- motion track hands and/or fingers during 72 gun fu, use these nodes 
as velocity sources for the fluid sim (so it looks like pools/vortexes moving across screen)
    - maybe also lerp color to blue
*/

ShaderDesc desc;
me.dir() + "./inkwash.wgsl" => desc.vertexPath;
me.dir() + "./inkwash.wgsl" => desc.fragmentPath;
null => desc.vertexLayout;

Shader shader(desc);

// render graph
GG.rootPass() --> ScreenPass screen_pass(shader);

while (1) {
    GG.nextFrame() => now;
}