#include "raylib.h"
#include "raymath.h"

#define MAX_HOURS 12
#define SQRT_3 1.73205080

#define OP_ADD 0
#define OP_SUB 1
#define OP_MUL 2
#define OP_DIV 3
#define OP_MOD 4

typedef struct Tile {
    int tix_per_hour; 
    int time_hours;
    int time_tix;


    bool enabled;

    int operation;

    bool selected; 

} Tile;


typedef struct Grid {
    Tile ***cells; //cells[q][r]
    int num_qs;
    int num_rs; 

} Grid;

typedef struct AxCoord {
    int q;
    int r;
} AxCoord;

typedef struct CubicCoord {
    int q;
    int r;
    int s;
} CubicCoord;


/************************
 * Function Definitions *
 ************************/

// dilation.h
void init_grid(Grid *);
void free_grid(Grid *);

void init_tile(Tile *);
void advance_tile(Tile *);
void draw_tile(Tile*, Vector2, float);
