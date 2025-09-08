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
- click to select bounding-box isection test
- adjust camera position and/or model scale to fit model in screen
- UI
    - sub-mesh explorer (include their names)
- run other raylib model examples for ideas
- modelLoadDesc option whether or not to consolidate geometries into a single drawcall

 Test
  - Rungholt 7M triangle scene(260 MB)
*/

GOrbitCamera cam --> GG.scene();
GG.scene().camera(cam);
GG.scene().ambient(@(.5, .5, .5));
cam.posY(5);

GModel model;

GMesh ground(new PlaneGeometry(20, 20, 20, 20), new FlatMaterial(Color.DARKGRAY)) --> GG.scene();
ground.material().wireframe(true);
ground.rotX(.5 * Math.pi);

GMesh bounding_box(new CubeGeometry, new FlatMaterial);
bounding_box.material().wireframe(true);
bounding_box.sca(0);

fun float max(vec3 v) {
    return Math.max(Math.max(v.x, v.y), v.z);
}

ModelLoadDesc load_desc;
UI_Bool combine_geos(load_desc.combine);
UI_Bool flip_texture(load_desc.flipTextures);

UI_Float light_intensity(max(GG.scene().ambient()));
UI_Bool show_bounding_box(false);
UI_Bool show_grid(true);
UI_Bool show_model_wireframe(false);
GWindow.files() @=> string files[];
while( true )
{
    GG.nextFrame() => now;

    UI.begin("GModel.ck");
        UI.text("Drag and drop an OBJ!");
        UI.separatorText("model info: ");
        UI.text("viewing: " + model.name());
        UI.text("mesh count: " + model.meshes.size());
        UI.text("geometry count: " + model.geometries.size());
        UI.text("material count: " + model.materials.size());

        UI.separatorText("model load options: ");
        if (UI.checkbox("flip textures", flip_texture)) flip_texture.val() => load_desc.flipTextures;
        if (UI.checkbox("combine geometries", combine_geos)) combine_geos.val() => load_desc.combine;

        UI.separatorText("scene params: ");
        if (UI.slider("light", light_intensity, 0.0, 1.0)) {
            GG.scene().ambient(light_intensity.val() * @(1,1,1));
        }
        if (UI.checkbox("show grid", show_grid)) {
            if (show_grid.val()) ground --> GG.scene();
            else ground.detach();
        }
        if (UI.checkbox("show bounding box", show_bounding_box)) {
            if (show_bounding_box.val()) bounding_box --> model;
            else bounding_box.detach();
        }
        if (UI.checkbox("show model wireframe", show_model_wireframe)) {
            for (auto material : model.materials) {
                material.wireframe(show_model_wireframe.val());
            }
        }
        UI.scenegraph(GG.scene());
    UI.end();

    // detect drag+drop files
    if (GWindow.files() != files) {
        GWindow.files() @=> files;

        // detach prev model
        model.detach(); 
        // load new model
        GModel new_model(files[0], load_desc) --> GG.scene();
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
        bounding_box.pos(center);
        bounding_box.sca(model.max - model.min);

        // <<< "scale", scale >>>;
        // <<<"min: ", model.min, "max: ", model.max, "center: ", (0.5* (model.min + model.max))>>>;
        // <<< "pos: ", model.posWorld(), "sca:", model.scaWorld()>>>;
    }
}
