#define MAX_HOURS 12
#define SQRT_3 1.73205080

typedef struct Tile {
    int tix_per_hour; 
    int time_hours;
    int time_tix;

    bool enabled;

} Tile;


typedef struct Grid {
    Tile **cells; //cells[q][r]
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
