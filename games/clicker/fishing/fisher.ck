/*

Core fishing mechanic ideas
- wait for a bite
- click to collect after bite
- click to cast

*/

@import "../../lib/g2d/ChuGL.chug"
@import "../../lib/g2d/g2d.ck"
@import "../../lib/spring.ck"
@import "../../lib/M.ck"

G2D g;

280 => int res;
g.resolution(res, res);
g.antialias(false);
GWindow.sizeLimits(0, 0, 0, 0, @(1, 1));

while (1) {
    GG.nextFrame() => now;
    now/second => float t;

    Math.sin(t) * .1 => float water_level;
    g.line(@(g.screen_min.x, water_level), @(g.screen_max.x, water_level));

    g.squareFilled(@(g.screen_max.x - .5, .5 + water_level), 1);

}