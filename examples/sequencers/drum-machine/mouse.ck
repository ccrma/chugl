// simplified Mouse class from examples/input/Mouse.ck  =======================
public class Mouse
{
    vec3 worldPos;

    // update mouse world position
    fun void selfUpdate() {
        while (true) {
            GG.nextFrame() => now;
            // calculate mouse world X and Y coords
            GG.camera().screenCoordToWorldPos(GWindow.mousePos(), 1.0) => worldPos;
        }
    }
}
