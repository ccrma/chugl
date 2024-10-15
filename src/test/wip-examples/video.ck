/*
Problem: seek requires video enabled, and causes pop because takes too long

Ideally we don't do video decoding on the audio thread

Options
- try to modify plm, add a seek_audio method that does not rely on video decoding
- do both audio AND video decoding on the graphics thread, and pass the audio buffer to the audio thread
on a command queue going from graphics --> audio

*/

Video video(me.dir() + "./bjork-all-is-full-of-love.mpg") => dac; 

<<< "VM Samplerate: ", 1::second / 1::samp >>>;
<<< "Framerate: ", video.framerate() >>>;
<<< "Samplerate: ", video.samplerate() >>>;
<<< "Duration: ", video.duration() >>>;

video.texture() @=> Texture video_texture;
(video.width() $ float) / video.height() => float video_aspect;

FlatMaterial plane_mat;
PlaneGeometry plane_geo;

GMesh plane(plane_geo, plane_mat) --> GG.scene();

GOrbitCamera camera --> GG.scene();
camera.clip(.01, 1000);
GG.scene().camera(camera);

// plane.scaX(100 * video_aspect);
// plane.scaY(-100);
// plane.scaZ(100);
plane.scaX(5 * video_aspect);
plane.scaY(-5);

plane_mat.colorMap(video_texture);
// plane_mat.scale(@(100, 100));

fun void seek(time timestamp) {
    video.seek(timestamp);
}

UI_Float rate(1.0);

while (true) {
    GG.nextFrame() => now;

    if (GWindow.keyDown(GWindow.Key_Left)) {
        spork ~ seek(video.timestamp() - 10::second);
    } else if (GWindow.keyDown(GWindow.Key_Right)) {
        spork ~ seek(video.timestamp() + 10::second);
    }

    if (UI.begin("")) {
        if (UI.slider("Rate", rate, -2.0, 2.0)) {
            rate.val() => video.rate;
        }
    }
    UI.end();
}