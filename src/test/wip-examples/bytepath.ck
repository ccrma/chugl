// import materials for 2D drawing
@import "b2.ck"

// most steam games (over half) default to 1920x1080
// 1920x1080 / 4 = 480x270

// set target resolution
GG.renderPass().resolution(480, 270);

// set MSAA
GG.renderPass().samples(1);

// pixelate the output
TextureSampler output_sampler;
TextureSampler.Filter_Nearest => output_sampler.filterMin;  
TextureSampler.Filter_Nearest => output_sampler.filterMag;  
TextureSampler.Filter_Nearest => output_sampler.filterMip;  
GG.outputPass().sampler(output_sampler);


b2DebugDraw_Circles circles --> GG.scene();

while (true) {
    GG.nextFrame() => now;
    circles.drawCircle(@(0,0), 1.0, .01, Color.WHITE);
    circles.update();
}
