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
    tile->operation = OP_ADD;
    tile->selected = false;
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
    const Vector2 midnight = Vector2Add(((Vector2) {0, -h_half} ), pos);
    const Vector2 two      = Vector2Add(((Vector2) {w_half, -h_quarter}), pos);
    const Vector2 four     = Vector2Add(((Vector2) {w_half, h_quarter}), pos);
    const Vector2 six      = Vector2Add(((Vector2) {0, h_half})         , pos);
    const Vector2 eight    = Vector2Add(((Vector2) {-w_half, h_quarter}), pos);
    const Vector2 ten      = Vector2Add(((Vector2) {-w_half, -h_quarter}), pos);
    
    Color clock_color = DARKGRAY;
    if (tile->enabled)
        clock_color = RAYWHITE;
    
    // background
    DrawTriangle(midnight, pos, two, clock_color);
    DrawTriangle(two,      pos, four, clock_color);
    DrawTriangle(four,     pos, six, clock_color);
    DrawTriangle(six,      pos, eight, clock_color);
    DrawTriangle(eight,    pos, ten,   clock_color);
    DrawTriangle(ten,      pos, midnight, clock_color);

    // border
    Color border_color = BLACK;
    if (tile->selected) border_color = YELLOW;
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

    DrawLine(pos.x, pos.y, dial_pos.x, dial_pos.y, BLACK);

}
