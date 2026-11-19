//--------------------------------------------------------------------
// name: rendertexture.ck
// desc: boilerplate rendering a scene to a rendertexture, 
//       and then sampling that texture in another scene.
// requires: ChuGL 0.3.0 (alpha) + chuck-1.5.5.6 or higher
// 
// author: Andrew Zhu Aday
//   date: Fall 2026
//--------------------------------------------------------------------

// orbit camera
GG.scene().camera(new GOrbitCamera);

// create our render texture
TextureDesc render_tex_desc;
true => render_tex_desc.resizable;
1    => render_tex_desc.heightRatio;
1    => render_tex_desc.widthRatio;
false => render_tex_desc.mips; // DONT generate mips, this messes with sampling
Texture render_tex(render_tex_desc);

// create a separate scene for our render texture
GScene render_tex_scene;
render_tex_scene.backgroundColor(Color.BLUE);

// add our render texture pass to the rendergraph
GG.rootPass() --> ScenePass render_tex_pass(render_tex_scene) --> GG.scenePass();

// tell our added pass to write to our custom texture
render_tex_pass.colorOutput(render_tex); 

// create our render tex scene
GSuzanne suzanne --> render_tex_scene;

// create our main scene
GCube cube --> GG.scene();
cube.colorMap(render_tex); // sample the render texture on our cube!!
cube.rotateX(Math.pi); // flip the cube; webgpu renders upside down

while (1) {
    GG.nextFrame() => now;
    suzanne.rotateY(-1.7 * GG.dt());
    cube.rotateY(GG.dt());
    cube.rotateX(1.1 * GG.dt());
}
