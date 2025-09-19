while (1) {
    GG.nextFrame() => now;

    if (UI.isKeyPressed(UI_Key.GamepadL1)) <<< "L1" >>>;
    if (UI.isKeyPressed(UI_Key.GamepadL2)) <<< "L2" >>>;
}