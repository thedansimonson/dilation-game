
typedef struct Tile {
    int frequency; 
    int time_hours;
    int time_minutes;
} Tile;


typedef struct Grid {
    Tile **cells;
    int num_cols;
    int num_rows; 

} Grid;

