public class CD {
    float cd; // cooldown in secs
    float t;

    fun CD(float _cd) { _cd => cd => t; }

    fun int update(float dt) {
        dt -=> t;
        if (t <= 0) {
            cd +=> t;
            return true;
        }
        return false;
    }
}

// TODO should really add matrix stuff to chuck core (use lengyel's book for reference)
// also other shader vector convenience stuff (minus swizzling)
// e.g. see M.rayAABBIsect
// adapted from: https://www.shadertoy.com/view/wd3XWH
public class BezierColor {

    fun static float fract(float x) {
        return x - (x $ int);
    }
    
    fun static vec3 mix(vec3 a, vec3 b, float t) { return (1-t)*a  + t*b; }

    fun static vec3 clamp(vec3 a, float min, float max) {
        return @(
            Math.clampf(a.x, min, max),
            Math.clampf(a.y, min, max),
            Math.clampf(a.z, min, max)
        );
    }

    //shader incoming relating to this palette
    // x is time
    fun static vec3 getPalette(float x, vec3 c1, vec3 c2, vec3 p1, vec3 p2)
    {
        fract(x/2.0) => float x2;
        fract(x) => x;
        // mat3 m = mat3(c1, p1, c2);
        // mat3 m2 = mat3(c2, p2, c1);
        1.0-x => float omx;
        @(omx*omx, 2.0*omx*x, x*x) => vec3 pws;

        // builtin matrix mult would be nice...
        return clamp(mix(
            c1 * pws.x + p1 * pws.y + c2 * pws.z,
            c2 * pws.x + p2 * pws.y + c1 * pws.z,
            ( x2 > .5 ? 0 : 1  ) 
        ), 0.,1.);
    }


    // used in shader:
    // vec3 pal = getPalette(-x, vec3(0.2, 0.5, .7), vec3(.9, 0.4, 0.1), vec3(1., 1.2, .5), vec3(1., -0.4, -.0));
    // vec3 pal = getPalette(-x, vec3(0.4, 0.3, .5), vec3(.9, 0.75, 0.4), vec3(.1, .8, 1.3), vec3(1.25, -0.1, .1));

    // #ifndef MOUSE_ONLY
    // col += .0025/(0.0005+pow(length(uv - point1(iTime)),1.75))*dt*0.12*pal(iTime*0.05 - .0);
    // col += .0025/(0.0005+pow(length(uv - point2(iTime)),1.75))*dt*0.12*pal2(iTime*0.05 + 0.675);
    // #endif
}