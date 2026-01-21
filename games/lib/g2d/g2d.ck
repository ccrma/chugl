/*
2D Immediate-mode vector graphics library

TODO 
- modify outputpass gamma so that we can undo the srgb texture
	- basically just make sure (.5, 0, 0) shows up as (127, 0, 0)
- add depth/layer option
- catmull-rom splines
- GPolygon triangulation
- thick lines (can they all be batch drawn?)
- disable/enable srgb view (currently chugl defaults to srgb backbuffer)
- camera shake (?)

*/

public class G2D extends GGen
{
	this --> GG.scene();

    // initialize batch drawers
    G2D_Circles circles[6];
		// create one for each blend mode
	// TODO @BUG the transparency stuff is giga broken
	for (int i; i < circles.size(); i++) {
		// if (i != Material.BLEND_MODE_REPLACE) circles[i].material.transparent(true);
		circles[i].material.blend(i);

		circles[i].mesh --> GG.scene();
	}

    G2D_Ellipse ellipses[6];
	for (int i; i < ellipses.size(); i++) {
		if (i != Material.BLEND_MODE_REPLACE) ellipses[i].material.transparent(true);
		ellipses[i].material.blend(i);

		ellipses[i].mesh --> GG.scene();
	}

    G2D_Lines lines;

    G2D_TriangleStrip tristrips[6];
	for (int i; i < tristrips.size(); i++) {
		if (i != Material.BLEND_MODE_REPLACE) ellipses[i].material.transparent(true);
		tristrips[i].material.blend(i);
		tristrips[i].mesh --> GG.scene();
	}

    G2D_SolidPolygon polygons[6];
	for (int i; i < polygons.size(); i++) {
		// replace is NOT transparent
		// if (i != Material.BLEND_MODE_REPLACE) polygons[i].solid_polygon_material.transparent(true);
		polygons[i].solid_polygon_material.blend(i);
		polygons[i].mesh --> GG.scene();
	}

    G2D_Sprite sprites[6]; 
	for (int blend_mode; blend_mode < sprites.size(); blend_mode++) {
		blend_mode => sprites[blend_mode].blend_mode;
		sprites[blend_mode].mesh --> GG.scene();
	}

    G2D_Capsule capsules;
	G2D_Text texts;

    // connect to scene
    lines.mesh --> GG.scene();
	capsules.mesh --> GG.scene();

	texts.mesh --> GG.scene();
	texts.mesh.posZ(0.01); // default on top

	// init camera
	GG.camera().orthographic();
	GG.camera().viewSize(10);
	GG.camera().posZ(GG.camera().clipFar() - 1.0);

	// disable tonemapping / HDR
	GG.outputPass().tonemap(OutputPass.ToneMap_None);
	// disable gamma
	GG.outputPass().gamma(false);
	// TODO: disable the srgb view on swapchain screen buffer

	fun void sortDepthByY(int b) { 
		for (auto s : sprites) b => s.sort_depth_by_y;
		// @TODO the other shaders
	}

	// ------------------- constants --------------------------
	Material.BLEND_MODE_ALPHA => int BLEND_ALPHA;
	Material.BLEND_MODE_ADD => int BLEND_ADD;
	Material.BLEND_MODE_SUBTRACT => int BLEND_SUB;
	Material.BLEND_MODE_MULTIPLY => int BLEND_MULT;

	@(1, 0) => vec2 RIGHT;
	@(-1, 0) => vec2 LEFT;
	@(0, 1) => vec2 UP;
	@(0, -1) => vec2 DOWN;

	// ------------------- params (updated every frame) --------------------------
	n2w(-1, -1) => vec2 screen_min; // bottom left 
	n2w(1, 1)   => vec2 screen_max; // top right
	screen_max.x - screen_min.x => float screen_w;
	screen_max.y - screen_min.y => float screen_h;

	// ------------------- state stacks --------------------------
	// note: these config stacks are cleared at the end of every frame to prevent accidental leaks
	// you *don't* have to call popXXX() for every pushXXX
	[null] @=> string font_stack[];
	[1.0] @=> float font_size_stack[];
	[Color.WHITE] @=> vec3 color_stack[];
	[Color.BLACK] @=> vec3 emission_stack[];
	[0.0] @=> float layer_stack[]; // z depth
	[0.0] @=> float polygon_radius_stack[]; // rounded corners
	[1.0] @=> float alpha_stack[];
	[Material.BLEND_MODE_ALPHA] @=> int blend_stack[];

	fun void pushFont(string s) { font_stack << s; }
	fun void popFont() { font_stack.popBack(); }
	fun void pushFontSize(float s) { font_size_stack << s; }
	fun void popFontSize() { font_size_stack.popBack(); }
	fun void pushColor(vec3 c) { color_stack << c; }
	fun void popColor() { color_stack.popBack(); }
	fun void pushEmission(vec3 c) { emission_stack << c; }
	fun void popEmission() { emission_stack.popBack(); }
	fun void pushLayer(float layer) { 
		layer_stack << layer; 
		layer => lines.z_layer;
	}
	fun void popLayer() { 
		layer_stack.popBack(); 
		layer_stack[-1] => lines.z_layer; 
	}
	fun float layer() { return layer_stack[-1]; }
	fun void pushTextMaxWidth(float w) { texts.max_width_stack << w; }
	fun void popTextMaxWidth() { texts.max_width_stack.popBack(); }
	fun void pushTextControlPoint(vec2 cp) { texts.control_point_stack << cp; }
	fun void pushTextControlPoint(float x, float y) { texts.control_point_stack << @(x, y); }
	fun void popTextControlPoint() { texts.control_point_stack.popBack(); }
	fun void pushPolygonRadius(float r) { polygon_radius_stack << r; }
	fun void popPolygonRadius() { polygon_radius_stack.popBack(); }

	fun void pushAlpha(float a) { alpha_stack << a; }
	fun void popAlpha() { alpha_stack.popBack(); }
	fun void pushBlend(int b) { blend_stack << b; }
	fun void popBlend() { blend_stack.popBack(); }

	// ----------- look & feel (aka antialias and resolution) ----------
	fun void antialias(int bool) {
		for (auto c : circles) c.antialias(bool);
		for (auto e : ellipses) e.antialias(bool);
		bool => texts.antialias;
		capsules.antialias(bool);
		GG.scenePass().msaa(bool);
		GG.outputPass().sampler(bool ? TextureSampler.linear() : TextureSampler.nearest());

		// set sprite sampler
		// TODO have push/pop sampler API?
		if (bool) {
			TextureSampler.linear() @=> TextureSampler sampler;
			TextureSampler.WRAP_CLAMP => sampler.wrapU;
			TextureSampler.WRAP_CLAMP => sampler.wrapV;
			TextureSampler.WRAP_CLAMP => sampler.wrapW;
			sampler @=> G2D_Sprite.sprite_sampler;
		}
		else TextureSampler.nearest() @=> G2D_Sprite.sprite_sampler;
	}

	fun void resolution(int w, int h) {
		TextureDesc texture_desc;
		false => texture_desc.mips;
		false => texture_desc.resizable;
		w => texture_desc.width;
		h => texture_desc.height;
		Texture color_target(texture_desc);
		GG.scenePass().colorOutput(color_target);
		GG.outputPass().input(color_target);

		// setting a fixed resolution color target means we need to lock camera aspect
		// this is an experimental API to disable the auto-update based on window framebuffer aspect
		// because the window resolution can change, but the aspect of the actual color 
		// target stays locked
		GG.camera().aspect(w$float/h);
	}
	
	// return whether abs(pos) is >= bounding_radius_ndc 
    fun static int offscreen(vec2 pos, float bounding_radius_ndc) {
        GG.camera().worldPosToNDC(@(pos.x, pos.y, 0)) => vec3 pos_ndc;
        return (Math.fabs(pos_ndc.x) > bounding_radius_ndc || Math.fabs(pos_ndc.y) > bounding_radius_ndc);
    }

    fun static int offscreen(vec2 pos) {
        return offscreen(pos, 1.0);
    }

	fun static vec2 world2ndc(vec2 pos) {
        return GG.camera().worldPosToNDC(@(pos.x, pos.y, 0)) $ vec2;
	}

	fun void backgroundColor(vec3 color) {
		GG.scene().backgroundColor(color);
	}

	// get the screen dimensions in worldspace units
	fun vec2 screenSize() {
		return ndc2world(1, 1) - ndc2world(-1, -1);
	}

	// in NDC (normalized device coordinates) the screen boundaries go from [-1, 1] in x and y
	fun vec2 NDCToWorldPos(float x, float y) {
		GG.camera().NDCToWorldPos(@(x, y, 0)) => vec3 world_pos;
		return world_pos $ vec2;
	}

	fun vec2 NDCToWorldPos(vec2 ndc) {
		GG.camera().NDCToWorldPos(@(ndc.x, ndc.y, 0)) => vec3 world_pos;
		return world_pos $ vec2;
	}

	fun vec2 ndc2world(vec2 ndc) {
		GG.camera().NDCToWorldPos(@(ndc.x, ndc.y, 0)) => vec3 world_pos;
		return world_pos $ vec2;
	}

	fun vec2 n2w(vec2 ndc) {
		GG.camera().NDCToWorldPos(@(ndc.x, ndc.y, 0)) => vec3 world_pos;
		return world_pos $ vec2;
	}

	fun vec2 n2w(float x, float y) {
		GG.camera().NDCToWorldPos(@(x, y, 0)) => vec3 world_pos;
		return world_pos $ vec2;
	}

	fun vec2 ndc2world(float x, float y) {
		GG.camera().NDCToWorldPos(@(x, y, 0)) => vec3 world_pos;
		return world_pos $ vec2;
	}

	// clamp p to the bounds of the screen (in world space)
	fun vec2 clampScreen(vec2 p) {
		.5 * GG.camera().viewSize() => float hh;
		hh * GG.camera().aspect() => float hw;
		GG.camera().posWorld() $ vec2 => vec2 c;
		return @(
			Math.clampf(p.x, c.x - hw, c.x + hw),
			Math.clampf(p.y, c.y - hh, c.y + hh)
		);
	}

	// get the bounds in world space of the screen
	// fun vec4 screenBounds() {
	// 	GG.camera().NDCToWorldPos(@(-1.0, -1.0, 0)) => vec3 bottom_left;
	// 	GG.camera().NDCToWorldPos(@(1.0, 1.0, 0)) => vec3 top_right;
	// 	return (Math.fabs(pos_ndc.x) > threshold || Math.fabs(pos_ndc.y) > threshold);
	// }

	// ----------- input --------------
	// returns world-coordinates of mouse pos
	fun vec2 mousePos() {
		GG.camera().screenCoordToWorldPos(GWindow.mousePos(), 1.0) => vec3 world_pos;
		return @(world_pos.x, world_pos.y);
	}

	fun vec2 mouseDeltaWorld() { 
		GWindow.mouseDeltaPos() => vec2 delta;

		GG.camera().viewSize() => float height_world;
		height_world * GG.camera().aspect() => float width_world;

		GWindow.windowSize() => vec2 w;
		(delta.x / w.x) * width_world => delta.x; 
		(delta.y / w.y) * height_world => delta.y; 
		-1 *=> delta.y; // flip y

		return delta;
	}

	fun int mouseLeftDown() { return GWindow.mouseLeftDown(); }
	fun int mouseLeftUp() { return GWindow.mouseLeftUp(); }
	fun int mouseLeft() { return GWindow.mouseLeft(); }

	fun int anyInput() { return GWindow.mouseLeft() || GWindow.mouseRight() || GWindow.keys().size(); }
	fun int anyInputDown() { return GWindow.mouseLeftDown() || GWindow.mouseRightDown() || GWindow.keysDown().size(); }
	fun int anyInputUp() { return GWindow.mouseLeftUp() || GWindow.mouseRightUp() || GWindow.keysUp().size(); }

	// ------------------- effects --------------------------
	Effect effects[0];
	int effects_count;
	fun void add(Effect e) {
		if (effects_count == effects.size()) effects << e;
		else e @=> effects[effects_count];
		effects_count++;
	}

	fun void remove(Effect e, int idx) {
		if (effects[idx] != e) {
			<<< "error, effect does not match idx", idx >>>;
			return;
		}

		// swap with last and decrement count
		effects[effects_count - 1] @=> effects[idx];
		e @=> effects[effects_count - 1];
		effects_count--;
	}

	// note: to stop an effect call effect.stop();
	fun void explode(vec2 pos) { add(new ExplodeEffect(pos, 1, 1, Color.WHITE, 0, Math.two_pi, ExplodeEffect.Shape_Lines)); }
	fun void explode(vec2 pos, float radius, dur d) { add(new ExplodeEffect(pos, d/second, radius, Color.WHITE, 0, Math.two_pi, ExplodeEffect.Shape_Lines)); }
	fun void explode(vec2 pos, float radius, dur d, vec3 color, float angle, float width, int type) { 
		add(new ExplodeEffect(pos, d/second, radius, color, angle, width, type)); 
	}

	fun void score(string s, vec2 pos, dur d, float dy, float size) { add(new ScoreEffect(s, pos, d/second, dy, size )); }
	fun void screenFlash(dur d) { add(new ScreenFlashEffect(d/second)); }

	fun void hitFlash(dur d, float size, vec2 pos, vec3 color) { add(new HitFlashEffect(d/second, size, pos, color)); }

    // ----------- ellipse ----------
	fun void ellipseFilled(vec2 c, vec2 ab, vec4 color) {
		ellipses[blend_stack[-1]].ellipse(c, ab, Math.max(ab.x, ab.y), layer_stack[-1], color);
	}

	fun void ellipseFilled(vec2 c, vec2 ab, vec3 color) {
		ellipses[blend_stack[-1]].ellipse(c, ab, Math.max(ab.x, ab.y), layer_stack[-1], 
			@(color.r, color.g, color.b, alpha_stack[-1])
		);
	}

	fun void ellipse(vec2 c, vec2 ab, vec4 color) {
		ellipses[blend_stack[-1]].ellipse(c, ab, 0, layer_stack[-1], color);
	}

	fun void ellipse(vec2 c, vec2 ab, float thickness, vec4 color) {
		ellipses[blend_stack[-1]].ellipse(c, ab, thickness, layer_stack[-1], color);
	}

    // ----------- circle ----------
	fun void circle(vec2 center, float radius, float thickness, vec3 color) {
        circles[blend_stack[-1]].circle(
			center, radius, thickness, @(color.r, color.g, color.b, alpha_stack[-1]), layer_stack[-1]
		);
	}

	fun void circleFilled(vec2 center, float radius, vec3 color) {
        circles[blend_stack[-1]].circle(center, radius, 1.0, @(color.r, color.g, color.b, alpha_stack[-1]), layer_stack[-1]);
	}

	fun void circleFilled(vec2 center, float radius, vec4 color) {
        circles[blend_stack[-1]].circle(center, radius, 1.0, color, layer_stack[-1]);
	}

	fun void circleFilled(vec2 center, float radius) {
		@(
			color_stack[-1].r,
			color_stack[-1].g,
			color_stack[-1].b,
			alpha_stack[-1]
		) => vec4 color;
        circles[blend_stack[-1]].circle(center, radius, 1.0, color, layer_stack[-1]);
	}

    // ----------- tristrip -----------
	fun void pushStrip(vec2 p) {
		tristrips[blend_stack[-1]].push(p, layer_stack[-1], color_stack[-1], alpha_stack[-1]);
	}

	fun void endStrip() {
		tristrips[blend_stack[-1]].end();
	}

    // ----------- line -----------
    // draws a dashed line from p1 to p2, each dash is length `segment_length`
	fun void dashed(vec2 p1, vec2 p2, vec3 color, float segment_length) {
		p2 - p1 => vec2 d;
		Math.euclidean(p1, p2) => float dist;
		d / dist => vec2 dir;
		dist / segment_length => float N;

		for (int i; i < N; 2 +=> i) {
			lines.segment(
				(p1 + i * segment_length * dir),
				(p1 + Math.min((i + 1), N) * segment_length * dir),
				color
			);
		}
	}

	fun void line(vec2 p1, vec2 p2, vec3 color) {
		lines.segment(p1, p2, color);
	}

	fun void line(vec2 p1, vec2 p2) {
		lines.segment(p1, p2, color_stack[-1]);
	}

	// assumes arr is circular buff and we draw from start_idx --> start_idx - 1 with wraparound
	fun void line(vec2 arr[], int start_idx, int count) {
		while (start_idx < 0) arr.size() +=> start_idx;
		arr.size() => int size;
		Math.min(size, count) => count;
		for (int i; i < count-1; i++) {
			lines.segment(
				arr[(start_idx + i) % size],
				arr[(start_idx + i + 1) % size],
				color_stack[-1]
			);
		}
	}

	// draw line segments
	fun void line(vec2 arr[], int loop) {
		for (int i; i < arr.size() - 1; i++) {
			lines.segment(
				arr[i],
				arr[i + 1],
				color_stack[-1]
			);
		}
		if (loop && arr.size() > 0) lines.segment(arr[-1], arr[0], color_stack[-1]);
	}

	// draws a polygon outline at given position and rotation
	fun void polygon(vec2 pos, float rot_radians, vec2 vertices[], vec2 scale, vec3 color) {
		if (vertices.size() < 2) return;

        Math.cos(rot_radians) => float cos_a;
        Math.sin(rot_radians) => float sin_a;

		// just draw as individual line segments for now
		for (int i; i < vertices.size(); i++) {
			vertices[i] => vec2 v1;
			vertices[(i + 1) % vertices.size()] => vec2 v2;

			// scale
			scale.x *=> v1.x; scale.y *=> v1.y;
			scale.x *=> v2.x; scale.y *=> v2.y;

			lines.segment(
				pos + @(
					cos_a * v1.x - sin_a * v1.y,
					sin_a * v1.x + cos_a * v1.y
				),
				pos + @(
					cos_a * v2.x - sin_a * v2.y,
					sin_a * v2.x + cos_a * v2.y
				),
                color
			);
		}

		// // to close the loop
		// vertices[-1] => vec2 v1;
		// vertices[0] => vec2 v2;

		// // scale
		// scale.x *=> v1.x; scale.y *=> v1.y;
		// scale.x *=> v2.x; scale.y *=> v2.y;

		// drawlines.segment(
		// 	pos + @(
		// 		cos_a * v1.x - sin_a * v1.y,
		// 		sin_a * v1.x + cos_a * v1.y
		// 	),
		// 	pos + @(
		// 		cos_a * v2.x - sin_a * v2.y,
		// 		sin_a * v2.x + cos_a * v2.y
		// 	)
		// );
	}

	fun void polygon(vec2 pos, float rot_radians, vec2 vertices[], vec2 scale) {
		polygon(pos, rot_radians, vertices, scale, color_stack[-1]);
	}

	fun void polygon(vec2 pos, float rot_radians, vec2 vertices[], vec3 color) {
		polygon(pos, rot_radians, vertices, @(1,1), color);
	}

	fun void polygon(vec2 pos, float rot_radians, vec2 vertices[]) {
		polygon(pos, rot_radians, vertices, @(1,1), color_stack[-1]);
	}

	[@(0.5, 0.5), @(-0.5, 0.5), @(-0.5, -0.5), @(0.5, -0.5)] @=> vec2 square_vertices[];
	fun void box(vec2 center, float width, float height, vec3 color) {
		polygon(center, 0, square_vertices, @(width, height), color);
	}

	fun void box(vec2 center, float width, float height) {
		polygon(center, 0, square_vertices, @(width, height));
	}

	fun void box(vec2 center, float width, float height, float rot_radians) {
		polygon(center, rot_radians, square_vertices, @(width, height));
	}

	fun void box(vec2 bottom_left, vec2 top_right) {
		line(bottom_left, @(top_right.x, bottom_left.y));
		line(@(top_right.x, bottom_left.y), top_right);
		line(top_right, @(bottom_left.x, top_right.y));
		line(@(bottom_left.x, top_right.y), bottom_left);
	}

	fun void square(vec2 pos, float rot_radians, float size, vec3 color) {
		polygon(pos, rot_radians, square_vertices, @(size, size), color);
	}

	32 => int circle_segments;
	vec2 circle_vertices[circle_segments];
	for (int i; i < circle_segments; i++) {
		Math.two_pi * i / circle_segments => float theta;
		@( Math.cos(theta), Math.sin(theta)) => circle_vertices[i];
	}
	fun void circle(vec2 pos, float radius, vec3 color) {
		for (int i; i < circle_vertices.size() - 1; i++) {
			lines.segment(pos + radius * circle_vertices[i], pos + radius * circle_vertices[i+1], color);
		}
		lines.segment(pos + radius * circle_vertices[-1], pos + radius * circle_vertices[0], color);
	}

	fun void circle(vec2 pos, float radius) {
		circle(pos, radius, color_stack[-1]);
	}

	vec2 dotted_circle_vertices[0];
	fun void circleDotted(vec2 pos, float radius, float start_theta, vec3 color) {
		dotted_circle_vertices.clear();
		// draw every other segment
		for (int i; i < 32; i++) {
			(Math.two_pi * i / 32) + start_theta => float theta;
			pos + radius * @(Math.cos(theta), Math.sin(theta)) => vec2 vertex;
			dotted_circle_vertices << vertex;
		}

		for (int i; i < 32; 2 +=> i) {
			lines.segment(dotted_circle_vertices[i], dotted_circle_vertices[i+1], color);
		}
	}

    // ---------- filled polygons ----------

	fun void polygonFilled(vec2 pos, vec2 rotation, vec2 vertices[], float radius) {
		polygons[blend_stack[-1]].polygonFilled(
			pos, rotation, 1.0, vertices, radius, color_stack[-1], layer_stack[-1], alpha_stack[-1]
		);
	}

	fun void polygonFilled(vec2 pos, float rot, vec2 vertices[], float radius) {
		polygons[blend_stack[-1]].polygonFilled(
			pos, @(Math.cos(rot), Math.sin(rot)), 1.0, vertices, radius, color_stack[-1], layer_stack[-1], alpha_stack[-1]
		);
	}

	fun void polygonFilled(vec2 pos, vec2 rotation, float sca, vec2 vertices[], float radius, vec3 color) {
		polygons[blend_stack[-1]].polygonFilled(
			pos, rotation, sca, vertices, radius, color, layer_stack[-1], alpha_stack[-1]
		);
	}

	fun void polygonFilled(vec2 pos, float rot, float sca, vec2 vertices[], float radius, vec3 color) {
		polygons[blend_stack[-1]].polygonFilled(
			pos, @(Math.cos(rot), Math.sin(rot)), sca, vertices, radius, color, layer_stack[-1], alpha_stack[-1]
		);
	}

	fun void boxFilled(
		vec2 position,
		vec2 rotation,
		float width,
		float height,
		vec3 color
	) {
		if (rotation.dot(rotation) == 0) @(1, 0) => rotation;
		.5 * width => float hw;
		.5 * height => float hh;
		polygons[blend_stack[-1]].polygonFilled(
			position, rotation, 1.0,
			[@(-hw, hh), @(-hw, -hh), @(hw, -hh), @(hw, hh)], 
			polygon_radius_stack[-1],
			color,
			layer_stack[-1],
			alpha_stack[-1]
		);
	}

	fun void boxFilled(
		vec2 position,
		float width,
		float height,
		vec3 color
	) {
		.5 * width => float hw;
		.5 * height => float hh;
		polygons[blend_stack[-1]].polygonFilled(
			position, @(1, 0), 1.0,
			[@(-hw, hh), @(-hw, -hh), @(hw, -hh), @(hw, hh)], 
			polygon_radius_stack[-1],
			color,
			layer_stack[-1],
			alpha_stack[-1]
		);
	}

	fun void boxDotted(vec2 pos, float w, float h, float segment_length) {
		.5 * w => float hw;
		.5 * h => float hh;
		dashed(pos + @(hw, -hh), pos + @(hw, hh), color_stack[-1], segment_length);
		dashed(pos + @(hw, hh), pos + @(-hw, hh), color_stack[-1], segment_length);
		dashed(pos + @(-hw, hh), pos + @(-hw, -hh), color_stack[-1], segment_length);
		dashed(pos + @(-hw, -hh), pos + @(hw, -hh), color_stack[-1], segment_length);
	}

	fun void boxFilled(vec2 top_left, vec2 bottom_right, vec3 color) {
		boxFilled(
			(top_left + bottom_right) * .5, // pos
			@(1, 0), // rot
			Math.fabs(top_left.x - bottom_right.x), // w
			Math.fabs(top_left.y - bottom_right.y), // h
			color
		);
	}

	fun void squareFilled(
		vec2 position,
		float rotation_radians,
		float l,
		vec3 color
	) {
		boxFilled(position, 
			@(Math.cos(rotation_radians), Math.sin(rotation_radians)), 
			l, l, color
		);
	}

	fun void capsuleFilled(
		vec2 pos, 
		float w /* p1 to p2 width */, 
		float radius, float rotation, vec3 color
	) {
		// @optimize: switch to Nick's speedy sin
		@(w * Math.cos(rotation), w * Math.sin(rotation)) => vec2 p; 
		capsules.capsule(.5 * p + pos, -.5 * p + pos, radius, @(color.r, color.g, color.b, 1.0));
	}

	fun void capsuleFilled(
		vec2 p1, vec2 p2, float radius, vec3 color
	) {
		capsules.capsule(p1, p2, radius, @(color.r, color.g, color.b, 1.0));
	}

    // ---------- text ----------

	fun void textLayer(float z) { texts.mesh.posZ(z); } // sets the starting Z position for all text


	// fun void text(
	// 	string s, string font, vec3 pos, float size, vec2 sca, float rot, 
	// 	vec3 color, float alpha
	// )
	fun void text(string s) { texts.text(s, font_stack[-1], @(0,0), font_size_stack[-1], @(1, 1), 0, color_stack[-1], alpha_stack[-1], layer_stack[-1]); }
	fun void text(string s, vec2 pos) { texts.text(s, font_stack[-1], pos, font_size_stack[-1], @(1,1), 0, color_stack[-1], alpha_stack[-1], layer_stack[-1]); }
	fun void text(string s, vec2 pos, float size) { texts.text(s, font_stack[-1], pos, size, @(1,1), 0, color_stack[-1], alpha_stack[-1], layer_stack[-1]); }
	fun void text(string s, vec2 pos, float size, float rot) { texts.text(s, font_stack[-1], pos, size, @(1, 1), rot, color_stack[-1], alpha_stack[-1], layer_stack[-1]); }
	fun void text(string s, vec2 pos, vec2 sca, float rot) { texts.text(s, font_stack[-1], pos, font_size_stack[-1], sca, rot, color_stack[-1], alpha_stack[-1], layer_stack[-1]); }
	fun void text(string s, vec3 pos) { texts.text(s, font_stack[-1], pos $ vec2, font_size_stack[-1], @(1,1), 0, color_stack[-1], alpha_stack[-1], pos.z); }
	fun void text(string s, vec3 pos, float size) { texts.text(s, font_stack[-1], pos $ vec2, size, @(1,1), 0, color_stack[-1], alpha_stack[-1], pos.z); }
	fun void text(string s, vec3 pos, float size, float rot) { texts.text(s, font_stack[-1], pos $ vec2, size, @(1, 1), rot, color_stack[-1], alpha_stack[-1], pos.z); }
	fun void text(string s, vec3 pos, vec2 sca, float rot) { texts.text(s, font_stack[-1], pos $ vec2, font_size_stack[-1], sca, rot, color_stack[-1], alpha_stack[-1], pos.z); }

    // ---------- sprites ----------
	fun void sprite(Texture tex, vec2 pos) {
		sprites[blend_stack[-1]].sprite(tex, @(pos.x, pos.y, layer_stack[-1]), @(1,1), 0, color_stack[-1], emission_stack[-1], alpha_stack[-1]);
	}

	fun void sprite(Texture tex, vec2 pos, float sca, float rot) {
		sprites[blend_stack[-1]].sprite(tex, @(pos.x, pos.y, layer_stack[-1]), @(sca,sca), rot, color_stack[-1], emission_stack[-1], alpha_stack[-1]);
	}

	fun void sprite(Texture tex, vec2 pos, float sca, float rot, vec3 color) {
		sprites[blend_stack[-1]].sprite(tex, @(pos.x, pos.y, layer_stack[-1]), @(sca,sca), rot, color, emission_stack[-1], alpha_stack[-1]);
	}
	
	fun void sprite(Texture tex, vec2 pos, vec2 sca, float rot) {
		sprites[blend_stack[-1]].sprite(tex, @(pos.x, pos.y, layer_stack[-1]), sca, rot, color_stack[-1], emission_stack[-1], alpha_stack[-1]);
	}

	fun void sprite(Texture tex, vec2 pos, vec2 sca, float rot, vec3 color) {
		sprites[blend_stack[-1]].sprite(tex, @(pos.x, pos.y, layer_stack[-1]), sca, rot, color, emission_stack[-1], alpha_stack[-1]);
	}

	// for 1d horizontal sprite sheets
	fun void sprite(
		Texture tex, int n_frames, int frame, vec2 pos, vec2 sca, float rot, vec3 color
	) {
		sprites[blend_stack[-1]].sprite(
			tex, @(n_frames, 1), @(frame$float / n_frames, 0), 
			@(pos.x, pos.y, layer_stack[-1]), sca, rot, color, emission_stack[-1], alpha_stack[-1]
		);
	}

	fun void sprite(
		Texture tex, vec2 sprite_sheet_frame_dim, vec2 offset, vec2 pos, vec2 sca, float rot, vec3 color
	) {
		sprites[blend_stack[-1]].sprite(
			tex, sprite_sheet_frame_dim, offset, 
			@(pos.x, pos.y, layer_stack[-1]), sca, rot, color, emission_stack[-1], alpha_stack[-1]
		);
	}



    // ----------- characters -----------

	// string to color map
	vec3 char_to_color['z' + 1];
	Color.RED => char_to_color['r'];
	Color.GREEN => char_to_color['g'];
	Color.BLUE => char_to_color['b'];
	Color.CYAN => char_to_color['c'];
	Color.YELLOW => char_to_color['y'];
	Color.PURPLE => char_to_color['p'];
	Color.WHITE => char_to_color['w'];
	Color.BLACK => char_to_color['l'];
	Color.ORANGE => char_to_color['o'];

	// draws an NxN sprite
	// assumes first character is a newline
	// e.g.
	// "
	// www
	// wbw
	// www
	// " => string sprite;
	fun void char(string s, vec2 pos, float size, float pixel_sca, vec3 color) {
		// first calculate the dim
		s.find('\n', 1) - 1 => int N;
		if (N <= 0) return;

		s.length() => int len;

		// for now assume entire size is 1x1
		size / N => float sz;
		pos - (N - 1)*.5*@(sz,-sz) => vec2 start;
		
		int tile;
		for (int i; i < len; ++i) {
			s.charAt(i) => int c;
			if (c == '\n' || c == '\t') continue;
			
			if (c != ' ') {
				tile / N => int y;
				tile - (N * y) => int x;
				// pushPolygonRadius(sz*.25);
				squareFilled(
					start + @(sz * x, -sz * y), 0, sz*pixel_sca, char_to_color[c] + color
				);
				// popPolygonRadius();
			}

			++tile;
		}
	}


    // ---------- misc shapes --------------------------------------

	fun void chevron(
		vec2 pos, float rot_rad, float angle_rad, float height, float thickness, vec3 color
	) {
		height * .5 => float hh;
		Math.tan(angle_rad * .5) => float tan_theta;
		// sadly polygons only supports convex shapes (which a chevron is not)
		// so drawing as two thick lines with miter join
		// @TODO how to support non-convex polygons? check freya shapes
		// does it require earcut triangulation?
		pushColor(color);
		polygonFilled(pos, rot_rad, 
			[
				@(0, hh),
				@(hh / tan_theta, 0),
				@(thickness + hh / tan_theta, 0),
				@(thickness, hh),
			],
			0);
		polygonFilled(pos, rot_rad, 
			[
				@(hh / tan_theta, 0),
				@(0, -hh),
				@(thickness, -hh),
				@(thickness + hh / tan_theta, 0),
			],
			0);
		popColor();
	}

	fun void diamond(vec2 pos, float rot, float w, float h) {
		polygonFilled(pos, rot, 
			[
				.5 * w * RIGHT,
				.5 * h * UP,
				.5 * w * LEFT,
				.5 * h * DOWN,
			],
			polygon_radius_stack[-1]
		);
	}

    // ---------- internal --------------------------------------
	fun static void assert(int t, string error) {
		if (!t) <<< error >>>;
	}

    fun void update(float dt) {
		// update effects *BEFORE* updating g2d drawers
		for (effects_count - 1 => int i; i >= 0; i--) { 
			effects[i] @=> Effect e;
			if (!e.update(this, dt)) remove(e, i);
			dt +=> e.uptime;
		}

		for (auto c : circles) c.update();
		for (auto e : ellipses) e.update();
		for (auto p : polygons) p.update();
		for (auto t : tristrips) t.update();
		for (auto s : sprites) s.update();
        lines.update();
		capsules.update();
		texts.update();

		{ // update state params 
			n2w(-1, -1) => screen_min; // bottom left 
			n2w(1, 1)   => screen_max; // top right
			screen_max.x - screen_min.x => screen_w;
			screen_max.y - screen_min.y => screen_h;
		}

		{ // clear state stacks (prevents accidental leak)
			font_stack[0] => string default_font; 
			font_stack.clear(); 
			font_stack << default_font;

			font_size_stack[0] => float default_font_size;
			font_size_stack.clear();
			font_size_stack << default_font_size;

			color_stack[0] => vec3 default_text_color; 
			color_stack.clear(); 
			color_stack << default_text_color;

			emission_stack.erase(1, emission_stack.size());

			layer_stack[0] => float default_layer; 
			layer_stack.clear(); 
			this.pushLayer(default_layer);

			polygon_radius_stack[0] => float default_polygon_radius;
			polygon_radius_stack.clear();
			polygon_radius_stack << default_polygon_radius;

			alpha_stack.erase(1, alpha_stack.size());
			blend_stack.erase(1, blend_stack.size());
		}
    }
}

// text pool
public class G2D_Text
{
	GText text_pool[0];
	int text_count;
	GGen mesh; // parent of all GTexts

	// stack of wrap widths
	[0.0] @=> float max_width_stack[];
	[@(0.5, 0.5)] @=> vec2 control_point_stack[];
	1 => int antialias;


	fun void text(
		string s, string font, vec2 pos, float size, vec2 sca, float rot, 
		vec3 color, float alpha, float layer
	) {
		if (text_count == text_pool.size()) {
			text_pool << new GText;
			text_pool[-1].align(1);
		}
		text_pool[text_count] @=> GText@ gtext;


		gtext --> mesh;
		gtext.pos(@(pos.x, pos.y, layer));
		gtext.size(size);
		gtext.sca(sca);
		gtext.rotZ(rot);
		gtext.color(color);
		gtext.text(s);
		gtext.alpha(alpha);
		gtext.antialias(antialias);
		if (font != null) gtext.font(font);
		gtext.maxWidth(max_width_stack[-1]); // @optimize: setting maxWidth triggers rebuild, set stale flag instead to batch rebuild
		gtext.controlPoints(control_point_stack[-1]);

		text_count++;
	}

	fun void update() {
		// detach remaining GText pool from the scene
		for (text_count => int i; i < text_pool.size(); i++) {
			// if parent is null, all subsequent GTexts are already detached
			if (text_pool[i].parent() == null) break;
			text_pool[i].detachParent();
		}

		max_width_stack.erase(1, max_width_stack.size());
		control_point_stack.erase(1, control_point_stack.size());

		0 => text_count;
	}
}

public class G2D_Circles
{
	me.dir() + "./g2d_circle_shader.wgsl" @=> string shader_path;	

	// set drawing shader
	static ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	null @=> shader_desc.vertexLayout; 

	// material shader (draws all line segments in 1 draw call)
	static Shader@ shader;
	if (shader == null) new Shader(shader_desc) @=> shader;

	Material material;
	material.shader(shader);
	antialias(true); // default antialiasing to true

	// default bindings
	float empty_float_arr[4];
	initStorageBuffers();

	// vertex buffers
	vec4 u_center_radius_thickness[0];
	vec4 u_colors[0];
	float u_layers[0];

	Geometry geo;
	geo.vertexCount(0);
	GMesh mesh(geo, material);

	fun void initStorageBuffers() {
		material.storageBuffer(0, empty_float_arr);
		material.storageBuffer(1, empty_float_arr);
		material.storageBuffer(2, empty_float_arr);
	}

	// set whether to antialias circles
	fun void antialias(int value) {
		material.uniformInt(3, value);
	}

	fun void circle(vec2 center, float radius, float thickness, vec4 color, float z_layer) {
		u_center_radius_thickness << @(center.x, center.y, radius, thickness);
		u_colors << color;
		u_layers << z_layer;
	}

	fun void update() {
		if (u_center_radius_thickness.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// update GPU vertex buffers
		material.storageBuffer(0, u_center_radius_thickness);
		material.storageBuffer(1, u_colors);
		material.storageBuffer(2, u_layers);
		geo.vertexCount(6 * u_center_radius_thickness.size());

		// reset
		u_center_radius_thickness.clear();
		u_colors.clear();
		u_layers.clear();
	}
}

public class G2D_Ellipse
{
	me.dir() + "./g2d_ellipse_shader.wgsl" @=> string shader_path;

	// set drawing shader
	static ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	null @=> shader_desc.vertexLayout; 

	// material shader (draws all line segments in 1 draw call)
	static Shader@ shader;
    if (shader == null) new Shader(shader_desc) @=> shader;

	Material material;
	material.shader(shader);
	antialias(true); // default antialiasing to true

	// default bindings
	float empty_float_arr[4];
	initStorageBuffers();

	// vertex buffers
	float u_data[0];

	Geometry geo;
	geo.vertexCount(0);

	GMesh mesh(geo, material);

	fun void initStorageBuffers() {
		material.storageBuffer(0, empty_float_arr);
	}

	// set whether to antialias
	fun void antialias(int value) {
		material.uniformInt(1, value);
	}

	// [ vec2 center, vec2 ab, f32 thickness, f32 layer, vec4 color]
	fun void ellipse(vec2 c, vec2 ab, float thickness, float layer, vec4 color) {
		u_data << c.x << c.y << ab.x << ab.y << thickness << layer << color.r << color.g << color.b << color.a;
	}

	fun void update() {
		if (u_data.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		if (u_data.size() % 10 != 0) 
			<<< "u_data not properly filled" >>>;

		// update GPU vertex buffers
		material.storageBuffer(0, u_data);
		geo.vertexCount(6 * u_data.size()/10);

		// reset
		u_data.clear();
	}
}

// batch draws simple line segments (no width)
public class G2D_Lines
{
	me.dir() + "./g2d_lines_shader.wgsl" @=> string shader_path;

	// set drawing shader
	static ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	[VertexFormat.Float3, VertexFormat.Float3] @=> shader_desc.vertexLayout;

	// material shader (draws all line segments in 1 draw call)
	static Shader@ shader;
    if (shader == null) new Shader(shader_desc) @=> shader;

	Material material;
	material.shader(shader);

	// z level
	float z_layer;

	// ==optimize== use lineStrip topology + index reset? but then requires using additional index buffer
	material.topology(Material.Topology_LineList); // list not strip!

	// vertex buffers
	vec3 u_positions[0];
	vec3 u_colors[0];

	Geometry geo; // just used to set vertex count
	geo.vertexCount(0);
	GMesh mesh(geo, material);

	fun void segment(vec2 p1, vec2 p2, vec3 color) {
		u_positions << @(p1.x, p1.y, z_layer) << @(p2.x, p2.y, z_layer);
		u_colors << color << color;
	}

	fun void update()
	{
		// update GPU vertex buffers
		geo.vertexAttribute(0, u_positions);
		geo.vertexCount(u_positions.size());
		geo.vertexAttribute(1, u_colors);

		// reset
		u_positions.clear();
		u_colors.clear();
	}
}

public class G2D_SolidPolygon
{
	me.dir() + "./g2d_solid_polygon_shader.wgsl" @=> string shader_path;

	// set drawing shader
	static ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	null => shader_desc.vertexLayout; // no vertex layout

	// material shader (draws all solid polygons in 1 draw call)	
	static Shader@ shader;
    if (shader == null) new Shader(shader_desc) @=> shader;

	Material solid_polygon_material;
	solid_polygon_material.shader(shader);

	// storage buffers
	int u_polygon_vertex_counts[0];
	vec2 u_polygon_vertices[0];
	vec4 u_polygon_transforms[0];
	vec4 u_polygon_colors[0];
	vec4 u_polygon_aabb[0];
	vec2 u_polygon_radius_and_depth[0];

	// initialize material uniforms
	// TODO: binding empty storage buffer crashes wgpu
	int empty_int_arr[1];
	float empty_float_arr[4];
	initStorageBuffers();

	Geometry solid_polygon_geo; // just used to set vertex count
	GMesh mesh(solid_polygon_geo, solid_polygon_material);

	@(Math.FLOAT_MAX, Math.FLOAT_MAX, -Math.FLOAT_MAX, -Math.FLOAT_MAX) => vec4 init_aabb;

	fun void initStorageBuffers() {
		solid_polygon_material.storageBuffer(0, empty_int_arr);
		solid_polygon_material.storageBuffer(1, empty_float_arr);
		solid_polygon_material.storageBuffer(2, empty_float_arr);
		solid_polygon_material.storageBuffer(3, empty_float_arr);
		solid_polygon_material.storageBuffer(4, empty_float_arr);
		solid_polygon_material.storageBuffer(5, empty_float_arr);
	}

	fun void polygonFilled(
		vec2 position,
		vec2 rotation,
		float scale,
		vec2 vertices[], 
		float radius,
		vec3 color,
		float z_layer,
		float alpha
	) {
		// <<< position, rotation, scale, radius, color, z_layer, alpha >>>;

		u_polygon_vertex_counts << u_polygon_vertices.size(); // offset
		u_polygon_vertex_counts << vertices.size(); // count

		init_aabb => vec4 aabb;
		for (auto vert : vertices) {
			vert * scale => vec2 v;
			u_polygon_vertices << v;

			// update aabb
			Math.min(aabb.x, v.x) => aabb.x;
			Math.min(aabb.y, v.y) => aabb.y;
			Math.max(aabb.z, v.x) => aabb.z;
			Math.max(aabb.w, v.y) => aabb.w;
		}

		u_polygon_aabb << @(aabb.x, aabb.y, aabb.z, aabb.w);

		u_polygon_transforms << @(
            position.x, position.y, // position
            rotation.x, rotation.y // rotation
        );

		u_polygon_radius_and_depth << @(radius, z_layer);

		u_polygon_colors << @(color.r, color.g, color.b, alpha);
	}

	// fun void polygonFilled(
	// 	vec2 position,
	// 	float rotation_radians,
	// 	vec2 vertices[], 
	// 	float radius,
	// 	vec3 color
	// ) {
	// 	polygonFilled(position, rotation_radians, 1.0, vertices, radius, color);
	// }

	fun void update() {
		if (u_polygon_aabb.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// upload
		{ // b2 solid polygon
			solid_polygon_material.storageBuffer(0, u_polygon_vertex_counts);
			solid_polygon_material.storageBuffer(1, u_polygon_vertices);
			solid_polygon_material.storageBuffer(2, u_polygon_transforms);
			solid_polygon_material.storageBuffer(3, u_polygon_colors);
			solid_polygon_material.storageBuffer(4, u_polygon_aabb);
			solid_polygon_material.storageBuffer(5, u_polygon_radius_and_depth);

			// update geometry vertices (6 vertices per polygon plane)
			solid_polygon_geo.vertexCount(6 * u_polygon_radius_and_depth.size());
		}

		// zero
		{ // b2 solid polygon
			u_polygon_vertex_counts.clear();
			u_polygon_vertices.clear();
			u_polygon_transforms.clear();
			u_polygon_colors.clear();
			u_polygon_aabb.clear();
			u_polygon_radius_and_depth.clear();
		}
	}
}

public class G2D_Sprite
{
	int sprite_count;

	int sort_depth_by_y;
	int blend_mode;

	FlatMaterial flat_materials[0];
	GMesh sprites[0];

    static PlaneGeometry plane_geo;
    static TextureSampler sprite_sampler;
    TextureSampler.Filter_Nearest => sprite_sampler.filterMin;
    TextureSampler.Filter_Nearest => sprite_sampler.filterMag;
    TextureSampler.Filter_Nearest => sprite_sampler.filterMip;
    TextureSampler.WRAP_CLAMP => sprite_sampler.wrapU;
    TextureSampler.WRAP_CLAMP => sprite_sampler.wrapV;
    TextureSampler.WRAP_CLAMP => sprite_sampler.wrapW;

	GGen mesh;
	mesh.name("G2D_Sprite Mesh");

	fun void _resizeSpritePool() {
		if (sprite_count == sprites.size()) {
			flat_materials << new FlatMaterial;
			flat_materials[-1].sampler(sprite_sampler);
			blend_mode => flat_materials[-1].blend;
			if (blend_mode != Material.BLEND_MODE_REPLACE) true => flat_materials[-1].transparent;
			sprites << new GMesh(plane_geo, flat_materials[-1]);
		}
	}

	// TODO: add instanced mode 
	fun void sprite(
		Texture sprite_sheet, vec2 sprite_sheet_frame_dim, vec2 offset, 
		vec3 pos, vec2 sca, float rot, vec3 color, vec3 emission, float alpha
	) {
		_resizeSpritePool();

		sprites[sprite_count] @=> GMesh@ sprite_mesh;
		flat_materials[sprite_count] @=> FlatMaterial@ sprite_material;
		sprite_material.color(color);
		sprite_material.colorMap(sprite_sheet);
		sprite_material.scale( // set UV sample area to 1 sprite frame
			@(1.0 / sprite_sheet_frame_dim.x, 1.0 / sprite_sheet_frame_dim.y)
		);
		sprite_material.offset(offset); // set UV sample offset
		sprite_material.emissive(@(emission.x, emission.y, emission.z, 0));
		sprite_material.alpha(alpha);

		sprite_mesh --> mesh;
		sprite_mesh.pos(pos);
		sprite_mesh.sca(sca);
		sprite_mesh.rotZ(rot);

		if (sort_depth_by_y) {
			if (sprite_mesh.posY() > 100 || sprite_mesh.posY() < -100) 
				<<< "warning sprite mesh Y is outside of expected sort range" >>>;
			Math.remap(
				sprite_mesh.posY(),
				100, -100,
				0, 1
			) + sprite_mesh.posZ() => sprite_mesh.posZ;
		}

		sprite_count++;
	}

	fun void sprite(Texture tex, vec3 pos, vec2 sca, float rot, vec3 color, vec3 emissive, float alpha) {
		sprite(tex, @(1,1), @(0,0), pos, sca, rot, color, emissive, alpha);
	}

    fun void update() {
		// disconnect unused sprite pool items
		for (sprite_count => int i; i < sprites.size(); i++) {
			if (sprites[i].parent() == null) break;
			sprites[i].detachParent();
		}

		G2D.assert(sprites.size() == flat_materials.size(), "sprites GMesh && material size mismatch");

		0 => sprite_count;
    }
}


public class G2D_Capsule
{	
	me.dir() + "./g2d_capsule_shader.wgsl" @=> string shader_path;

	// set drawing shader
	static ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	null @=> shader_desc.vertexLayout; 

	// material shader (draws all line segments in 1 draw call)
	static Shader@ shader;
    if (shader == null) new Shader(shader_desc) @=> shader;

	Material material;
	material.shader(shader);
	antialias(true);

	// default bindings
	float empty_float_arr[4];
	initStorageBuffers();

	// vertex buffers
	vec2 u_positions[0];
	float u_radius[0];
	vec4 u_colors[0];

	Geometry geo;
	geo.vertexCount(0);
	GMesh mesh(geo, material);

	fun void initStorageBuffers() {
		material.storageBuffer(0, empty_float_arr); // p1, p2
		material.storageBuffer(1, empty_float_arr); // radius
		material.storageBuffer(2, empty_float_arr); // color
	}

	fun void antialias(int a) {
		material.uniformInt(3, a);
	}

	fun void capsule(vec2 p1, vec2 p2, float radius, vec4 color) {
		u_positions << p1 << p2;
		u_radius << radius;
		u_colors << color;
	}

    fun void update() {
		if (u_radius.size() == 0) {
			initStorageBuffers(); // needed because empty storage buffers cause WGPU to crash on bindgroup creation
			return;
		}

		// update GPU vertex buffers
		material.storageBuffer(0, u_positions); // p1, p2
		material.storageBuffer(1, u_radius); // radius
		material.storageBuffer(2, u_colors); // color
		geo.vertexCount(6 * u_radius.size());

		// reset
        u_positions.clear();
        u_radius.clear();
        u_colors.clear();
    }
}

public class G2D_TriangleStrip
{	
	me.dir() + "./g2d_tristrip_shader.wgsl" @=> string shader_path;

	// set drawing shader
	static ShaderDesc shader_desc;
	shader_path => shader_desc.vertexPath;
	shader_path => shader_desc.fragmentPath;
	[VertexFormat.Float3, VertexFormat.Float4] @=> shader_desc.vertexLayout;

	// material shader (draws all line segments in 1 draw call)
	static Shader@ shader;
    if (shader == null) new Shader(shader_desc) @=> shader;

	Material material;
	material.shader(shader);
	material.topology(Material.Topology_TriangleStrip); 

	// vertex buffers
	vec3 u_positions[0];
	vec4 u_colors[0];
	int indices[0];
	int indices_count;

	Geometry geo; // just used to set vertex count
	geo.vertexCount(0);
	GMesh mesh(geo, material);

	fun void push(vec2 p, float z_layer, vec3 c, float alpha) {
		u_positions << @(p.x, p.y, z_layer);
		u_colors << @(c.r, c.g, c.b, alpha);
		indices << indices_count++;
	}

	fun void end() {
		indices << 0xFFFFFFFF; // @TODO make a constant in Material
	}

	fun void update()
	{
		// update GPU vertex buffers
		geo.vertexAttribute(0, u_positions);
		geo.vertexCount(u_positions.size());
		geo.vertexAttribute(1, u_colors);
		geo.indices(indices);

		// reset
		u_positions.clear();
		u_colors.clear();
		indices.clear();
		0 => indices_count;
	}
}

// ==============================
//             FX
// ==============================
public class Effect {
	float uptime; // time in seconds since this effect was initialized
	GG.camera().posZ() - 1 => float layer; // default renders on top of everything

	0 => static int END;
	1 => static int STILL_GOING;

	// returns false to remove from pool
	fun int update(G2D g, float dt) { 
		<<< "Effect.update(G2D g, float dt) unimplemented" >>>;
		return END; 
	}

	// helper fns
    fun static vec2 randomDir() {
		Math.random2f(0, Math.two_pi) => float angle;
        return @(Math.cos(angle), Math.sin(angle));
    }

    fun static vec2 randomDir(float angle, float width) {
		angle + Math.random2f(-width/2, width/2) => angle;
        return @(Math.cos(angle), Math.sin(angle));
	 }

    fun static float expImpulse( float x, float k ) {
        k*x => float h;
        return h*Math.exp(1.0-h);
    }
}

// spawns an explosion of lines going in random directions that gradually shorten
public class ExplodeEffect extends Effect {
	vec2 pos;
	float max_dur; 
	vec3 color;
	int type;

	0 => static int Shape_Lines;
	1 => static int Shape_Squares;

	// internal
	Math.random2(10, 16) => int num; // number of lines
	vec2 dir[num];
	float lengths[num];
	float durations[num];
	vec2 end[num];

	fun @construct(
		vec2 pos, float max_dur, float radius, vec3 color, float angle, float width,
		int type
	) {
		pos => this.pos;
		max_dur => this.max_dur;
		color => this.color;
		type => this.type;

		// init
		for (int i; i < num; i++) {
			Math.random2f(.1, .2) => lengths[i]; // maybe scale with radius
			Math.random2f(.5 * max_dur, max_dur) => durations[i];
			randomDir(angle, width) => dir[i];
			pos + dir[i] * Math.random2f(.5 * radius, radius) => end[i];
		}
	}

	fun int update(G2D g, float dt) {
		if (uptime > max_dur) return END;

		g.pushLayer(layer);
		for (int i; i < num; i++) {
			// update line 
			uptime / durations[i] => float t;
			// if animation still in progress for this line
			if (t < 1) {
				// update position
				pos + t * (end[i] - pos) => vec2 p;
				// shrink lengths linearly down to 0
				lengths[i] * (1 - t) => float len;
				// draw
				if (type == Shape_Lines) g.line(p, p + len * dir[i], color);
				else if (type == Shape_Squares) g.squareFilled(p, 0, .5 * len, color);
			}
		}
		g.popLayer();
		return STILL_GOING;
	}
}

// 
class ScoreEffect extends Effect {
	string s;
	vec2 pos;
	float max_dur;
	float dy;
	float size;

	// internal

	fun @construct(string s, vec2 pos, float max_dur, float dy, float size) {
		s => this.s;
		pos => this.pos;
		max_dur => this.max_dur;
		dy => this.dy;
		size => this.size;
	}

	fun int update(G2D g, float dt) {
		if (uptime > max_dur) return END;
		uptime / max_dur => float t;

		// removing alpha because the smoothness doesn't match the 8bit feel
		// g.pushAlpha(Math.sqrt((1 - t)));

		// quadratic ease
		1 - (1 - t) * (1 - t) => t;
		g.pushLayer(layer);
		g.text(s, pos + @(0, t * dy), size);
		g.popLayer();

		// g.popAlpha();
		return STILL_GOING;
	}
}

class HitFlashEffect extends Effect {
	float max_dur;
	vec2 pos;
	float size;
	vec3 color;
	
	fun @construct(float max_dur, float size, vec2 pos, vec3 color) {
		max_dur => this.max_dur;
		size => this.size;
		pos => this.pos;
		color => this.color;
	}

	fun int update(G2D g, float dt) {
		if (uptime > max_dur) return END;

		g.pushLayer(1);
		g.squareFilled(pos, 0, size, color);
		g.popLayer();

		return STILL_GOING;
	}
}

class ScreenFlashEffect extends Effect {
	float max_dur;
	
	fun @construct(float max_dur) {
		max_dur => this.max_dur;
	}

	fun int update(G2D g, float dt) {
		if (uptime > max_dur) return END;
		uptime / max_dur => float t;

		g.pushBlend(g.BLEND_ADD);
		g.pushLayer(GG.camera().posZ() - 1);
		g.squareFilled(
			GG.camera().pos() $ vec2,
			0, 
			GG.camera().viewSize() * 1,
			Color.WHITE * expImpulse(t, 9)
		);
		g.popBlend();

		return STILL_GOING;
	}
}