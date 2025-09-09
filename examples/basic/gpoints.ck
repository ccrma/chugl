//-----------------------------------------------------------------------------
// name: gpoints.ck
// desc: stuff you can do with GPoints! (with UI for exploring options)
// requires: ChuGL + chuck-1.5.3.0 or higher
//
// author: Andrew Zhu Aday (https://ccrma.stanford.edu/~azaday/)
//         Ge Wang (https://ccrma.stanford.edu/~ge/)
// date: Fall 2023
//-----------------------------------------------------------------------------
// add GPoints GGen to scene
GPoints points --> GG.scene();

// array of vec3s
[@(-1, -1, 0), @(1, -1, 0), @(0, 1, 0)] @=> vec3 positions[];
// array of colors
[Color.WHITE] @=> vec3 colors[];
// array of point sizes
[1.0] @=> float sizes[];

UI_Float3 ui_positions[0];
UI_Float3 ui_colors[0];
UI_Float ui_sizes[0];

// populate per-point attributes
for (int i; i < positions.size(); ++i) {
    ui_positions << new UI_Float3(positions[i]);
}
for (int i; i < colors.size(); ++i) {
    ui_colors << new UI_Float3(colors[i]);
}
for (int i; i < sizes.size(); ++i) {
    ui_sizes << new UI_Float(sizes[i]);
}

// set positions
points.positions(positions);

// UI variables for point material params
UI_Float ui_global_size(points.size());
UI_Float3 ui_global_color(points.color());

// render loop
while (true)
{
    // synchronize
    GG.nextFrame() => now;

    // begin UI
    if (UI.begin("GPoints"))
    {
        if (UI.slider("point size", ui_global_size, 0.01, 1))
        { ui_global_size.val() => points.size; }
        if (UI.colorEdit("point color", ui_global_color, 0))
        { ui_global_color.val() => points.color; }

        // add separator
        UI.separatorText("point positions");

        // UI to change positions of individual points
        for (int i; i < positions.size(); i++)
        {
            // for selecting individual points
            UI.pushID(i);
            // add draggable slider
            if (UI.drag("##point_pos", ui_positions[i], .01)) {
                ui_positions[i].val() => positions[i];
                positions => points.positions;
            }
            // (no line break)
            UI.sameLine();
            // add "remove a point" button
            if (UI.button("Remove##point_pos")) {
                positions.erase(i);
                ui_positions.erase(i);
                positions => points.positions;
            }

            // pop ID stack
            UI.popID();
        }

        if (UI.button("Add Position")) {
            positions << @(0,0, 0);
            ui_positions << new UI_Float3;
            positions => points.positions;
        }

        UI.separatorText("point colors");

        for (int i; i < colors.size(); i++) {
            UI.pushID(i);

            if (UI.colorEdit("##point_color", ui_colors[i], 0)) {
                ui_colors[i].val() => colors[i];
                colors => points.colors;
            }

            UI.sameLine();
            if (UI.button("Remove##point_color")) {
                colors.erase(i);
                ui_colors.erase(i);
                colors => points.colors;
            }

            UI.popID();
        }

        if (UI.button("Add Color")) {
            colors << @(1, 1, 1);
            ui_colors << new UI_Float3(@(1, 1, 1));
            colors => points.colors;
        }

        UI.separatorText("point sizes");

        for (int i; i < sizes.size(); i++) {
            UI.pushID(i);

            if (UI.slider("##point_sizes", ui_sizes[i], 0.01, 1.0)) {
                ui_sizes[i].val() => sizes[i];
                sizes => points.sizes;
            }

            UI.sameLine();

            if (UI.button("Remove##point_size")) {
                sizes.erase(i);
                ui_sizes.erase(i);
                sizes => points.sizes;
            }

            UI.popID();
        }

        if (UI.button("Add Size")) {
            sizes << 1;
            ui_sizes << new UI_Float(1);
            sizes => points.sizes;
        }
    }
    
    // end UI
    UI.end();
}
