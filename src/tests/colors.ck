Color.RED => vec3 red_rgb;
Color.rgb2hsv(red_rgb) => vec3 red_hsv;
Color.hsv2rgb(red_hsv) => vec3 red_hsv2rgb;

<<< "red rgb", red_rgb >>>;
<<< "red hsv", red_hsv >>>;
<<< "red hsv2rgb", red_hsv2rgb >>>;

Color.BEIGE => vec3 beige_rgb;
Color.rgb2hsv(beige_rgb) => vec3 beige_hsv;
Color.hsv2rgb(beige_hsv) => vec3 beige_hsv2rgb;

<<< "beige rgb", beige_rgb >>>;
<<< "beige hsv", beige_hsv >>>;
<<< "beige hsv2rgb", beige_hsv2rgb >>>;

@(
    Math.random2f(0, 1),
    Math.random2f(0, 1),
    Math.random2f(0, 1)
 ) => vec3 rand_rgb;
Color.rgb2hsv(rand_rgb) => vec3 rand_hsv;
Color.hsv2rgb(rand_hsv) => vec3 rand_hsv2rgb;

<<< "rand rgb", rand_rgb >>>;
<<< "rand hsv", rand_hsv >>>;
<<< "rand hsv2rgb", rand_hsv2rgb >>>;

// FlatMaterial mat;
// GPlane plane --> GG.scene();
// plane.mat(mat);
// plane.posZ(-5);

// while (true) { 
//     GG.nextFrame() => now; 
// }

