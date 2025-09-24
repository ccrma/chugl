GG.camera().orthographic();
GG.camera().viewSize(10);

GamepadViz gamepads[16];

class GamepadViz extends GGen {
    // background
    GPlane bg --> this;
    bg.sca(@(12, 8, 1));
    bg.color(Color.random());

    // name
    GText gamepad_name --> this;
    gamepad_name.posY(4.5);
    gamepad_name.size(.8);

    // dpad
    GGen dpad --> this;
    GPlane dpad_left --> dpad;
    GPlane dpad_right --> dpad;
    GPlane dpad_up --> dpad;
    GPlane dpad_down --> dpad;
    dpad.posX(-4);
    dpad_left.posX(-1);
    dpad_right.posX(1);
    dpad_up.posY(1);
    dpad_down.posY(-1);

    // abxy
    GGen abxy --> this;
    GPlane button_a --> abxy;
    GPlane button_b --> abxy;
    GPlane button_x --> abxy;
    GPlane button_y --> abxy;
    abxy.posX(4);
    button_a.posY(-1);
    button_b.posX(1);
    button_x.posX(-1);
    button_y.posY(1);

    // bumpers
    GPlane bumper_left --> dpad;
    bumper_left.posY(2.25).sca(@(3, .5, 1));
    GPlane bumper_right --> abxy;
    bumper_right.posY(2.25).sca(@(3, .5, 1));

    // center buttons
    GCircle button_back --> this;
    GCircle button_guide --> this;
    GCircle button_start --> this;
    button_back.sca(.5).posX(-1);
    button_guide.sca(.5);
    button_start.sca(.5).posX(1);

    // joysticks
    GCircle joystick_l --> GCircle joystick_slot_l --> this;
    GCircle joystick_r --> GCircle joystick_slot_r --> this;
    joystick_slot_l.sca(2).pos(-2, -2.5);
    joystick_slot_r.sca(2).pos(2, -2.5);
    joystick_l.color(Color.BLACK);
    joystick_r.color(Color.BLACK);
    joystick_l.sca(.62).posZ(1);
    joystick_r.sca(.62).posZ(1);

    // triggers
    GPlane trigger_left --> GPlane trigger_left_bg --> dpad;
    trigger_left_bg.posY(3.).sca(@(3, .5, 1));
    trigger_left.posZ(1); // position on top
    trigger_left.color(Color.RED);
    GPlane trigger_right --> GPlane trigger_right_bg --> abxy;
    trigger_right_bg.posY(3.).sca(@(3, .5, 1));
    trigger_right.posZ(1); // position on top
    trigger_right.color(Color.RED);

    // update the visuals of this gamepad to match the given id
    fun void update(int id) {
        if (Gamepad.available(id)) {
            id + ": " + Gamepad.name(id) => gamepad_name.text;
        } else {
            id + ": No Controller Detected" => gamepad_name.text;
        }

        dpad_left.color(Gamepad.button(id, Gamepad.BUTTON_DPAD_LEFT) ? Color.RED : Color.WHITE);
        dpad_right.color(Gamepad.button(id, Gamepad.BUTTON_DPAD_RIGHT) ? Color.RED : Color.WHITE);
        dpad_up.color(Gamepad.button(id, Gamepad.BUTTON_DPAD_UP) ? Color.RED : Color.WHITE);
        dpad_down.color(Gamepad.button(id, Gamepad.BUTTON_DPAD_DOWN) ? Color.RED : Color.WHITE);

        button_a.color(Gamepad.button(id, Gamepad.BUTTON_A) ? Color.GREEN : Color.WHITE);
        button_b.color(Gamepad.button(id, Gamepad.BUTTON_B) ? Color.RED : Color.WHITE);
        button_x.color(Gamepad.button(id, Gamepad.BUTTON_X) ? Color.BLUE : Color.WHITE);
        button_y.color(Gamepad.button(id, Gamepad.BUTTON_Y) ? Color.YELLOW : Color.WHITE);

        bumper_left.color(Gamepad.button(id, Gamepad.BUTTON_LEFT_BUMPER) ? Color.RED : Color.WHITE);
        bumper_right.color(Gamepad.button(id, Gamepad.BUTTON_RIGHT_BUMPER) ? Color.RED : Color.WHITE);

        button_back.color(Gamepad.button(id, Gamepad.BUTTON_BACK) ? Color.RED : Color.WHITE);
        button_guide.color(Gamepad.button(id, Gamepad.BUTTON_GUIDE) ? Color.RED : Color.WHITE);
        button_start.color(Gamepad.button(id, Gamepad.BUTTON_START) ? Color.RED : Color.WHITE);

        // thumb button
        joystick_l.color(Gamepad.button(id, Gamepad.BUTTON_LEFT_THUMB) ? Color.RED : Color.BLACK);
        joystick_r.color(Gamepad.button(id, Gamepad.BUTTON_RIGHT_THUMB) ? Color.RED : Color.BLACK);

        // thumb axis
        joystick_l.posX(.2 * Gamepad.axis(id, Gamepad.AXIS_LEFT_X));
        joystick_l.posY(.2 * -Gamepad.axis(id, Gamepad.AXIS_LEFT_Y));
        joystick_r.posX(.2 * Gamepad.axis(id, Gamepad.AXIS_RIGHT_X));
        joystick_r.posY(.2 * -Gamepad.axis(id, Gamepad.AXIS_RIGHT_Y));

        // triggers
        trigger_left.scaX(Gamepad.axis(id, Gamepad.AXIS_LEFT_TRIGGER) * .5 + .5);
        trigger_right.scaX(Gamepad.axis(id, Gamepad.AXIS_RIGHT_TRIGGER) * .5 + .5);
    }
}

while (1) {
    GG.nextFrame() => now;

    Gamepad.available() @=> int gamepad_ids[];

    0 => int max_id;
    // get the max id
    for (auto id : gamepad_ids) {
        Math.max(id, max_id) => max_id;
    }

    (max_id % 2) + max_id => int slots; // round upwards to even number

    // compute scaling to fit in grid
    1 => float slot_y; // slot size as proportion of screen
    if (slots > 0) {
        2.0 / slots => slot_y;
    }

    // draw each connected controller
    for (int gp_id; gp_id <= max_id; gp_id++) {
        gamepads[gp_id].sca(slot_y);
        gamepads[gp_id] --> GG.scene();

        // position in grid
        if (slots == 0) {
            gamepads[gp_id].pos(0, 0);
        } else {
            if (gp_id % 2) {
                gamepads[gp_id].posX(4);
            } else {
                gamepads[gp_id].posX(-4);
            }
            gamepads[gp_id].posY(10 * (slot_y *  (gp_id / 2) + slot_y / 2)  - 5.0);
        }

        // update
        gamepads[gp_id].update(gp_id);
    }

    // disconnect remaining gamepad visualizers
    for (max_id + 1 => int i; i < gamepads.size(); i++) {
        gamepads[i].detachParent();
    }
}