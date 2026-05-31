#!/usr/bin/env lua
--[[ **********************************************************************************************

    rluaparser v6.0 - A simple raylib header parser to generate automatic Lua bindings

    FEATURES:
        - Scans raylib.h to generate C binding header (raylib-lua.h)
        - Generates LuaLS type annotations (_meta.lua) for full LSP support
        - Hybrid type system with automatic resource management (__gc)
        - Support for RAYLIB_STRIP_PREFIX global namespace toggle

    NOTES:
        - Designed for raylib 6.0 and Lua 5.5
        - Resolves alias chains (e.g. Texture2D -> Texture) automatically
        - TODO: Missing low-level callbacks:
            - [ ] LoadFileData
            - [ ] SaveFileData
            - [ ] LoadFileText
            - [ ] SaveFileText
            - [ ] AudioCallback
        - TODO: Generate type annotations for RAYLIB_STRIP_PREFIX

    DEPENDENCIES:
        - Lua 5.5 (Standard library only, no external dependencies)

    USAGE:
        lua rluaparser.lua <path/to/raylib.h> <output.h> [output.lua]

    LICENSE: zlib/libpng

    rluaparser is licensed under an unmodified zlib/libpng license, which is an OSI-certified,
    BSD-like license that allows static linking with closed source software:

    Copyright (c) 2026 yilisharcs

    This software is provided "as-is", without any express or implied warranty. In no event
    will the authors be held liable for any damages arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose, including commercial
    applications, and to alter it and redistribute it freely, subject to the following restrictions:

      1. The origin of this software must not be misrepresented; you must not claim that you
      wrote the original software. If you use this software in a product, an acknowledgment
      in the product documentation would be appreciated but is not required.

      2. Altered source versions must be plainly marked as such, and must not be misrepresented
      as being the original software.

      3. This notice may not be removed or altered from any source distribution.

********************************************************************************************** ]]

-- PARSER ====================================================================================== {{{

local Parser = {}

function Parser.preprocess(text)
    -- strip block comments
    text = text:gsub("/%*.-%*/", "")

    local result, acc, line_nr = {}, {}, 0
    for raw_line in text:gmatch("([^\n]+)") do
        line_nr = line_nr + 1
        local line = raw_line

        if line:match("^%s*//") then
            -- standalone comment
            local c = line:match("^%s*//(.+)$")
            c = c and c:match("^%s*(.-)%s*$")
            if c then
                acc[#acc + 1] = c
            end
        else
            -- collect trailing
            local t_part, t_comment = line:match("^(.-)//(.+)$")
            if t_part then
                -- trim before
                t_part = t_part:match("^%s*(.-)%s*$")
                if #t_part > 0 then
                    -- trim after
                    local comment = t_comment:match("^%s*(.-)%s*$")
                    -- snapshots acc into notes, then resets
                    local notes = #acc > 0 and { table.unpack(acc) } or nil
                    acc = {} -- start fresh for next block
                    result[#result + 1] = {
                        text = t_part,
                        comment = comment,
                        notes = notes,
                        line = line_nr,
                    }
                end
            else
                -- plain code. trim again
                local trimmed = line:match("^%s*(.-)%s*$")
                if #trimmed > 0 then
                    -- snapshots acc into notes, then resets
                    local notes = #acc > 0 and { table.unpack(acc) } or nil
                    acc = {} -- start fresh for next block
                    result[#result + 1] = {
                        text = trimmed,
                        notes = notes,
                        line = line_nr,
                    }
                end
            end
        end
    end

    return result
end

-- keywords for type-name splitting
local C_KEYWORDS = {
    bool = true,
    char = true,
    const = true,
    double = true,
    enum = true,
    float = true,
    int = true,
    long = true,
    short = true,
    signed = true,
    struct = true,
    union = true,
    unsigned = true,
    void = true,
}

-- split "unsigned char *name" into type="unsigned char *", name="name"
function Parser.split_type_name(s)
    local tokens = {}
    -- split by whitespace
    for tok in s:gmatch("%S+") do
        tokens[#tokens + 1] = tok
    end
    if #tokens == 0 then
        return "", ""
    end

    local name_tok = tokens[#tokens]
    -- strip leading * from name, as they are pointer qualifiers on the type
    local pointer_prefix = name_tok:match("^(%*+)")
    if pointer_prefix then
        name_tok = name_tok:sub(#pointer_prefix + 1)
    end
    if #name_tok == 0 or C_KEYWORDS[name_tok] then
        return s, ""
    end

    -- all tokens except last form the type; reattach any * prefixes
    local type_tokens = {}
    for i = 1, #tokens - 1 do
        type_tokens[#type_tokens + 1] = tokens[i]
    end
    if pointer_prefix then
        type_tokens[#type_tokens + 1] = pointer_prefix
    end
    return table.concat(type_tokens, " "), name_tok
end

-- }}}

-- SCANNERS ==================================================================================== {{{
local matchers = {
    -- RLAPI function declaration
    {
        match = function(line)
            return line.text:match("^RLAPI")
        end,
        consume = function(lines, idx)
            local line = lines[idx]
            local retType, funcName, params = line.text:match("^RLAPI%s+(.-)%s*([%w_]+)%s*%((.-)%)%s*;%s*$")
            if not retType then
                io.stderr:write("WARN: failed to parse RLAPI: ", line.text, "\n")
                return nil, 1
            end
            local node = {
                type = "function",
                name = funcName,
                retType = retType,
                params = {},
                notes = line.notes,
                comment = line.comment,
            }
            if params == "void" then
                goto finalize
            end

            for param in params:gmatch("[^,]+") do
                local trimmed = param:match("^%s*(.-)%s*$")
                if trimmed:match("^%.%.%.$") then
                    node.params[#node.params + 1] = {
                        name = "...",
                        isVariadic = true,
                    }
                else
                    local pType, pName = Parser.split_type_name(trimmed)
                    node.params[#node.params + 1] = {
                        type = pType,
                        name = pName,
                    }
                end
            end

            ::finalize::

            return node, 1
        end,
    },
    -- typedef struct ... { ... } Name;
    {
        match = function(line)
            return line.text:match("^typedef struct") and line.text:match("{")
        end,
        consume = function(lines, idx)
            local line = lines[idx]
            local structName = line.text:match("^typedef struct%s+([%w_]+)%s*{")
            local fields = {}
            local i = idx + 1
            while i <= #lines do
                local l = lines[i]
                local closeName = l.text:match("^}%s*([%w_]+)%s*;%s*$")
                if closeName then
                    structName = structName or closeName
                    return {
                        type = "struct",
                        name = structName,
                        fields = fields,
                        notes = line.notes,
                        comment = line.comment,
                    },
                        -- lines consumed: from typedef struct to closing } Name;
                        i - idx + 1
                end
                local fieldText = l.text:match("^%s*(.-);%s*$")
                if fieldText then
                    -- handle comma-separated fields like "float m0, m4, m8, m12;"
                    local clean = fieldText:gsub(",%s+", ",")
                    local fType, fNames = clean:match("^(.-)%s+([%w_,]+)%s*$")
                    if not fType then
                        fType, fNames = Parser.split_type_name(clean)
                    end
                    if fType and fNames then
                        for fName in fNames:gmatch("[^,]+") do
                            fields[#fields + 1] = {
                                type = fType,
                                name = fName,
                                comment = l.comment,
                            }
                        end
                    end
                end
                -- advance to next line in struct body
                i = i + 1
            end
            io.stderr:write("WARN: unclosed struct: ", line.text, "\n")
            return nil, #lines - idx + 1
        end,
    },
    -- typedef enum { ... } Name;
    {
        match = function(line)
            return line.text:match("^typedef enum") and line.text:match("{")
        end,
        consume = function(lines, idx)
            local line = lines[idx]
            local text = line.text
            local fields = {}
            -- handle single-line typedef enum { a, b } Name; or typedef enum Name { a, b } Name;
            local body, closeName = text:match("{(.-)}%s*([%w_]+)%s*;%s*$")
            if closeName then
                if closeName == "bool" then
                    return nil, 1
                end
                for part in body:gmatch("[^,]+") do
                    local trimmed = part:match("^%s*(.-)%s*$")
                    local name, value = trimmed:match("^([%w_]+)%s*=%s*(.+)$")
                    if name then
                        fields[#fields + 1] = { name = name, value = value }
                    else
                        name = trimmed:match("^([%w_]+)%s*$")
                        if name then
                            fields[#fields + 1] = {
                                name = name,
                                value = nil,
                            }
                        end
                    end
                end
                return {
                    type = "enum",
                    name = closeName,
                    fields = fields,
                    notes = line.notes,
                    comment = line.comment,
                },
                    1
            end
            local i = idx + 1
            while i <= #lines do
                local l = lines[i]
                closeName = l.text:match("^}%s*([%w_]+)%s*;%s*$")
                if closeName then
                    if closeName ~= "bool" then
                        return {
                            type = "enum",
                            name = closeName,
                            fields = fields,
                            notes = line.notes,
                            comment = line.comment,
                        },
                            i - idx + 1
                    end
                    return nil, i - idx + 1
                end
                local fText = l.text:match("^%s*(.-),?%s*$")
                if fText and #fText > 0 then
                    local name, value = fText:match("^([%w_]+)%s*=%s*(.+)$")
                    if name then
                        fields[#fields + 1] = {
                            name = name,
                            value = value,
                            comment = l.comment,
                        }
                    else
                        name = fText:match("^([%w_]+)%s*$")
                        if name then
                            fields[#fields + 1] = {
                                name = name,
                                value = nil,
                                comment = l.comment,
                            }
                        end
                    end
                end
                i = i + 1
            end
            io.stderr:write("WARN: unclosed enum: ", text, "\n")
            return nil, #lines - idx + 1
        end,
    },
    -- #define NAME value
    {
        match = function(line)
            local text = line.text
            if not text:match("^#define") then
                return false
            elseif text:match("^#define%s+RL_") then
                return false
            elseif text:match("^#define%s+RAYLIB_H%s*$") then
                return false
            else
                return true
            end
        end,
        consume = function(lines, idx)
            local line = lines[idx]
            local name, value = line.text:match("^#define%s+([%w_]+)%s+(.*)$")
            if not name then
                return nil, 1
            end
            -- filter function-like macros (value contains ( but not as leading expression)
            -- e.g. __declspec(dllexport) vs (PI/180.0f)
            if value:match("%S%(") and not value:match("CLITERAL") then
                return nil, 1
            end
            local category
            if name:match("^RAYLIB_VERSION") then
                category = "version"
            elseif value:match("CLITERAL") then
                category = "color"
            elseif value:match('^"') then
                category = "string"
            elseif value:match("%.") or value:match("%df$") or value:match("^PI$") then
                category = "float"
            elseif value:match("^[%w_]+$") then
                category = "alias"
            else
                category = "integer"
            end
            return {
                type = "define",
                name = name,
                value = value,
                category = category,
                notes = line.notes,
                comment = line.comment,
            },
                1
        end,
    },
    -- typedef <type> <name>;  (alias, forward decl, or callback)
    {
        match = function(line)
            local text = line.text
            return text:match("^typedef") and not text:match("^typedef%s+(enum|struct)")
        end,
        consume = function(lines, idx)
            local line = lines[idx]
            local text = line.text
            -- callback typedef: typedef <ret> (*<name>)(<params>);
            if text:match("%(%*") then
                local retType, cbName, params = text:match("^typedef%s+(.-)%s*%(%*([%w_]+)%)%s*%((.-)%)%s*;%s*$")
                if retType and cbName then
                    local node = {
                        type = "function",
                        name = cbName,
                        retType = retType,
                        isCallback = true,
                        notes = line.notes,
                        comment = line.comment,
                    }
                    if params and params ~= "void" then
                        node.params = {}
                        for param in params:gmatch("[^,]+") do
                            local trimmed = param:match("^%s*(.-)%s*$")
                            local pType, pName = Parser.split_type_name(trimmed)
                            node.params[#node.params + 1] = {
                                type = pType,
                                name = pName,
                            }
                        end
                    end
                    return node, 1
                end
                io.stderr:write("WARN: failed to parse callback typedef: ", text, "\n")
                return nil, 1
            end
            -- regular alias: typedef <type> <name>;
            local aliasedType, aliasName = text:match("^typedef%s+(.-)%s*([%w_]+)%s*;%s*$")
            if aliasedType and aliasName then
                aliasedType = aliasedType:match("^%s*(.-)%s*$")
                return {
                    type = "alias",
                    name = aliasName,
                    aliasedType = aliasedType,
                    notes = line.notes,
                    comment = line.comment,
                },
                    1
            end
            io.stderr:write("WARN: failed to parse typedef: ", text, "\n")
            return nil, 1
        end,
    },
}

function Parser.scan(lines)
    local ast = {
        function_xs = {},
        struct_xs = {},
        enum_xs = {},
        define_xs = {},
        alias_xs = {},
    }
    local idx = 1
    while idx <= #lines do
        local line = lines[idx]
        local matched = false
        for _, m in ipairs(matchers) do
            if m.match(line) then
                local node, consumed = m.consume(lines, idx)
                if node then
                    local key = node.type .. "_xs"
                    -- push node onto its type-specific list in the ast table
                    ast[key][#ast[key] + 1] = node
                end
                -- always consume
                idx = idx + consumed
                matched = true
                break
            end
        end
        if not matched then
            if
                -- blank lines
                not line.text:match("^%s*$")
                -- preproc conds
                and not line.text:match("^#if")
                and not line.text:match("^#else")
                and not line.text:match("^#endif")
                and not line.text:match("^#ifdef")
                and not line.text:match("^#ifndef")
                and not line.text:match("^#elif")
                and not line.text:match("^#pragma")
                and not line.text:match("^#error")
                and not line.text:match("^#undef")
                and not line.text:match("^#include")
                -- unused defines
                and not line.text:match("^#define%s+RAYLIB_H")
                and not line.text:match("^#define%s+RL_%w+_TYPE%s*$")
                and not line.text:match("^#define%s+RL_MALLOC")
                and not line.text:match("^#define%s+RL_CALLOC")
                and not line.text:match("^#define%s+RL_REALLOC")
                and not line.text:match("^#define%s+RL_FREE")
                and not line.text:match('^extern%s+"C"')
                -- final closing brace
                and not line.text:match("^}%s*$")
            then
                io.stderr:write("WARN: unrecognized: ", line.text, "\n")
            end
            idx = idx + 1
        end
    end
    return ast
end

function Parser.analyze(ast)
    -- build alias resolution map
    local type_map = {}
    for _, alias in ipairs(ast.alias_xs) do
        type_map[alias.name] = alias.aliasedType
    end
    -- resolve alias chains (e.g. Color -> unsigned int -> stop)
    for name, resolved in pairs(type_map) do
        local seen = {}
        while type_map[resolved] and not seen[resolved] do
            seen[resolved] = true
            resolved = type_map[resolved]
        end
        type_map[name] = resolved
    end
    ast.type_map = type_map

    -- fill sequential enum values
    for _, enum in ipairs(ast.enum_xs) do
        local val = 0
        for _, field in ipairs(enum.fields) do
            if field.value == nil then
                field.value = val
            else
                val = tonumber(field.value) or val
            end
            val = val + 1
        end
    end

    -- classify mutated types
    local mutated_types = {}
    for _, f in ipairs(ast.function_xs) do
        if f.name:match("^Unload") or f.name:match("^Export") then
            goto next_func
        end
        for _, p in ipairs(f.params or {}) do
            if not p.type then
                goto next_param
            end
            local base = p.type:match("^(.-)%s*%*$")
            if not base or base:match("const%s+") then
                goto next_param
            end
            base = base:match("^%s*(.-)%s*$")
            mutated_types[base] = true
            ::next_param::
        end
        ::next_func::
    end

    -- resolve mutated aliases to base types
    for name, _ in pairs(mutated_types) do
        local resolved = type_map[name]
        if resolved then
            mutated_types[resolved] = true
        end
    end
    ast.mutated_types = mutated_types
end
-- }}}

-- C EMITTER =================================================================================== {{{

-- value types: structs passed by value (Lua tables)
local VALUE_STRUCTS = {
    -- math
    Matrix = true,
    Quaternion = true,
    Vector2 = true,
    Vector3 = true,
    Vector4 = true,
    -- geometry and color
    Color = true,
    Rectangle = true,
    -- cameras
    Camera = true,
    Camera2D = true,
    Camera3D = true,
    -- collision
    BoundingBox = true,
    Ray = true,
    RayCollision = true,
    -- asset
    GlyphInfo = true,
    NPatchInfo = true,
    -- nested and auxiliary types
    BoneInfo = true,
    MaterialMap = true,
    ModelSkeleton = true,
    Transform = true,
    -- system and VR
    AutomationEvent = true,
    AutomationEventList = true,
    FilePathList = true,
    VrDeviceInfo = true,
    VrStereoConfig = true,
}

-- resource types: structs passed by pointer with __gc/unload
local RESOURCE_TYPES = {
    -- visual
    Image = "UnloadImage",
    RenderTexture = "UnloadRenderTexture",
    RenderTexture2D = "UnloadRenderTexture",
    Texture = "UnloadTexture",
    Texture2D = "UnloadTexture",
    TextureCubemap = "UnloadTexture",
    -- 3D
    Mesh = "UnloadMesh",
    Model = "UnloadModel",
    ModelAnimation = "", -- Requires count for unloading, skip __gc for now
    -- audio
    AudioStream = "UnloadAudioStream",
    Music = "UnloadMusicStream",
    Sound = "UnloadSound",
    Wave = "UnloadWave",
    -- other
    Font = "UnloadFont",
    Material = "UnloadMaterial",
    Shader = "UnloadShader",
}

-- stylua: ignore
local PRIMITIVE_TYPES = {
    int                     = { check = "(int)luaL_checkinteger",                push = "lua_pushinteger"       },
    float                   = { check = "(float)luaL_checknumber",               push = "lua_pushnumber"        },
    bool                    = { check = "lua_toboolean",                         push = "lua_pushboolean"       },
    double                  = { check = "luaL_checknumber",                      push = "lua_pushnumber"        },
    long                    = { check = "(long)luaL_checkinteger",               push = "lua_pushinteger"       },
    unsigned                = { check = "(unsigned)luaL_checkinteger",           push = "lua_pushinteger"       },
    unsigned_int            = { check = "(unsigned int)luaL_checkinteger",       push = "lua_pushinteger"       },
    unsigned_char           = { check = "(unsigned char)luaL_checkinteger",      push = "lua_pushinteger"       },
    char                    = { check = "(char)luaL_checkinteger",               push = "lua_pushinteger"       },
    unsigned_short          = { check = "(unsigned short)luaL_checkinteger",     push = "lua_pushinteger"       },
    short                   = { check = "(short)luaL_checkinteger",              push = "lua_pushinteger"       },
    const_char_ptr          = { check = "luaL_checkstring",                      push = "lua_pushstring"        },
    char_ptr                = { check = "luaL_checkstring",                      push = "lua_pushstring"        },
    void_ptr                = { check = "lua_touserdata",                        push = "lua_pushlightuserdata" },
    unsigned_char_ptr       = { check = "(unsigned char *)lua_touserdata",       push = "lua_pushlightuserdata" },
    const_unsigned_char_ptr = { check = "(const unsigned char *)lua_touserdata", push = "lua_pushlightuserdata" },
}

-- normalize "const char *" -> "const_char_ptr"
function Parser.normalizeType(t)
    if not t then
        return ""
    else
        return t:gsub("%s*%*%s*", "_ptr"):gsub("%s+", "_"):gsub("_+", "_"):gsub("^_", ""):gsub("_$", "")
    end
end

function Parser.getCheckExpression(normType, rawType, index)
    if PRIMITIVE_TYPES[normType] then
        return ("%s(L, %d)"):format(PRIMITIVE_TYPES[normType].check, index)
    elseif normType:match("_ptr$") or normType:match("Callback$") then
        return ("(%s)lua_touserdata(L, %d)"):format(rawType, index)
    elseif RESOURCE_TYPES[normType] then
        return ('*(%s*)RLUA_CHECK_Resource(L, %d, "%s")'):format(rawType, index, normType)
    else
        return ("RLUA_CHECK_%s(L, %d)"):format(normType, index)
    end
end

function Parser.getPushStatement(normType, rawType, source)
    if PRIMITIVE_TYPES[normType] then
        return ("%s(L, %s);"):format(PRIMITIVE_TYPES[normType].push, source)
    elseif normType:match("_ptr$") then
        return ("lua_pushlightuserdata(L, %s);"):format(source)
    elseif RESOURCE_TYPES[normType] then
        return ('RLUA_PUSH_Resource(L, &%s, sizeof(%s), "%s");'):format(source, rawType, normType)
    else
        return ("RLUA_PUSH_%s(L, %s);"):format(normType, source)
    end
end

-- render function wrapper
function Parser.renderFunction(func)
    local t = {}
    for _, note in ipairs(func.notes or {}) do
        t[#t + 1] = ("// %s"):format(note)
    end

    -- discard typedefs but keep the comments
    if func.isCallback then
        if #t > 0 then
            return table.concat(t, "\n") .. "\n"
        else
            return nil
        end
    end

    if func.comment then
        t[#t + 1] = ("// %s"):format(func.comment)
    end
    t[#t + 1] = ("static int rl_%s(lua_State *L)"):format(func.name)
    t[#t + 1] = "{"

    if func.name == "SetTraceLogCallback" then
        t[#t + 1] = [[
    if (lua_isnil(L, 1)) {
        if (RLUA_LogRef != LUA_REFNIL) {
            luaL_unref(L, LUA_REGISTRYINDEX, RLUA_LogRef);
            RLUA_LogRef = LUA_REFNIL;
        }
        SetTraceLogCallback(NULL);
    } else {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        if (RLUA_LogRef != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, RLUA_LogRef);
        lua_pushvalue(L, 1);
        RLUA_LogRef = luaL_ref(L, LUA_REGISTRYINDEX);
        SetTraceLogCallback(RLUA_TraceLogTrampoline);
    }
    return 0;
}
]]
        t[#t + 1] = ""
        return table.concat(t, "\n")
    end

    local isVariadic = false
    for _, p in ipairs(func.params or {}) do
        if p.isVariadic then
            isVariadic = true
            break
        end
    end

    if isVariadic then
        if func.name == "TraceLog" then
            t[#t + 1] =
                -- c
                [[
    int n = lua_gettop(L);
    if (n < 2) return luaL_error(L, "TraceLog requires at least 2 arguments");
    int logLevel = (int)luaL_checkinteger(L, 1);
    if (n == 2) {
        TraceLog(logLevel, "%s", luaL_checkstring(L, 2));
    } else {
        lua_getglobal(L, "string");
        lua_getfield(L, -1, "format");
        for (int i = 2; i <= n; i++) lua_pushvalue(L, i);
        lua_call(L, n - 1, 1);
        TraceLog(logLevel, "%s", lua_tostring(L, -1));
        lua_pop(L, 2);
    }
    return 0;]]
        elseif func.name == "TextFormat" then
            t[#t + 1] =
                -- c
                [[
    int n = lua_gettop(L);
    if (n < 1) return luaL_error(L, "TextFormat requires at least 1 argument");
    if (n == 1) {
        lua_pushstring(L, TextFormat("%s", luaL_checkstring(L, 1)));
    } else {
        lua_getglobal(L, "string");
        lua_getfield(L, -1, "format");
        for (int i = 1; i <= n; i++) lua_pushvalue(L, i);
        lua_call(L, n, 1);
        const char *formatted = lua_tostring(L, -1);
        lua_pushstring(L, TextFormat("%s", formatted));
        lua_insert(L, 1);
        lua_settop(L, 1);
    }
    return 1;]]
        else
            t[#t + 1] = ("    // TODO: Hand-write variadic wrapper for %s"):format(func.name)
            t[#t + 1] = "    return 0;"
        end
    else
        -- check for "pointer return + out-param count" pattern
        local retNorm = Parser.normalizeType(func.retType)
        local countParam = nil
        if func.retType:match("%*$") then
            for _, p in ipairs(func.params or {}) do
                if p.type == "int *" or p.type == "unsigned int *" then
                    countParam = p
                    break
                end
            end
        end

        -- check for value-struct mutation candidates
        local mutates = {}
        if func.name:match("^Unload") or func.name:match("^Export") then
            goto end_mutates
        end
        for i, p in ipairs(func.params or {}) do
            local base = p.type:match("^(.-)%s*%*$")
            if not base or base:match("const%s+") then
                goto next_mutate
            end
            base = base:match("^%s*(.-)%s*$")
            if VALUE_STRUCTS[base] then
                mutates[i] = base
            end
            ::next_mutate::
        end
        ::end_mutates::

        -- unpack arguments from the Lua stack
        for i, p in ipairs(func.params or {}) do
            if p == countParam then
                t[#t + 1] = ("    int %s = 0;"):format(p.name)
            elseif mutates[i] then
                t[#t + 1] = ("    %s %s = RLUA_CHECK_%s(L, %d);"):format(mutates[i], p.name, mutates[i], i)
            else
                local pNorm = Parser.normalizeType(p.type)
                local expr = Parser.getCheckExpression(pNorm, p.type, i)
                local declType = p.type
                if pNorm == "char_ptr" or pNorm == "const_char_ptr" then
                    declType = "const char *"
                end
                t[#t + 1] = ("    %s %s = %s;"):format(declType, p.name, expr)
            end
        end

        -- construct the raylib C function call
        local callArgs = {}
        for i, p in ipairs(func.params or {}) do
            if p == countParam then
                callArgs[#callArgs + 1] = "&" .. p.name
            elseif mutates[i] then
                callArgs[#callArgs + 1] = "&" .. p.name
            else
                local arg = p.name
                local pNorm = Parser.normalizeType(p.type)
                if pNorm == "char_ptr" then
                    arg = ("(%s)%s"):format(p.type, p.name)
                end
                callArgs[#callArgs + 1] = arg
            end
        end
        local args = table.concat(callArgs, ", ")

        if retNorm == "void" then
            t[#t + 1] = ("    %s(%s);"):format(func.name, args)
            for i, p in ipairs(func.params or {}) do
                if mutates[i] then
                    t[#t + 1] = ("    RLUA_WRITEBACK_%s(L, %d, %s);"):format(mutates[i], i, p.name)
                end
            end
            t[#t + 1] = "    return 0;"
        else
            t[#t + 1] = ("    %s result = %s(%s);"):format(func.retType, func.name, args)
            for i, p in ipairs(func.params or {}) do
                if mutates[i] then
                    t[#t + 1] = ("    RLUA_WRITEBACK_%s(L, %d, %s);"):format(mutates[i], i, p.name)
                end
            end
            if countParam then
                local baseType = func.retType:gsub("%s*%*$", "")
                t[#t + 1] = ('    RLUA_PUSH_View(L, result, %s, "%s", true);'):format(countParam.name, baseType)
                t[#t + 1] = "    return 1;"
            else
                t[#t + 1] = "    " .. Parser.getPushStatement(retNorm, func.retType, "result")
                t[#t + 1] = "    return 1;"
            end
        end
    end

    t[#t + 1] = "}"
    t[#t + 1] = ""
    return table.concat(t, "\n")
end

-- render struct check (table -> C struct)
function Parser.renderStructCheck(struct)
    local t = {}
    if struct.notes then
        for _, note in ipairs(struct.notes) do
            t[#t + 1] = ("// %s"):format(note)
        end
    end
    t[#t + 1] = ("static %s RLUA_CHECK_%s(lua_State *L, int index)"):format(struct.name, struct.name)
    t[#t + 1] = "{"
    t[#t + 1] = ("    %s result = { 0 };"):format(struct.name)
    t[#t + 1] = "    if (lua_istable(L, index)) {"

    local sorted = {}
    for i, f in ipairs(struct.fields or {}) do
        sorted[#sorted + 1] = { field = f, index = i }
    end
    table.sort(sorted, function(a, b)
        local na = a.field.name:match("^m(%d+)$")
        local nb = b.field.name:match("^m(%d+)$")
        if na and nb then
            return tonumber(na) < tonumber(nb)
        end
        return a.index < b.index
    end)

    for _, entry in ipairs(sorted) do
        local field = entry.field
        local name, array_size = field.name:match("([%w_]+)%[(%d+)%]")
        if not name then
            t[#t + 1] = ('        lua_getfield(L, index, "%s");'):format(field.name)
            local fNorm = Parser.normalizeType(field.type)
            local expr = Parser.getCheckExpression(fNorm, field.type, -1)
            local comment = field.comment and (" // %s"):format(field.comment) or ""
            t[#t + 1] = ("        result.%s = %s;%s"):format(field.name, expr, comment)
            t[#t + 1] = "        lua_pop(L, 1);"
        else
            t[#t + 1] = ('        lua_getfield(L, index, "%s");'):format(name)
            if field.type == "char" then
                local comment = field.comment and (" // %s"):format(field.comment) or ""
                t[#t + 1] = ("        if (lua_isstring(L, -1)) { strncpy(result.%s, lua_tostring(L, -1), %s - 1); }%s"):format(
                    name,
                    array_size,
                    comment
                )
            else
                t[#t + 1] = "        if (lua_istable(L, -1)) {"
                t[#t + 1] = ("            for (int i = 0; i < %s; i++) {"):format(array_size)
                t[#t + 1] = "                lua_geti(L, -1, i + 1);"
                local fNorm = Parser.normalizeType(field.type)
                local expr = Parser.getCheckExpression(fNorm, field.type, -1)
                t[#t + 1] = ("                result.%s[i] = %s;"):format(name, expr)
                t[#t + 1] = "                lua_pop(L, 1);"
                t[#t + 1] = "            }"
                t[#t + 1] = "        }"
            end
            t[#t + 1] = "        lua_pop(L, 1);"
        end
    end

    t[#t + 1] = "    }"
    t[#t + 1] = "    return result;"
    t[#t + 1] = "}"
    return table.concat(t, "\n")
end

-- render struct push (C struct -> Lua table)
function Parser.renderStructPush(struct)
    local t = {}
    t[#t + 1] = ("static void RLUA_PUSH_%s(lua_State *L, %s result)"):format(struct.name, struct.name)
    t[#t + 1] = "{"
    t[#t + 1] = ("    lua_createtable(L, 0, %d);"):format(#(struct.fields or {}))

    local sorted = {}
    for i, f in ipairs(struct.fields or {}) do
        sorted[#sorted + 1] = { field = f, index = i }
    end
    table.sort(sorted, function(a, b)
        local na = a.field.name:match("^m(%d+)$")
        local nb = b.field.name:match("^m(%d+)$")
        if na and nb then
            return tonumber(na) < tonumber(nb)
        end
        return a.index < b.index
    end)

    for _, entry in ipairs(sorted) do
        local field = entry.field
        local name, array_size = field.name:match("([%w_]+)%[(%d+)%]")
        if not name then
            local fNorm = Parser.normalizeType(field.type)
            local comment = field.comment and (" // %s"):format(field.comment) or ""
            t[#t + 1] = "    " .. Parser.getPushStatement(fNorm, field.type, "result." .. field.name) .. comment
            t[#t + 1] = ('    lua_setfield(L, -2, "%s");'):format(field.name)
        else
            if field.type == "char" then
                local comment = field.comment and (" // %s"):format(field.comment) or ""
                t[#t + 1] = ("    lua_pushstring(L, result.%s);%s"):format(name, comment)
                t[#t + 1] = ('    lua_setfield(L, -2, "%s");'):format(name)
            else
                t[#t + 1] = ("    lua_createtable(L, %s, 0);"):format(array_size)
                t[#t + 1] = ("    for (int i = 0; i < %s; i++) {"):format(array_size)
                local fNorm = Parser.normalizeType(field.type)
                t[#t + 1] = "        " .. Parser.getPushStatement(fNorm, field.type, "result." .. name .. "[i]")
                t[#t + 1] = "        lua_seti(L, -2, i + 1);"
                t[#t + 1] = "    }"
                t[#t + 1] = ('    lua_setfield(L, -2, "%s");'):format(name)
            end
        end
    end

    t[#t + 1] = "}"
    return table.concat(t, "\n")
end

-- render struct writeback (C struct -> Lua table)
function Parser.renderStructWriteback(struct, ast)
    local t = {}
    t[#t + 1] = ("static void RLUA_WRITEBACK_%s(lua_State *L, int index, %s val)"):format(struct.name, struct.name)
    t[#t + 1] = "{"
    t[#t + 1] = "    if (lua_istable(L, index)) {"

    local sorted = {}
    for i, f in ipairs(struct.fields or {}) do
        sorted[#sorted + 1] = { field = f, index = i }
    end
    table.sort(sorted, function(a, b)
        local na = a.field.name:match("^m(%d+)$")
        local nb = b.field.name:match("^m(%d+)$")
        if na and nb then
            return tonumber(na) < tonumber(nb)
        end
        return a.index < b.index
    end)

    for _, entry in ipairs(sorted) do
        local f = entry.field
        local fNorm = Parser.normalizeType(f.type)
        local comment = f.comment and (" // %s"):format(f.comment) or ""
        if PRIMITIVE_TYPES[fNorm] then
            t[#t + 1] = "        " .. Parser.getPushStatement(fNorm, f.type, "val." .. f.name) .. comment
            t[#t + 1] = ('        lua_setfield(L, index, "%s");'):format(f.name)
        elseif VALUE_STRUCTS[fNorm] then
            t[#t + 1] = ("        RLUA_PUSH_%s(L, val.%s);%s"):format(fNorm, f.name, comment)
            t[#t + 1] = ('        lua_setfield(L, index, "%s");'):format(f.name)
        elseif ast.type_map[f.type] then
            local resolved = ast.type_map[f.type]
            local rNorm = Parser.normalizeType(resolved)
            if PRIMITIVE_TYPES[rNorm] then
                t[#t + 1] = "        " .. Parser.getPushStatement(rNorm, resolved, "val." .. f.name) .. comment
                t[#t + 1] = ('        lua_setfield(L, index, "%s");'):format(f.name)
            end
        end
    end
    t[#t + 1] = "    }"
    t[#t + 1] = "}"
    return table.concat(t, "\n")
end

-- render function registry (luaL_Reg array)
function Parser.renderFunctionRegistry(ast)
    local function_names = {}
    for _, f in ipairs(ast.function_xs) do
        function_names[f.name] = true
    end

    local t = {
        "// raylib functions list",
        "static const struct luaL_Reg raylib_functions[] = {",
    }
    for _, f in ipairs(ast.function_xs) do
        if not f.isCallback then
            t[#t + 1] = ('    {"%s", rl_%s},'):format(f.name, f.name)
        end
    end
    for _, d in ipairs(ast.define_xs) do
        if d.category == "alias" and function_names[d.value] then
            t[#t + 1] = ('    {"%s", rl_%s},'):format(d.name, d.value)
        end
    end
    t[#t + 1] = ""
    t[#t + 1] = "    { NULL, NULL }  // sentinel"
    t[#t + 1] = "};"
    t[#t + 1] = ""
    return table.concat(t, "\n")
end

-- render constants registry
function Parser.renderDefineRegistry(ast)
    local function_names = {}
    for _, f in ipairs(ast.function_xs) do
        function_names[f.name] = true
    end

    local t = { "static void rLuaRegisterConstants(lua_State *L)", "{" }

    -- enum constants
    for _, enum in ipairs(ast.enum_xs) do
        for _, field in ipairs(enum.fields) do
            local v = field.value
            if type(v) == "string" then
                v = tonumber(v) or v
            end
            if type(v) == "number" then
                t[#t + 1] = ("    lua_pushinteger(L, %d);"):format(v)
            else
                t[#t + 1] = ("    lua_pushinteger(L, %s);"):format(tostring(v))
            end
            t[#t + 1] = ('    lua_setfield(L, -2, "%s");'):format(field.name)
        end
    end

    -- #define constants
    for _, d in ipairs(ast.define_xs) do
        if d.category == "alias" and function_names[d.value] then
            goto continue_def
        end

        if d.category == "version" then
            goto continue_def
        end

        local pusher = "lua_pushinteger"
        if d.category == "float" then
            pusher = "lua_pushnumber"
        elseif d.category == "string" then
            pusher = "lua_pushstring"
        elseif d.category == "color" then
            pusher = "RLUA_PUSH_Color"
        end

        t[#t + 1] = ("    %s(L, %s);"):format(pusher, d.value)
        t[#t + 1] = ('    lua_setfield(L, -2, "%s");'):format(d.name)

        ::continue_def::
    end

    t[#t + 1] = "}"
    t[#t + 1] = ""
    return table.concat(t, "\n")
end

-- resource destructors (__gc)
function Parser.renderDestructors()
    local t = { "// --- Resource Destructors (__gc) ---", "" }
    local sorted = {}
    for name, func in pairs(RESOURCE_TYPES) do
        if func ~= "" then
            sorted[#sorted + 1] = { name = name, func = func }
        end
    end
    table.sort(sorted, function(a, b)
        return a.name < b.name
    end)

    for _, item in ipairs(sorted) do
        t[#t + 1] = ("static int rl_%s_gc(lua_State *L)"):format(item.name)
        t[#t + 1] = "{"
        t[#t + 1] = ('    RLUA_Handle *h = (RLUA_Handle *)luaL_checkudata(L, 1, "%s");'):format(item.name)
        t[#t + 1] = "    if (h->data && h->owned) {"
        t[#t + 1] = ("        %s(*(%s*)h->data);"):format(item.func, item.name)
        t[#t + 1] = "        RL_FREE(h->data);"
        t[#t + 1] = "    }"
        t[#t + 1] = "    return 0;"
        t[#t + 1] = "}"
        t[#t + 1] = ""
    end
    return table.concat(t, "\n")
end

-- resource indexers (__index)
function Parser.renderIndexers(ast)
    local structs_by_name = {}
    for _, s in ipairs(ast.struct_xs) do
        structs_by_name[s.name] = s
    end

    local t = { "// --- Resource Indexers (__index) ---", "" }
    local sorted = {}
    for name, _ in pairs(RESOURCE_TYPES) do
        sorted[#sorted + 1] = name
    end
    table.sort(sorted)

    for _, tname in ipairs(sorted) do
        local struct = structs_by_name[tname]
        if struct then
            t[#t + 1] = ("static int rl_%s_index(lua_State *L)"):format(tname)
            t[#t + 1] = "{"
            t[#t + 1] = ('    RLUA_Handle *h = (RLUA_Handle *)luaL_checkudata(L, 1, "%s");'):format(tname)
            t[#t + 1] = "    if (lua_isnumber(L, 2)) {"
            t[#t + 1] = "        int i = lua_tointeger(L, 2) - 1;"
            t[#t + 1] = '        if (i < 0 || i >= h->count) return luaL_error(L, "index out of bounds");'
            t[#t + 1] = ("        %s *ptr = &((%s *)h->data)[i];"):format(tname, tname)
            t[#t + 1] = ('        RLUA_PUSH_View(L, ptr, 1, "%s", false);'):format(tname)
            t[#t + 1] = "        return 1;"
            t[#t + 1] = "    }"
            t[#t + 1] = "    const char *key = luaL_checkstring(L, 2);"
            t[#t + 1] = ("    %s *data = (%s *)h->data;"):format(tname, tname)

            for _, field in ipairs(struct.fields or {}) do
                if not field.name:match("%[") then
                    local comment = field.comment and (" // %s"):format(field.comment) or ""
                    t[#t + 1] = ('    if (strcmp(key, "%s") == 0) {%s'):format(field.name, comment)
                    local fNorm = Parser.normalizeType(field.type)
                    if VALUE_STRUCTS[fNorm] then
                        t[#t + 1] = ("        RLUA_PUSH_%s(L, data->%s);"):format(field.type, field.name)
                    elseif RESOURCE_TYPES[fNorm] then
                        t[#t + 1] = ('        RLUA_PUSH_View(L, &data->%s, 1, "%s", false);'):format(field.name, fNorm)
                    else
                        t[#t + 1] = "        " .. Parser.getPushStatement(fNorm, field.type, "data->" .. field.name)
                    end
                    t[#t + 1] = "        return 1;"
                    t[#t + 1] = "    }"
                end
            end
            t[#t + 1] = "    return 0;"
            t[#t + 1] = "}"
            t[#t + 1] = ""
        end
    end
    return table.concat(t, "\n")
end

-- metatable registries
function Parser.renderMetatableRegistries(ast)
    local structs_by_name = {}
    for _, s in ipairs(ast.struct_xs) do
        structs_by_name[s.name] = s
    end

    local t = { "static void rLuaRegisterMetatables(lua_State *L)", "{" }
    local sorted = {}
    for name, _ in pairs(RESOURCE_TYPES) do
        sorted[#sorted + 1] = name
    end
    table.sort(sorted)

    for _, tname in ipairs(sorted) do
        t[#t + 1] = ('    luaL_newmetatable(L, "%s");'):format(tname)
        if structs_by_name[tname] then
            t[#t + 1] = ("    lua_pushcfunction(L, rl_%s_index);"):format(tname)
            t[#t + 1] = '    lua_setfield(L, -2, "__index");'
        end
        local func = RESOURCE_TYPES[tname]
        if func ~= "" then
            t[#t + 1] = ("    lua_pushcfunction(L, rl_%s_gc);"):format(tname)
            t[#t + 1] = '    lua_setfield(L, -2, "__gc");'
        end
        t[#t + 1] = "    lua_pop(L, 1);"
        t[#t + 1] = ""
    end

    t[#t + 1] = "}"
    return table.concat(t, "\n")
end

-- type alias marshallers (Camera -> Camera3D, Quaternion -> Vector4, etc.)
function Parser.renderTypeAliases(ast)
    local structs_by_name = {}
    for _, s in ipairs(ast.struct_xs) do
        structs_by_name[s.name] = s
    end

    local t = {}
    local sorted = {}
    for name in pairs(ast.type_map) do
        sorted[#sorted + 1] = name
    end
    table.sort(sorted)
    for _, aliasName in ipairs(sorted) do
        local resolvedType = ast.type_map[aliasName]
        if RESOURCE_TYPES[resolvedType] then
            t[#t + 1] = ('#define RLUA_CHECK_%s(L, idx) (*(%s*)RLUA_CHECK_Resource(L, idx, "%s"))'):format(
                aliasName,
                aliasName,
                resolvedType
            )
            t[#t + 1] = ('#define RLUA_PUSH_%s(L, val) RLUA_PUSH_Resource(L, &val, sizeof(%s), "%s")'):format(
                aliasName,
                aliasName,
                resolvedType
            )
        elseif structs_by_name[resolvedType] then
            t[#t + 1] = ("#define RLUA_CHECK_%s RLUA_CHECK_%s"):format(aliasName, resolvedType)
            t[#t + 1] = ("#define RLUA_PUSH_%s RLUA_PUSH_%s"):format(aliasName, resolvedType)
            if ast.mutated_types[aliasName] or ast.mutated_types[resolvedType] then
                t[#t + 1] = ("#define RLUA_WRITEBACK_%s RLUA_WRITEBACK_%s"):format(aliasName, resolvedType)
            end
        elseif resolvedType:match("%*$") then
            local base = resolvedType:gsub("%s*%*$", "")
            if structs_by_name[base] then
                t[#t + 1] = ('#define RLUA_CHECK_%s(L, idx) (%s)RLUA_CHECK_Resource(L, idx, "%s")'):format(
                    aliasName,
                    aliasName,
                    base
                )
                t[#t + 1] = ('#define RLUA_PUSH_%s(L, val) RLUA_PUSH_View(L, val, 1, "%s", false)'):format(
                    aliasName,
                    base
                )
            end
        end
    end
    return table.concat(t, "\n")
end

function Parser.emit_c(ast, out)
    local sections = {}

    -- header (boilerplate before implementation)
    sections[#sections + 1] = string.format(
        -- c
        [[
/**********************************************************************************************
*
*   raylib-lua v6.0 - raylib Lua bindings for raylib v6.0
*
*   AUTO-GENERATED by tools/rLuaParser/rluaparser.lua
*
*   Parsed: %d functions, %d structs, %d enums, %d defines, %d aliases
*
*   NOTES:
*
*   The following types are treated as Lua tables with named fields, same as in C:
*       Matrix, Vector2, Vector3, Vector4, Color, Rectangle, Ray, Camera, BoundingBox
*
*   The following types are opaque userdata with field access and automatic memory management (__gc):
*       Image, Texture2D, RenderTexture2D, Mesh, Model, Shader, Font, Sound, Music, Wave
*
*   Remember that ALL raylib types have REFERENCE SEMANTICS in Lua.
*   Tables (value types) are passed to C by copying fields, but multiple references
*   on the Lua side point to the same table object.
*
*   Some raylib functions take pointers to objects to modify (e.g. UpdateCamera(), etc.)
*   For table-based types like Camera, the binding automatically writes modified fields
*   back to the original Lua table. For resource types like Image, changes are made
*   directly to the memory block.
*
*   CONTRIBUTORS:
*       Ghassan Al-Mashareqa (ghassan@ghassan.pl): Original binding creation (for raylib 1.3)
*       Ramon Santamaria (@raysan5): Review, update and maintenance
*       yilisharcs: Modernization and automatic generator (for raylib 6.0)
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2015-2017 Ghassan Al-Mashareqa and Ramon Santamaria (@raysan5)
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
**********************************************************************************************/

#pragma once

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef RLUA_STATIC
    #define RLUADEF static            // Functions just visible to module including this file
#else
    #ifdef __cplusplus
        #define RLUADEF extern "C"    // Functions visible from other files (no name mangling of functions in C++)
    #else
        #define RLUADEF extern        // Functions visible from other files
    #endif
#endif

RLUADEF lua_State *rlua_open(void);
RLUADEF void rlua_close(lua_State *L);
]],
        #ast.function_xs,
        #ast.struct_xs,
        #ast.enum_xs,
        #ast.define_xs,
        #ast.alias_xs
    )

    sections[#sections + 1] =
        -- c
        [[
#ifdef RLUA_IMPLEMENTATION
#include "raylib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>     // Required for: va_list - Only used by TraceLogCallback
#include <pthread.h>

// --- Global State ---
static lua_State *RLUA_State = NULL;
static int RLUA_LogRef = LUA_REFNIL;
static pthread_mutex_t RLUA_LogMutex = PTHREAD_MUTEX_INITIALIZER;

// --- Marshalling Helpers ---

typedef struct {
    void *data;
    int count;
    const char *tname;
    bool owned;
} RLUA_Handle;

static void* RLUA_CHECK_Resource(lua_State *L, int index, const char *tname) {
    RLUA_Handle *h = (RLUA_Handle *)luaL_checkudata(L, index, tname);
    return h->data;
}

static void RLUA_PUSH_Resource(lua_State *L, void *data, size_t size, const char *tname) {
    RLUA_Handle *h = (RLUA_Handle *)lua_newuserdata(L, sizeof(RLUA_Handle));
    h->data = RL_MALLOC(size);
    memcpy(h->data, data, size);
    h->count = 1;
    h->tname = tname;
    h->owned = true;
    luaL_setmetatable(L, tname);
}

static void RLUA_PUSH_View(lua_State *L, const void *data, int count, const char *tname, bool owned) {
    RLUA_Handle *h = (RLUA_Handle *)lua_newuserdata(L, sizeof(RLUA_Handle));
    h->data = (void *)data;
    h->count = count;
    h->tname = tname;
    h->owned = owned;
    luaL_setmetatable(L, tname);
}

// --- Callback Trampolines ---
static void RLUA_TraceLogTrampoline(int logLevel, const char *text, va_list args) {
    if (!RLUA_State || RLUA_LogRef == LUA_REFNIL) return;
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);
    pthread_mutex_lock(&RLUA_LogMutex);
    lua_State *L = RLUA_State;
    lua_rawgeti(L, LUA_REGISTRYINDEX, RLUA_LogRef);
    lua_pushinteger(L, logLevel);
    lua_pushstring(L, buffer);
    if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
        TraceLog(LOG_ERROR, "LUA: TraceLogCallback: %s", lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    pthread_mutex_unlock(&RLUA_LogMutex);
}
]]

    -- type aliases
    local aliases = Parser.renderTypeAliases(ast)
    if #aliases > 0 then
        sections[#sections + 1] = "// --- Type Aliases ---\n"
        sections[#sections + 1] = aliases
        sections[#sections + 1] = "\n"
    end

    -- generated struct marshallers
    sections[#sections + 1] = "// --- Generated Marshallers ---\n"
    for _, s in ipairs(ast.struct_xs) do
        if VALUE_STRUCTS[s.name] then
            sections[#sections + 1] = Parser.renderStructCheck(s)
            sections[#sections + 1] = Parser.renderStructPush(s)
            if ast.mutated_types[s.name] then
                sections[#sections + 1] = Parser.renderStructWriteback(s, ast)
            end
            sections[#sections + 1] = "\n"
        end
    end

    -- destructors
    sections[#sections + 1] = Parser.renderDestructors()

    -- indexers
    sections[#sections + 1] = Parser.renderIndexers(ast)

    -- function wrappers
    sections[#sections + 1] = "// --- Wrappers ---\n"
    for _, f in ipairs(ast.function_xs) do
        sections[#sections + 1] = Parser.renderFunction(f)
    end

    -- registries
    sections[#sections + 1] = "// --- Registries ---\n"
    sections[#sections + 1] = Parser.renderMetatableRegistries(ast)
    sections[#sections + 1] = Parser.renderDefineRegistry(ast)
    sections[#sections + 1] = Parser.renderFunctionRegistry(ast)

    -- rlua_open / rlua_close
    sections[#sections + 1] =
        -- c
        [[
RLUADEF lua_State *rlua_open(void) {
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    RLUA_State = L;
    rLuaRegisterMetatables(L);

#ifdef RAYLIB_STRIP_PREFIX
    // Register functions as globals
    lua_pushglobaltable(L);
    luaL_setfuncs(L, raylib_functions, 0);
    lua_pop(L, 1);

    // Register constants as globals
    lua_pushglobaltable(L);
    rLuaRegisterConstants(L);
    lua_pop(L, 1);

    // rl table mirrors globals via __index = _G
    lua_newtable(L);
    lua_newtable(L);
    lua_pushglobaltable(L);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "rl");
#else
    lua_newtable(L);
    rLuaRegisterConstants(L);
    luaL_setfuncs(L, raylib_functions, 0);
    lua_setglobal(L, "rl");
#endif

    return L;
}

RLUADEF void rlua_close(lua_State *L) {
    if (RLUA_LogRef != LUA_REFNIL) {
        luaL_unref(L, LUA_REGISTRYINDEX, RLUA_LogRef);
        RLUA_LogRef = LUA_REFNIL;
    }
    RLUA_State = NULL;
    lua_close(L);
}

#endif
]]

    out:write(table.concat(sections, "\n"))
end

-- }}}

-- LUA EMITTER ================================================================================= {{{

function Parser.cToLuaType(c_type, ast)
    if not c_type or c_type == "" then
        return "any"
    end
    local t = c_type:gsub("%s*const%s*", " "):match("^%s*(.-)%s*$")
    if t == "const char *" or t == "const char*" or t == "char *" or t == "char*" then
        return "string"
    end
    if t == "bool" then
        return "boolean"
    end
    if
        t == "int"
        or t == "unsigned int"
        or t == "char"
        or t == "unsigned char"
        or t == "short"
        or t == "unsigned short"
        or t == "long"
        or t == "unsigned long"
        or t == "unsigned"
        or t:match("^RL_")
    then
        return "integer"
    end
    if t == "float" or t == "double" then
        return "number"
    end
    if t:match("%*$") or t:match("%* ") then
        return "userdata"
    end
    local resolved = ast.type_map and ast.type_map[t]
    if resolved then
        return Parser.cToLuaType(resolved, ast)
    end
    return ("rl.%s"):format(t)
end

function Parser.cToLuaTypename(c_type, ast)
    if not c_type or c_type == "" or c_type:match("^void$") then
        return "nil"
    end
    return Parser.cToLuaType(c_type, ast)
end

function Parser.emit_lua(ast, out)
    local t = {}

    local funcs_by_name = {}
    for _, f in ipairs(ast.function_xs) do
        funcs_by_name[f.name] = f
    end

    local function render_lua_func(f, name)
        for _, note in ipairs(f.notes or {}) do
            if not note:match("^%s*[-=]+%s*$") then
                t[#t + 1] = ("--- %s"):format(note)
            end
        end
        if f.comment then
            t[#t + 1] = ("--- %s"):format(f.comment)
        end

        local isVariadic = false
        for _, p in ipairs(f.params or {}) do
            if p.isVariadic then
                isVariadic = true
                break
            end
        end

        for _, p in ipairs(f.params or {}) do
            if not p.isVariadic then
                local pname = p.name == "end" and "end_" or p.name
                t[#t + 1] = ("---@param %s %s"):format(pname, Parser.cToLuaTypename(p.type, ast))
            end
        end
        if isVariadic then
            t[#t + 1] = "---@param ... any"
        end
        local ret_type = Parser.cToLuaTypename(f.retType, ast)
        if ret_type ~= "nil" then
            t[#t + 1] = ("---@return %s"):format(ret_type)
        end
        local pnames = {}
        for _, p in ipairs(f.params or {}) do
            if p.isVariadic then
                pnames[#pnames + 1] = "..."
            else
                local pname = p.name == "end" and "end_" or p.name
                pnames[#pnames + 1] = pname
            end
        end
        t[#t + 1] = ("function rl.%s(%s) end"):format(name, table.concat(pnames, ", "))
        t[#t + 1] = ""
    end

    t[#t + 1] = "---@meta _"
    t[#t + 1] = "--[[ **********************************************************************************************"
    t[#t + 1] = ""
    t[#t + 1] = "    raylib-lua v6.0 - raylib Lua type definitions for LuaLS"
    t[#t + 1] = ""
    t[#t + 1] = "    AUTO-GENERATED by tools/rLuaParser/rluaparser.lua"
    t[#t + 1] = ""
    t[#t + 1] = "    LICENSE: zlib/libpng"
    t[#t + 1] = ""
    t[#t + 1] = "    Copyright (c) 2026 yilisharcs"
    t[#t + 1] = ""
    t[#t + 1] = "************************************************************************************************ ]]"
    t[#t + 1] = ""
    t[#t + 1] = 'error("Cannot require a meta file")'
    t[#t + 1] = ""

    -- struct class definitions
    local sorted_structs = {}
    for _, s in ipairs(ast.struct_xs) do
        sorted_structs[#sorted_structs + 1] = s
    end
    table.sort(sorted_structs, function(a, b)
        return a.name < b.name
    end)

    for _, s in ipairs(sorted_structs) do
        for _, note in ipairs(s.notes or {}) do
            if not note:match("^%s*[-=]+%s*$") then
                t[#t + 1] = ("--- %s"):format(note)
            end
        end
        if s.comment then
            t[#t + 1] = ("--- %s"):format(s.comment)
        end
        t[#t + 1] = ("---@class rl.%s"):format(s.name)
        local sorted_fields = {}
        for i, f in ipairs(s.fields or {}) do
            sorted_fields[#sorted_fields + 1] = { field = f, index = i }
        end
        table.sort(sorted_fields, function(a, b)
            local na = a.field.name:match("^m(%d+)$")
            local nb = b.field.name:match("^m(%d+)$")
            if na and nb then
                return tonumber(na) < tonumber(nb)
            end
            return a.index < b.index
        end)
        for _, entry in ipairs(sorted_fields) do
            local f = entry.field
            if f.comment then
                t[#t + 1] = ("--- %s"):format(f.comment)
            end
            local baseName, arraySize = f.name:match("([%w_]+)%[(%d+)%]")
            if baseName then
                if f.type == "char" then
                    t[#t + 1] = ("---@field %s string"):format(baseName)
                else
                    local entries = {}
                    local fType = Parser.cToLuaType(f.type, ast)
                    for i = 1, tonumber(arraySize) do
                        entries[#entries + 1] = ("[%d]: %s"):format(i, fType)
                    end
                    t[#t + 1] = ("---@field %s { %s }"):format(baseName, table.concat(entries, ", "))
                end
            else
                t[#t + 1] = ("---@field %s %s"):format(f.name, Parser.cToLuaType(f.type, ast))
            end
        end
        t[#t + 1] = ""
    end

    -- module table declaration
    t[#t + 1] = "---@class (partial) rl"
    t[#t + 1] = "rl = {}"
    t[#t + 1] = ""

    -- enum type aliases
    for _, enum in ipairs(ast.enum_xs) do
        t[#t + 1] = ("---@alias rl.%s integer"):format(enum.name)
    end
    t[#t + 1] = ""

    -- enum constants
    for _, enum in ipairs(ast.enum_xs) do
        for _, field in ipairs(enum.fields) do
            if field.comment then
                t[#t + 1] = ("--- %s"):format(field.comment)
            end
            t[#t + 1] = ("---@type rl.%s"):format(enum.name)
            t[#t + 1] = ("rl.%s = %s"):format(field.name, field.value or "nil")
        end
    end

    -- define constants
    for _, d in ipairs(ast.define_xs) do
        if d.category ~= "version" then
            if d.category == "alias" and funcs_by_name[d.value] then
                render_lua_func(funcs_by_name[d.value], d.name)
            else
                if d.comment then
                    t[#t + 1] = ("--- %s"):format(d.comment)
                end
                local def_type = "integer"
                local val = "nil"
                if d.category == "float" then
                    def_type = "number"
                    val = d.value:gsub("([%d.])f", "%1"):gsub("PI", "rl.PI")
                elseif d.category == "string" then
                    def_type = "string"
                    val = d.value
                elseif d.category == "color" then
                    def_type = "rl.Color"
                    val = "nil"
                elseif d.category == "alias" then
                    val = "nil"
                elseif d.category == "integer" then
                    val = d.value
                end
                t[#t + 1] = ("---@type %s"):format(def_type)
                t[#t + 1] = ("rl.%s = %s"):format(d.name, val)
            end
        end
    end

    t[#t + 1] = ""

    -- function declarations
    for _, f in ipairs(ast.function_xs) do
        render_lua_func(f, f.name)
    end

    out:write(table.concat(t, "\n"))
end

-- }}}

-- EXECUTION ==============================================================================================

local input_f, c_out_f, lua_out_f = ...

if not input_f or not c_out_f then
    io.stderr:write("Usage: rluaparser.lua <path/to/raylib.h> <output.h> [output.lua]\n")
    os.exit(1)
end

local rl <close> = assert(io.open(input_f, "r"))
local content = rl:read("*a")

local major = tonumber(content:match("#define%s+RAYLIB_VERSION_MAJOR%s+(%d+)"))
local minor = tonumber(content:match("#define%s+RAYLIB_VERSION_MINOR%s+(%d+)"))

local EXPECTED_MAJOR = 6
local EXPECTED_MINOR = 0

-- manually verify that version changes didn't introduce any bugs in the parser!
if major ~= EXPECTED_MAJOR or minor ~= EXPECTED_MINOR then
    io.stderr:write(
        ("ERROR: Version mismatch.\nExpected raylib %d.%d, found %s.%s\n"):format(
            EXPECTED_MAJOR,
            EXPECTED_MINOR,
            major or "unknown",
            minor or "unknown"
        )
    )
    os.exit(1)
end

local lines = Parser.preprocess(content)
local ast = Parser.scan(lines)
Parser.analyze(ast)

-- io.stderr:write(("functions: %d\n"):format(#ast.function_xs))
-- io.stderr:write(("structs:   %d\n"):format(#ast.struct_xs))
-- io.stderr:write(("enums:     %d\n"):format(#ast.enum_xs))
-- io.stderr:write(("defines:   %d\n"):format(#ast.define_xs))
-- io.stderr:write(("aliases:   %d\n"):format(#ast.alias_xs))

local c_out <close> = assert(io.open(c_out_f, "w"))
Parser.emit_c(ast, c_out)

if lua_out_f then
    local lua_out <close> = assert(io.open(lua_out_f, "w"))
    Parser.emit_lua(ast, lua_out)
end
