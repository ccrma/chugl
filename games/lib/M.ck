public class M {
    0.017453292519943 => static float DEG2RAD;
    57.295779513082320 => static float RAD2DEG;
    1.4142135624 => static float ROOT2;
    .7071067812 => static float INV_ROOT2;

    // TODO: add to ulib_color
    fun static vec3 srgbToLinear(vec3 c) {
        2.2 => float g;
        return @(
            Math.pow(c.r, g),
            Math.pow(c.g, g),
            Math.pow(c.b, g)
        );
    }

    fun static vec2 rot2vec(float radians) {
        return @( Math.cos(radians), Math.sin(radians) );
    }

    fun static float angle(vec2 a, vec2 b) {
        b - a => vec2 n;
        return Math.atan2(n.y, n.x);
    }

    fun static float angle(vec2 a) {
        return Math.atan2(a.y, a.x);
    }

    fun static vec2 dir(vec2 a, vec2 b) {
        b - a => vec2 n;
        n.normalize();
        return n;
    }

    fun static vec2 normalize(vec2 n) {
        // note: can also call n.normalize() to normalize in-place
        return n / Math.hypot(n.x, n.y); // hypot is the magnitude
    }

    fun static float mag(vec2 n) {
        return  Math.hypot(n.x, n.y); 
    }

    fun static float dist(vec2 a, vec2 b) {
        return Math.euclidean(a, b);
    }

    fun static float dist2(vec2 a, vec2 b) {
        return (b - a).dot(b - a);
    }

    fun static float cross(vec2 a, vec2 b) {
        return a.x * b.y - a.y * b.x;
    }

    fun static vec2 perp(vec2 a) {
        return @(-a.y, a.x);
    }

    // unwinds rad to be in range [0, 2pi]
    fun static float unwind(float rad) {
        while (rad > Math.two_pi) Math.two_pi -=> rad;
        while (rad < 0)           Math.two_pi +=> rad;
        return rad;
    }

    // compute the angle between two angles in radians
    fun static float deltaAngle(float rad1, float rad2) {
        unwind(rad1) => rad1;
        unwind(rad2) => rad2;
        Math.fabs(rad1 - rad2) => float delta;
        Math.min(delta, Math.fabs(Math.two_pi - delta)) => delta;
        return delta;
    }

    // returns a clamped between [b, c] inclusive
    fun static int clamp(int a, int b, int c) {
        return Math.min(Math.max(a, b), c);
    }

    fun static vec2 clamp(vec2 a, vec2 b, vec2 c) {
        return @(
            Math.clampf(a.x, b.x, c.x),
            Math.clampf(a.y, b.y, c.y)
        );
    }

    fun static float rnd(float a, float b) { return Math.random2f(a, b); }

    // returns -1 if negative, +1 if pos
    fun static float sign(float x) {
        return 2*(x >= 0) - 1;
    }

    fun static float fract(float x) {
        return x - (x $ int);
    }

    // =====================================================================
    // Tweens (most take a function that outputs from 0-1 over an input 0-1
    // =====================================================================

    // remaps [0,1] -> [0,1], such that the corners are mapped to 0 and the center to 1.
    // raised to power k
    // https://graphtoy.com/?f1(x,t)=cond(abs(x-.5)%3E.5,0,pow(4*x*(1-x),1/4))&v1=true&f2(x,t)=cond(abs(x-.5)%3E.5,0,pow(4*x*(1-x),1/2))&v2=true&f3(x,t)=cond(abs(x-.5)%3E.5,0,pow(4*x*(1-x),1))&v3=true&f4(x,t)=cond(abs(x-.5)%3E.5,0,pow(4*x*(1-x),2))&v4=true&f5(x,t)=cond(abs(x-.5)%3E.5,0,pow(4*x*(1-x),4))&v5=true&f6(x,t)=&v6=false&grid=1&coords=0.5,0.5,0.75
    fun static float parabola( float x, float k )
    {
        return Math.pow( 4.0*x*(1.0-x), k );
    }

    fun static float easeOutSine(float x) {
        return Math.sin((x * Math.PI) / 2);
    }
    
    fun static float easeOutQuad(float x) {
        return 1 - (1 - x) * (1 - x);
    }

    fun static float easeInOutCubic(float x) {
        if (x < 0.5) 
            return 4 * x * x * x;
        else 
            return 1 - Math.pow(-2 * x + 2, 3) / 2;
    }

    fun static float easeOutBack(float x) {
        1.70158 => float c1;
        c1 + 1 => float c3;
        
        return 1 + c3 * Math.pow(x - 1, 3) + c1 * Math.pow(x - 1, 2);
    }

    fun static vec3 lerp(float x, vec3 a, vec3 b) {
        return (1.0 - x) * a + x * b;
    }

    fun static vec2 lerp(float x, vec2 a, vec2 b) {
        return (1.0 - x) * a + x * b;
    }

    fun static float lerp(float t, float a, float b) {
        return a + t * (b - a);
    }

    // grows fast, decays slow
    // Use k to control the stretching of the function. Its maximum, which is 1, happens at exactly x = 1/k.
    fun static float expImpulse( float x, float k ) {
        k*x => float h;
        return h*Math.exp(1.0-h);
    }

    // ===================================================
    // Random
    // ===================================================

    fun static vec2 randomDir() {
        return rot2vec(Math.random2f(0, Math.two_pi));
    }

    fun static vec2 randomPointInCircle(vec2 center, float min_r, float max_r) {
        Math.random2f(0, Math.two_pi) => float theta;
        Math.random2f(min_r, max_r) => float radius;
        return center + radius * @(Math.cos(theta), Math.sin(theta));
    }


    fun static vec2 randomPointInArea(vec2 center, float hw, float hh) {
        return center + @(
            Math.random2f(-hw, hw),
            Math.random2f(-hh, hh)
        );
    }

    // returns a weighted-choice from a PDF
    // e.g. [5, 3, 2]
    // return 0: 50% chance
    // return 1: 30% chance
    // return 2: 20% chance
    fun static int choose(float pdf[]) {
        0.0 => float total;
        0.0 => float accum;

        for (auto p : pdf) p +=> total;
        Math.random2f(0.0, total) => float pick;

        for (int i; i < pdf.size(); i++) {
            pdf[i] +=> accum;
            if (accum >= pick) return i;
        }
        return pdf.size() - 1;
    }

    // choose n random numbers (no repeats) from [0, m-1]
    fun static int[] choose(int m, int n) {
        if (n > m) {
            <<< "error, choosing ", n, "from only", m >>>;
            return null;
        }
        int pool[m];
        for (int i; i < m; i++) i => pool[i];
        pool.shuffle();
        pool.erase(n, pool.size());
        return pool;
    }

    // return a random element from the array with uniform distribution
    fun static int randomElement(int arr[]) {
        return arr[Math.random2(0, arr.size() - 1)];
    }

    // given an event happens once every `period_secs`, returns
    // the amount of time you need to wait until the next occurance.
    // returns the amount of time in seconds until the next event occurs
    fun static float poisson(float period_sec) {
        (1 / 1000.0) => float dt_sec;
        dt_sec / period_sec => float success_rate;

        0 => int count;
        while (Math.randomf() > success_rate) count++;

        return count * dt_sec;
    }

    // ===================================================
    // Intersection Collision Testing
    // ===================================================

    // returns true if overlap between 2 circles
    // with position p1, p2 and radii r1, r2
    fun static int circlesOverlap(vec2 p1, float r1, vec2 p2, float r2) {
        return Math.euclidean(p1, p2) < (r1 + r2);
    }

    // Returns 1 if the lines intersect, otherwise 0. In addition, if the lines 
    // intersect the intersection point may be stored in the floats i_x and i_y.
    // modified and improved from: https://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
    fun static int intersect(
        vec2 p0, vec2 p1,
        vec2 p2, vec2 p3
    ) {
        p1 - p0 => vec2 s1;
        p3 - p2 => vec2 s2;

        cross(s1, s2) => float det;
        if (Math.fabs(det) < .000001) return false; // lines are colinear

        p0 - p2 => vec2 p0p2;
        perp(s1).dot(p0p2) / det => float s;
        if (s < 0 || s > 1) return false;
        perp(s2).dot(p0p2) / det => float t;
        if (t < 0 || t > 1) return false;
        return true;
    }

    // isection between line segment ab and circle c,r
    // returns the t that isects the circle s.t. lerp(t, p1, p2) is the isection point
    // returns @(isect?, t1, t2)
    // if no isections, isect = 0
    // https://www.mathworks.com/matlabcentral/answers/401724-how-to-check-if-a-line-segment-intersects-a-circle
    fun static vec2 isect(vec2 P1, vec2 P2, vec2 C, float r) {
        P2 - P1 => vec2 d;
        P1 - C => vec2 f;
        d.dot(d) => float a;
        2*f.dot(d) => float b;
        f.dot(f) - r*r => float c;
        b*b-4*a*c => float discriminant;

        // no intersection
        if( discriminant < 0 )
            return @(0, -Math.FLOAT_MAX);

        Math.sqrt(discriminant) => discriminant;
  
        // either solution may be on or off the ray so need to test both
        // t1 is always the smaller value, because BOTH discriminant and
        // a are nonnegative.
        (-b - discriminant)/(2*a) => float t1;
        (-b + discriminant)/(2*a) => float t2;

        // 3x HIT cases:
        //          -o->             --|-->  |            |  --|->
        // Impale(t1 hit,t2 hit), Poke(t1 hit,t2>1), ExitWound(t1<0, t2 hit), 
        // 3x MISS cases:
        //       ->  o                     o ->              | -> |
        // FallShort (t1>1,t2>1), Past (t1<0,t2<0), CompletelyInside(t1<0, t2>1)
    
        // t1 is the intersection, and it's closer than t2
        // (since t1 uses -b - discriminant)
        if( t1 >= 0 && t1 <= 1 )
            // Impale, Poke
            return @(1, t1);

        // here t1 didn't intersect so we are either started
        // inside the sphere or completely past it
        if( t2 >= 0 && t2 <= 1 )
            // ExitWound
            return @(1, t2);

        // no intn: FallShort, Past, CompletelyInside
        return @(0, -Math.FLOAT_MAX);
    }

    // @(lowerBound.x, lowerBound.y, upperBound.x, upperBound.y)
    fun static vec4 aabb(vec2 c, float hw, float hh) {
        return @(
            c.x-hw, c.y-hh,
            c.x+hw, c.y+hh
        );
    }

    fun static int inside( vec2 p, vec2 bottom_left, vec2 top_right ) {
        return (
            p.x >= bottom_left.x && p.x <= top_right.x
            &&
            p.y >= bottom_left.y && p.y <= top_right.y
        );
    }


    fun static int inside( vec2 p, vec4 aabb ) {
        return (
            p.x >= aabb.x && p.x <= aabb.z
            &&
            p.y >= aabb.y && p.y <= aabb.w
        );
    }

    // returns true if 2 aabbs isect
    // params are in form @(minx, miny, maxx, maxy)
    fun static int aabbIsect(vec4 a, vec4 b) {
        return overlap(a.x, a.z, b.x, b.z) && overlap(a.y, a.w, b.y, b.w);
    }

    // a_c is center
    fun static int aabbIsect(vec2 a_c, vec2 a_hw_hh, vec2 b_c, vec2 b_hw_hh) {
        return 
            overlap(a_c.x - a_hw_hh.x, a_c.x + a_hw_hh.x, b_c.x - b_hw_hh.x, b_c.x + b_hw_hh.x) 
            && 
            overlap(a_c.y - a_hw_hh.y, a_c.y + a_hw_hh.y, b_c.y - b_hw_hh.y, b_c.y + b_hw_hh.y);
    }

    // returns @(minx, miny, maxx, maxy)
    fun static vec4 bbox(vec2 vertices[]) {
        @(Math.FLOAT_MAX, Math.FLOAT_MAX) => vec2 bbox_min;
        @(-Math.FLOAT_MAX, -Math.FLOAT_MAX) => vec2 bbox_max;
        for (auto v : vertices) {
            Math.min(v.x, bbox_min.x) => bbox_min.x;
            Math.min(v.y, bbox_min.y) => bbox_min.y;
            Math.max(v.x, bbox_max.x) => bbox_max.x;
            Math.max(v.y, bbox_max.y) => bbox_max.y;
        }
        return @(bbox_min.x, bbox_min.y, bbox_max.x, bbox_max.y);
    }

    // returns true if there is an overlap between two 1D line segments
    // a0---a1 and b0---b1
    fun static int overlap(float b0, float b1, float a0, float a1) {
        return (a0 <= b0 && b0 <= a1) || (a0 <= b1 && b1 <= a1)
               ||
               (b0 <= a0 && a0 <= b1) || (b0 <= a1 && a1 <= b1);
    }
}