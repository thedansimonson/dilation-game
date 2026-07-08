#include "raylib.h"
#include "raymath.h"
#include "dilation.h"
#include <stdlib.h>
#include <stdio.h>


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
            grid->cells[i][j] = malloc(sizeof(Tile *));
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
            free(&grid->cells[i][j]);
        }
        free(grid->cells[i]);
    }
    free(grid->cells);

}

void draw_grid(Grid *grid, int width, int height)
{
    
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
}

void advance_tile(Tile* tile)
{
    tile->time_tix += 1;
    
    if (tile->time_tix >= tile->tix_per_hour)
    {
        tile->time_tix %= tile->tix_per_hour;
        tile->time_hours += 1;
        if (tile->time_hours >= MAX_HOURS)
        {
            tile->time_hours %= MAX_HOURS;
        }

    }
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
    // not even sure if these are technically right, but it doesn't matter
    const Vector2 midnight = Vector2Add(((Vector2) {h_half, 0} ), pos);
    const Vector2 two      = Vector2Add(((Vector2) {h_quarter,   w_half}), pos);
    const Vector2 four     = Vector2Add(((Vector2) {-h_quarter,  w_half}), pos);
    const Vector2 six      = Vector2Add(((Vector2) {-h_half, 0})         , pos);
    const Vector2 eight    = Vector2Add(((Vector2) {-h_quarter, -w_half}), pos);
    const Vector2 ten      = Vector2Add(((Vector2) { h_quarter, -w_half}), pos);
    
    Color clock_color = RAYWHITE;
    DrawTriangle(midnight, pos, two, clock_color);
    DrawTriangle(two,   pos, four, clock_color);
    DrawTriangle(four,  pos, six, clock_color);
    DrawTriangle(six,   pos,  eight, clock_color);
    DrawTriangle(eight, pos, ten,   clock_color);
    DrawTriangle(ten,   pos, midnight, clock_color);

}
