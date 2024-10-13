Video video(me.dir() + "./bjork-all-is-full-of-love.mpg");

video.texture() @=> Texture video_texture;
(video.width() $ float) / video.height() => float video_aspect;

FlatMaterial plane_mat;
// PhongMaterial plane_mat;
PlaneGeometry plane_geo;
// SuzanneGeometry plane_geo;

GMesh plane(plane_geo, plane_mat) --> GG.scene();

// TODO debug phong material emissive map
// GPlane plane --> GG.scene();

GOrbitCamera camera --> GG.scene();
camera.clip(.01, 1000);
GG.scene().camera(camera);

plane.scaX(100 * video_aspect);
plane.scaY(-100);
plane.scaZ(100);

plane_mat.colorMap(video_texture);
// plane_mat.emissiveMap(video_texture);

plane_mat.scale(@(100, 100));

while (true) {
    GG.nextFrame() => now;
}