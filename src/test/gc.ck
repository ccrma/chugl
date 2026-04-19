Video video(me.dir() + "../../examples/basic/bjork.mpg");

while(1) {
    repeat(1) GG.nextFrame() => now;
    new Video(me.dir() + "../../examples/basic/bjork.mpg") @=> video;
}