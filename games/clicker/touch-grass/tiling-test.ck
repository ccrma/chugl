
@import "../../lib/g2d/ChuGL.chug"
@import "../../lib/g2d/g2d.ck"
@import "../../lib/spring.ck"
@import "../../lib/M.ck"

G2D g;

GG.outputPass().gamma(true);
Texture.load(me.dir() + "./assets/block.png" ) @=> Texture block_tex;

/*
Helper class for working with an angled, isometric grid. 
- worldspace: normal xy world coordinates, std basis vectors
- gridspace: i_hat and j_hat vectors based on the isometric grid's angle and tile_hw. 
    NOT necessarily orthonogonal, NOT necessarily normalized
- cellspace: discrete. the integer coordinates of the tile. e.g. (0,0), (1,0)
*/
class IsoGrid {
    26.5 * M.DEG2RAD => float angle_rad;
    1 => float tile_hw;

    // normalized gridspace basis vectors
    vec2 i_norm;
    vec2 j_norm;

    // gridspace basis vectors (not normalized)
    vec2 i_hat; 
    vec2 j_hat;

    // conversion matrices
    vec4 grid_to_world_mat;
    vec4 world_to_grid_mat;

    init(angle_rad, tile_hw);

    fun void init(float angle_rad, float tile_hw) {
        angle_rad => this.angle_rad;
        tile_hw => this.tile_hw;

        // tile params
        @(Math.cos(angle_rad), Math.sin(angle_rad)) => i_norm;
        @(-i_norm.x, i_norm.y) => j_norm; // reflect over y axis
        tile_hw / i_norm.x => float mag;
        // grid basis vectors
        i_norm * mag => i_hat;
        j_norm * mag => j_hat;

        M.mat(i_hat, j_hat) => grid_to_world_mat;
        M.inv(grid_to_world_mat) => world_to_grid_mat;
    }

    fun vec2 world2cell(vec2 world) {
        return grid2cell( M.mult(world_to_grid_mat, world) );
    }

    fun vec2 grid2cell(vec2 grid) {
        return @(
            (grid.x + Math.sgn(grid.x) * .5) $ int,
            (grid.y + Math.sgn(grid.y) * .5) $ int
        ); 
    }
}

UI_Float offset_y(-0.16);
IsoGrid grid;

// tile params
fun void drawTile(vec2 ij) {
    (ij.x + Math.sgn(ij.x) * .5) $ int => int grid_x;
    (ij.y + Math.sgn(ij.y) * .5) $ int => int grid_y;
    M.mult(grid.grid_to_world_mat, @(grid_x, grid_y)) => vec2 grid_pos_world;

    // normalized distance from top of screen [0, 1]
    (1.0 / GG.camera().viewSize()) * (grid_pos_world.y - g.screen_max.y) => float y_sort;

    g.pushLayer(g.layer() - y_sort);
    g.sprite(block_tex, grid_pos_world + @(0, offset_y.val()), @(2 * grid.tile_hw, -2 * grid.tile_hw), 0);
    g.popLayer();

    g.pushLayer(1);
    g.pushColor(Color.GRAY);
    g.text(grid_x + "," + grid_y, grid_pos_world, .2);
    g.popColor();
    g.popLayer();
}

while (1) {
    GG.nextFrame() => now;

    g.pushLayer(1);
    g.line(@(0, 0), grid.i_hat, Color.RED);
    g.line(@(0, 0), grid.j_hat, Color.GREEN);
    g.popLayer();

    drawTile(@(0, 0));

    UI.slider("sprite offset y", offset_y, -1, 1);

    // draw grid
    g.pushLayer(1);
    for (-12 => int i; i < 12; ++i) {
        i + .5 => float t;
        // "x" axis
        g.line(-100 * grid.i_hat + t * grid.j_hat, 100 * grid.i_hat + t * grid.j_hat);
        // "y" axis
        g.line(-100 * grid.j_hat + t * grid.i_hat, 100 * grid.j_hat + t * grid.i_hat);
    }
    g.popLayer();

    // conversion matrices

    { // project mouse cursor onto grid
        g.mousePos() => vec2 mouse;
        M.mult(grid.world_to_grid_mat, mouse) => vec2 ij;
        drawTile(ij);
    }
}
