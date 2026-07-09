#include "raylib.h"
#include "raymath.h"

#define MAX_HOURS 12
#define SQRT_3 1.73205080

#define OP_ADD 0
#define OP_SUB 1
#define OP_MUL 2
#define OP_DIV 3
#define OP_MOD 4

#define PALEYELLOW     CLITERAL(Color){ 253, 249, 128, 255 }

// this should always be out of the grid space
#define NULL_AX (AxCoord) {-2, -2}

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
    float q;
    float r;
} AxCoord;

typedef struct CubicCoord {
    float q;
    float r;
    float s;
} CubicCoord;

typedef struct PixelCoord {
    int x;
    int y;
} PixelCoord;

/************************
 * Function Definitions *
 ************************/

// dilation.h
void init_grid(Grid *);
void free_grid(Grid *);
void update_grid(Grid *);
void draw_grid(Grid *, int, int, int);
AxCoord pixel_to_pointy(PixelCoord, Grid *, float);
CubicCoord axial_to_cube(AxCoord);
AxCoord cube_to_axial(CubicCoord);
CubicCoord cube_round(CubicCoord);
AxCoord axial_round(AxCoord);
float cube_distance(CubicCoord, CubicCoord);

void init_tile(Tile *);
void advance_tile(Tile *);
void draw_tile(Tile*, Vector2, float);
