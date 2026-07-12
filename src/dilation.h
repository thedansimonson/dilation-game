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
#define GRAYOUT        CLITERAL(Color){ 64, 64, 64, 128 }
#define DANKPURPLE     CLITERAL(Color){ 32, 16, 32, 255 } 

// this should always be out of the grid space
#define NULL_AX (AxCoord) {-2, -2}

#define ISOLATE 1
#define ALL_SYNC 2

#define NUM_ADJACENT_DELTAS 6

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

typedef struct Level {
    Grid level_grid;
    char* hint;
} Level;

/************************
 * Function Definitions *
 ************************/

// dilation.h
void init_grid(Grid *);
void free_grid(Grid *);
void update_grid(Grid *);
Vector2 probe_tile_pos(Grid*, int, int, int);
void draw_grid(Grid *, int, int, int);
AxCoord pixel_to_pointy(PixelCoord, Grid *, float);
CubicCoord axial_to_cube(AxCoord);
AxCoord cube_to_axial(CubicCoord);
CubicCoord cube_round(CubicCoord);
AxCoord axial_round(AxCoord);
float cube_distance(CubicCoord, CubicCoord);
int count_active_tiles(Grid *);
int check_fail_condition(Grid *);

void init_tile(Tile *);
void advance_tile(Tile *);
bool tiles_mergeable(Tile *, Tile *);
void merge_tiles(Tile *, Tile *);
void draw_tile(Tile*, Vector2, float);
