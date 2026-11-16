/*
Added: ChuGL 0.3.2
Desc: Tests GLines.positions(vec2[], int) does not segfault. Previously the 
bounds (second param) was not be respected and the impl was writing off the 
end of the internal vertex buffer.

Credits: Ben Hoang for reporting
*/


vec2 pts[1000];
@(-1, -1) => pts[0];
@(1, 0) => pts[1];
@(1, 1) => pts[2];

GLines test --> GG.scene();
test.positions(pts, 3);

GG.nextFrame() => now;