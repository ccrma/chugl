public class Spring {
    float x, k, d;
    float target_x; // rest position
    float v;

    fun Spring(float x, float k, float d) {
        x => this.x;
        k => this.k;
        d => this.d;
        x => this.target_x;
    }

    fun void update(float dt) {
        // hack of clamping to 59fps to prevent numerical instability.
        // a more robust solution would be stepping with a fixed timestep (e.g. in a while loop)
        // or using backwards / symplectic euler integration
        Math.min(.017, dt) => dt; 
        -k*(x - target_x) - d*v => float a;
        a*dt +=> v;
        v*dt +=> x;
    }
    
    fun void pull(float f) { f +=> x; }
}

/*

translation:
k = 4200
d = 20
f = .1

Rotation
k = 1000
d = 10
f = .1

weapon switch
k = 500
d = 20
f = .3

*/


if (1) {

Spring y_spring(0, 4200, 20);
Spring s(0, 4 * Math.pi, Math.pi/128);

GPlane g --> GG.scene();

UI_Float x(s.x);
UI_Float k(s.k);
UI_Float d(s.d);
UI_Float f;

    while (1) {
        GG.nextFrame() => now;

        if (UI.slider("x", x, 0, 10)) x.val() => s.x;
        if (UI.slider("k", k, 0, 10000)) k.val() => s.k;
        if (UI.slider("d", d, 0, 10)) d.val() => s.d;
        UI.slider("f", f, 0, 10);

        if (GWindow.keyDown(GWindow.Key_Space)) {
            s.pull(f.val());
            y_spring.pull(.1);
        }

        s.update(GG.dt());
        y_spring.update(GG.dt());
        // y_spring.x => g.posY;

        s.x => g.rotZ;
        // 1.0 + s.x => g.sca;

    }
}