/*******************************************************************************************
*
*   rlualauncher v2.0 - raylib Lua Launcher
*
*   DEPENDENCIES:
*
*   raylib 6.0 - This program uses latest raylib version (www.raylib.com)
*   Lua 5.5    - https://luabinaries.sourceforge.net/download.html
*
*   COMPILATION:
*
*       ./build.lua                             # X11 (default)
*       CFLAGS="-D_GLFW_WAYLAND" ./build.lua    # Wayland
*
*   USAGE:
*
*   Just launch your raylib .lua file from the command line:
*
*       ./build/rLuaLauncher core_basic_window.lua
*
*   NOTE: Windows is not currently supported.
*   TODO: The original had drag-and-drop support.
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2016-2018 Ramon Santamaria (@raysan5)
*   Copyright (c) 2026 yilisharcs
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
********************************************************************************************/

#include "raylib.h"             // raylib library

#define RLUA_IMPLEMENTATION
#include "raylib-lua.h"         // raylib Lua binding

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    const char *entryLua = (argc > 1) ? argv[1] : "main.lua";

    if (argc > 2)
    {
        TraceLog(LOG_WARNING, "Too many arguments provided");
        TraceLog(LOG_INFO, "Usage: %s [script.lua] (defaults to main.lua)", argv[0]);
        return 1;
    }

    lua_State *L = rlua_open();
    if (L == NULL) {
        TraceLog(LOG_ERROR, "LUA: Failed to initialize Lua state");
        return 1;
    }

    if (luaL_dofile(L, entryLua) != LUA_OK) {
        TraceLog(LOG_ERROR, "LUA: %s", lua_tostring(L, -1));
        rlua_close(L);
        return 1;
    }

    rlua_close(L);
    return 0;
}
