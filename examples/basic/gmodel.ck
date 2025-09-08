//--------------------------------------------------------------------
// name: gmodel.ck
// desc: How to use GModel to loading 3D models from assets files
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//   date: Summer 2025
//--------------------------------------------------------------------

/*
TODO:
- drag + drop
    - have GText/console error msg on
        - invalid model type (only supports OBJ)
        - invalid file type (e.g. fstat shows what was dragged is not a file)
        - out of memory
- wireframe mode
- click to select bounding-box isection test
- add floor grid
- adjust camera position and/or model scale to fit model in screen
- UI
    - sub-mesh explorer (include their names)
- run other raylib model examples for ideas
- modelLoadDesc option whether or not to consolidate geometries into a single drawcall

 Test
  - obj with missing .mtl fileA
  - Rungholt 7M triangle scene(260 MB)
  - try dragging a non-obj file onto the window
  - try dragging an obj file without .obj extension onto window
  - try dragging a non-file into window
*/

// GG.rootPass() --> ScenePass sp(GG.scene());

GOrbitCamera cam --> GG.scene();
GG.scene().camera(cam);
cam.posY(5);

GModel model;
<<< model.name() >>>;

GMesh ground(new PlaneGeometry(10, 10, 20, 20), new FlatMaterial) --> GG.scene();
ground.material().wireframe(true);
ground.rotX(.5 * Math.pi);

UI_Float light_intensity(GG.scene().light().intensity());
GWindow.files() @=> string files[];
while( true )
{
    GG.nextFrame() => now;

    UI.begin("GModel.ck");
        if (UI.slider("Light Intensity", light_intensity, 0.0, 1.0)) {
            GG.scene().light().intensity(light_intensity.val());
        }
        // UI.text("Model Viewer");
        UI.text("Viewing: " + model.name());
    UI.end();

    // detect drag+drop files
    if (GWindow.files() != files) {
        GWindow.files() @=> files;
        // text.text(files[0]);

        // detach prev model
        model.detach(); 
        // load new model
        GModel new_model(files[0]) --> GG.scene();
        new_model @=> model;
    }
}
