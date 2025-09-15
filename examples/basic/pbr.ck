//--------------------------------------------------------------------
// name: pbr.ck
// desc: physically-based rendering
// requires: ChuGL + chuck-1.5.5.5 or higher
// 
// author: Andrew Zhu Aday
//   date: Fall 2024
//--------------------------------------------------------------------

// use an orbit camera as main camera
GOrbitCamera camera => GG.scene().camera;
// scene ambient light
GG.scene().ambient( @(.05,.05,.05) );

// scenegraph setup
7 => int NUM_ROWS;
// material
PBRMaterial material[NUM_ROWS][NUM_ROWS];
FlatMaterial light_material;

// geometry
// SuzanneGeometry geo;
SphereGeometry geo;

// init pbr meshes
for (int i; i < NUM_ROWS; i++) {
    for (int j; j < NUM_ROWS; j++) {
        material[i][j].color(Color.RED); 
        material[i][j].metallic( (i $ float) / NUM_ROWS );
        material[i][j].roughness( Math.clampf((j $ float) / NUM_ROWS, 0.05, 1) );
        // add mesh to scene
        GMesh mesh(geo, material[i][j]) --> GG.scene();
        // position the new mesh
        mesh.pos(@(
            (j - NUM_ROWS / 2) * 1.5,
            (i - NUM_ROWS / 2) * 1.5,
            0
        ));
    }
}

// rendering the light source locations
GGen point_light_axis;
GPointLight point_lights[1];
GMesh point_light_meshes[0];
point_light_meshes << new GMesh(geo, light_material);
point_light_meshes[0].sca(0.1);
point_lights[0].pos(@(.8, 0, 0));
// connect mesh to light position
point_light_meshes[0] --> point_lights[0] --> point_light_axis --> GG.scene();

// UI variables
UI_Int num_point_lights(point_lights.size());
UI_Float dir_light_rotation;
UI_Float3 bg_color(GG.scene().backgroundColor());
UI_Float3 ambient_light(GG.scene().ambient());
UI_Float3 dir_light_color(GG.scene().light().color());
UI_Float dir_light_intensity(GG.scene().light().intensity());

// material properties
UI_Float3 color(material[0][0].color());
UI_Float3 camera_target(camera.target());

// UI handler
fun void ui()
{
    // render loop
    while (true)
    {
        // synchronize
        GG.nextFrame() => now;
        // begin UI
        if (UI.begin("Lighting Example", null, 0))
        {
            if (UI.colorEdit("material color", color, 0)) {
                for (int i; i < NUM_ROWS; i++) {
                    for (int j; j < NUM_ROWS; j++) {
                        color.val() => material[i][j].color;
                    }
                }
            }

            if (UI.colorEdit("background color", bg_color, 0)) {
                bg_color.val() => GG.scene().backgroundColor;
            }

            if (UI.colorEdit("ambient light", ambient_light, 0)) {
                ambient_light.val() => GG.scene().ambient;
            }

            if (UI.colorEdit("dir light color", dir_light_color, 0)) {
                dir_light_color.val() => GG.scene().light().color;
            }

            if (UI.slider("dir light intensity", dir_light_intensity, 0, 10)) {
                dir_light_intensity.val() => GG.scene().light().intensity;
            }

            if (UI.slider("dir light rotation", dir_light_rotation, -Math.PI, Math.PI)) {
                dir_light_rotation.val() => GG.scene().light().rotX;
            }   

            if (UI.inputInt("num point lights", num_point_lights)) {
                if (num_point_lights.val() < 0) {
                    num_point_lights.val(0);
                }
                if (num_point_lights.val() > point_lights.size()) {
                    for (point_lights.size() => int i; i < num_point_lights.val(); i++) {
                        point_lights << new GPointLight;
                        GMesh new_point_light_mesh(geo, light_material);
                        new_point_light_mesh.sca(0.1);
                        point_light_meshes << new_point_light_mesh;
                        point_light_meshes[i] --> point_lights[i] --> point_light_axis --> GG.scene();
                    }
                }
                else if (num_point_lights.val() < point_lights.size()) {
                    for (num_point_lights.val() => int i; i < point_lights.size(); i++) {
                        point_lights[i].detach(); // detach from scenegraph
                    }
                    // remove from arrays
                    point_lights.erase(num_point_lights.val(), point_lights.size());
                    point_light_meshes.erase(num_point_lights.val(), point_light_meshes.size());
                }
                // update light positions to form equally spaced circle 
                for (int i; i < point_lights.size(); i++) {
                    point_lights[i].pos(.8 * @(
                        Math.cos(i * Math.TWO_PI / point_lights.size()),
                        0,
                        Math.sin(i * Math.TWO_PI / point_lights.size())
                    ));
                }
            }
            
            // camera target (point around which camera rotates)
            if (UI.drag("camera target", camera_target, 0.01, 0, 0, "%0.2f", 0)) {
                camera_target.val() => camera.target;
            }
        }
        
        // end UI
        UI.end();
    }
} spork ~ ui();

// render loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;
    // rotate the light source(s)
    GG.dt() => point_light_axis.rotateY;
}
