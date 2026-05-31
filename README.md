<img align="left" src="logo/raylib-lua_256x256.png" width=256>

Lua 5.5 bindings for raylib v6.0, a simple and easy-to-use library to enjoy videogames programming (www.raylib.com)

raylib-lua binding is self-contained in a header-only file: [raylib-lua.h](src/raylib-lua.h). Just include that file
in your project to allow loading and execution of raylib code written in Lua. Check [code examples](examples) for reference.
As a bonus, include [\_meta.lua](src/_meta.lua) to have LSP support.

raylib-lua could be useful for prototyping, tools development, graphic applications, embedded systems and education.

<br><br>

### Build and Usage

A Linux build script is provided to compile the library and the launcher:

```bash
./build.lua
```

It supports both X11 (default) and Wayland backends. For Wayland:

```bash
CFLAGS="-D_GLFW_WAYLAND" ./build.lua
```

### rLuaLauncher

A raylib-lua launcher is also provided: [rluaLauncher](tools/rLuaLauncher/rlualauncher.c). This launcher allows you to run raylib-lua
programs from the command line:

```bash
./build/rlualauncher examples/core/core_basic_window.lua
```

Note that the launcher can also be compiled for other platforms, just link with the Lua library and raylib library.
For more details, just check comments on sources.

### rLuaParser

The bindings are automatically generated using [rLuaParser](tools/rLuaParser/rluaparser.lua), which replaces the old C parser. It
parses `raylib.h` to generate the C header-only binding and Lua metadata. The current implementation
may be incomplete; any help or contribution is welcome!

# License

raylib-lua is licensed under an unmodified zlib/libpng license, which is an OSI-certified, 
BSD-like license that allows static linking with closed source software. Check [LICENSE](LICENSE) for further details.
	
*Copyright (c) 2016-2019 Ghassan Al-Mashareqa and Ramon Santamaria ([@raysan5](https://twitter.com/raysan5))*
*Copyright (c) 2026 yilisharcs ([@yilisharcs](https://twitter.com/yilisharcs))*
