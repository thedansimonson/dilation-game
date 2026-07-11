#include "raylib.h"
#include "raymath.h"
#include "dilation.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


/************************
 * grid stuff down here *
 ************************/

void init_grid(Grid* grid)
{
    grid->cells = malloc(sizeof(Tile ***) * grid->num_qs);
    for (int i = 0; i < grid->num_qs; i++)
    {
        grid->cells[i] = malloc(sizeof(Tile **) * grid->num_rs);
        for (int j = 0; j < grid->num_rs; j++)
        {
            // This is weird but it's got to do with a bug compiling for WASM
            grid->cells[i][j] = malloc(24); //RL_MALLOC(sizeof(Tile*));
            init_tile(grid->cells[i][j]);
        }
    }
}

void free_grid(Grid *grid)
{
    
    for (int i = 0; i < grid->num_qs; i++)
    {
        for (int j = 0; j < grid->num_rs; j++)
        {
            free(grid->cells[i][j]);
        }
        free(grid->cells[i]);
    }
    free(grid->cells);

}

void update_grid(Grid *grid)
{
    
    for (int i = 0; i < grid->num_qs; i++)
    {
        for (int j = 0; j < grid->num_rs; j++)
        {
            advance_tile(grid->cells[i][j]);
        }
    }
}

void draw_grid(Grid *grid, int pos_x, int pos_y, int width)
{
    // since "height" is indirectly determined by width @ grid size,
    // there's no need for a height param. 
    
    const float cell_width = width / grid->num_qs;
    const float cw2 = cell_width / 2;
    const float cell_height = cell_width * 2 / SQRT_3;
    const float ch34 = cell_height * 3 / 4;
    
    Vector2 tile_pos = { 0 };
    for (int i = 0; i < grid->num_qs; i++)
    {
        for (int j = 0; j < grid->num_rs; j++)
        {
            // TODO: THIS TILE POS IS WRONG
            // LOOK AT THE FUCKING TUTORIAL YOU SCHMUCK
            tile_pos = (Vector2) { pos_x + i*cell_width + j*cw2, 
                                   pos_y + j*ch34 };
            draw_tile(grid->cells[i][j], tile_pos, cell_width);
        }
    }
}

AxCoord pixel_to_pointy(PixelCoord pix, Grid *grid, float width)
{
    AxCoord output = { 0 };

    const float cell_width = width / grid->num_qs;
    const float cw2 = cell_width / 2;
    const float cell_height = cell_width * 2 / SQRT_3;
    const float ch34 = cell_height * 3 / 4;

    float x = ((float) pix.x) + cell_width*SQRT_3;
    float y = ((float) pix.y) + ch34/2; //- ch34;
    
    printf("conversion < %f %f > ", x, y);

    output.q = (int)(x/cell_width - y / 2 / ch34) - 1;
    output.r = (int)(y/ch34);
    return output;
}

CubicCoord axial_to_cube(AxCoord ax)
{
    CubicCoord output = { 0 };
    output.q = ax.q;
    output.r = ax.r;
    output.s = -ax.q-ax.r;
    return output;
}

AxCoord cube_to_axial(CubicCoord cube)
{
    AxCoord output = { 0 };
    output.q = cube.q;
    output.r = cube.r;
    return output;
}

CubicCoord cube_round(CubicCoord ax)
{
    float q = round(ax.q);
    float r = round(ax.r);
    float s = round(ax.s);

    int q_diff = fabsf(q - ax.q);
    int r_diff = fabsf(r - ax.r);
    int s_diff = fabsf(s - ax.s);

    if (q_diff > r_diff && q_diff > s_diff) q = -r-s;
    else if (r_diff > s_diff) r = -q-s;
    else s = -q-r;

    return (CubicCoord) {q, r, s};
}

AxCoord axial_round(AxCoord ax)
{
    return cube_to_axial(cube_round(axial_to_cube(ax)));
}

CubicCoord cube_subtract(CubicCoord a, CubicCoord b)
{
    return (CubicCoord) {a.q - b.q, a.r - b.r, a.s - b.s}; 
}

CubicCoord cube_add(CubicCoord a, CubicCoord b)
{
    return (CubicCoord) {a.q + b.q, a.r + b.r, a.s + b.s}; 
}

float cube_distance(CubicCoord a, CubicCoord b)
{
    CubicCoord raw_delta = cube_subtract(a,b);
    CubicCoord delta = (CubicCoord) { fabsf(raw_delta.q), 
                                      fabsf(raw_delta.r),
                                      fabsf(raw_delta.s) };
    float max_dim = delta.q >= delta.r && delta.q >= delta.s? delta.q :
                    delta.r >= delta.q && delta.r >= delta.s? delta.r :
                                                              delta.s;
    return max_dim;
}

int count_active_tiles(Grid* grid)
{   
    int output = 0;
    for (int i = 0; i < grid->num_qs; i++)
    {
        for (int j = 0; j < grid->num_rs; j++)
        {
            output += grid->cells[i][j]->enabled;
        }
    }
    return output;
}

static const CubicCoord adjacency_deltas[NUM_ADJACENT_DELTAS] = 
                                              {(CubicCoord) { 1,  0, -1},
                                               (CubicCoord) { 1, -1,  0}, 
                                               (CubicCoord) { 0, -1,  1}, 
                                               (CubicCoord) { -1, 0,  1}, 
                                               (CubicCoord) { -1, 1,  0}, 
                                               (CubicCoord) {  0, 1, -1}};

int check_fail_condition(Grid* grid)
{
    // check for unique failure conditions
    
    // first check for isolates
    for (int i = 0; i < grid->num_qs; i++)
    {
        for (int j = 0; j < grid->num_rs; j++)
        {
            bool okay = false;
            if (grid->cells[i][j]->enabled)
            {
                CubicCoord local = axial_to_cube((AxCoord) {i,j});
                for (int d = 0; d < NUM_ADJACENT_DELTAS; d++)
                {
                    AxCoord neighbor = cube_to_axial(cube_add(local, 
                                                              adjacency_deltas[d]));
                    int q = (int) neighbor.q;
                    int r = (int) neighbor.r;
                    if (q >= 0 && q < grid->num_qs && r >= 0 && r < grid->num_rs 
                        && grid->cells[q][r]->enabled)
                    {
                        okay = true;
                        break;
                    }
                }
            }
            else okay = true; // cell not enabled
            if (!okay) 
            {
                printf("< %i %i > was determined to be an isolate.\n", i, j);
                return ISOLATE;
            }
        }
    }
    
    // now check if all the clocks are in sync. 
    bool checked_anything = false;
    int tph_cache = 0;
    bool sync_okay = false;
    for (int i = 0; i < grid->num_qs; i++)
    {
        for (int j = 0; j < grid->num_rs; j++)
        {
            if (grid->cells[i][j]->enabled)
            {
                if (!checked_anything)
                {
                    checked_anything = true;
                    tph_cache = grid->cells[i][j]->tix_per_hour;
                }
                else
                {
                    if(tph_cache != grid->cells[i][j]->tix_per_hour)
                    {
                        sync_okay = true;
                        break;
                    }
                }

            }
        }
        if (sync_okay == true) break;
    }
    if (!sync_okay) return ALL_SYNC;
    return 0;

}

/************************
 * tile stuff down here *
 ************************/

void init_tile(Tile* tile)
{
    tile->tix_per_hour = 1200;
    tile->time_hours = 0;
    tile->time_tix = 0;

    tile->enabled = false;
    tile->operation = OP_ADD;
    tile->selected = false;
}

void advance_tile(Tile* tile)
{
    tile->time_tix += 1;
    
    if (tile->time_tix >= tile->tix_per_hour)
    {
        tile->time_tix %= tile->tix_per_hour;
        tile->time_hours += tile->tix_per_hour > 0? 1 : -1;
        tile->time_hours %= MAX_HOURS;
    }
}

bool tiles_mergeable(Tile* a, Tile* b)
{   
    int a_hours = a->time_hours > 0? a->time_hours : a->time_hours + 12;
    int b_hours = b->time_hours > 0? b->time_hours : b->time_hours + 12;
    return a_hours == b_hours;
}


// Yea, tehy're flipped. It's because if you add, time slows down.
// THis is counter-intuitive. The user doesn't need ot know the details, just
// needs ot see the icon.
static const char *op_add_icon = "+";
static const char *op_sub_icon = "-";
static const char *op_mul_icon = "/";
static const char *op_div_icon = "x";
static const char *op_mod_icon = "%";

void merge_tiles(Tile* a, Tile* b)
{

    b->enabled = false;
    
    // originally, this was with tix per hour, but it proved unintuitive
    float a_period = 1.0 / a->tix_per_hour;
    float b_period = 1.0 / b->tix_per_hour;
    float result_period = 2;

    char *op_icon = NULL;
    switch (b->operation)
    {
        default:
        case OP_ADD:
            result_period = a_period + b_period;
            op_icon = op_add_icon;
            break;
        case OP_SUB:
            result_period = a_period - b_period;
            op_icon = op_sub_icon;
            break;
        case OP_MUL:
            result_period = a_period * b_period;
            op_icon = op_mul_icon;
            break;
        case OP_DIV:
            result_period = a_period / b_period;
            op_icon = op_div_icon;
            break;
        
    }
    printf("obj %i %s selected %i = ",
           a->tix_per_hour,
           op_icon,
           b->tix_per_hour);
    // we convert back to tix per hour
    if (result_period < 0.001) result_period = 0.001;
    a->tix_per_hour = (int) 1/result_period;
    printf("obj %i\n", a->tix_per_hour);
    printf("in actual math: %f OP %f = %f\n", 
           a_period, b_period, result_period); 

    if (a->tix_per_hour <= 2) a->tix_per_hour = 3;
}



void draw_tile(Tile* tile, Vector2 pos, float width)
{
    // TODO: This shit is calculated for every tile. Optimize at grid level!
    // Should only be done once and then just offset by pos. All our hexes
    // will always be the same size. 
    const float height = width * 2 / SQRT_3;
    const float h_half    = height / 2;
    const float h_quarter = height / 4;
    const float w_half    = width  / 2;
    //const float w_quarter = width  / 4;


    //clockwise, starting with 12 to 2
    const Vector2 midnight = Vector2Add(((Vector2) {0, -h_half} ), pos);
    const Vector2 two      = Vector2Add(((Vector2) {w_half, -h_quarter}), pos);
    const Vector2 four     = Vector2Add(((Vector2) {w_half, h_quarter}), pos);
    const Vector2 six      = Vector2Add(((Vector2) {0, h_half})         , pos);
    const Vector2 eight    = Vector2Add(((Vector2) {-w_half, h_quarter}), pos);
    const Vector2 ten      = Vector2Add(((Vector2) {-w_half, -h_quarter}), pos);
    
    Color clock_color = DARKGRAY;
    if (tile->enabled)
    {
        if (tile->selected) clock_color = PALEYELLOW;
        else clock_color = RAYWHITE;
    }
    // background
    DrawTriangle(midnight, pos, two, clock_color);
    DrawTriangle(two,      pos, four, clock_color);
    DrawTriangle(four,     pos, six, clock_color);
    DrawTriangle(six,      pos, eight, clock_color);
    DrawTriangle(eight,    pos, ten,   clock_color);
    DrawTriangle(ten,      pos, midnight, clock_color);

    // border
    Color border_color = BLACK;
    //if (tile->selected) border_color = YELLOW;
    DrawLine(midnight.x, midnight.y, two.x, two.y, border_color);
    DrawLine(two.x, two.y, four.x, four.y, border_color);
    DrawLine(four.x, four.y, six.x, six.y, border_color);
    DrawLine(six.x, six.y, eight.x, eight.y, border_color);
    DrawLine(eight.x, eight.y, ten.x, ten.y, border_color);
    DrawLine(ten.x, ten.y, midnight.x, midnight.y, border_color);
    
    if (!tile->enabled) return;
    
    // TODO: 
    // These should probably be defined at the tile level and offset by pos.
    // They don't change except by offset. Later, I guess
    const Vector2 midnight_outer = Vector2Add(midnight, 
                                              (Vector2) {0, h_half/8});
    const Vector2 midnight_inner = Vector2Add(pos, (Vector2) {0, -h_half/2});
    DrawLine(midnight_outer.x, midnight_outer.y, 
             midnight_inner.x, midnight_inner.y, BLACK);
    
    const Vector2 two_outer = Vector2Add(two, 
                                         (Vector2) {-w_half/8, h_quarter/8});
    const Vector2 two_inner = Vector2Add(pos, 
                                         (Vector2) {w_half/2, -h_quarter/2});
    DrawLine(two_outer.x, two_outer.y, 
             two_inner.x, two_inner.y, BLACK);


    const Vector2 four_outer = Vector2Add(four, 
                                         (Vector2) {-w_half/8, -h_quarter/8});
    const Vector2 four_inner = Vector2Add(pos, 
                                         (Vector2) {w_half/2, h_quarter/2});
    DrawLine(four_outer.x, four_outer.y, 
             four_inner.x, four_inner.y, BLACK);

    const Vector2 six_outer = Vector2Add(six, 
                                         (Vector2) {0, -h_half/8});
    const Vector2 six_inner = Vector2Add(pos, 
                                         (Vector2) {0,  h_half/2});
    DrawLine(six_outer.x, six_outer.y, 
             six_inner.x, six_inner.y, BLACK);


    const Vector2 eight_outer = Vector2Add(eight, 
                                     (Vector2) {w_half/8, -h_quarter/8});
    const Vector2 eight_inner = Vector2Add(pos, 
                                         (Vector2) {-w_half/2, h_quarter/2});
    DrawLine(eight_outer.x, eight_outer.y, 
             eight_inner.x, eight_inner.y, BLACK);


    const Vector2 ten_outer = Vector2Add(ten, 
                                         (Vector2) {w_half/8, h_quarter/8});
    const Vector2 ten_inner = Vector2Add(pos, 
                                         (Vector2) {-w_half/2, -h_quarter/2});
    DrawLine(ten_outer.x, ten_outer.y, 
             ten_inner.x, ten_inner.y, BLACK);

    // Dial Position
    const float dial_length = w_half/2;
    const float dial_theta  = ((((float) tile->time_hours) / MAX_HOURS * 2*PI) 
                                - PI/2);
    Vector2 dial_pos = (Vector2) { dial_length * cos(dial_theta), 
                                   dial_length * sin(dial_theta) };
    dial_pos = Vector2Add(pos, dial_pos);
    
    //DrawLine(pos.x, pos.y, dial_pos.x, dial_pos.y, 3.0, BLACK);
    DrawLineEx(pos, dial_pos, 3.0, BLACK);
    
    char* operator = "?";
    switch (tile->operation)
    {
        case OP_ADD:
            operator = op_add_icon;
            break;
        case OP_SUB:
            operator = op_sub_icon;
            break;
        case OP_MUL:
            operator = op_mul_icon;
            break;
        case OP_DIV:
            operator = op_div_icon;
            break;
        case OP_MOD:
            operator = op_mod_icon;
            break;
    }
    DrawText(operator, pos.x, pos.y, 10, BLACK);

}
