/*
Mr. driller is awesome
So is Motherlode

- press on rythm to get faster resources.
    - I actually hate rhythm stuff because of variable latency with different kboards
    - but maybe we can quantize the mining to music, REZ-style

- ideas:
    - destroying blocks harvests their resource
    - have a "shop" block that very randomly spawns, allow spending resources to buy upgrades
        - high hp, you need to try to path towards it
        - shop items:
            - steel shoes: don't take damage from falling on spikes
            - spring: enables jumping
            - delts: mine faster sideways
            - +long arms: mining hits 2 blocks in same direction
    - level up from mining blocks?
        - upgrades:
            - dmg per hit
            - hit cooldown
    - other block types
        - mine blocks that explode
        - spike traps that kill on contact
            - spawn logic: increase chance if there are more empty blocks above
        - invincible blocks?
        - coin blocks (mine to get coin to spend at shop)
    - goal: survive XX minutes
    - difficulty curve
        - block hp increases
        - camera moves down faster?
        - camera livable "band" narrows and moves, i.e. top and bottom visible edges
    - contraints
        - no enemies
        - no upward movement. this might change, follow gameplay needs
        - no mouse. kb only


collision:
    - only need to check the 9grid squares immediately around player

following risk-reward model, there needs to be things that encourage players to be in the riskiest areas
    - at very top, near off-screen death zone
    - at very bottom, with minimal visibility of what's further down
*/

@import "../lib/g2d/ChuGL.chug"
@import "../lib/g2d/g2d.ck"
@import "../lib/M.ck"
@import "../lib/T.ck"
@import "../bytepath/topograph.ck"

G2D g;
g.antialias(true);

.67 => float aspect;
GWindow.sizeLimits(0, 0, 0, 0, @(aspect, 1));
GWindow.center();

GG.camera().viewSize(20);


0 => int TileType_None; // empty space
1 => int TileType_Dirt; // shovel
2 => int TileType_Wood; // axe
3 => int TileType_Stone; // pickaxe
4 => int TileType_Count; 

[
    Color.BLACK,
    Color.GREEN,
    Color.BROWN,
    Color.GRAY,
] @=> vec3 tile_colors[];


class Tile {
    int type;
    int max_hp;
    int hp;

    fun Tile(int type, int hp)  {
        type => this.type;
        hp => this.max_hp;
        hp => this.hp;
    }

    fun void empty() {
        TileType_None => type;
        0 => max_hp;
        0 => hp;
    }
    
    fun void randomize() {
        Math.random2(0, TileType_Count - 1) => type;
        if (type == TileType_None) 0 => max_hp;
        else                       Math.random2(0, HP_MAX) => max_hp;
        max_hp => hp;
    }
}

fun vec2 tilepos(int r, int c) {
    return @(
        -((MINE_W.val()) * .5) + .5 + c,
        (MINE_H.val() * .5) - .5 - r
    );
}


UI_Int MINE_W(7);
UI_Int MINE_H(20);

Tile tilemap[MINE_H.val()][MINE_W.val()];

5 => int HP_MAX;

// player params
4 => int player_row;
int player_col;
TileType_Dirt => int player_tool;

int score;
int highscore;

UI_Float shift_cd(2.0);
shift_cd.val() => float shift_cd_rem;

0 => int Room_Start;
1 => int Room_Play;
int room;

float gametime;

fun int mine(int r, int c) { // returns true if tile was originally empty
    tilemap[r][c] @=> Tile tile;
    (tile.hp == 0) => int empty;

    // only do dmg if tool matches
    if (player_tool == tile.type) {
        Math.max(0, tile.hp - 1) => tile.hp;

        !empty && (tile.hp == 0) => int destroyed;
        if (destroyed) {
            tile.max_hp +=> score;

            g.score("+" + tile.max_hp, tilepos(r, c), .5::second, .5,  0.6);
        }
    }

    return empty;
}

fun void shift() {
    // shift everything up (can easily optimize)
    tilemap[0] @=> Tile bottom_row[];
    for (int row; row < MINE_H.val() - 1; row++) {
        tilemap[row+1] @=> tilemap[row];
    }
    // shift bottom to top
    bottom_row @=> tilemap[-1];
    // randomize new row
    for (int col; col < MINE_W.val(); col++) {
        bottom_row[col].randomize();
    }
}

fun void die() {
    g.explode(tilepos(player_row+1, player_col), 5, 3::second, Color.WHITE, 0, Math.two_pi, ExplodeEffect.Shape_Squares);
    Room_Start => room;
    Math.max(score, highscore) => highscore;
}

fun void init() {
    0 => gametime;
    0 => score;

    // init tiles
    for (int row; row < MINE_H.val(); row++) {
        for (int col; col < MINE_W.val(); col++) {
            // first 5 rows empty
            if (row < 5) tilemap[row][col].empty();
            else tilemap[row][col].randomize();
        }
    }

    // init player
    0 => player_row;
    0 => player_col;
    1 => player_tool;
}

while (1) {
    GG.nextFrame() => now;
    GG.dt() => float dt;
    if (room == Room_Play) dt +=> gametime;

    // difficulty scaling
    // difficulty incr every 60 seconds
    (1 + (gametime / 6)) $ int => int difficulty; 

    // score
    {
        g.n2w(.9, 1-(.1*aspect)) => vec2 pos;
        g.pushTextControlPoint(@(1, 1));
        g.text("HI " + highscore, pos, .8);
        g.popTextControlPoint();

        g.n2w(-.9, 1-(.1*aspect)) => pos;
        g.pushTextControlPoint(@(0, 1));
        g.text(score + "", pos, .8);
        1 -=> pos.y;
        g.text("L" + difficulty, pos, .8);
        g.popTextControlPoint();
    }

    if (room == Room_Start) {
        g.text("[arrow keys] move/mine", @(0, 1));
        g.text("[up] change tool", @(0, -1));
        if (g.anyInputDown()) {
            init();
            Room_Play => room;
        }
        continue;
    }

    // controls
    if (GWindow.keyDown(GWindow.KEY_UP)) {
        // cycle tools
        if (player_tool == TileType_Stone) TileType_Dirt => player_tool;
        else ++player_tool;
    }

    if (GWindow.keyDown(GWindow.KEY_RIGHT) && player_col < MINE_W.val() - 1) {
        if (mine(player_row, player_col + 1)) player_col++;
    }
    if (GWindow.keyDown(GWindow.KEY_LEFT) && player_col > 0) {
        if (mine(player_row, player_col - 1)) player_col--;
    }
    if (GWindow.keyDown(GWindow.KEY_DOWN) && player_row < MINE_H.val() - 1) {
        if (mine(player_row + 1, player_col)) {
            player_row++;
            // shift();
        }
    }

    // camera shift
    dt -=> shift_cd_rem;
    if (shift_cd_rem < 0) {
        shift_cd.val() +=> shift_cd_rem;
        shift();
        player_row--;
        // die condition
        if (player_row < 0) {
            die();
        }
    }

    // draw player
    g.pushLayer(1);
	g.circleFilled(tilepos(player_row,player_col), .45, tile_colors[player_tool]);
    g.popLayer();

    // draw
    for (int row; row < MINE_H.val(); row++) {
        for (int col; col < MINE_W.val(); col++) {
            tilemap[row][col] @=> Tile tile;
            tilepos(row, col) => vec2 pos;
            g.square(pos, 0, 1.0, Color.WHITE);
            if (tilemap[row][col].hp != 0) {
                g.pushColor(Color.WHITE);
                g.text("" + tilemap[row][col].hp, pos, .5);
                g.popColor();
                g.squareFilled(pos, 0, .9, tile_colors[tilemap[row][col].type]);
            }
        }
    }



	// fun void square(vec2 pos, float rot_radians, float size, vec3 color) {

    

    // ui
}
