vec2 pts[1000];
@(-1, -1) => pts[0];
@(1, 0) => pts[1];
@(1, 1) => pts[2];

GLines test --> GG.scene();
test.positions(pts, 3);

GG.nextFrame() => now;