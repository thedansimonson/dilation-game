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

const int GRIDSIZE = 460;

static RenderTexture2D target = { 0 };  // Render texture to render our game

static Grid active_grid = { 0 };
static Tile test_tile = (Tile) {60, 2, 10, true, OP_ADD, false};
static const int GRIDPOS_X = 100;
static const int GRIDPOS_Y = 200;

// UI-related funcs
static Tile* selected_tile = NULL;
static CubicCoord selected_tile_pos = { 0 };

static bool start_score = false;
static int round_score = 0;
static int total_score = 0;
static int level_counter = 0;
static bool isolated_cells = false;
static bool too_long = false;

static int tutorial_state = 0;
#define TUTORIAL_LENGTH 5
static char *tutorial_entries[TUTORIAL_LENGTH] = 
    {"Click a hexagon to start.",
     "With a selected hex, you can click to merge it with an adjacent one.",
     "You can only merge when the dials have the same value.",
     "The speed of the dial after merging depends on the [+ - x or /] \nspecified in each dial.",
     "Taking too long, or making it impossible to merge all clocks, \nwill end the game."
    };

static int splash_screen_counter = 3000;

#define NEW_GAME 0
#define LEVEL_ACTIVE 1
#define LEVEL_SUCCESS 2
#define LEVEL_NEW 3
#define GAME_OVER 4
#define TUTORIAL 5
#define SPLASH_SCREEN 6

static int game_state = NEW_GAME;

// TODO: Define global variables here, recommended to make them static

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
static void UpdateDrawFrame(void);      // Update and Draw one frame
static void UpdateDrawFrame_BetweenLevels(void);      // Update and Draw one frame
static void UpdateDrawFrame_ActiveLevel(void);      // Update and Draw one frame

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
    InitAudioDevice();
    
    // TODO: Load resources / Initialize variables at this point
    
    // Render texture to draw, enables screen scaling
    // NOTE: If screen is scaled, mouse input should be scaled proportionally
    target = LoadRenderTexture(screenWidth, screenHeight);
    SetTextureFilter(target.texture, TEXTURE_FILTER_BILINEAR);
    
    active_grid.num_qs = 8;
    active_grid.num_rs = 8;
    init_grid(&active_grid);
    game_state = SPLASH_SCREEN;
    //game_state = LEVEL_NEW;
    

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 60, 1);
#else
    SetTargetFPS(60);     // Set our game frames-per-second
    //--------------------------------------------------------------------------------------
    
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
    
    CloseAudioDevice();
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
    if (game_state == NEW_GAME)
    {
        round_score = 0;
        total_score = 0;
        level_counter = 0;
        game_state = LEVEL_NEW;
    }
    if (game_state == LEVEL_NEW)
    {
        int new_level_size = level_counter + 2;

        free_grid(&active_grid);
        active_grid = (Grid) { 0 };

        active_grid.num_qs = new_level_size;
        active_grid.num_rs = new_level_size;
        init_grid(&active_grid);

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
                                    GetRandomValue(0,OP_DIV),     // op (starting with add)
                                    false
                                   };
                memcpy(active_grid.cells[i][j], &test_tile, sizeof(Tile));

            }
        }
        game_state = LEVEL_ACTIVE;
        round_score = active_grid.num_qs * active_grid.num_rs * 1000;
        start_score = false;
        level_counter++;
    }
    if (game_state == LEVEL_SUCCESS || game_state == GAME_OVER) 
        UpdateDrawFrame_BetweenLevels();
    else if (game_state == SPLASH_SCREEN)
        UpdateDrawFrame_SplashScreen();
    else UpdateDrawFrame_ActiveLevel();
}
void UpdateDrawFrame_ActiveLevel(void)
{
    // Update
    //----------------------------------------------------------------------------------
    // TODO: Update variables / Implement example logic at this point
    

    if (start_score) round_score--;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        // on first click, start scoring.
        start_score = true; 

        tutorial_state += 1;
        tutorial_state %= TUTORIAL_LENGTH;

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
            printf("This is a valid tile.\n");
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
                bool syncable = tiles_mergeable(selected_tile, object_tile);

                // if all of the conditions are true, merge
                if (adjacent && object_tile->enabled && syncable)
                {
                    merge_tiles(object_tile, selected_tile);

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
            if (selected_tile != NULL)
            {
                selected_tile->selected = false;
                selected_tile = NULL;
                selected_tile_pos = axial_to_cube(NULL_AX);
            }
        }
    }

    //advance_tile(&test_tile);
    update_grid(&active_grid);
    if (count_active_tiles(&active_grid) == 1)
    {
        total_score += round_score;
        game_state = LEVEL_SUCCESS;
    }
    
    isolated_cells = check_fail_condition(&active_grid) == ISOLATE;
    too_long = round_score <= 0;
    if ((too_long || isolated_cells) && game_state != LEVEL_SUCCESS)
    {
        round_score = 0;
        game_state = GAME_OVER;
    }
    

    //----------------------------------------------------------------------------------

    // Draw
    //----------------------------------------------------------------------------------
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);
        ClearBackground(DARKBROWN);
        
        draw_grid(&active_grid, GRIDPOS_X, GRIDPOS_Y, GRIDSIZE);
        
        if (level_counter == 1) DrawText(tutorial_entries[tutorial_state], 
                                         20, 20, 20, YELLOW);

        DrawText(TextFormat("Round %i Score: %i", level_counter, round_score), 
                 0, 670, 20, YELLOW);
        DrawText(TextFormat("Total Score: %i", total_score), 0, 700, 20, PALEYELLOW);
        
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

void UpdateDrawFrame_SplashScreen(void)
{
    
    // Update
    //-----------------------

    if (IsKeyPressed(KEY_SPACE)) 
        game_state = NEW_GAME;

    splash_screen_counter-=4;
    if (splash_screen_counter <= 0) splash_screen_counter = 2000;

    BeginTextureMode(target);
        ClearBackground(DARKBROWN);
        
        draw_grid(&active_grid, 
                  -splash_screen_counter/2 + screenWidth/2, 
                  -splash_screen_counter/2 + screenHeight/2, splash_screen_counter);
        DrawRectangle(0, 0, screenWidth, screenHeight, GRAYOUT);

        DrawText("Dilation", 20, 200, 100, YELLOW);

        DrawText("Press [Space] to begin...", 
                        screenWidth / 2, screenHeight / 2 - 60, 20, YELLOW);
        
    EndTextureMode();
    
    // Render to screen (main framebuffer)
    BeginDrawing();
        ClearBackground(RAYWHITE);
        
        // Draw render texture to screen, scaled if required
        DrawTexturePro(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, -(float)target.texture.height }, 
            (Rectangle){ 0, 0, (float)target.texture.width, (float)target.texture.height }, (Vector2){ 0, 0 }, 0.0f, WHITE);

        // TODO: Draw everything that requires to be drawn at this point, maybe UI?

    EndDrawing();

}

void UpdateDrawFrame_BetweenLevels(void)
{

    // Update
    //-----------------------

    if (IsKeyPressed(KEY_SPACE)) 
        switch (game_state)
        {
            case LEVEL_SUCCESS:
                game_state = LEVEL_NEW;
                break;
            case GAME_OVER:
                game_state = NEW_GAME;
                break;
        }
    
    // Draw
    //----------------------------------------------------------------------------------
    // Render game screen to a texture, 
    // it could be useful for scaling or further shader postprocessing
    BeginTextureMode(target);
        ClearBackground(DARKBROWN);
        
        draw_grid(&active_grid, GRIDPOS_X, GRIDPOS_Y, GRIDSIZE);
        DrawRectangle(0, 0, screenWidth, screenHeight, GRAYOUT);
        DrawText(TextFormat("Round %i Score: %i", level_counter, round_score), 
                            screenWidth / 2, screenHeight / 2, 20, YELLOW);
        DrawText(TextFormat("Total Score: %i", total_score), 
                            screenWidth / 2, screenHeight / 2 + 30, 20, PALEYELLOW);

        if (game_state == GAME_OVER)
        {
            DrawText("Game over.", 20, 200, 100, YELLOW);
            if (too_long) DrawText("You were too slow...", 20, 300, 20, PALEYELLOW);
            if (isolated_cells) 
                DrawText("You created an unsolvable situation...", 20, 300, 20, PALEYELLOW);
            DrawText("Press [Space] to begin again!", 20, screenHeight/2 + 60, 20, YELLOW);
        }
        else
        {
            DrawText("Press [Space] to continue...", 
                            screenWidth / 2, screenHeight / 2 - 60, 20, YELLOW);
        }
        
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
