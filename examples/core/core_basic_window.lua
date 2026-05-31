--------------------------------------------------------------------------------------
--
--  raylib [core] example - Basic window
--
--  This example has been created using raylib 6.0 (www.raylib.com)
--  raylib is licensed under an unmodified zlib/libpng license (View raylib.h for details)
--
--  Copyright (c) 2014-2016 Ramon Santamaria (@raysan5)
--  Copyright (c) 2026 yilisharcs
--
--------------------------------------------------------------------------------------

-- Initialization
--------------------------------------------------------------------------------------
local screenWidth = 800
local screenHeight = 450

rl.InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window")

rl.SetTargetFPS(60);                -- Set our game to run at 60 frames-per-second
--------------------------------------------------------------------------------------

-- Main game loop
while not rl.WindowShouldClose() do -- Detect window close button or ESC key
    -- Update
    ---------------------------------------------------------------------------------------
    -- TODO: Update your variables here
    ---------------------------------------------------------------------------------------

    -- Draw
    ---------------------------------------------------------------------------------------
    rl.BeginDrawing()

        rl.ClearBackground(rl.RAYWHITE)

        rl.DrawText("Congrats! You created your first window!", 190, 200, 20, rl.LIGHTGRAY)

    rl.EndDrawing()
    ---------------------------------------------------------------------------------------
end

-- De-Initialization
--------------------------------------------------------------------------------------
rl.CloseWindow()           -- Close window and OpenGL context
--------------------------------------------------------------------------------------
