//--------------------------------------------------------------------
// name: gmodel.ck
// desc: how to use GModel to loading 3D models from assets files
//       (this also works by drag-and-drop for OBJ files)
// requires: chugl/chuck-1.5.5.5
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//   date: Summer 2025
//--------------------------------------------------------------------

// set new orbit camera as main camera
GG.scene().camera(new GOrbitCamera);
// position the main camera
GG.scene().camera().posY(5);
// set ambient lights on scene
GG.scene().ambient( @(.5, .5, .5) );

// add a text object to scene
GText text --> GG.scene();
// set text
text.text("drag and drop files here!");
// text size
text.size(.2);

// an empty GModel
GModel model;

// instantiate mesh for ground
GMesh ground(new PlaneGeometry(20, 20, 20, 20), new FlatMaterial(Color.DARKGRAY)) --> GG.scene();
// set wireframe
ground.material().wireframe(true);
// rotate around X axis
ground.rotX(.5 * Math.pi);

// create a bounding box
GMesh bbox(new CubeGeometry, new FlatMaterial);
// set wireframe 
bbox.material().wireframe(true);

// get the max component of a vec3
fun float max(vec3 v) {
    return Math.max(Math.max(v.x, v.y), v.z);
}

ModelLoadDesc desc;
// model loading options for ChuGL UI
UI_Bool combine(desc.combine);
UI_Bool flip(desc.flipTextures);

// scene options for ChuGL UI
UI_Float light_intensity(max(GG.scene().ambient()));
UI_Bool show_bounding_box(false);
UI_Bool show_grid(true);
UI_Bool show_model_wireframe(false);

// for drag and drop
GWindow.files() @=> string files[];

// render loop (in this example, it's the UI)
while( true )
{
    // synchronize
    GG.nextFrame() => now;

    // begin UI
    UI.begin("GModel.ck");
    {
        // set text
        UI.text("Drag and drop an OBJ!");
        UI.separatorText("model info: ");
        UI.text("viewing: " + model.name());
        UI.text("mesh count: " + model.meshes.size());
        UI.text("geometry count: " + model.geometries.size());
        UI.text("material count: " + model.materials.size());

        // model load options
        UI.separatorText("model load options: ");
        if (UI.checkbox("flip textures", flip)) flip.val() => desc.flipTextures;
        if (UI.checkbox("combine geometries", combine)) combine.val() => desc.combine;

        // scene options
        UI.separatorText("scene params: ");
        if (UI.slider("light", light_intensity, 0.0, 1.0)) {
            GG.scene().ambient(light_intensity.val() * @(1,1,1));
        }
        if (UI.checkbox("show grid", show_grid)) {
            if (show_grid.val()) ground --> GG.scene();
            else ground.detach();
        }
        if (UI.checkbox("show bounding box", show_bounding_box)) {
            if (show_bounding_box.val()) bbox --> GG.scene();
            else bbox.detach();
        }
        if (UI.checkbox("show model wireframe", show_model_wireframe)) {
            for (auto material : model.materials) {
                material.wireframe(show_model_wireframe.val());
            }
        }

        // add widget for exploring current scene graph
        UI.scenegraph(GG.scene());
    }
    UI.end();

    // detect drag+drop files
    if (GWindow.files() != files)
    {
        // get array of filenames
        GWindow.files() @=> files;

        // remove "drag and drop"
        text --< GG.scene();

        // detach prev model
        model.detach(); 
        // create new model
        GModel new_model(files[0], desc) --> GG.scene();
        // copy reference
        new_model @=> model;

        // fit to grid
        max(model.max - model.min) => float scale;
        if (scale > 20) {
            20.0/scale => scale;
            model.sca(scale);
        } else {
            1.0 => scale;
        }

        // reposition above grid
        model.posY(-model.min.y * scale);

        // center
        scale * 0.5 * (model.min + model.max) => vec3 center;
        model.posX(-center.x).posZ(-center.z);

        // update bounding box
        bbox.sca(scale * (model.max - model.min));
        bbox.posY(bbox.scaY() * 0.5);

        // show bounding box
        if (show_bounding_box.val()) bbox --> GG.scene();
        else bbox.detach();

        // <<< "scale", scale >>>;
        // <<< "min: ", model.min, "max: ", model.max, "center: ", (0.5* (model.min + model.max))>>>;
        // <<< "pos: ", model.posWorld(), "sca:", model.scaWorld()>>>;
    }
}
