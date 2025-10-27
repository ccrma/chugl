ShaderDesc desc;
me.dir() + "./shader.wgsl" => desc.vertexPath;
me.dir() + "./shader.wgsl" => desc.fragmentPath;
null => desc.vertexLayout;

Shader shader(desc);
shader.name("screen shader");

// render graph
GG.rootPass() --> ScreenPass screen_pass(shader);

while (1) {
    GG.nextFrame() => now;
}