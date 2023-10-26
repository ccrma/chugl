// controls
InputManager IM;
spork ~ IM.start(0);

MouseManager MM;
spork ~ MM.start(0);

FlyCam flycam;
flycam.init(IM, MM);
spork ~ flycam.selfUpdate();

// ====================================

// GPlane a, b, c;
// GSphere a, b, c;
GCube a, b, c;
a --> b --> c --> GGen group --> GG.scene();
-5 => GG.camera().posZ;
GG.camera().lookAt(@(0,0,0));
GG.hideCursor();

b.sca(.5);
a.sca(.5);
-.1 => b.posZ;
-.1 => a.posZ;


Color.RED => a.mat().color;
Color.GREEN => b.mat().color;
Color.BLUE => c.mat().color;

1 => a.mat().transparent;
1 => b.mat().transparent;
1 => c.mat().transparent;

FileTexture tex;
tex.path("./tests/textures/awesomeface.png");
FlatMaterial flatMat;
flatMat.diffuseMap(tex);
true => flatMat.transparent;

GPlane p1, p2, p3;
p1 --> GGen textureGroup --> GG.scene();
p2 --> textureGroup;
p3 --> textureGroup;
p1.mat(flatMat);
p2.mat(flatMat);
p3.mat(flatMat);
p2.posX(.2);
p3.posX(.4);
textureGroup.pos(@(0, 1, 0));

UI_Window window;
// window.text("");

UI_SliderFloat alphaslider_a, alphaslider_b, alphaslider_c;
alphaslider_a.text("sphere a");
alphaslider_a.range(0, 1.0);
alphaslider_b.text("sphere b");
alphaslider_b.range(0, 1.0);
alphaslider_c.text("sphere c");
alphaslider_c.range(0, 1.0);
UI_Checkbox checkbox_a, checkbox_b, checkbox_c;
checkbox_a.text("sphere a transparent");
checkbox_b.text("sphere b transparent");
checkbox_c.text("sphere c transparent");

window.add(alphaslider_a);
window.add(alphaslider_b);
window.add(alphaslider_c);
window.add(checkbox_a);
window.add(checkbox_b);
window.add(checkbox_c);

fun void sliderListener(Material @ mat, UI_SliderFloat @ slider) {
    while (true) {
        slider => now;
        mat.alpha(slider.val());
        <<< "alpha: " + mat.alpha() >>>;
    }
} 
spork ~ sliderListener(a.mat(), alphaslider_a);
spork ~ sliderListener(b.mat(), alphaslider_b);
spork ~ sliderListener(c.mat(), alphaslider_c);

fun void checkboxListener(Material @ mat, UI_Checkbox @ checkbox) {
    while (true) {
        checkbox => now;
        checkbox.val() => mat.transparent;
        <<< "transparent: " + mat.transparent() >>>;
    }
}
spork ~ checkboxListener(a.mat(), checkbox_a);
spork ~ checkboxListener(b.mat(), checkbox_b);
spork ~ checkboxListener(c.mat(), checkbox_c);



while (true) {
    .2 * GG.dt() => textureGroup.rotateX;
    .2 * GG.dt() => group.rotateY;
    GG.nextFrame() => now; 
}