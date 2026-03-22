/*

Lvls
1: cartoon face
2: 
3: art from other game?
4: ???
5: scary face / crazy features

*/

@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/spring.ck"
@import "../lib/M.ck"

GWindow.sizeLimits(0, 0, 0, 0, @(9, 16));
GWindow.center();

G2D g;
g.backgroundColor(Color.WHITE);

// render with correct aspect ratio
fun void sprite(Texture tex, vec2 pos, float sca) {
    tex.width() $ float / tex.height() => float aspect;
    g.sprite(tex, pos, sca * @(aspect, 1), 0);
}

class Face {
    Texture@ features[0];
    Texture@ face;

    static TextureLoadDesc load_desc;
    true => load_desc.flip_y;

    fun Face(string path) {
        FileIO dir;
        dir.open(path);
        dir.dirList() @=> string images[];

        for (auto s : images) {
            if (s.charAt(0) == '.') continue; // ignore .DS_Store

            Texture.load(path + "/" + s, load_desc) @=> Texture tex;

            if (s.find("face") == 0) tex @=> this.face;
            else features << tex;
        }

        features.shuffle();
    }
}

[
    new Face(me.dir() + "./assets/face1"),
    new Face(me.dir() + "./assets/face2"),
    new Face(me.dir() + "./assets/face3"),
    new Face(me.dir() + "./assets/face4"),
    new Face(me.dir() + "./assets/face5"),
] @=> Face faces[];

vec2 positions[0];
vec2 pos;
-1 => int face_feature_idx;
g.DOWN => vec2 dir;

0 => int face_idx;
faces[0] @=> Face@ face;

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
    g.mousePos() => vec2 mouse_pos;

    if (GWindow.mouseLeftDown()) {
        if (face_feature_idx == face.features.size()) {
            // reset game
            (face_idx + 1) % faces.size() => face_idx;
            faces[face_idx] @=> face;

            -1 => face_feature_idx;
            positions.clear();
        }
        else if (face_feature_idx < 0) {
            0 => face_feature_idx;
        }
        else {
            positions << pos;
            face_feature_idx++;
        }

        // randomize direction
        if (maybe && maybe) {
            g.UP => dir;
            1.5 * @(0, g.screen_min.y) => pos;
        } else {
            g.DOWN => dir;
            1.5 * @(0, g.screen_max.y) => pos;
        }
    }

    if (face_feature_idx >= 0 && face_feature_idx < face.features.size()) {
        5 * dt * dir +=> pos;
        mouse_pos.x => pos.x;

        g.pushLayer(1);
        g.pushColor(2*Color.WHITE);
        sprite(face.features[face_feature_idx], pos, 10);
        g.popColor();
        g.popLayer();
    }

    sprite(face.face, @(0, 0), 10);
    // draw features
    g.pushLayer(1);
    g.pushColor(2*Color.WHITE);
    for (int i; i < positions.size(); ++i) 
        sprite(face.features[i], positions[i], 10);
    g.popColor();
    g.popLayer();
}