@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"

G2D g;

adc => PoleZero p => LiSa lisa => dac;
false => int lisa_recording;

// pole location to block DC and ultra low frequencies
.99 => p.blockZero;

//alloc memory
2::second => lisa.duration;
lisa.loop0(false); // default true
lisa.rampDown(10::ms);
// lisa.loopEnd(1::second); // only works if loop(true);

// 10::ms => lisa.recRamp;
// 400::ms => loopme.recRamp;

//set playback rate
// 1.5 => loopme.rate;
// 1 => loopme.loop;
// 1 => loopme.bi;
// 1 => loopme.play;

float samples[512];

// UI_DrawList demo params
UI_Float sz(36.0);
UI_Float thickness( 3.0);
UI_Int ngon_sides( 6);
// UI_Bool circle_segments_override( false);
// UI_Int circle_segments_override_v( 12);
// UI_Bool curve_segments_override( false);
// UI_int curve_segments_override_v( 8);
UI_Float4 colf(1.0, 1.0, 0.4, 1.0);



while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;

    if (UI.button(lisa_recording ? "stop" : "record")) {
        !lisa_recording => lisa_recording;
        lisa_recording => lisa.record;
    }

    if (UI.button(lisa.playing(0) ? "pause" : "play")) {
        if (lisa.playing(0)) { // pause
            0::ms => lisa.playPos;
            lisa.play(false);
        } else { // play
            0::ms => lisa.playPos;
            lisa.play(true);
        }
    }

    // get sample values from lisa
    lisa.duration() / samples.size() => dur interval;
    for (int i; i < samples.size(); i++) {
        lisa.valueAt(interval * i) => samples[i];
    }
    UI.getCursorScreenPos() => vec2 waveform_plot_p;
    UI.plotLines(
        "##Waveform", samples, 0, 
        "Waveform", -1, 1, 
        @(samples.size(), 200)
    );

    // playhead
    if (lisa_recording) {
        lisa.recPos() / lisa.duration() => float progress;
        waveform_plot_p.x + progress * samples.size() => float x;
        waveform_plot_p.y => float y;
        UI_DrawList.addLine(@(x, y), @(x, y + 200), @(1,0,0,1), 1.0); 
    }
    if (lisa.playing(0 /*which voice */)) {
        lisa.playPos() / lisa.duration() => float progress;
        waveform_plot_p.x + progress * samples.size() => float x;
        waveform_plot_p.y => float y;
        UI_DrawList.addLine(@(x, y), @(x, y + 200), @(1,1,1,1), 1.0); 
    }

    UI_DrawList.addCircleFilled(@(0, 0), 10.0, @(1, 255, 1, 1), 32);


// =================================
// UI_DrawList demo
// =================================

    if (UI.begin("DrawList")) {
        // Draw gradients
        UI.text("Gradients");
        @(UI.calcItemWidth(), UI.getFrameHeight()) => vec2 gradient_size;
        {
            UI.getCursorScreenPos() => vec2 p0;
            @(p0.x + gradient_size.x, p0.y + gradient_size.y) => vec2 p1;
            @(0, 0, 0, 1) => vec4 col_a;
            @(1, 1, 1, 1) => vec4 col_b;
            UI_DrawList.addRectFilledMultiColor(p0, p1, col_a, col_b, col_b, col_a);
            if (UI.invisibleButton("##gradient1", gradient_size, UI_ButtonFlags.MouseButtonLeft)) {
                <<< "gradient clicked" >>>;
            }
        }
    
        UI.text("All primitives");
        UI.drag("Size", sz, 0.2, 2.0, 100.0, "%.0f", UI_SliderFlags.None);
        UI.drag("Thickness", thickness, 0.05f, 1.0f, 8.0f, "%.02f", UI_SliderFlags.None);
        UI.slider("N-gon sides", ngon_sides, 3, 12);
        // ImGui::Checkbox("##circlesegmentoverride", &circle_segments_override);
        // ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        // circle_segments_override |= ImGui::SliderInt("Circle segments override", &circle_segments_override_v, 3, 40);
        // ImGui::Checkbox("##curvessegmentoverride", &curve_segments_override);
        // ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
        // curve_segments_override |= ImGui::SliderInt("Curves segments override", &curve_segments_override_v, 3, 40);
        UI.colorEdit("Color", colf);

        // Draw a bunch of primitives
        UI.getCursorScreenPos() => vec2 p;
        10.0f => float spacing;
        colf.val() => vec4 col;
        UI_DrawFlags.RoundCornersTopLeft | UI_DrawFlags.RoundCornersBottomRight => int corners_tl_br;
        sz.val() / 5.0f => float rounding;
        0 => int circle_segments;
        // const int circle_segments = circle_segments_override ? circle_segments_override_v : 0;
        0 => int curve_segments;
        // const int curve_segments = curve_segments_override ? curve_segments_override_v : 0;

        // Control points for bezier curves
        [ @(0.0f, sz.val() * 0.6f), @(sz.val() * 0.5f, -sz.val() * 0.4f), @(sz.val(), sz.val()) ] @=> vec2 cp3[];
        [ @(0.0f, 0.0f), @(sz.val() * 1.3f, sz.val() * 0.3f), @(sz.val() - sz.val() * 1.3f, sz.val() - sz.val() * 0.3f), @(sz.val(), sz.val()) ] @=> vec2 cp4[];

        p.x + 4.0f => float x;
        p.y + 4.0f => float y;
        for (int n; n < 2; n++)
        {
            // First line uses a thickness of 1.0f, second line uses the configurable thickness
            (n == 0) ? 1.0f : thickness.val() => float th;
            UI_DrawList.addNgon(
                @(x + sz.val()*0.5f, y + sz.val()*0.5f), 
                sz.val()*0.5f, col, ngon_sides.val(), th);                 
            sz.val() + spacing +=> x;  // N-gon

            UI_DrawList.addCircle(@(x + sz.val()*0.5f, y + sz.val()*0.5f), sz.val()*0.5f, col, circle_segments, th);          
            sz.val() + spacing +=> x;  // Circle

            UI_DrawList.addEllipse(@(x + sz.val()*0.5f, y + sz.val()*0.5f), @(sz.val()*0.5f, sz.val()*0.3f), col, -0.3f, circle_segments, th); 
            sz.val() + spacing +=> x;	// Ellipse

            UI_DrawList.addRect(@(x, y), @(x + sz.val(), y + sz.val()), col, 0.0f, UI_DrawFlags.None, th);          
            sz.val() + spacing +=> x;  // Square

            UI_DrawList.addRect(@(x, y), @(x + sz.val(), y + sz.val()), col, rounding, UI_DrawFlags.None, th);      
            sz.val() + spacing +=> x;  // Square with all rounded corners

            UI_DrawList.addRect(@(x, y), @(x + sz.val(), y + sz.val()), col, rounding, corners_tl_br, th);         
            sz.val() + spacing +=> x;  // Square with two rounded corners

            UI_DrawList.addTriangle(@(x+sz.val()*0.5f,y), @(x+sz.val(), y+sz.val()-0.5f), @(x, y+sz.val()-0.5f), col, th);
            sz.val() + spacing +=> x;  // Triangle

            UI_DrawList.addTriangle(@(x+sz.val()*0.2f,y), @(x, y+sz.val()-0.5f), @(x+sz.val()*0.4f, y+sz.val()-0.5f), col, th);
            sz.val()*0.4f + spacing +=> x; // Thin triangle

            { // a |_| looking shape
                [ 
                    @(0.0f, 0.0f), @(0.3f, 0.0f), @(0.3f, 0.7f), @(0.7f, 0.7f), @(0.7f, 0.0f), @(1.0f, 0.0f), @(1.0f, 1.0f), @(0.0f, 1.0f) 
                ] @=> vec2 pos_norms[];
                for (auto p : pos_norms)
                    UI_DrawList.pathLineTo(@(x + 0.5f + (sz.val() * p.x) $ int, y + 0.5f + (sz.val() * p.y) $ int));
                UI_DrawList.pathStroke(col, UI_DrawFlags.Closed, th);          
                sz.val() + spacing +=> x;  // Concave Shape
            }

            UI_DrawList.addLine(@(x, y), @(x + sz.val(), y), col, th); 
            sz.val() + spacing +=> x;  // Horizontal line (note: drawing a filled rectangle will be faster!)

            UI_DrawList.addLine(@(x, y), @(x, y + sz.val()), col, th);
            spacing +=> x;       // Vertical line (note: drawing a filled rectangle will be faster!)

            UI_DrawList.addLine(@(x, y), @(x + sz.val(), y + sz.val()), col, th);
            sz.val() + spacing +=> x;  // Diagonal line

            // Path
            UI_DrawList.pathArcTo(@(x + sz.val()*0.5f, y + sz.val()*0.5f), sz.val()*0.5f, 3.141592f, 3.141592f * -0.5f, 0);
            UI_DrawList.pathStroke(col, UI_DrawFlags.None, th);
            sz.val() + spacing +=> x;

            // Quadratic Bezier Curve (3 control points)
            UI_DrawList.addBezierQuadratic(@(x + cp3[0].x, y + cp3[0].y), @(x + cp3[1].x, y + cp3[1].y), @(x + cp3[2].x, y + cp3[2].y), col, th, curve_segments);
            sz.val() + spacing +=> x;

            // Cubic Bezier Curve (4 control points)
            UI_DrawList.addBezierCubic(@(x + cp4[0].x, y + cp4[0].y), @(x + cp4[1].x, y + cp4[1].y), @(x + cp4[2].x, y + cp4[2].y), @(x + cp4[3].x, y + cp4[3].y), col, th, curve_segments);

            p.x + 4 => x;
            sz.val() + spacing +=> y;
        }

        // Filled shapes
        UI.text("fillled shapes");
        UI_DrawList.addNgonFilled(
            @(x + sz.val() * 0.5f, y + sz.val() * 0.5f), 
            sz.val() * 0.5f, 
            col, 
            ngon_sides.val());             
        sz.val() + spacing +=> x;  // N-gon

        UI_DrawList.addCircleFilled(@(x + sz.val() * 0.5f, y + sz.val() * 0.5f), sz.val() * 0.5f, col, circle_segments);      
        sz.val() + spacing +=> x;  // Circle

        UI_DrawList.addEllipseFilled(@(x + sz.val() * 0.5f, y + sz.val() * 0.5f), @(sz.val() * 0.5f, sz.val() * 0.3f), col, -0.3f, circle_segments); 
        sz.val() + spacing +=> x;// Ellipse

        UI_DrawList.addRectFilled(@(x, y), @(x + sz.val(), y + sz.val()), col);                                    
        sz.val() + spacing +=> x;  // Square

        UI_DrawList.addRectFilled(@(x, y), @(x + sz.val(), y + sz.val()), col, 10.0f, UI_DrawFlags.None);                             
        sz.val() + spacing +=> x;  // Square with all rounded corners

        UI_DrawList.addRectFilled(@(x, y), @(x + sz.val(), y + sz.val()), col, 10.0f, corners_tl_br);
        sz.val() + spacing +=> x;  // Square with two rounded corners

        // draw_list->AddTriangleFilled(@(x+sz.val()*0.5f,y), @(x+sz.val(), y+sz.val()-0.5f), @(x, y+sz.val()-0.5f), col);  sz.val() + spacing +=> x;  // Triangle
        // //draw_list->AddTriangleFilled(@(x+sz.val()*0.2f,y), @(x, y+sz.val()-0.5f), @(x+sz.val()*0.4f, y+sz.val()-0.5f), col); x += sz.val()*0.4f + spacing; // Thin triangle
        // PathConcaveShape(draw_list, x, y, sz.val()); draw_list->PathFillConcave(col);                                 sz.val() + spacing +=> x;  // Concave shape
        // draw_list->AddRectFilled(@(x, y), @(x + sz.val(), y + thickness), col);                             sz.val() + spacing +=> x;  // Horizontal line (faster than AddLine, but only handle integer thickness)
        // draw_list->AddRectFilled(@(x, y), @(x + thickness, y + sz.val()), col);                             x += spacing * 2.0f;// Vertical line (faster than AddLine, but only handle integer thickness)
        // draw_list->AddRectFilled(@(x, y), @(x + 1, y + 1), col);                                      x += sz.val();            // Pixel (faster than AddLine)

        // // Path
        // draw_list->PathArcTo(@(x + sz.val() * 0.5f, y + sz.val() * 0.5f), sz.val() * 0.5f, 3.141592f * -0.5f, 3.141592f);
        // draw_list->PathFillConvex(col);
        // sz.val() + spacing +=> x;

        // // Quadratic Bezier Curve (3 control points)
        // draw_list->PathLineTo(@(x + cp3[0].x, y + cp3[0].y));
        // draw_list->PathBezierQuadraticCurveTo(@(x + cp3[1].x, y + cp3[1].y), @(x + cp3[2].x, y + cp3[2].y), curve_segments);
        // draw_list->PathFillConvex(col);
        // sz.val() + spacing +=> x;

        // draw_list->AddRectFilledMultiColor(@(x, y), @(x + sz.val(), y + sz.val()), IM_COL32(0, 0, 0, 255), IM_COL32(255, 0, 0, 255), IM_COL32(255, 255, 0, 255), IM_COL32(0, 255, 0, 255));
        // sz.val() + spacing +=> x;

        // ImGui::Dummy(@((sz.val() + spacing) * 13.2f, (sz.val() + spacing) * 3.0f));
        // ImGui::PopItemWidth();
        // ImGui::EndTabItem();
    }
    UI.end();
}
