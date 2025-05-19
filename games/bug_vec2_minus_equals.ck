/*
Crashes with:
[1]    segmentation fault  chuck gameloop.ck
[1]    trace trap  chuck bug_vec2_minus_equals.ck
*/

while (1) {
    vec2 foo;
    @(1, 2) -=> foo;
}
