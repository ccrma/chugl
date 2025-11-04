ShaderDesc desc;
me.dir() + "./shader.wgsl" => desc.vertexPath;
me.dir() + "./shader.wgsl" => desc.fragmentPath;
null => desc.vertexLayout;

Shader shader(desc);
shader.name("screen shader");

// render graph
GG.rootPass() --> ScreenPass screen_pass(shader);

// set uniforms
// (be sure to initialize all uniforms before the first call to GG.nextFrame())
screen_pass.material().uniformFloat(0, 0);

while (1) {
    GG.nextFrame() => now;
    screen_pass.material().uniformFloat(0, .5 + .5 * Math.sin(now/second));
}