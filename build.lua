#!/usr/bin/env lua

local u <close> = assert(io.popen("uname -s"), "popen failed")
local uname = u:read("*l")

-- TODO: add Windows support (MinGW or MSVC toolchain, Win32 platform libs)
if uname ~= "Linux" then
    print("ERROR: this build script only supports Linux (detected: " .. tostring(uname) .. ")")
    os.exit(1)
end

local CC = os.getenv("CC") or "gcc"
local AR = os.getenv("AR") or "ar"
local CFLAGS = os.getenv("CFLAGS") or ""
local LDFLAGS = os.getenv("LDFLAGS") or ""
local BUILD = os.getenv("BUILD_DIR") or "build"
local RAYLIB_SRC_PATH = os.getenv("RAYLIB_SRC_PATH") or "../raylib/src"

-- library types: STATIC or SHARED
local RAYLIB_LIBTYPE = os.getenv("RAYLIB_LIBTYPE") or "STATIC"
local LUA_LIBTYPE = os.getenv("LUA_LIBTYPE") or "STATIC"

-- backend: X11 default (alt: -D_GLFW_WAYLAND in CFLAGS)
local glfw_flag = CFLAGS:match("-D_GLFW_[%w_]+") or "-D_GLFW_X11"
local platform_libs = glfw_flag:find("WAYLAND") and "-lwayland-client -lwayland-cursor -lwayland-egl -lxkbcommon"
    or "-lX11"

local common_libs = table.concat({
    "-lGL", -- OpenGL
    "-lm", -- math functions, used internally
    "-lpthread", -- POSIX threads
    "-ldl", -- dlopen/dlsym
    "-lrt", -- real-time extensions
    platform_libs,
}, " ")

local function exists(path)
    local f <close> = io.open(path, "r")
    return f ~= nil
end

local function run(cmd)
    print(cmd)
    local ok = os.execute(cmd)
    if not ok then
        os.exit(1)
    end
end

os.execute("mkdir -p " .. BUILD)

-- libraylib
local raylib_lib = (RAYLIB_LIBTYPE == "SHARED") and (BUILD .. "/libraylib.so") or (BUILD .. "/libraylib.a")

-- invalidate cache on backend switch
local marker_path = BUILD .. "/.backend"
local m_in <close> = io.open(marker_path, "r")
local old_backend = m_in and m_in:read("*l") or ""

if old_backend ~= glfw_flag and exists(raylib_lib) then
    print("INFO: backend changed; rebuilding raylib")
    os.execute("rm " .. raylib_lib)
end

-- generate wayland protocol headers from bundled XML
if glfw_flag:find("WAYLAND") then
    local wl_deps = RAYLIB_SRC_PATH .. "/external/glfw/deps/wayland"
    local protocols = {
        "fractional-scale-v1.xml",
        "idle-inhibit-unstable-v1.xml",
        "pointer-constraints-unstable-v1.xml",
        "relative-pointer-unstable-v1.xml",
        "viewporter.xml",
        "wayland.xml",
        "xdg-activation-v1.xml",
        "xdg-decoration-unstable-v1.xml",
        "xdg-shell.xml",
    }
    for _, xml in ipairs(protocols) do
        local base = xml:gsub("%.xml$", "")
        run(("wayland-scanner client-header %s/%s %s/%s-client-protocol.h"):format(wl_deps, xml, BUILD, base))
        run(("wayland-scanner private-code %s/%s %s/%s-client-protocol-code.h"):format(wl_deps, xml, BUILD, base))
    end
end

if not exists(raylib_lib) then
    local raylib_sources = {
        RAYLIB_SRC_PATH .. "/raudio.c",
        RAYLIB_SRC_PATH .. "/rcore.c",
        RAYLIB_SRC_PATH .. "/rglfw.c",
        RAYLIB_SRC_PATH .. "/rmodels.c",
        RAYLIB_SRC_PATH .. "/rshapes.c",
        RAYLIB_SRC_PATH .. "/rtext.c",
        RAYLIB_SRC_PATH .. "/rtextures.c",
    }

    local raylib_cflags = table.concat({
        "-std=c99",
        "-DPLATFORM_DESKTOP_GLFW",
        "-DGRAPHICS_API_OPENGL_33",
        "-D_GNU_SOURCE",
        glfw_flag,
        -- raylib default, good balance of speed and perf
        "-O1",
        "-Wall",
        -- required for shared, safe for static
        "-fPIC",
        CFLAGS,
        -- [[ INCLUDES ]]
        "-I" .. RAYLIB_SRC_PATH,
        "-I" .. RAYLIB_SRC_PATH .. "/external/glfw/include",
        "-I" .. BUILD,
    }, " ")

    if RAYLIB_LIBTYPE == "SHARED" then
        run(table.concat({
            CC,
            "-o",
            raylib_lib,
            "-shared",
            raylib_cflags,
            table.concat(raylib_sources, " "),
            -- [[ LIBS ]]
            common_libs,
            LDFLAGS,
        }, " "))
    else
        -- compile raylib into object files for the static lib archive
        local objs = {}
        for _, src in ipairs(raylib_sources) do
            local obj = BUILD .. "/" .. src:match("([^/]+)%.c$") .. ".o"
            run(table.concat({
                CC,
                "-c",
                src,
                "-o",
                obj,
                raylib_cflags,
            }, " "))
            table.insert(objs, obj)
        end
        -- generate archive
        run(table.concat({
            AR,
            "rcs",
            raylib_lib,
            table.concat(objs, " "),
        }, " "))
        -- cleanup artifacts
        run("rm " .. table.concat(objs, " "))
    end

    local m <close> = io.open(marker_path, "w")
    if m then
        m:write(glfw_flag)
    end
else
    print(raylib_lib .. " found; skipping (cached)")
end

-- tools/rLuaLauncher/rlualauncher.c
local lua_lib_path = "src/external/lua/lib/liblua55.a"
if LUA_LIBTYPE == "SHARED" then
    lua_lib_path = "-llua55"
    -- deploy .so for shared build
    if exists("src/external/lua/lib/liblua55.so") then
        run("cp src/external/lua/lib/liblua55.so " .. BUILD .. "/liblua55.so")
    end
end

local raylib_link = (RAYLIB_LIBTYPE == "SHARED") and "-lraylib" or raylib_lib

run(table.concat({
    CC,
    "-o",
    BUILD .. "/rlualauncher",
    "-std=c99",
    -- raylib default, good balance of speed and perf
    "-O1",
    "-Wall",
    CFLAGS,
    -- [[ INCLUDES ]]
    "-I" .. RAYLIB_SRC_PATH,
    "-Isrc",
    "-Isrc/external/lua/include",
    -- [[ SOURCES ]]
    "tools/rLuaLauncher/rlualauncher.c",
    -- prepend lib dirs to the linker path
    "-L" .. BUILD,
    "-Lsrc/external/lua/lib",
    -- [[ LIBS ]]
    raylib_link,
    lua_lib_path,
    common_libs,
    LDFLAGS,
    -- embed the executable's dir as the dll search path at runtime
    "-Wl,-rpath,'$ORIGIN'",
}, " "))

print("---")
print(raylib_lib)
print(BUILD .. "/rlualauncher")
if LUA_LIBTYPE == "SHARED" then
    print(BUILD .. "/liblua55.so")
end
