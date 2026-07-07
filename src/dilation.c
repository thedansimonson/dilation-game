#include "dilation.h"


/************************
 * grid stuff down here *
 ************************/

void init_grid(Grid* grid)
{
    grid->cells = malloc(sizeof(Tile **) * grid->num_qs);
    for (int i = 0; i < grid->num_qs; i++)
    {
        grid->cells[i] = malloc(sizeof(Tile *) * grid->num_rs);
        for (int j = 0; j < grid->num_rs; j++)
        {
            grid->cells[i][j] = malloc(sizeof(Tile));
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

