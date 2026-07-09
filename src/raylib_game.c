/*******************************************************************************************
*
*   raylib gamejam template
*
*   Code licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2022-2026 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include "dilation.h"

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>      // Emscripten library
#endif

#include <stdio.h>                          // Required for: printf()
#include <stdlib.h>                         // Required for: 
#include <string.h>                         // Required for:

//----------------------------------------------------------------------------------
// Defines and Macros
//----------------------------------------------------------------------------------
// Simple log system to avoid printf() calls if required
// NOTE: Avoiding those calls, also avoids const strings memory usage
#define SUPPORT_LOG_INFO
#if defined(SUPPORT_LOG_INFO)
    #define LOG(...) printf(__VA_ARGS__)
#else
    #define LOG(...)
#endif

//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
typedef enum { 
    SCREEN_LOGO = 0, 
    SCREEN_TITLE, 
    SCREEN_GAMEPLAY, 
    SCREEN_ENDING
} GameScreen;

// TODO: Define your custom data types here

//----------------------------------------------------------------------------------
// Global Variables Definition (local to this module)
//----------------------------------------------------------------------------------
static const int screenWidth = 720;
static const int screenHeight = 720;

const int GRIDSIZE = 400;

static RenderTexture2D target = { 0 };  // Render texture to render our game

static Grid active_grid = { 0 };
static Tile test_tile = (Tile) {60, 2, 10, true, OP_ADD, false};

// UI-related funcs
static Tile* selected_tile = NULL;
static CubicCoord selected_tile_pos = { 0 };

static bool start_score = false;
static int round_score = 0;

// TODO: Define global variables here, recommended to make them static

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);      // Update and Draw one frame

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
#if !defined(_DEBUG)
    SetTraceLogLevel(LOG_NONE);         // Disable raylib trace log messages
#endif

    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "Dilation");
    
    // TODO: Load resources / Initialize variables at this point
    
    // Render texture to draw, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);     // Set our game frames-per-second
    //--------------------------------------------------------------------------------------
    
    active_grid.num_qs = 8;
    active_grid.num_rs = 8;
    init_grid(&active_grid);
    memcpy(active_grid.cells[0][0], &test_tile, sizeof(Tile)); 

    test_tile = (Tile) {120, 4, 10, true, OP_ADD, false};
    memcpy(active_grid.cells[1][0], &test_tile, sizeof(Tile));

    test_tile = (Tile) {1, 6, 10, true, OP_ADD, false};
    memcpy(active_grid.cells[3][3], &test_tile, sizeof(Tile));
    
    // random init
    for (int i = 0; i < active_grid.num_qs; i++)
    {
        for (int j = 0; j < active_grid.num_rs; j++)
        {
            int tph = GetRandomValue(10, 60); // tix_per_hour
            test_tile = (Tile) {tph,
                                GetRandomValue(0,11),    // time_hours
                                GetRandomValue(0,tph-1), // time_tix
                                true,                    //enabled
                                OP_ADD,                       // op (starting with add)
                                false
                               };
            memcpy(active_grid.cells[i][j], &test_tile, sizeof(Tile));

        }
    }

    round_score = 400000;

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button
    {
        UpdateDrawFrame();
    }
#endif

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadRenderTexture(target);
    
    // TODO: Unload all loaded resources at this point

    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}


//--------------------------------------------------------------------------------------------
// Module Functions Definition
//--------------------------------------------------------------------------------------------
// Update and draw frame
void UpdateDrawFrame(void)
{
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update variables / Implement example logic at this point
    
    const int GRIDPOS_X = 200;
    const int GRIDPOS_Y = 200;

    if (start_score) round_score--;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        // on first click, start scoring.
        start_score = true; 

        Vector2 mouse_pos = GetMousePosition();
        printf("mouse_pos: < %f %f > ", mouse_pos.x, mouse_pos.y);
        AxCoord ax_pos = pixel_to_pointy((PixelCoord) { (int) mouse_pos.x - GRIDPOS_X, 
                                                        (int) mouse_pos.y - GRIDPOS_Y},
                                         &active_grid,
                                         GRIDSIZE);
        printf("ax_pos: < %f %f > \n", ax_pos.q, ax_pos.r);
        if (ax_pos.q >= 0 && ax_pos.q < active_grid.num_qs && 
            ax_pos.r >= 0 && ax_pos.r < active_grid.num_rs)
        {
            if (selected_tile == NULL)
            {
                
                // first selection -- select a tile to merge
                selected_tile = active_grid.cells[(int) ax_pos.q][(int) ax_pos.r];
                selected_tile->selected = true;
                selected_tile_pos = axial_to_cube(ax_pos);

                if (!selected_tile->enabled)
                {
                    selected_tile->selected = false;
                    selected_tile = NULL;
                    selected_tile_pos = axial_to_cube(NULL_AX);
                }
            }
            else
            {
                // with a selected tile, we're on the second tile now.
                Tile *object_tile = active_grid.cells[(int) ax_pos.q][(int) ax_pos.r];
                CubicCoord object_tile_pos = axial_to_cube(ax_pos);

                // are the tiles adjacent?
                float dist = cube_distance(selected_tile_pos, object_tile_pos);
                bool adjacent = dist < 1.0001 && dist > 0.99;
                bool same_as_selected = dist < 0.001;

                // is the tile enabled?

                // are the tile hours in sync?
                bool syncable = selected_tile->time_hours == object_tile->time_hours;

                // if all of the conditions are true, merge
                if (adjacent && object_tile->enabled && syncable)
                {
                    printf("obj %i OP(%i) selected %i = ",
                           object_tile->tix_per_hour,
                           selected_tile->operation,
                           selected_tile->tix_per_hour);
                    selected_tile->enabled = false;
                    switch (selected_tile->operation)
                    {
                        default:
                        case OP_ADD:
                            object_tile->tix_per_hour += selected_tile->tix_per_hour;
                            break;
                        case OP_SUB:
                            object_tile->tix_per_hour -= selected_tile->tix_per_hour;
                            break;
                        case OP_MUL:
                            object_tile->tix_per_hour *= selected_tile->tix_per_hour;
                            break;
                        case OP_DIV:
                            object_tile->tix_per_hour /= selected_tile->tix_per_hour;
                            break;
                        case OP_MOD:
                            object_tile->tix_per_hour %= selected_tile->tix_per_hour;
                            break;
                        
                    }
                    printf("obj %i\n", object_tile->tix_per_hour);

                    // de-select tile
                    selected_tile->selected = false;
                    selected_tile = NULL;
                    selected_tile_pos = axial_to_cube(NULL_AX);
                }
                else if (same_as_selected) 
                {
                    // de-select tile
                    selected_tile->selected = false;
                    selected_tile = NULL;
                    selected_tile_pos = axial_to_cube(NULL_AX);
                }
            }
        }
        else
        {
            // clicking out of the game area de-selects
            selected_tile->selected = false;
            selected_tile = NULL;
            selected_tile_pos = axial_to_cube(NULL_AX);
        }
    }

    //advance_tile(&test_tile);
    update_grid(&active_grid);

    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);
        ClearBackground(DARKBROWN);
        
        
        draw_grid(&active_grid, GRIDPOS_X, GRIDPOS_Y, GRIDSIZE);
        
    EndTextureMode();
    
    // Render to screen (main framebuffer)
    BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw render texture to screen, scaled if required
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, 
            (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

        // TODO: Draw everything that requires to be drawn at this point, maybe UI?

    EndDrawing();
    //----------------------------------------------------------------------------------  
}
