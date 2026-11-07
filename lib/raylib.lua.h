// SPDX-FileCopyrightText: 2026 yilisharcs
// SPDX-License-Identifier: Zlib

#ifndef RAYLIB_LUA_H
#define RAYLIB_LUA_H

#include "raylib.h"
#include "rcamera.h"
#include "lauxlib.h"
// #include "lua.h"     // already included
// #include "lualib.h"  // does not belong here

void rLuaRegister(lua_State *L);
void rLuaLock(void);
void rLuaUnlock(void);
void rLuaShutdown(void);

#ifdef RLUA_IMPLEMENTATION

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>

static __thread int rlua_audio_in_callback;

// boy oh boy, i sure love repetition
#define REG(push, name) (push, lua_setfield(L, -2, (name)))

static void rluaSetOwnership(lua_State *L, int index, int ownership)
{
    if (index < 0) index = lua_gettop(L) + index + 1;
    lua_pushinteger(L, ownership);
    lua_setiuservalue(L, index, 1);
}

static int rluaGetOwnership(lua_State *L, int index)
{
    lua_getiuservalue(L, index, 1);
    int ownership = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return ownership;
}

static void rluaTypeError(lua_State *L, const char *expected, int index)
{
    const char *actual = luaL_typename(L, index);

    if (luaL_getmetafield(L, index, "__name") == LUA_TSTRING)
    {
        actual = lua_tostring(L, -1);
    }

    luaL_error(L, "%s expected, got %s", expected, actual);
}

// raymath's functions are exposed as metamethods
#define RAYMATH_STATIC_INLINE
// #include <math.h>    // already included
#include "raymath.h"

static const char rlua_rendertexture_mt[] = "raylib.RenderTexture";

// [HELPERS] must be forward-declared
static Vector2 *rluaVector2ArrayCheck(lua_State *L, int index, int *count);
static Vector3 *rluaVector3ArrayCheck(lua_State *L, int index, int *count);
static Matrix *rluaMatrixArrayCheck(lua_State *L, int index, int *count);
static float *rluaFloatArrayCheck(lua_State *L, int index, int *count);
static int *rluaIntArrayCheck(lua_State *L, int index, int *count);
static int rluaPushStringArray(lua_State *L, char **values, int count);
static int rluaPushIntArray(lua_State *L, const int *values, int count);
static int rluaPushUIntArray(lua_State *L, const unsigned int *values, int count);
static int rluaPushColorArray(lua_State *L, const Color *colors, int count);

// Matrix and Quaternion are defined after some of their consumers
static int rluaMatrixIs(lua_State *L, int index);
static Matrix rluaMatrixCheck(lua_State *L, int index);
static void rluaMatrixPush(lua_State *L, Matrix m);
static Quaternion rluaQuaternionCheck(lua_State *L, int index);
static Texture *rluaTextureCheck(lua_State *L, int index);
static Font *rluaFont_CheckValid(lua_State *L, int index);
static Camera2D rluaCamera2DCheck(lua_State *L, int index);
static Camera3D rluaCamera3DCheck(lua_State *L, int index);
static void rluaCamera3D_SetFields(lua_State *L, int index, Camera3D camera);
static Ray rluaRayCheck(lua_State *L, int index);

// Type methods are resolved by __index before their definitions
static int rluaIsTextureValid(lua_State *L);
static int rluaUpdateTexture(lua_State *L);
static int rluaUpdateTextureRec(lua_State *L);
static int rluaGenTextureMipmaps(lua_State *L);
static int rluaSetTextureFilter(lua_State *L);
static int rluaSetTextureWrap(lua_State *L);
static int rluaDrawTexture(lua_State *L);
static int rluaDrawTextureV(lua_State *L);
static int rluaDrawTextureEx(lua_State *L);
static int rluaDrawTextureRec(lua_State *L);
static int rluaDrawTexturePro(lua_State *L);
static int rluaDrawTextureNPatch(lua_State *L);
static int rluaIsRenderTextureValid(lua_State *L);
static int rluaIsFontValid(lua_State *L);
static int rluaExportFontAsCode(lua_State *L);
static int rluaDrawTextEx(lua_State *L);
static int rluaDrawTextPro(lua_State *L);
static int rluaDrawTextCodepoint(lua_State *L);
static int rluaDrawTextCodepoints(lua_State *L);
static int rluaMeasureTextEx(lua_State *L);
static int rluaMeasureTextCodepoints(lua_State *L);
static int rluaGetGlyphIndex(lua_State *L);
static int rluaGetGlyphInfo(lua_State *L);
static int rluaGetGlyphAtlasRec(lua_State *L);
static int rluaIsShaderValid(lua_State *L);
static int rluaGetShaderLocation(lua_State *L);
static int rluaGetShaderLocationAttrib(lua_State *L);
static int rluaSetShaderValue(lua_State *L);
static int rluaSetShaderValueV(lua_State *L);
static int rluaSetShaderValueMatrix(lua_State *L);
static int rluaSetShaderValueTexture(lua_State *L);
static int rluaUploadMesh(lua_State *L);
static int rluaUpdateMeshBuffer(lua_State *L);
static int rluaDrawMesh(lua_State *L);
static int rluaDrawMeshInstanced(lua_State *L);
static int rluaGetMeshBoundingBox(lua_State *L);
static int rluaGenMeshTangents(lua_State *L);
static int rluaExportMesh(lua_State *L);
static int rluaExportMeshAsCode(lua_State *L);
static int rluaIsModelValid(lua_State *L);
static int rluaGetModelBoundingBox(lua_State *L);
static int rluaDrawModel(lua_State *L);
static int rluaDrawModelEx(lua_State *L);
static int rluaDrawModelWires(lua_State *L);
static int rluaDrawModelWiresEx(lua_State *L);
static int rluaSetModelMeshMaterial(lua_State *L);
static int rluaUpdateModelAnimation(lua_State *L);
static int rluaUpdateModelAnimationEx(lua_State *L);
static int rluaIsModelAnimationValid(lua_State *L);
static int rluaIsMaterialValid(lua_State *L);
static int rluaSetMaterialTexture(lua_State *L);
static int rluaIsWaveValid(lua_State *L);
static int rluaWaveCopy(lua_State *L);
static int rluaWaveCrop(lua_State *L);
static int rluaWaveFormat(lua_State *L);
static int rluaExportWave(lua_State *L);
static int rluaExportWaveAsCode(lua_State *L);
static int rluaLoadWaveSamples(lua_State *L);
static int rluaIsSoundValid(lua_State *L);
static int rluaUpdateSound(lua_State *L);
static int rluaPlaySound(lua_State *L);
static int rluaStopSound(lua_State *L);
static int rluaPauseSound(lua_State *L);
static int rluaResumeSound(lua_State *L);
static int rluaIsSoundPlaying(lua_State *L);
static int rluaSetSoundVolume(lua_State *L);
static int rluaSetSoundPitch(lua_State *L);
static int rluaSetSoundPan(lua_State *L);
static int rluaIsMusicValid(lua_State *L);
static int rluaPlayMusicStream(lua_State *L);
static int rluaIsMusicStreamPlaying(lua_State *L);
static int rluaUpdateMusicStream(lua_State *L);
static int rluaStopMusicStream(lua_State *L);
static int rluaPauseMusicStream(lua_State *L);
static int rluaResumeMusicStream(lua_State *L);
static int rluaSeekMusicStream(lua_State *L);
static int rluaSetMusicVolume(lua_State *L);
static int rluaSetMusicPitch(lua_State *L);
static int rluaSetMusicPan(lua_State *L);
static int rluaGetMusicTimeLength(lua_State *L);
static int rluaGetMusicTimePlayed(lua_State *L);
static int rluaIsAudioStreamValid(lua_State *L);
static int rluaUpdateAudioStream(lua_State *L);
static int rluaIsAudioStreamProcessed(lua_State *L);
static int rluaPlayAudioStream(lua_State *L);
static int rluaPauseAudioStream(lua_State *L);
static int rluaResumeAudioStream(lua_State *L);
static int rluaIsAudioStreamPlaying(lua_State *L);
static int rluaStopAudioStream(lua_State *L);
static int rluaSetAudioStreamVolume(lua_State *L);
static int rluaSetAudioStreamPitch(lua_State *L);
static int rluaSetAudioStreamPan(lua_State *L);
static int rluaSetAudioStreamCallback(lua_State *L);
static int rluaAttachAudioStreamProcessor(lua_State *L);
static int rluaDetachAudioStreamProcessor(lua_State *L);
static int rluaGetCameraMatrix(lua_State *L);
static int rluaUpdateCamera(lua_State *L);
static int rluaUpdateCameraPro(lua_State *L);
static int rluaDrawBillboard(lua_State *L);
static int rluaDrawBillboardRec(lua_State *L);
static int rluaDrawBillboardPro(lua_State *L);
static int rluaExportAutomationEventList(lua_State *L);

// Image methods are resolved by rluaImageIndex before their definitions
static int rluaIsImageValid(lua_State *L);
static int rluaExportImage(lua_State *L);
static int rluaExportImageToMemory(lua_State *L);
static int rluaExportImageAsCode(lua_State *L);
static int rluaImageCopy(lua_State *L);
static int rluaImageFromImage(lua_State *L);
static int rluaImageFromChannel(lua_State *L);
static int rluaImageText(lua_State *L);
static int rluaImageTextEx(lua_State *L);
static int rluaImageFormat(lua_State *L);
static int rluaImageToPOT(lua_State *L);
static int rluaImageCrop(lua_State *L);
static int rluaImageAlphaCrop(lua_State *L);
static int rluaImageAlphaClear(lua_State *L);
static int rluaImageAlphaMask(lua_State *L);
static int rluaImageAlphaPremultiply(lua_State *L);
static int rluaImageBlurGaussian(lua_State *L);
static int rluaImageKernelConvolution(lua_State *L);
static int rluaImageResize(lua_State *L);
static int rluaImageResizeNN(lua_State *L);
static int rluaImageResizeCanvas(lua_State *L);
static int rluaImageMipmaps(lua_State *L);
static int rluaImageDither(lua_State *L);
static int rluaImageFlipVertical(lua_State *L);
static int rluaImageFlipHorizontal(lua_State *L);
static int rluaImageRotate(lua_State *L);
static int rluaImageRotateCW(lua_State *L);
static int rluaImageRotateCCW(lua_State *L);
static int rluaImageColorTint(lua_State *L);
static int rluaImageColorInvert(lua_State *L);
static int rluaImageColorGrayscale(lua_State *L);
static int rluaImageColorContrast(lua_State *L);
static int rluaImageColorBrightness(lua_State *L);
static int rluaImageColorReplace(lua_State *L);
static int rluaLoadImageColors(lua_State *L);
static int rluaLoadImagePalette(lua_State *L);
static int rluaGetImageAlphaBorder(lua_State *L);
static int rluaGetImageColor(lua_State *L);
static int rluaImageClearBackground(lua_State *L);
static int rluaImageDrawPixel(lua_State *L);
static int rluaImageDrawPixelV(lua_State *L);
static int rluaImageDrawLine(lua_State *L);
static int rluaImageDrawLineV(lua_State *L);
static int rluaImageDrawLineEx(lua_State *L);
static int rluaImageDrawLineStrip(lua_State *L);
static int rluaImageDrawTriangle(lua_State *L);
static int rluaImageDrawTriangleGradient(lua_State *L);
static int rluaImageDrawTriangleLines(lua_State *L);
static int rluaImageDrawTriangleFan(lua_State *L);
static int rluaImageDrawTriangleStrip(lua_State *L);
static int rluaImageDrawRectangle(lua_State *L);
static int rluaImageDrawRectangleV(lua_State *L);
static int rluaImageDrawRectangleRec(lua_State *L);
static int rluaImageDrawRectanglePro(lua_State *L);
static int rluaImageDrawRectangleLines(lua_State *L);
static int rluaImageDrawRectangleLinesEx(lua_State *L);
static int rluaImageDrawRectangleGradientEx(lua_State *L);
static int rluaImageDrawCircle(lua_State *L);
static int rluaImageDrawCircleV(lua_State *L);
static int rluaImageDrawCircleLines(lua_State *L);
static int rluaImageDrawCircleLinesV(lua_State *L);
static int rluaImageDrawCircleGradient(lua_State *L);
static int rluaImageDrawImage(lua_State *L);
static int rluaImageDrawImageEx(lua_State *L);
static int rluaImageDrawImageRec(lua_State *L);
static int rluaImageDrawImagePro(lua_State *L);
static int rluaImageDrawText(lua_State *L);
static int rluaImageDrawTextEx(lua_State *L);
static int rluaImageDrawTextPro(lua_State *L);
static int rluaSetWindowIcon(lua_State *L);
static void rluaRayPush(lua_State *L, Ray ray);

// raymath utils {{{

static int rluaClamp(lua_State *L)
{
    float value = (float)luaL_checknumber(L, 1);
    float min = (float)luaL_checknumber(L, 2);
    float max = (float)luaL_checknumber(L, 3);
    lua_pushnumber(L, Clamp(value, min, max));
    return 1;
}

static int rluaLerp(lua_State *L)
{
    float start = (float)luaL_checknumber(L, 1);
    float end = (float)luaL_checknumber(L, 2);
    float amount = (float)luaL_checknumber(L, 3);
    lua_pushnumber(L, Lerp(start, end, amount));
    return 1;
}

static int rluaNormalize(lua_State *L)
{
    float value = (float)luaL_checknumber(L, 1);
    float start = (float)luaL_checknumber(L, 2);
    float end = (float)luaL_checknumber(L, 3);
    lua_pushnumber(L, Normalize(value, start, end));
    return 1;
}

static int rluaRemap(lua_State *L)
{
    float value = (float)luaL_checknumber(L, 1);
    float inputStart = (float)luaL_checknumber(L, 2);
    float inputEnd = (float)luaL_checknumber(L, 3);
    float outputStart = (float)luaL_checknumber(L, 4);
    float outputEnd = (float)luaL_checknumber(L, 5);
    lua_pushnumber(L, Remap(value, inputStart, inputEnd, outputStart, outputEnd));
    return 1;
}

static int rluaWrap(lua_State *L)
{
    float value = (float)luaL_checknumber(L, 1);
    float min = (float)luaL_checknumber(L, 2);
    float max = (float)luaL_checknumber(L, 3);
    lua_pushnumber(L, Wrap(value, min, max));
    return 1;
}

static int rluaFloatEquals(lua_State *L)
{
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    lua_pushboolean(L, FloatEquals(x, y));
    return 1;
}

static void rluaRaymathRegister(lua_State *L)
{
    REG(lua_pushcfunction(L, rluaClamp), "Clamp");
    REG(lua_pushcfunction(L, rluaLerp), "Lerp");
    REG(lua_pushcfunction(L, rluaNormalize), "Normalize");
    REG(lua_pushcfunction(L, rluaRemap), "Remap");
    REG(lua_pushcfunction(L, rluaWrap), "Wrap");
    REG(lua_pushcfunction(L, rluaFloatEquals), "FloatEquals");
}

// }}}

// Vector2, 2 components {{{

static const char rlua_vector2_mt[] = "raylib.Vector2";

static int rluaVector2Is(lua_State *L, int index)
{
    if (lua_type(L, index) != LUA_TTABLE) return 0;
    if (!lua_getmetatable(L, index)) return 0;

    luaL_getmetatable(L, rlua_vector2_mt);
    int equal = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);

    return equal;
}

static Vector2 rluaVector2Check(lua_State *L, int index)
{
    if (!rluaVector2Is(L, index))
    {
        rluaTypeError(L, "Vector2", index);
    }

    Vector2 v;

    lua_getfield(L, index, "x");
    v.x = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "y");
    v.y = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return v;
}

static void rluaVector2Push(lua_State *L, Vector2 v)
{
    lua_createtable(L, 0, 2);

    REG(lua_pushnumber(L, v.x), "x");
    REG(lua_pushnumber(L, v.y), "y");

    luaL_setmetatable(L, rlua_vector2_mt);
}

static int rluaVector2(lua_State *L)
{
    Vector2 v = {
        .x = (float)luaL_checknumber(L, 1),
        .y = (float)luaL_checknumber(L, 2),
    };
    rluaVector2Push(L, v);
    return 1;
}

// metamethods {{{

static int rluaVector2ToString(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    lua_pushfstring(L, "Vector2(%f, %f)", v.x, v.y);
    return 1;
}

static Vector2 rluaVector2_CoerceOperand(lua_State *L, int index)
{
    if (lua_type(L, index) == LUA_TNUMBER)
    {
        float n = (float)luaL_checknumber(L, index);
        return (Vector2) { n, n };
    }
    return rluaVector2Check(L, index);
}

static int rluaVector2Add(lua_State *L)
{
    Vector2 v1 = rluaVector2_CoerceOperand(L, 1);
    Vector2 v2 = rluaVector2_CoerceOperand(L, 2);
    rluaVector2Push(L, Vector2Add(v1, v2));
    return 1;
}

static int rluaVector2Subtract(lua_State *L)
{
    Vector2 v1 = rluaVector2_CoerceOperand(L, 1);
    Vector2 v2 = rluaVector2_CoerceOperand(L, 2);
    rluaVector2Push(L, Vector2Subtract(v1, v2));
    return 1;
}

static int rluaVector2Multiply(lua_State *L)
{
    if (rluaMatrixIs(L, 2))
    {
        return luaL_error(L, "attempt to mul a 'Vector2' with a 'Matrix'; use Matrix * Vector2 instead");
    }

    Vector2 v1 = rluaVector2_CoerceOperand(L, 1);
    Vector2 v2 = rluaVector2_CoerceOperand(L, 2);
    rluaVector2Push(L, Vector2Multiply(v1, v2));
    return 1;
}

static int rluaVector2Divide(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    rluaVector2Push(L, Vector2Divide(v1, v2));
    return 1;
}

static int rluaVector2Negate(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    rluaVector2Push(L, Vector2Negate(v));
    return 1;
}

static int rluaVector2Equals(lua_State *L)
{
    if (!rluaVector2Is(L, 1) || !rluaVector2Is(L, 2))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    Vector2 p = rluaVector2Check(L, 1);
    Vector2 q = rluaVector2Check(L, 2);
    lua_pushboolean(L, Vector2Equals(p, q));
    return 1;
}

// }}}

// methods {{{

static int rluaVector2Length(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    lua_pushnumber(L, Vector2Length(v));
    return 1;
}

static int rluaVector2LengthSqr(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    lua_pushnumber(L, Vector2LengthSqr(v));
    return 1;
}

static int rluaVector2DotProduct(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    lua_pushnumber(L, Vector2DotProduct(v1, v2));
    return 1;
}

static int rluaVector2CrossProduct(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    lua_pushnumber(L, Vector2CrossProduct(v1, v2));
    return 1;
}

static int rluaVector2Distance(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    lua_pushnumber(L, Vector2Distance(v1, v2));
    return 1;
}

static int rluaVector2DistanceSqr(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    lua_pushnumber(L, Vector2DistanceSqr(v1, v2));
    return 1;
}

static int rluaVector2Angle(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    lua_pushnumber(L, Vector2Angle(v1, v2));
    return 1;
}

static int rluaVector2LineAngle(lua_State *L)
{
    Vector2 start = rluaVector2Check(L, 1);
    Vector2 end = rluaVector2Check(L, 2);
    lua_pushnumber(L, Vector2LineAngle(start, end));
    return 1;
}

static int rluaVector2Normalize(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    rluaVector2Push(L, Vector2Normalize(v));
    return 1;
}

static int rluaVector2Transform(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    Matrix m = rluaMatrixCheck(L, 2);
    rluaVector2Push(L, Vector2Transform(v, m));
    return 1;
}

static int rluaVector2Lerp(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    float amount = (float)luaL_checknumber(L, 3);
    rluaVector2Push(L, Vector2Lerp(v1, v2, amount));
    return 1;
}

static int rluaVector2Reflect(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    Vector2 normal = rluaVector2Check(L, 2);
    rluaVector2Push(L, Vector2Reflect(v, normal));
    return 1;
}

static int rluaVector2Min(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    rluaVector2Push(L, Vector2Min(v1, v2));
    return 1;
}

static int rluaVector2Max(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    rluaVector2Push(L, Vector2Max(v1, v2));
    return 1;
}

static int rluaVector2Rotate(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    float angle = (float)luaL_checknumber(L, 2);
    rluaVector2Push(L, Vector2Rotate(v, angle));
    return 1;
}

static int rluaVector2MoveTowards(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    Vector2 target = rluaVector2Check(L, 2);
    float maxDistance = (float)luaL_checknumber(L, 3);
    rluaVector2Push(L, Vector2MoveTowards(v, target, maxDistance));
    return 1;
}

static int rluaVector2Invert(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    rluaVector2Push(L, Vector2Invert(v));
    return 1;
}

static int rluaVector2Clamp(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    Vector2 min = rluaVector2Check(L, 2);
    Vector2 max = rluaVector2Check(L, 3);
    rluaVector2Push(L, Vector2Clamp(v, min, max));
    return 1;
}

static int rluaVector2ClampValue(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    float min = (float)luaL_checknumber(L, 2);
    float max = (float)luaL_checknumber(L, 3);
    rluaVector2Push(L, Vector2ClampValue(v, min, max));
    return 1;
}

static int rluaVector2Refract(lua_State *L)
{
    Vector2 v = rluaVector2Check(L, 1);
    Vector2 n = rluaVector2Check(L, 2);
    float r = (float)luaL_checknumber(L, 3);
    rluaVector2Push(L, Vector2Refract(v, n, r));
    return 1;
}

// }}}

static void rluaVector2Register(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_vector2_mt))
    {
        REG(lua_pushcfunction(L, rluaVector2ToString), "__tostring");
        REG(lua_pushcfunction(L, rluaVector2Add), "__add");
        REG(lua_pushcfunction(L, rluaVector2Subtract), "__sub");
        REG(lua_pushcfunction(L, rluaVector2Multiply), "__mul");
        REG(lua_pushcfunction(L, rluaVector2Divide), "__div");
        REG(lua_pushcfunction(L, rluaVector2Negate), "__unm");
        REG(lua_pushcfunction(L, rluaVector2Equals), "__eq");

        lua_newtable(L); // M.__index

        REG(lua_pushcfunction(L, rluaVector2Length), "length");
        REG(lua_pushcfunction(L, rluaVector2LengthSqr), "lengthSqr");
        REG(lua_pushcfunction(L, rluaVector2DotProduct), "dotProduct");
        REG(lua_pushcfunction(L, rluaVector2CrossProduct), "crossProduct");
        REG(lua_pushcfunction(L, rluaVector2Distance), "distance");
        REG(lua_pushcfunction(L, rluaVector2DistanceSqr), "distanceSqr");
        REG(lua_pushcfunction(L, rluaVector2Angle), "angle");
        REG(lua_pushcfunction(L, rluaVector2LineAngle), "lineAngle");
        REG(lua_pushcfunction(L, rluaVector2Normalize), "normalize");
        REG(lua_pushcfunction(L, rluaVector2Transform), "transform");
        REG(lua_pushcfunction(L, rluaVector2Lerp), "lerp");
        REG(lua_pushcfunction(L, rluaVector2Reflect), "reflect");
        REG(lua_pushcfunction(L, rluaVector2Min), "min");
        REG(lua_pushcfunction(L, rluaVector2Max), "max");
        REG(lua_pushcfunction(L, rluaVector2Rotate), "rotate");
        REG(lua_pushcfunction(L, rluaVector2MoveTowards), "moveTowards");
        REG(lua_pushcfunction(L, rluaVector2Invert), "invert");
        REG(lua_pushcfunction(L, rluaVector2Clamp), "clamp");
        REG(lua_pushcfunction(L, rluaVector2ClampValue), "clampValue");
        REG(lua_pushcfunction(L, rluaVector2Refract), "refract");

        lua_setfield(L, -2, "__index"); // M.__index
    }

    lua_pushstring(L, "Vector2");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaVector2), "Vector2");
}

// }}}

// Vector3, 3 components {{{

static const char rlua_vector3_mt[] = "raylib.Vector3";

static int rluaVector3Is(lua_State *L, int index)
{
    if (lua_type(L, index) != LUA_TTABLE) return 0;
    if (!lua_getmetatable(L, index)) return 0;

    luaL_getmetatable(L, rlua_vector3_mt);
    int equal = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);

    return equal;
}

static Vector3 rluaVector3Check(lua_State *L, int index)
{
    if (!rluaVector3Is(L, index))
    {
        rluaTypeError(L, "Vector3", index);
    }

    Vector3 v;

    lua_getfield(L, index, "x");
    v.x = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "y");
    v.y = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "z");
    v.z = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return v;
}

static void rluaVector3Push(lua_State *L, Vector3 v)
{
    lua_createtable(L, 0, 3);

    REG(lua_pushnumber(L, v.x), "x");
    REG(lua_pushnumber(L, v.y), "y");
    REG(lua_pushnumber(L, v.z), "z");

    luaL_setmetatable(L, rlua_vector3_mt);
}

static int rluaVector3(lua_State *L)
{
    Vector3 v = {
        .x = (float)luaL_checknumber(L, 1),
        .y = (float)luaL_checknumber(L, 2),
        .z = (float)luaL_checknumber(L, 3),
    };
    rluaVector3Push(L, v);
    return 1;
}

// metamethods {{{

static int rluaVector3ToString(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    lua_pushfstring(L, "Vector3(%f, %f, %f)", v.x, v.y, v.z);
    return 1;
}

static Vector3 rluaVector3_CoerceOperand(lua_State *L, int index)
{
    if (lua_type(L, index) == LUA_TNUMBER)
    {
        float n = (float)luaL_checknumber(L, index);
        return (Vector3) { n, n, n };
    }
    return rluaVector3Check(L, index);
}

static int rluaVector3Add(lua_State *L)
{
    Vector3 v1 = rluaVector3_CoerceOperand(L, 1);
    Vector3 v2 = rluaVector3_CoerceOperand(L, 2);
    rluaVector3Push(L, Vector3Add(v1, v2));
    return 1;
}

static int rluaVector3Subtract(lua_State *L)
{
    Vector3 v1 = rluaVector3_CoerceOperand(L, 1);
    Vector3 v2 = rluaVector3_CoerceOperand(L, 2);
    rluaVector3Push(L, Vector3Subtract(v1, v2));
    return 1;
}

static int rluaVector3Multiply(lua_State *L)
{
    if (rluaMatrixIs(L, 2))
    {
        return luaL_error(L, "attempt to mul a 'Vector3' with a 'Matrix'; use Matrix * Vector3 instead");
    }

    Vector3 v1 = rluaVector3_CoerceOperand(L, 1);
    Vector3 v2 = rluaVector3_CoerceOperand(L, 2);
    rluaVector3Push(L, Vector3Multiply(v1, v2));
    return 1;
}

static int rluaVector3Divide(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    rluaVector3Push(L, Vector3Divide(v1, v2));
    return 1;
}

static int rluaVector3Negate(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    rluaVector3Push(L, Vector3Negate(v));
    return 1;
}

static int rluaVector3Equals(lua_State *L)
{
    if (!rluaVector3Is(L, 1) || !rluaVector3Is(L, 2))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    Vector3 p = rluaVector3Check(L, 1);
    Vector3 q = rluaVector3Check(L, 2);
    lua_pushboolean(L, Vector3Equals(p, q));
    return 1;
}

// }}}

// methods {{{

static void rluaVector3_SetFields(lua_State *L, int index, Vector3 v)
{
    index = lua_absindex(L, index);

    lua_pushnumber(L, v.x);
    lua_setfield(L, index, "x");

    lua_pushnumber(L, v.y);
    lua_setfield(L, index, "y");

    lua_pushnumber(L, v.z);
    lua_setfield(L, index, "z");
}

static int rluaVector3CrossProduct(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    rluaVector3Push(L, Vector3CrossProduct(v1, v2));
    return 1;
}

static int rluaVector3Perpendicular(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    rluaVector3Push(L, Vector3Perpendicular(v));
    return 1;
}

static int rluaVector3Length(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    lua_pushnumber(L, Vector3Length(v));
    return 1;
}

static int rluaVector3LengthSqr(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    lua_pushnumber(L, Vector3LengthSqr(v));
    return 1;
}

static int rluaVector3DotProduct(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    lua_pushnumber(L, Vector3DotProduct(v1, v2));
    return 1;
}

static int rluaVector3Distance(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    lua_pushnumber(L, Vector3Distance(v1, v2));
    return 1;
}

static int rluaVector3DistanceSqr(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    lua_pushnumber(L, Vector3DistanceSqr(v1, v2));
    return 1;
}

static int rluaVector3Angle(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    lua_pushnumber(L, Vector3Angle(v1, v2));
    return 1;
}

static int rluaVector3Normalize(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    rluaVector3Push(L, Vector3Normalize(v));
    return 1;
}

static int rluaVector3Project(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    rluaVector3Push(L, Vector3Project(v1, v2));
    return 1;
}

static int rluaVector3Reject(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    rluaVector3Push(L, Vector3Reject(v1, v2));
    return 1;
}

static int rluaVector3OrthoNormalize(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    Vector3OrthoNormalize(&v1, &v2);
    rluaVector3_SetFields(L, 1, v1);
    rluaVector3_SetFields(L, 2, v2);
    return 0;
}

static int rluaVector3Transform(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    Matrix m = rluaMatrixCheck(L, 2);
    rluaVector3Push(L, Vector3Transform(v, m));
    return 1;
}

static int rluaVector3RotateByQuaternion(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    Quaternion q = rluaQuaternionCheck(L, 2);
    rluaVector3Push(L, Vector3RotateByQuaternion(v, q));
    return 1;
}

static int rluaVector3RotateByAxisAngle(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    Vector3 axis = rluaVector3Check(L, 2);
    float angle = (float)luaL_checknumber(L, 3);
    rluaVector3Push(L, Vector3RotateByAxisAngle(v, axis, angle));
    return 1;
}

static int rluaVector3MoveTowards(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    Vector3 target = rluaVector3Check(L, 2);
    float maxDistance = (float)luaL_checknumber(L, 3);
    rluaVector3Push(L, Vector3MoveTowards(v, target, maxDistance));
    return 1;
}

static int rluaVector3Lerp(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    float amount = (float)luaL_checknumber(L, 3);
    rluaVector3Push(L, Vector3Lerp(v1, v2, amount));
    return 1;
}

static int rluaVector3CubicHermite(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 tangent1 = rluaVector3Check(L, 2);
    Vector3 v2 = rluaVector3Check(L, 3);
    Vector3 tangent2 = rluaVector3Check(L, 4);
    float amount = (float)luaL_checknumber(L, 5);
    rluaVector3Push(L, Vector3CubicHermite(v1, tangent1, v2, tangent2, amount));
    return 1;
}

static int rluaVector3Reflect(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    Vector3 normal = rluaVector3Check(L, 2);
    rluaVector3Push(L, Vector3Reflect(v, normal));
    return 1;
}

static int rluaVector3Min(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    rluaVector3Push(L, Vector3Min(v1, v2));
    return 1;
}

static int rluaVector3Max(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    rluaVector3Push(L, Vector3Max(v1, v2));
    return 1;
}

static int rluaVector3Barycenter(lua_State *L)
{
    Vector3 p = rluaVector3Check(L, 1);
    Vector3 a = rluaVector3Check(L, 2);
    Vector3 b = rluaVector3Check(L, 3);
    Vector3 c = rluaVector3Check(L, 4);
    rluaVector3Push(L, Vector3Barycenter(p, a, b, c));
    return 1;
}

static int rluaVector3Unproject(lua_State *L)
{
    Vector3 source = rluaVector3Check(L, 1);
    Matrix projection = rluaMatrixCheck(L, 2);
    Matrix view = rluaMatrixCheck(L, 3);
    rluaVector3Push(L, Vector3Unproject(source, projection, view));
    return 1;
}

static int rluaVector3Invert(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    rluaVector3Push(L, Vector3Invert(v));
    return 1;
}

static int rluaVector3Clamp(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    Vector3 min = rluaVector3Check(L, 2);
    Vector3 max = rluaVector3Check(L, 3);
    rluaVector3Push(L, Vector3Clamp(v, min, max));
    return 1;
}

static int rluaVector3ClampValue(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    float min = (float)luaL_checknumber(L, 2);
    float max = (float)luaL_checknumber(L, 3);
    rluaVector3Push(L, Vector3ClampValue(v, min, max));
    return 1;
}

static int rluaVector3Refract(lua_State *L)
{
    Vector3 v = rluaVector3Check(L, 1);
    Vector3 n = rluaVector3Check(L, 2);
    float r = (float)luaL_checknumber(L, 3);
    rluaVector3Push(L, Vector3Refract(v, n, r));
    return 1;
}

// }}}

static void rluaVector3Register(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_vector3_mt))
    {
        REG(lua_pushcfunction(L, rluaVector3ToString), "__tostring");
        REG(lua_pushcfunction(L, rluaVector3Add), "__add");
        REG(lua_pushcfunction(L, rluaVector3Subtract), "__sub");
        REG(lua_pushcfunction(L, rluaVector3Multiply), "__mul");
        REG(lua_pushcfunction(L, rluaVector3Divide), "__div");
        REG(lua_pushcfunction(L, rluaVector3Negate), "__unm");
        REG(lua_pushcfunction(L, rluaVector3Equals), "__eq");

        lua_newtable(L); // M.__index

        REG(lua_pushcfunction(L, rluaVector3CrossProduct), "crossProduct");
        REG(lua_pushcfunction(L, rluaVector3Perpendicular), "perpendicular");
        REG(lua_pushcfunction(L, rluaVector3Length), "length");
        REG(lua_pushcfunction(L, rluaVector3LengthSqr), "lengthSqr");
        REG(lua_pushcfunction(L, rluaVector3DotProduct), "dotProduct");
        REG(lua_pushcfunction(L, rluaVector3Distance), "distance");
        REG(lua_pushcfunction(L, rluaVector3DistanceSqr), "distanceSqr");
        REG(lua_pushcfunction(L, rluaVector3Angle), "angle");
        REG(lua_pushcfunction(L, rluaVector3Normalize), "normalize");
        REG(lua_pushcfunction(L, rluaVector3Project), "project");
        REG(lua_pushcfunction(L, rluaVector3Reject), "reject");
        REG(lua_pushcfunction(L, rluaVector3OrthoNormalize), "orthoNormalize");
        REG(lua_pushcfunction(L, rluaVector3Transform), "transform");
        REG(lua_pushcfunction(L, rluaVector3RotateByQuaternion), "rotateByQuaternion");
        REG(lua_pushcfunction(L, rluaVector3RotateByAxisAngle), "rotateByAxisAngle");
        REG(lua_pushcfunction(L, rluaVector3MoveTowards), "moveTowards");
        REG(lua_pushcfunction(L, rluaVector3Lerp), "lerp");
        REG(lua_pushcfunction(L, rluaVector3CubicHermite), "cubicHermite");
        REG(lua_pushcfunction(L, rluaVector3Reflect), "reflect");
        REG(lua_pushcfunction(L, rluaVector3Min), "min");
        REG(lua_pushcfunction(L, rluaVector3Max), "max");
        REG(lua_pushcfunction(L, rluaVector3Barycenter), "barycenter");
        REG(lua_pushcfunction(L, rluaVector3Unproject), "unproject");
        REG(lua_pushcfunction(L, rluaVector3Invert), "invert");
        REG(lua_pushcfunction(L, rluaVector3Clamp), "clamp");
        REG(lua_pushcfunction(L, rluaVector3ClampValue), "clampValue");
        REG(lua_pushcfunction(L, rluaVector3Refract), "refract");

        lua_setfield(L, -2, "__index"); // M.__index
    }

    lua_pushstring(L, "Vector3");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaVector3), "Vector3");
}

// }}}

// Vector4, 4 components {{{

static const char rlua_vector4_mt[] = "raylib.Vector4";

static int rluaVector4Is(lua_State *L, int index)
{
    if (lua_type(L, index) != LUA_TTABLE) return 0;
    if (!lua_getmetatable(L, index)) return 0;

    luaL_getmetatable(L, rlua_vector4_mt);
    int equal = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);

    return equal;
}

static Vector4 rluaVector4Check(lua_State *L, int index)
{
    if (!rluaVector4Is(L, index))
    {
        rluaTypeError(L, "Vector4", index);
    }

    Vector4 v;

    lua_getfield(L, index, "x");
    v.x = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "y");
    v.y = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "z");
    v.z = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "w");
    v.w = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return v;
}

static void rluaVector4Push(lua_State *L, Vector4 v)
{
    lua_createtable(L, 0, 4);

    REG(lua_pushnumber(L, v.x), "x");
    REG(lua_pushnumber(L, v.y), "y");
    REG(lua_pushnumber(L, v.z), "z");
    REG(lua_pushnumber(L, v.w), "w");

    luaL_setmetatable(L, rlua_vector4_mt);
}

static int rluaVector4(lua_State *L)
{
    Vector4 v = {
        .x = (float)luaL_checknumber(L, 1),
        .y = (float)luaL_checknumber(L, 2),
        .z = (float)luaL_checknumber(L, 3),
        .w = (float)luaL_checknumber(L, 4),
    };
    rluaVector4Push(L, v);
    return 1;
}

// metamethods {{{

static int rluaVector4ToString(lua_State *L)
{
    Vector4 v = rluaVector4Check(L, 1);
    lua_pushfstring(L, "Vector4(%f, %f, %f, %f)", v.x, v.y, v.z, v.w);
    return 1;
}

static Vector4 rluaVector4_CoerceOperand(lua_State *L, int index)
{
    if (lua_type(L, index) == LUA_TNUMBER)
    {
        float n = (float)luaL_checknumber(L, index);
        return (Vector4) { n, n, n, n };
    }
    return rluaVector4Check(L, index);
}

static int rluaVector4Add(lua_State *L)
{
    Vector4 v1 = rluaVector4_CoerceOperand(L, 1);
    Vector4 v2 = rluaVector4_CoerceOperand(L, 2);
    rluaVector4Push(L, Vector4Add(v1, v2));
    return 1;
}

static int rluaVector4Subtract(lua_State *L)
{
    Vector4 v1 = rluaVector4_CoerceOperand(L, 1);
    Vector4 v2 = rluaVector4_CoerceOperand(L, 2);
    rluaVector4Push(L, Vector4Subtract(v1, v2));
    return 1;
}

static int rluaVector4Multiply(lua_State *L)
{
    if (rluaMatrixIs(L, 2))
    {
        return luaL_error(L, "attempt to perform unsupported mul between a 'Vector4' and a 'Matrix'");
    }

    Vector4 v1 = rluaVector4_CoerceOperand(L, 1);
    Vector4 v2 = rluaVector4_CoerceOperand(L, 2);
    rluaVector4Push(L, Vector4Multiply(v1, v2));
    return 1;
}

static int rluaVector4Divide(lua_State *L)
{
    Vector4 v1 = rluaVector4Check(L, 1);
    Vector4 v2 = rluaVector4Check(L, 2);
    rluaVector4Push(L, Vector4Divide(v1, v2));
    return 1;
}

static int rluaVector4Negate(lua_State *L)
{
    Vector4 v = rluaVector4Check(L, 1);
    rluaVector4Push(L, Vector4Negate(v));
    return 1;
}

static int rluaVector4Equals(lua_State *L)
{
    if (!rluaVector4Is(L, 1) || !rluaVector4Is(L, 2))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    Vector4 p = rluaVector4Check(L, 1);
    Vector4 q = rluaVector4Check(L, 2);
    lua_pushboolean(L, Vector4Equals(p, q));
    return 1;
}

// }}}

// methods {{{

static int rluaVector4Length(lua_State *L)
{
    Vector4 v = rluaVector4Check(L, 1);
    lua_pushnumber(L, Vector4Length(v));
    return 1;
}

static int rluaVector4LengthSqr(lua_State *L)
{
    Vector4 v = rluaVector4Check(L, 1);
    lua_pushnumber(L, Vector4LengthSqr(v));
    return 1;
}

static int rluaVector4DotProduct(lua_State *L)
{
    Vector4 v1 = rluaVector4Check(L, 1);
    Vector4 v2 = rluaVector4Check(L, 2);
    lua_pushnumber(L, Vector4DotProduct(v1, v2));
    return 1;
}

static int rluaVector4Distance(lua_State *L)
{
    Vector4 v1 = rluaVector4Check(L, 1);
    Vector4 v2 = rluaVector4Check(L, 2);
    lua_pushnumber(L, Vector4Distance(v1, v2));
    return 1;
}

static int rluaVector4DistanceSqr(lua_State *L)
{
    Vector4 v1 = rluaVector4Check(L, 1);
    Vector4 v2 = rluaVector4Check(L, 2);
    lua_pushnumber(L, Vector4DistanceSqr(v1, v2));
    return 1;
}

static int rluaVector4Normalize(lua_State *L)
{
    Vector4 v = rluaVector4Check(L, 1);
    rluaVector4Push(L, Vector4Normalize(v));
    return 1;
}

static int rluaVector4Min(lua_State *L)
{
    Vector4 v1 = rluaVector4Check(L, 1);
    Vector4 v2 = rluaVector4Check(L, 2);
    rluaVector4Push(L, Vector4Min(v1, v2));
    return 1;
}

static int rluaVector4Max(lua_State *L)
{
    Vector4 v1 = rluaVector4Check(L, 1);
    Vector4 v2 = rluaVector4Check(L, 2);
    rluaVector4Push(L, Vector4Max(v1, v2));
    return 1;
}

static int rluaVector4Lerp(lua_State *L)
{
    Vector4 v1 = rluaVector4Check(L, 1);
    Vector4 v2 = rluaVector4Check(L, 2);
    float amount = (float)luaL_checknumber(L, 3);
    rluaVector4Push(L, Vector4Lerp(v1, v2, amount));
    return 1;
}

static int rluaVector4MoveTowards(lua_State *L)
{
    Vector4 v = rluaVector4Check(L, 1);
    Vector4 target = rluaVector4Check(L, 2);
    float maxDistance = (float)luaL_checknumber(L, 3);
    rluaVector4Push(L, Vector4MoveTowards(v, target, maxDistance));
    return 1;
}

static int rluaVector4Invert(lua_State *L)
{
    Vector4 v = rluaVector4Check(L, 1);
    rluaVector4Push(L, Vector4Invert(v));
    return 1;
}

// }}}

static void rluaVector4Register(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_vector4_mt))
    {
        REG(lua_pushcfunction(L, rluaVector4ToString), "__tostring");
        REG(lua_pushcfunction(L, rluaVector4Add), "__add");
        REG(lua_pushcfunction(L, rluaVector4Subtract), "__sub");
        REG(lua_pushcfunction(L, rluaVector4Multiply), "__mul");
        REG(lua_pushcfunction(L, rluaVector4Divide), "__div");
        REG(lua_pushcfunction(L, rluaVector4Negate), "__unm");
        REG(lua_pushcfunction(L, rluaVector4Equals), "__eq");

        lua_newtable(L); // M.__index

        REG(lua_pushcfunction(L, rluaVector4Length), "length");
        REG(lua_pushcfunction(L, rluaVector4LengthSqr), "lengthSqr");
        REG(lua_pushcfunction(L, rluaVector4DotProduct), "dotProduct");
        REG(lua_pushcfunction(L, rluaVector4Distance), "distance");
        REG(lua_pushcfunction(L, rluaVector4DistanceSqr), "distanceSqr");
        REG(lua_pushcfunction(L, rluaVector4Normalize), "normalize");
        REG(lua_pushcfunction(L, rluaVector4Min), "min");
        REG(lua_pushcfunction(L, rluaVector4Max), "max");
        REG(lua_pushcfunction(L, rluaVector4Lerp), "lerp");
        REG(lua_pushcfunction(L, rluaVector4MoveTowards), "moveTowards");
        REG(lua_pushcfunction(L, rluaVector4Invert), "invert");

        lua_setfield(L, -2, "__index"); // M.__index
    }

    lua_pushstring(L, "Vector4");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaVector4), "Vector4");
}

// }}}

// Quaternion, 4 components {{{

static const char rlua_quaternion_mt[] = "raylib.Quaternion";

static int rluaQuaternionIs(lua_State *L, int index)
{
    if (lua_type(L, index) != LUA_TTABLE) return 0;
    if (!lua_getmetatable(L, index)) return 0;

    luaL_getmetatable(L, rlua_quaternion_mt);
    int equal = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);

    return equal;
}

static Quaternion rluaQuaternionCheck(lua_State *L, int index)
{
    if (!rluaQuaternionIs(L, index))
    {
        rluaTypeError(L, "Quaternion", index);
    }

    Quaternion q;

    lua_getfield(L, index, "x");
    q.x = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "y");
    q.y = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "z");
    q.z = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "w");
    q.w = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return q;
}

static void rluaQuaternionPush(lua_State *L, Quaternion q)
{
    lua_createtable(L, 0, 4);

    REG(lua_pushnumber(L, q.x), "x");
    REG(lua_pushnumber(L, q.y), "y");
    REG(lua_pushnumber(L, q.z), "z");
    REG(lua_pushnumber(L, q.w), "w");

    luaL_setmetatable(L, rlua_quaternion_mt);
}

static int rluaQuaternion(lua_State *L)
{
    Quaternion q = {
        .x = (float)luaL_checknumber(L, 2),
        .y = (float)luaL_checknumber(L, 3),
        .z = (float)luaL_checknumber(L, 4),
        .w = (float)luaL_checknumber(L, 5),
    };
    rluaQuaternionPush(L, q);
    return 1;
}

// factories {{{

static int rluaQuaternionIdentity(lua_State *L)
{
    rluaQuaternionPush(L, QuaternionIdentity());
    return 1;
}

static int rluaQuaternionFromAxisAngle(lua_State *L)
{
    Vector3 axis = rluaVector3Check(L, 1);
    float angle = (float)luaL_checknumber(L, 2);
    rluaQuaternionPush(L, QuaternionFromAxisAngle(axis, angle));
    return 1;
}

static int rluaQuaternionFromEuler(lua_State *L)
{
    float pitch = (float)luaL_checknumber(L, 1);
    float yaw = (float)luaL_checknumber(L, 2);
    float roll = (float)luaL_checknumber(L, 3);
    rluaQuaternionPush(L, QuaternionFromEuler(pitch, yaw, roll));
    return 1;
}

static int rluaQuaternionFromVector3ToVector3(lua_State *L)
{
    Vector3 from = rluaVector3Check(L, 1);
    Vector3 to = rluaVector3Check(L, 2);
    rluaQuaternionPush(L, QuaternionFromVector3ToVector3(from, to));
    return 1;
}

static int rluaQuaternionFromMatrix(lua_State *L)
{
    Matrix m = rluaMatrixCheck(L, 1);
    rluaQuaternionPush(L, QuaternionFromMatrix(m));
    return 1;
}

// }}}

// metamethods {{{

static int rluaQuaternionToString(lua_State *L)
{
    Quaternion q = rluaQuaternionCheck(L, 1);
    lua_pushfstring(L, "Quaternion(%f, %f, %f, %f)", q.x, q.y, q.z, q.w);
    return 1;
}

static Quaternion rluaQuaternion_CoerceOperand(lua_State *L, int index)
{
    if (lua_type(L, index) == LUA_TNUMBER)
    {
        float n = (float)luaL_checknumber(L, index);
        return (Quaternion) { n, n, n, n };
    }
    return rluaQuaternionCheck(L, index);
}

static int rluaQuaternionAdd(lua_State *L)
{
    Quaternion q1 = rluaQuaternion_CoerceOperand(L, 1);
    Quaternion q2 = rluaQuaternion_CoerceOperand(L, 2);
    rluaQuaternionPush(L, QuaternionAdd(q1, q2));
    return 1;
}

static int rluaQuaternionSubtract(lua_State *L)
{
    Quaternion q1 = rluaQuaternion_CoerceOperand(L, 1);
    Quaternion q2 = rluaQuaternion_CoerceOperand(L, 2);
    rluaQuaternionPush(L, QuaternionSubtract(q1, q2));
    return 1;
}

static int rluaQuaternionMultiply(lua_State *L)
{
    if (lua_type(L, 1) == LUA_TNUMBER)
    {
        float n = (float)luaL_checknumber(L, 1);
        Quaternion q = rluaQuaternionCheck(L, 2);
        rluaQuaternionPush(L, QuaternionScale(q, n));
        return 1;
    }

    if (lua_type(L, 2) == LUA_TNUMBER)
    {
        Quaternion q = rluaQuaternionCheck(L, 1);
        float n = (float)luaL_checknumber(L, 2);
        rluaQuaternionPush(L, QuaternionScale(q, n));
        return 1;
    }

    if (rluaMatrixIs(L, 2))
    {
        Quaternion q = rluaQuaternionCheck(L, 1);
        Matrix m = rluaMatrixCheck(L, 2);
        rluaQuaternionPush(L, QuaternionTransform(q, m));
        return 1;
    }

    Quaternion q1 = rluaQuaternionCheck(L, 1);
    Quaternion q2 = rluaQuaternionCheck(L, 2);
    rluaQuaternionPush(L, QuaternionMultiply(q1, q2));
    return 1;
}

static int rluaQuaternionDivide(lua_State *L)
{
    Quaternion q1 = rluaQuaternionCheck(L, 1);
    Quaternion q2 = rluaQuaternionCheck(L, 2);
    rluaQuaternionPush(L, QuaternionDivide(q1, q2));
    return 1;
}

static int rluaQuaternionEquals(lua_State *L)
{
    if (!rluaQuaternionIs(L, 1) || !rluaQuaternionIs(L, 2))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    Quaternion p = rluaQuaternionCheck(L, 1);
    Quaternion q = rluaQuaternionCheck(L, 2);
    lua_pushboolean(L, QuaternionEquals(p, q));
    return 1;
}

// }}}

// methods {{{

static int rluaQuaternionLength(lua_State *L)
{
    Quaternion q = rluaQuaternionCheck(L, 1);
    lua_pushnumber(L, QuaternionLength(q));
    return 1;
}

static int rluaQuaternionNormalize(lua_State *L)
{
    Quaternion q = rluaQuaternionCheck(L, 1);
    rluaQuaternionPush(L, QuaternionNormalize(q));
    return 1;
}

static int rluaQuaternionInvert(lua_State *L)
{
    Quaternion q = rluaQuaternionCheck(L, 1);
    rluaQuaternionPush(L, QuaternionInvert(q));
    return 1;
}

static int rluaQuaternionLerp(lua_State *L)
{
    Quaternion q1 = rluaQuaternionCheck(L, 1);
    Quaternion q2 = rluaQuaternionCheck(L, 2);
    float amount = (float)luaL_checknumber(L, 3);
    rluaQuaternionPush(L, QuaternionLerp(q1, q2, amount));
    return 1;
}

static int rluaQuaternionNlerp(lua_State *L)
{
    Quaternion q1 = rluaQuaternionCheck(L, 1);
    Quaternion q2 = rluaQuaternionCheck(L, 2);
    float amount = (float)luaL_checknumber(L, 3);
    rluaQuaternionPush(L, QuaternionNlerp(q1, q2, amount));
    return 1;
}

static int rluaQuaternionSlerp(lua_State *L)
{
    Quaternion q1 = rluaQuaternionCheck(L, 1);
    Quaternion q2 = rluaQuaternionCheck(L, 2);
    float amount = (float)luaL_checknumber(L, 3);
    rluaQuaternionPush(L, QuaternionSlerp(q1, q2, amount));
    return 1;
}

static int rluaQuaternionCubicHermiteSpline(lua_State *L)
{
    Quaternion q1 = rluaQuaternionCheck(L, 1);
    Quaternion outTangent1 = rluaQuaternionCheck(L, 2);
    Quaternion q2 = rluaQuaternionCheck(L, 3);
    Quaternion inTangent2 = rluaQuaternionCheck(L, 4);
    float t = (float)luaL_checknumber(L, 5);
    rluaQuaternionPush(L, QuaternionCubicHermiteSpline(q1, outTangent1, q2, inTangent2, t));
    return 1;
}

static int rluaQuaternionToAxisAngle(lua_State *L)
{
    Quaternion q = rluaQuaternionCheck(L, 1);
    Vector3 axis;
    float angle;
    QuaternionToAxisAngle(q, &axis, &angle);
    rluaVector3Push(L, axis);
    lua_pushnumber(L, angle);
    return 2;
}

static int rluaQuaternionToEuler(lua_State *L)
{
    Quaternion q = rluaQuaternionCheck(L, 1);
    rluaVector3Push(L, QuaternionToEuler(q));
    return 1;
}

static int rluaQuaternionToMatrix(lua_State *L)
{
    Quaternion q = rluaQuaternionCheck(L, 1);
    rluaMatrixPush(L, QuaternionToMatrix(q));
    return 1;
}

static int rluaQuaternionTransform(lua_State *L)
{
    Quaternion q = rluaQuaternionCheck(L, 1);
    Matrix m = rluaMatrixCheck(L, 2);
    rluaQuaternionPush(L, QuaternionTransform(q, m));
    return 1;
}

// }}}

static void rluaQuaternionRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_quaternion_mt))
    {
        REG(lua_pushcfunction(L, rluaQuaternionToString), "__tostring");
        REG(lua_pushcfunction(L, rluaQuaternionAdd), "__add");
        REG(lua_pushcfunction(L, rluaQuaternionSubtract), "__sub");
        REG(lua_pushcfunction(L, rluaQuaternionMultiply), "__mul");
        REG(lua_pushcfunction(L, rluaQuaternionDivide), "__div");
        REG(lua_pushcfunction(L, rluaQuaternionEquals), "__eq");

        lua_newtable(L); // M.__index

        REG(lua_pushcfunction(L, rluaQuaternionLength), "length");
        REG(lua_pushcfunction(L, rluaQuaternionNormalize), "normalize");
        REG(lua_pushcfunction(L, rluaQuaternionInvert), "invert");
        REG(lua_pushcfunction(L, rluaQuaternionLerp), "lerp");
        REG(lua_pushcfunction(L, rluaQuaternionNlerp), "nlerp");
        REG(lua_pushcfunction(L, rluaQuaternionSlerp), "slerp");
        REG(lua_pushcfunction(L, rluaQuaternionCubicHermiteSpline), "cubicHermiteSpline");
        REG(lua_pushcfunction(L, rluaQuaternionToMatrix), "toMatrix");
        REG(lua_pushcfunction(L, rluaQuaternionToAxisAngle), "toAxisAngle");
        REG(lua_pushcfunction(L, rluaQuaternionToEuler), "toEuler");
        REG(lua_pushcfunction(L, rluaQuaternionTransform), "transform");

        lua_setfield(L, -2, "__index"); // M.__index
    }

    lua_pushstring(L, "Quaternion");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    lua_newtable(L); // rl.Quaternion

    REG(lua_pushcfunction(L, rluaQuaternionIdentity), "identity");
    REG(lua_pushcfunction(L, rluaQuaternionFromVector3ToVector3), "fromVector3ToVector3");
    REG(lua_pushcfunction(L, rluaQuaternionFromMatrix), "fromMatrix");
    REG(lua_pushcfunction(L, rluaQuaternionFromAxisAngle), "fromAxisAngle");
    REG(lua_pushcfunction(L, rluaQuaternionFromEuler), "fromEuler");

    lua_newtable(L); // constructor metatable
    REG(lua_pushcfunction(L, rluaQuaternion), "__call");
    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "Quaternion");
}

// }}}

// Matrix, 16 components {{{

static const char rlua_matrix_mt[] = "raylib.Matrix";

static int rluaMatrixIs(lua_State *L, int index)
{
    if (lua_type(L, index) != LUA_TTABLE) return 0;
    if (!lua_getmetatable(L, index)) return 0;

    luaL_getmetatable(L, rlua_matrix_mt);
    int equal = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);

    return equal;
}

static Matrix rluaMatrixCheck(lua_State *L, int index)
{
    if (!rluaMatrixIs(L, index))
    {
        rluaTypeError(L, "Matrix", index);
    }

    Matrix m;

    lua_getfield(L, index, "m0");
    m.m0 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m1");
    m.m1 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m2");
    m.m2 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m3");
    m.m3 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m4");
    m.m4 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m5");
    m.m5 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m6");
    m.m6 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m7");
    m.m7 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m8");
    m.m8 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m9");
    m.m9 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m10");
    m.m10 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m11");
    m.m11 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m12");
    m.m12 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m13");
    m.m13 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m14");
    m.m14 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "m15");
    m.m15 = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return m;
}

static void rluaMatrixPush(lua_State *L, Matrix m)
{
    lua_createtable(L, 0, 16);

    REG(lua_pushnumber(L, m.m0), "m0");
    REG(lua_pushnumber(L, m.m1), "m1");
    REG(lua_pushnumber(L, m.m2), "m2");
    REG(lua_pushnumber(L, m.m3), "m3");
    REG(lua_pushnumber(L, m.m4), "m4");
    REG(lua_pushnumber(L, m.m5), "m5");
    REG(lua_pushnumber(L, m.m6), "m6");
    REG(lua_pushnumber(L, m.m7), "m7");
    REG(lua_pushnumber(L, m.m8), "m8");
    REG(lua_pushnumber(L, m.m9), "m9");
    REG(lua_pushnumber(L, m.m10), "m10");
    REG(lua_pushnumber(L, m.m11), "m11");
    REG(lua_pushnumber(L, m.m12), "m12");
    REG(lua_pushnumber(L, m.m13), "m13");
    REG(lua_pushnumber(L, m.m14), "m14");
    REG(lua_pushnumber(L, m.m15), "m15");

    luaL_setmetatable(L, rlua_matrix_mt);
}

static int rluaMatrix(lua_State *L)
{
    int n = lua_gettop(L) - 1;
    if (n != 16)
    {
        return luaL_error(L, "bad number of arguments to 'Matrix' (expected 16 numbers, got %d)", n);
    }

    Matrix m = {
        .m0 = (float)luaL_checknumber(L, 2),
        .m1 = (float)luaL_checknumber(L, 3),
        .m2 = (float)luaL_checknumber(L, 4),
        .m3 = (float)luaL_checknumber(L, 5),
        .m4 = (float)luaL_checknumber(L, 6),
        .m5 = (float)luaL_checknumber(L, 7),
        .m6 = (float)luaL_checknumber(L, 8),
        .m7 = (float)luaL_checknumber(L, 9),
        .m8 = (float)luaL_checknumber(L, 10),
        .m9 = (float)luaL_checknumber(L, 11),
        .m10 = (float)luaL_checknumber(L, 12),
        .m11 = (float)luaL_checknumber(L, 13),
        .m12 = (float)luaL_checknumber(L, 14),
        .m13 = (float)luaL_checknumber(L, 15),
        .m14 = (float)luaL_checknumber(L, 16),
        .m15 = (float)luaL_checknumber(L, 17),
    };
    rluaMatrixPush(L, m);
    return 1;
}

// factories {{{

static int rluaMatrixIdentity(lua_State *L)
{
    rluaMatrixPush(L, MatrixIdentity());
    return 1;
}

static int rluaMatrixTranslate(lua_State *L)
{
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float z = (float)luaL_checknumber(L, 3);
    rluaMatrixPush(L, MatrixTranslate(x, y, z));
    return 1;
}

static int rluaMatrixRotate(lua_State *L)
{
    Vector3 axis = rluaVector3Check(L, 1);
    float angle = (float)luaL_checknumber(L, 2);
    rluaMatrixPush(L, MatrixRotate(axis, angle));
    return 1;
}

static int rluaMatrixRotateX(lua_State *L)
{
    float angle = (float)luaL_checknumber(L, 1);
    rluaMatrixPush(L, MatrixRotateX(angle));
    return 1;
}

static int rluaMatrixRotateY(lua_State *L)
{
    float angle = (float)luaL_checknumber(L, 1);
    rluaMatrixPush(L, MatrixRotateY(angle));
    return 1;
}

static int rluaMatrixRotateZ(lua_State *L)
{
    float angle = (float)luaL_checknumber(L, 1);
    rluaMatrixPush(L, MatrixRotateZ(angle));
    return 1;
}

static int rluaMatrixRotateXYZ(lua_State *L)
{
    Vector3 angle = rluaVector3Check(L, 1);
    rluaMatrixPush(L, MatrixRotateXYZ(angle));
    return 1;
}

static int rluaMatrixRotateZYX(lua_State *L)
{
    Vector3 angle = rluaVector3Check(L, 1);
    rluaMatrixPush(L, MatrixRotateZYX(angle));
    return 1;
}

static int rluaMatrixScale(lua_State *L)
{
    float x = (float)luaL_checknumber(L, 1);
    float y = (float)luaL_checknumber(L, 2);
    float z = (float)luaL_checknumber(L, 3);
    rluaMatrixPush(L, MatrixScale(x, y, z));
    return 1;
}

static int rluaMatrixFrustum(lua_State *L)
{
    double left = luaL_checknumber(L, 1);
    double right = luaL_checknumber(L, 2);
    double bottom = luaL_checknumber(L, 3);
    double top = luaL_checknumber(L, 4);
    double nearPlane = luaL_checknumber(L, 5);
    double farPlane = luaL_checknumber(L, 6);
    rluaMatrixPush(L, MatrixFrustum(left, right, bottom, top, nearPlane, farPlane));
    return 1;
}

static int rluaMatrixPerspective(lua_State *L)
{
    double fovY = luaL_checknumber(L, 1);
    double aspect = luaL_checknumber(L, 2);
    double nearPlane = luaL_checknumber(L, 3);
    double farPlane = luaL_checknumber(L, 4);
    rluaMatrixPush(L, MatrixPerspective(fovY, aspect, nearPlane, farPlane));
    return 1;
}

static int rluaMatrixOrtho(lua_State *L)
{
    double left = luaL_checknumber(L, 1);
    double right = luaL_checknumber(L, 2);
    double bottom = luaL_checknumber(L, 3);
    double top = luaL_checknumber(L, 4);
    double nearPlane = luaL_checknumber(L, 5);
    double farPlane = luaL_checknumber(L, 6);
    rluaMatrixPush(L, MatrixOrtho(left, right, bottom, top, nearPlane, farPlane));
    return 1;
}

static int rluaMatrixLookAt(lua_State *L)
{
    Vector3 eye = rluaVector3Check(L, 1);
    Vector3 target = rluaVector3Check(L, 2);
    Vector3 up = rluaVector3Check(L, 3);
    rluaMatrixPush(L, MatrixLookAt(eye, target, up));
    return 1;
}

static int rluaMatrixCompose(lua_State *L)
{
    Vector3 translation = rluaVector3Check(L, 1);
    Quaternion rotation = rluaQuaternionCheck(L, 2);
    Vector3 scale = rluaVector3Check(L, 3);
    rluaMatrixPush(L, MatrixCompose(translation, rotation, scale));
    return 1;
}

// }}}

// metamethods {{{

static int rluaMatrixToString(lua_State *L)
{
    Matrix m = rluaMatrixCheck(L, 1);
    lua_pushfstring(L,
        "Matrix(%f, %f, %f, %f, %f, %f, %f, %f, "
               "%f, %f, %f, %f, %f, %f, %f, %f)",
        m.m0, m.m1, m.m2, m.m3, m.m4, m.m5, m.m6, m.m7,
        m.m8, m.m9, m.m10, m.m11, m.m12, m.m13, m.m14, m.m15);
    return 1;
}

static int rluaMatrixAdd(lua_State *L)
{
    Matrix m1 = rluaMatrixCheck(L, 1);
    Matrix m2 = rluaMatrixCheck(L, 2);
    rluaMatrixPush(L, MatrixAdd(m1, m2));
    return 1;
}

static int rluaMatrixSubtract(lua_State *L)
{
    Matrix m1 = rluaMatrixCheck(L, 1);
    Matrix m2 = rluaMatrixCheck(L, 2);
    rluaMatrixPush(L, MatrixSubtract(m1, m2));
    return 1;
}

static int rluaMatrixMultiply(lua_State *L)
{
    if (rluaMatrixIs(L, 1) && rluaMatrixIs(L, 2))
    {
        Matrix m1 = rluaMatrixCheck(L, 1);
        Matrix m2 = rluaMatrixCheck(L, 2);
        rluaMatrixPush(L, MatrixMultiply(m1, m2));
        return 1;
    }

    if (rluaMatrixIs(L, 1) && rluaVector2Is(L, 2))
    {
        Vector2 v = rluaVector2Check(L, 2);
        Matrix m = rluaMatrixCheck(L, 1);
        rluaVector2Push(L, Vector2Transform(v, m));
        return 1;
    }

    if (rluaMatrixIs(L, 1) && rluaVector3Is(L, 2))
    {
        Vector3 v = rluaVector3Check(L, 2);
        Matrix m = rluaMatrixCheck(L, 1);
        rluaVector3Push(L, Vector3Transform(v, m));
        return 1;
    }

    if (rluaMatrixIs(L, 1) && rluaVector4Is(L, 2))
    {
        return luaL_error(L, "attempt to perform unsupported mul between a 'Matrix' and a 'Vector4'");
    }

    if (rluaMatrixIs(L, 1) && rluaQuaternionIs(L, 2))
    {
        return luaL_error(L, "attempt to mul a 'Matrix' with a 'Quaternion'; use Quaternion * Matrix instead");
    }

    if (rluaMatrixIs(L, 1) && lua_type(L, 2) == LUA_TNUMBER)
    {
        Matrix m = rluaMatrixCheck(L, 1);
        float n = (float)luaL_checknumber(L, 2);
        rluaMatrixPush(L, MatrixMultiplyValue(m, n));
        return 1;
    }

    if (lua_type(L, 1) == LUA_TNUMBER && rluaMatrixIs(L, 2))
    {
        float n = (float)luaL_checknumber(L, 1);
        Matrix m = rluaMatrixCheck(L, 2);
        rluaMatrixPush(L, MatrixMultiplyValue(m, n));
        return 1;
    }

    return luaL_error(L, "attempt to mul a '%s' by a '%s'", luaL_typename(L, 1), luaL_typename(L, 2));
}

// }}}

// methods {{{

static int rluaMatrixDeterminant(lua_State *L)
{
    Matrix m = rluaMatrixCheck(L, 1);
    lua_pushnumber(L, MatrixDeterminant(m));
    return 1;
}

static int rluaMatrixTrace(lua_State *L)
{
    Matrix m = rluaMatrixCheck(L, 1);
    lua_pushnumber(L, MatrixTrace(m));
    return 1;
}

static int rluaMatrixTranspose(lua_State *L)
{
    Matrix m = rluaMatrixCheck(L, 1);
    rluaMatrixPush(L, MatrixTranspose(m));
    return 1;
}

static int rluaMatrixInvert(lua_State *L)
{
    Matrix m = rluaMatrixCheck(L, 1);
    rluaMatrixPush(L, MatrixInvert(m));
    return 1;
}

static int rluaMatrixDecompose(lua_State *L)
{
    Matrix m = rluaMatrixCheck(L, 1);
    Vector3 translation;
    Quaternion rotation;
    Vector3 scale;
    MatrixDecompose(m, &translation, &rotation, &scale);
    rluaVector3Push(L, translation);
    rluaQuaternionPush(L, rotation);
    rluaVector3Push(L, scale);
    return 3;
}

// }}}

static void rluaMatrixRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_matrix_mt))
    {
        REG(lua_pushcfunction(L, rluaMatrixToString), "__tostring");
        REG(lua_pushcfunction(L, rluaMatrixAdd), "__add");
        REG(lua_pushcfunction(L, rluaMatrixSubtract), "__sub");
        REG(lua_pushcfunction(L, rluaMatrixMultiply), "__mul");

        lua_newtable(L); // M.__index

        REG(lua_pushcfunction(L, rluaMatrixDeterminant), "determinant");
        REG(lua_pushcfunction(L, rluaMatrixTrace), "trace");
        REG(lua_pushcfunction(L, rluaMatrixTranspose), "transpose");
        REG(lua_pushcfunction(L, rluaMatrixInvert), "invert");
        REG(lua_pushcfunction(L, rluaMatrixDecompose), "decompose");

        lua_setfield(L, -2, "__index"); // M.__index
    }

    lua_pushstring(L, "Matrix");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    lua_newtable(L); // rl.Matrix

    REG(lua_pushcfunction(L, rluaMatrixIdentity), "identity");
    REG(lua_pushcfunction(L, rluaMatrixTranslate), "translate");
    REG(lua_pushcfunction(L, rluaMatrixRotate), "rotate");
    REG(lua_pushcfunction(L, rluaMatrixRotateX), "rotateX");
    REG(lua_pushcfunction(L, rluaMatrixRotateY), "rotateY");
    REG(lua_pushcfunction(L, rluaMatrixRotateZ), "rotateZ");
    REG(lua_pushcfunction(L, rluaMatrixRotateXYZ), "rotateXYZ");
    REG(lua_pushcfunction(L, rluaMatrixRotateZYX), "rotateZYX");
    REG(lua_pushcfunction(L, rluaMatrixScale), "scale");
    REG(lua_pushcfunction(L, rluaMatrixFrustum), "frustum");
    REG(lua_pushcfunction(L, rluaMatrixPerspective), "perspective");
    REG(lua_pushcfunction(L, rluaMatrixOrtho), "ortho");
    REG(lua_pushcfunction(L, rluaMatrixLookAt), "lookAt");
    REG(lua_pushcfunction(L, rluaMatrixCompose), "compose");

    lua_newtable(L); // constructor metatable
    REG(lua_pushcfunction(L, rluaMatrix), "__call");
    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "Matrix");
}

// }}}

// Color, 4 components {{{

static const char rlua_color_mt[] = "raylib.Color";

static int rluaColorIs(lua_State *L, int index)
{
    if (lua_type(L, index) != LUA_TTABLE) return 0;
    if (!lua_getmetatable(L, index)) return 0;

    luaL_getmetatable(L, rlua_color_mt);
    int equal = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);

    return equal;
}

static Color rluaColorCheck(lua_State *L, int index)
{
    if (!rluaColorIs(L, index))
    {
        rluaTypeError(L, "Color", index);
    }

    Color c;

    lua_getfield(L, index, "r");
    c.r = (unsigned char)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "g");
    c.g = (unsigned char)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "b");
    c.b = (unsigned char)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "a");
    c.a = (unsigned char)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    return c;
}

static void rluaColorPush(lua_State *L, Color c)
{
    lua_createtable(L, 0, 4);

    REG(lua_pushinteger(L, c.r), "r");
    REG(lua_pushinteger(L, c.g), "g");
    REG(lua_pushinteger(L, c.b), "b");
    REG(lua_pushinteger(L, c.a), "a");

    luaL_setmetatable(L, rlua_color_mt);
}

static int rluaColor(lua_State *L)
{
    int n = lua_gettop(L) - 1;

    if (n != 4)
    {
        return luaL_error(L, "bad number of arguments to 'Color' (expected 4 numbers, got %d)", n);
    }

    Color c = {
        .r = (unsigned char)luaL_checkinteger(L, 2),
        .g = (unsigned char)luaL_checkinteger(L, 3),
        .b = (unsigned char)luaL_checkinteger(L, 4),
        .a = (unsigned char)luaL_checkinteger(L, 5),
    };
    rluaColorPush(L, c);
    return 1;
}

// factories {{{

static int rluaColorFromHSV(lua_State *L)
{
    float hue = (float)luaL_checknumber(L, 1);
    float saturation = (float)luaL_checknumber(L, 2);
    float value = (float)luaL_checknumber(L, 3);
    rluaColorPush(L, ColorFromHSV(hue, saturation, value));
    return 1;
}

static int rluaColorFromNormalized(lua_State *L)
{
    Vector4 normalized = rluaVector4Check(L, 1);
    rluaColorPush(L, ColorFromNormalized(normalized));
    return 1;
}

static int rluaGetColor(lua_State *L)
{
    unsigned int hexValue = (unsigned int)luaL_checkinteger(L, 1);
    rluaColorPush(L, GetColor(hexValue));
    return 1;
}

// }}}

// metamethods {{{

static int rluaColorToString(lua_State *L)
{
    Color c = rluaColorCheck(L, 1);
    lua_pushfstring(L, "Color(%d, %d, %d, %d)", c.r, c.g, c.b, c.a);
    return 1;
}

static int rluaColorIsEqual(lua_State *L)
{
    if (!rluaColorIs(L, 1) || !rluaColorIs(L, 2))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    Color c1 = rluaColorCheck(L, 1);
    Color c2 = rluaColorCheck(L, 2);
    lua_pushboolean(L, ColorIsEqual(c1, c2));
    return 1;
}

// }}}

// methods {{{

static int rluaFade(lua_State *L)
{
    Color c = rluaColorCheck(L, 1);
    float alpha = (float)luaL_checknumber(L, 2);
    rluaColorPush(L, Fade(c, alpha));
    return 1;
}

static int rluaColorToInt(lua_State *L)
{
    Color c = rluaColorCheck(L, 1);
    // widen the 32-bit pattern as unsigned so 0xRRGGBBAA stays positive
    lua_pushinteger(L, (lua_Integer)(unsigned int)ColorToInt(c));
    return 1;
}

static int rluaColorNormalize(lua_State *L)
{
    Color c = rluaColorCheck(L, 1);
    rluaVector4Push(L, ColorNormalize(c));
    return 1;
}

static int rluaColorToHSV(lua_State *L)
{
    Color c = rluaColorCheck(L, 1);
    rluaVector3Push(L, ColorToHSV(c));
    return 1;
}

static int rluaColorTint(lua_State *L)
{
    Color c = rluaColorCheck(L, 1);
    Color tint = rluaColorCheck(L, 2);
    rluaColorPush(L, ColorTint(c, tint));
    return 1;
}

static int rluaColorBrightness(lua_State *L)
{
    Color c = rluaColorCheck(L, 1);
    float factor = (float)luaL_checknumber(L, 2);
    rluaColorPush(L, ColorBrightness(c, factor));
    return 1;
}

static int rluaColorContrast(lua_State *L)
{
    Color c = rluaColorCheck(L, 1);
    float contrast = (float)luaL_checknumber(L, 2);
    rluaColorPush(L, ColorContrast(c, contrast));
    return 1;
}

static int rluaColorAlpha(lua_State *L)
{
    Color c = rluaColorCheck(L, 1);
    float alpha = (float)luaL_checknumber(L, 2);
    rluaColorPush(L, ColorAlpha(c, alpha));
    return 1;
}

static int rluaColorAlphaBlend(lua_State *L)
{
    Color dst = rluaColorCheck(L, 1);
    Color src = rluaColorCheck(L, 2);
    Color tint = rluaColorCheck(L, 3);
    rluaColorPush(L, ColorAlphaBlend(dst, src, tint));
    return 1;
}

static int rluaColorLerp(lua_State *L)
{
    Color c1 = rluaColorCheck(L, 1);
    Color c2 = rluaColorCheck(L, 2);
    float factor = (float)luaL_checknumber(L, 3);
    rluaColorPush(L, ColorLerp(c1, c2, factor));
    return 1;
}

// }}}

static void rluaColorRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_color_mt))
    {
        REG(lua_pushcfunction(L, rluaColorToString), "__tostring");
        REG(lua_pushcfunction(L, rluaColorIsEqual), "__eq");

        lua_newtable(L); // M.__index

        REG(lua_pushcfunction(L, rluaFade), "fade");
        REG(lua_pushcfunction(L, rluaColorToInt), "toInt");
        REG(lua_pushcfunction(L, rluaColorNormalize), "normalize");
        REG(lua_pushcfunction(L, rluaColorToHSV), "toHSV");
        REG(lua_pushcfunction(L, rluaColorTint), "tint");
        REG(lua_pushcfunction(L, rluaColorBrightness), "brightness");
        REG(lua_pushcfunction(L, rluaColorContrast), "contrast");
        REG(lua_pushcfunction(L, rluaColorAlpha), "alpha");
        REG(lua_pushcfunction(L, rluaColorAlphaBlend), "alphaBlend");
        REG(lua_pushcfunction(L, rluaColorLerp), "lerp");

        lua_setfield(L, -2, "__index"); // M.__index
    }

    lua_pushstring(L, "Color");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    lua_newtable(L); // rl.Color

    REG(lua_pushcfunction(L, rluaColorFromNormalized), "fromNormalized");
    REG(lua_pushcfunction(L, rluaColorFromHSV), "fromHSV");
    REG(lua_pushcfunction(L, rluaGetColor), "GetColor");
    REG(lua_pushcfunction(L, rluaGetColor), "fromHex");

    lua_newtable(L); // constructor metatable
    REG(lua_pushcfunction(L, rluaColor), "__call");
    lua_setmetatable(L, -2);

    lua_setfield(L, -2, "Color");
}

// }}}

// Rectangle, 4 components [POD] {{{

static float rluaRectangle_Field(lua_State *L, int index, const char *name)
{
    index = lua_absindex(L, index);

    lua_pushstring(L, name);
    lua_rawget(L, index);

    int isnum = 0;
    lua_Number value = lua_tonumberx(L, -1, &isnum);

    if (!isnum)
    {
        luaL_error(L, "Rectangle field '%s' expected number, got %s", name, luaL_typename(L, -1));
    }

    lua_pop(L, 1);

    return (float)value;
}

static Rectangle rluaRectangleCheck(lua_State *L, int index)
{
    if (lua_type(L, index) != LUA_TTABLE)
    {
        rluaTypeError(L, "Rectangle", index);
    }

    Rectangle r = {
        .x = rluaRectangle_Field(L, index, "x"),
        .y = rluaRectangle_Field(L, index, "y"),
        .width = rluaRectangle_Field(L, index, "width"),
        .height = rluaRectangle_Field(L, index, "height"),
    };

    return r;
}

static void rluaRectanglePush(lua_State *L, Rectangle r)
{
    lua_createtable(L, 0, 4);

    REG(lua_pushnumber(L, r.x), "x");
    REG(lua_pushnumber(L, r.y), "y");
    REG(lua_pushnumber(L, r.width), "width");
    REG(lua_pushnumber(L, r.height), "height");
}

// }}}

// Image, pixel data stored in CPU memory (RAM) [RESOURCE] {{{

static const char rlua_image_mt[] = "raylib.Image";


static Image *rluaImageCheck(lua_State *L, int index)
{
    return (Image *)luaL_checkudata(L, index, rlua_image_mt);
}

static void rluaImagePush(lua_State *L, Image image)
{
    Image *ud = (Image *)lua_newuserdatauv(L, sizeof(Image), 1);
    *ud = image;
    luaL_setmetatable(L, rlua_image_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaImage_Release(lua_State *L, int index)
{
    Image *image = luaL_testudata(L, index, rlua_image_mt);
    if (image == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadImage(*image);
    *image = (Image){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadImage(lua_State *L);

// metamethods {{{

static int rluaImageToString(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    lua_pushfstring(
        L,
        "Image(width=%d, height=%d, mipmaps=%d, format=%d)",
        image->width,
        image->height,
        image->mipmaps,
        image->format
    );
    return 1;
}

static int rluaImageIndex(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "width") == 0) lua_pushinteger(L, image->width);
    else if (strcmp(field, "height") == 0) lua_pushinteger(L, image->height);
    else if (strcmp(field, "mipmaps") == 0) lua_pushinteger(L, image->mipmaps);
    else if (strcmp(field, "format") == 0) lua_pushinteger(L, image->format);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadImage);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsImageValid);
    else if (strcmp(field, "export") == 0) lua_pushcfunction(L, rluaExportImage);
    else if (strcmp(field, "exportToMemory") == 0) lua_pushcfunction(L, rluaExportImageToMemory);
    else if (strcmp(field, "exportAsCode") == 0) lua_pushcfunction(L, rluaExportImageAsCode);
    else if (strcmp(field, "copy") == 0) lua_pushcfunction(L, rluaImageCopy);
    else if (strcmp(field, "fromImage") == 0) lua_pushcfunction(L, rluaImageFromImage);
    else if (strcmp(field, "fromChannel") == 0) lua_pushcfunction(L, rluaImageFromChannel);
    else if (strcmp(field, "ImageFormat") == 0) lua_pushcfunction(L, rluaImageFormat);
    else if (strcmp(field, "toPOT") == 0) lua_pushcfunction(L, rluaImageToPOT);
    else if (strcmp(field, "crop") == 0) lua_pushcfunction(L, rluaImageCrop);
    else if (strcmp(field, "alphaCrop") == 0) lua_pushcfunction(L, rluaImageAlphaCrop);
    else if (strcmp(field, "alphaClear") == 0) lua_pushcfunction(L, rluaImageAlphaClear);
    else if (strcmp(field, "alphaMask") == 0) lua_pushcfunction(L, rluaImageAlphaMask);
    else if (strcmp(field, "alphaPremultiply") == 0) lua_pushcfunction(L, rluaImageAlphaPremultiply);
    else if (strcmp(field, "blurGaussian") == 0) lua_pushcfunction(L, rluaImageBlurGaussian);
    else if (strcmp(field, "kernelConvolution") == 0) lua_pushcfunction(L, rluaImageKernelConvolution);
    else if (strcmp(field, "resize") == 0) lua_pushcfunction(L, rluaImageResize);
    else if (strcmp(field, "resizeNN") == 0) lua_pushcfunction(L, rluaImageResizeNN);
    else if (strcmp(field, "resizeCanvas") == 0) lua_pushcfunction(L, rluaImageResizeCanvas);
    else if (strcmp(field, "ImageMipmaps") == 0) lua_pushcfunction(L, rluaImageMipmaps);
    else if (strcmp(field, "dither") == 0) lua_pushcfunction(L, rluaImageDither);
    else if (strcmp(field, "flipVertical") == 0) lua_pushcfunction(L, rluaImageFlipVertical);
    else if (strcmp(field, "flipHorizontal") == 0) lua_pushcfunction(L, rluaImageFlipHorizontal);
    else if (strcmp(field, "rotate") == 0) lua_pushcfunction(L, rluaImageRotate);
    else if (strcmp(field, "rotateCW") == 0) lua_pushcfunction(L, rluaImageRotateCW);
    else if (strcmp(field, "rotateCCW") == 0) lua_pushcfunction(L, rluaImageRotateCCW);
    else if (strcmp(field, "colorTint") == 0) lua_pushcfunction(L, rluaImageColorTint);
    else if (strcmp(field, "colorInvert") == 0) lua_pushcfunction(L, rluaImageColorInvert);
    else if (strcmp(field, "colorGrayscale") == 0) lua_pushcfunction(L, rluaImageColorGrayscale);
    else if (strcmp(field, "colorContrast") == 0) lua_pushcfunction(L, rluaImageColorContrast);
    else if (strcmp(field, "colorBrightness") == 0) lua_pushcfunction(L, rluaImageColorBrightness);
    else if (strcmp(field, "colorReplace") == 0) lua_pushcfunction(L, rluaImageColorReplace);
    else if (strcmp(field, "loadColors") == 0) lua_pushcfunction(L, rluaLoadImageColors);
    else if (strcmp(field, "loadPalette") == 0) lua_pushcfunction(L, rluaLoadImagePalette);
    else if (strcmp(field, "getAlphaBorder") == 0) lua_pushcfunction(L, rluaGetImageAlphaBorder);
    else if (strcmp(field, "getColor") == 0) lua_pushcfunction(L, rluaGetImageColor);
    else if (strcmp(field, "clearBackground") == 0) lua_pushcfunction(L, rluaImageClearBackground);
    else if (strcmp(field, "drawPixel") == 0) lua_pushcfunction(L, rluaImageDrawPixel);
    else if (strcmp(field, "drawPixelV") == 0) lua_pushcfunction(L, rluaImageDrawPixelV);
    else if (strcmp(field, "drawLine") == 0) lua_pushcfunction(L, rluaImageDrawLine);
    else if (strcmp(field, "drawLineV") == 0) lua_pushcfunction(L, rluaImageDrawLineV);
    else if (strcmp(field, "drawLineEx") == 0) lua_pushcfunction(L, rluaImageDrawLineEx);
    else if (strcmp(field, "drawLineStrip") == 0) lua_pushcfunction(L, rluaImageDrawLineStrip);
    else if (strcmp(field, "drawTriangle") == 0) lua_pushcfunction(L, rluaImageDrawTriangle);
    else if (strcmp(field, "drawTriangleGradient") == 0) lua_pushcfunction(L, rluaImageDrawTriangleGradient);
    else if (strcmp(field, "drawTriangleLines") == 0) lua_pushcfunction(L, rluaImageDrawTriangleLines);
    else if (strcmp(field, "drawTriangleFan") == 0) lua_pushcfunction(L, rluaImageDrawTriangleFan);
    else if (strcmp(field, "drawTriangleStrip") == 0) lua_pushcfunction(L, rluaImageDrawTriangleStrip);
    else if (strcmp(field, "drawRectangle") == 0) lua_pushcfunction(L, rluaImageDrawRectangle);
    else if (strcmp(field, "drawRectangleV") == 0) lua_pushcfunction(L, rluaImageDrawRectangleV);
    else if (strcmp(field, "drawRectangleRec") == 0) lua_pushcfunction(L, rluaImageDrawRectangleRec);
    else if (strcmp(field, "drawRectanglePro") == 0) lua_pushcfunction(L, rluaImageDrawRectanglePro);
    else if (strcmp(field, "drawRectangleLines") == 0) lua_pushcfunction(L, rluaImageDrawRectangleLines);
    else if (strcmp(field, "drawRectangleLinesEx") == 0) lua_pushcfunction(L, rluaImageDrawRectangleLinesEx);
    else if (strcmp(field, "drawRectangleGradientEx") == 0) lua_pushcfunction(L, rluaImageDrawRectangleGradientEx);
    else if (strcmp(field, "drawCircle") == 0) lua_pushcfunction(L, rluaImageDrawCircle);
    else if (strcmp(field, "drawCircleV") == 0) lua_pushcfunction(L, rluaImageDrawCircleV);
    else if (strcmp(field, "drawCircleLines") == 0) lua_pushcfunction(L, rluaImageDrawCircleLines);
    else if (strcmp(field, "drawCircleLinesV") == 0) lua_pushcfunction(L, rluaImageDrawCircleLinesV);
    else if (strcmp(field, "drawCircleGradient") == 0) lua_pushcfunction(L, rluaImageDrawCircleGradient);
    else if (strcmp(field, "drawImage") == 0) lua_pushcfunction(L, rluaImageDrawImage);
    else if (strcmp(field, "drawImageEx") == 0) lua_pushcfunction(L, rluaImageDrawImageEx);
    else if (strcmp(field, "drawImageRec") == 0) lua_pushcfunction(L, rluaImageDrawImageRec);
    else if (strcmp(field, "drawImagePro") == 0) lua_pushcfunction(L, rluaImageDrawImagePro);
    else if (strcmp(field, "drawText") == 0) lua_pushcfunction(L, rluaImageDrawText);
    else if (strcmp(field, "drawTextEx") == 0) lua_pushcfunction(L, rluaImageDrawTextEx);
    else if (strcmp(field, "drawTextPro") == 0) lua_pushcfunction(L, rluaImageDrawTextPro);
    else lua_pushnil(L);

    return 1;
}

static int rluaImageGc(lua_State *L)
{
    rluaImage_Release(L, 1);
    return 0;
}
// }}}

// methods {{{

static int rluaUnloadImage(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    (void)image;

    rluaImage_Release(L, 1);
    return 0;
}
// }}}

// functions {{{

static Image *rluaImage_CheckValid(lua_State *L, int index)
{
    Image *image = rluaImageCheck(L, index);

    if (!IsImageValid(*image))
    {
        luaL_error(L, "invalid Image");
        return NULL;
    }

    return image;
}

static int rluaImage_SizeValid(int width, int height)
{
    return (width > 0) && (height > 0);
}

static Image *rluaImage_ArrayCheck(lua_State *L, int index, int *count)
{
    luaL_checktype(L, index, LUA_TTABLE);
    int n = (int)luaL_len(L, index);
    Image *values = (Image *)lua_newuserdatauv(
        L,
        sizeof(Image) * (n > 0 ? n : 1),
        0
    );

    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(L, index, i);
        values[i - 1] = *rluaImage_CheckValid(L, -1);
        lua_pop(L, 1);
    }

    *count = n;
    return values;
}

static void rluaImage_PushOrNil(lua_State *L, Image image)
{
    if (!IsImageValid(image))
    {
        UnloadImage(image);
        lua_pushnil(L);
        return;
    }
    rluaImagePush(L, image);
}

// loading {{{

static int rluaLoadImage(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    rluaImage_PushOrNil(L, LoadImage(fileName));
    return 1;
}

static int rluaLoadImageRaw(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    int width = (int)luaL_checkinteger(L, 2);
    int height = (int)luaL_checkinteger(L, 3);
    int format = (int)luaL_checkinteger(L, 4);
    int headerSize = (int)luaL_checkinteger(L, 5);
    rluaImage_PushOrNil(L, LoadImageRaw(fileName, width, height, format, headerSize));
    return 1;
}

static int rluaLoadImageAnim(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    int frames = 0;
    Image image = LoadImageAnim(fileName, &frames);

    if (!IsImageValid(image))
    {
        UnloadImage(image);
        lua_pushnil(L);
        lua_pushinteger(L, frames);
        return 2;
    }

    rluaImagePush(L, image);
    lua_pushinteger(L, frames);
    return 2;
}

static int rluaLoadImageAnimFromMemory(lua_State *L)
{
    const char *fileType = luaL_checkstring(L, 1);
    size_t dataLen = 0;
    const unsigned char *fileData = (const unsigned char *)luaL_checklstring(L, 2, &dataLen);
    int frames = 0;
    Image image = LoadImageAnimFromMemory(fileType, fileData, (int)dataLen, &frames);

    if (!IsImageValid(image))
    {
        UnloadImage(image);
        lua_pushnil(L);
        lua_pushinteger(L, frames);
        return 2;
    }

    rluaImagePush(L, image);
    lua_pushinteger(L, frames);
    return 2;
}

static int rluaLoadImageFromMemory(lua_State *L)
{
    const char *fileType = luaL_checkstring(L, 1);
    size_t dataLen = 0;
    const unsigned char *fileData = (const unsigned char *)luaL_checklstring(L, 2, &dataLen);
    rluaImage_PushOrNil(L, LoadImageFromMemory(fileType, fileData, (int)dataLen));
    return 1;
}

static int rluaLoadImageFromTexture(lua_State *L)
{
    Texture *texture = rluaTextureCheck(L, 1);

    if (!IsTextureValid(*texture))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, LoadImageFromTexture(*texture));
    return 1;
}

static int rluaLoadImageFromScreen(lua_State *L)
{
    rluaImage_PushOrNil(L, LoadImageFromScreen());
    return 1;
}

static int rluaIsImageValid(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    lua_pushboolean(L, IsImageValid(*image));
    return 1;
}

static int rluaExportImage(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    const char *fileName = luaL_checkstring(L, 2);

    if (!IsImageValid(*image))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushboolean(L, ExportImage(*image, fileName));
    return 1;
}

static int rluaExportImageToMemory(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    const char *fileType = luaL_checkstring(L, 2);

    if (!IsImageValid(*image))
    {
        lua_pushnil(L);
        return 1;
    }

    int fileSize = 0;
    unsigned char *data = ExportImageToMemory(*image, fileType, &fileSize);

    if (data == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushlstring(L, (const char *)data, (size_t)fileSize);
    MemFree(data);
    return 1;
}

static int rluaExportImageAsCode(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    const char *fileName = luaL_checkstring(L, 2);

    if (!IsImageValid(*image))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushboolean(L, ExportImageAsCode(*image, fileName));
    return 1;
}

// }}}

// generation {{{

static int rluaGenImageColor(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    Color color = rluaColorCheck(L, 3);

    if ((width <= 0) || (height <= 0))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, GenImageColor(width, height, color));
    return 1;
}

static int rluaGenImageGradientLinear(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    int direction = (int)luaL_checkinteger(L, 3);
    Color start = rluaColorCheck(L, 4);
    Color end = rluaColorCheck(L, 5);

    if (!rluaImage_SizeValid(width, height))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, GenImageGradientLinear(width, height, direction, start, end));
    return 1;
}

static int rluaGenImageGradientRadial(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    float density = (float)luaL_checknumber(L, 3);
    Color inner = rluaColorCheck(L, 4);
    Color outer = rluaColorCheck(L, 5);

    if (!rluaImage_SizeValid(width, height))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, GenImageGradientRadial(width, height, density, inner, outer));
    return 1;
}

static int rluaGenImageGradientSquare(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    float density = (float)luaL_checknumber(L, 3);
    Color inner = rluaColorCheck(L, 4);
    Color outer = rluaColorCheck(L, 5);

    if (!rluaImage_SizeValid(width, height))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, GenImageGradientSquare(width, height, density, inner, outer));
    return 1;
}

static int rluaGenImageChecked(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    int checksX = (int)luaL_checkinteger(L, 3);
    int checksY = (int)luaL_checkinteger(L, 4);
    Color col1 = rluaColorCheck(L, 5);
    Color col2 = rluaColorCheck(L, 6);

    if (!rluaImage_SizeValid(width, height))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, GenImageChecked(width, height, checksX, checksY, col1, col2));
    return 1;
}

static int rluaGenImageWhiteNoise(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    float factor = (float)luaL_checknumber(L, 3);

    if (!rluaImage_SizeValid(width, height))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, GenImageWhiteNoise(width, height, factor));
    return 1;
}

static int rluaGenImagePerlinNoise(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    int offsetX = (int)luaL_checkinteger(L, 3);
    int offsetY = (int)luaL_checkinteger(L, 4);
    float scale = (float)luaL_checknumber(L, 5);

    if (!rluaImage_SizeValid(width, height))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, GenImagePerlinNoise(width, height, offsetX, offsetY, scale));
    return 1;
}

static int rluaGenImageCellular(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    int tileSize = (int)luaL_checkinteger(L, 3);

    if (!rluaImage_SizeValid(width, height))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, GenImageCellular(width, height, tileSize));
    return 1;
}

static int rluaGenImageText(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    const char *text = luaL_checkstring(L, 3);

    if (!rluaImage_SizeValid(width, height))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, GenImageText(width, height, text));
    return 1;
}

// }}}

// manipulation {{{

static int rluaImageCopy(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);

    if (!IsImageValid(*image))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, ImageCopy(*image));
    return 1;
}

static int rluaImageFromImage(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    Rectangle rec = rluaRectangleCheck(L, 2);

    if (!IsImageValid(*image))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, ImageFromImage(*image, rec));
    return 1;
}

static int rluaImageFromChannel(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    int selectedChannel = (int)luaL_checkinteger(L, 2);

    if (!IsImageValid(*image))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaImage_PushOrNil(L, ImageFromChannel(*image, selectedChannel));
    return 1;
}

static int rluaImageText(lua_State *L)
{
    Font font = GetFontDefault();

    if (!IsFontValid(font))
    {
        return luaL_error(L, "default font not available");
    }

    const char *text = luaL_checkstring(L, 1);
    int fontSize = (int)luaL_checkinteger(L, 2);
    Color color = rluaColorCheck(L, 3);
    rluaImage_PushOrNil(L, ImageText(text, fontSize, color));
    return 1;
}

static int rluaImageTextEx(lua_State *L)
{
    Font *font = rluaFont_CheckValid(L, 1);
    const char *text = luaL_checkstring(L, 2);
    float fontSize = (float)luaL_checknumber(L, 3);
    float spacing = (float)luaL_checknumber(L, 4);
    Color tint = rluaColorCheck(L, 5);
    rluaImage_PushOrNil(L, ImageTextEx(*font, text, fontSize, spacing, tint));
    return 1;
}

static int rluaImageFormat(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageFormat(image, (int)luaL_checkinteger(L, 2));
    return 0;
}

static int rluaImageToPOT(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageToPOT(image, rluaColorCheck(L, 2));
    return 0;
}

static int rluaImageCrop(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageCrop(image, rluaRectangleCheck(L, 2));
    return 0;
}

static int rluaImageAlphaCrop(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageAlphaCrop(image, (float)luaL_checknumber(L, 2));
    return 0;
}

static int rluaImageAlphaClear(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    Color color = rluaColorCheck(L, 2);
    float threshold = (float)luaL_checknumber(L, 3);
    ImageAlphaClear(image, color, threshold);
    return 0;
}

static int rluaImageAlphaMask(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    Image *alphaMask = rluaImage_CheckValid(L, 2);
    ImageAlphaMask(image, *alphaMask);
    return 0;
}

static int rluaImageAlphaPremultiply(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageAlphaPremultiply(image);
    return 0;
}

static int rluaImageBlurGaussian(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageBlurGaussian(image, (int)luaL_checkinteger(L, 2));
    return 0;
}

static int rluaImageKernelConvolution(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    int kernelSize = 0;
    float *kernel = rluaFloatArrayCheck(L, 2, &kernelSize);
    ImageKernelConvolution(image, kernel, kernelSize);
    return 0;
}

static int rluaImageResize(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    int newWidth = (int)luaL_checkinteger(L, 2);
    int newHeight = (int)luaL_checkinteger(L, 3);
    ImageResize(image, newWidth, newHeight);
    return 0;
}

static int rluaImageResizeNN(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    int newWidth = (int)luaL_checkinteger(L, 2);
    int newHeight = (int)luaL_checkinteger(L, 3);
    ImageResizeNN(image, newWidth, newHeight);
    return 0;
}

static int rluaImageResizeCanvas(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    int newWidth = (int)luaL_checkinteger(L, 2);
    int newHeight = (int)luaL_checkinteger(L, 3);
    int offsetX = (int)luaL_checkinteger(L, 4);
    int offsetY = (int)luaL_checkinteger(L, 5);
    Color fill = rluaColorCheck(L, 6);
    ImageResizeCanvas(image, newWidth, newHeight, offsetX, offsetY, fill);
    return 0;
}

static int rluaImageMipmaps(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageMipmaps(image);
    return 0;
}

static int rluaImageDither(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    int rBpp = (int)luaL_checkinteger(L, 2);
    int gBpp = (int)luaL_checkinteger(L, 3);
    int bBpp = (int)luaL_checkinteger(L, 4);
    int aBpp = (int)luaL_checkinteger(L, 5);
    ImageDither(image, rBpp, gBpp, bBpp, aBpp);
    return 0;
}

static int rluaImageFlipVertical(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageFlipVertical(image);
    return 0;
}

static int rluaImageFlipHorizontal(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageFlipHorizontal(image);
    return 0;
}

static int rluaImageRotate(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageRotate(image, (int)luaL_checkinteger(L, 2));
    return 0;
}

static int rluaImageRotateCW(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageRotateCW(image);
    return 0;
}

static int rluaImageRotateCCW(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageRotateCCW(image);
    return 0;
}

static int rluaImageColorTint(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageColorTint(image, rluaColorCheck(L, 2));
    return 0;
}

static int rluaImageColorInvert(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageColorInvert(image);
    return 0;
}

static int rluaImageColorGrayscale(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageColorGrayscale(image);
    return 0;
}

static int rluaImageColorContrast(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageColorContrast(image, (int)luaL_checkinteger(L, 2));
    return 0;
}

static int rluaImageColorBrightness(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    ImageColorBrightness(image, (int)luaL_checkinteger(L, 2));
    return 0;
}

static int rluaImageColorReplace(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    Color color = rluaColorCheck(L, 2);
    Color replace = rluaColorCheck(L, 3);
    ImageColorReplace(image, color, replace);
    return 0;
}

static int rluaLoadImageColors(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);

    if (!IsImageValid(*image))
    {
        lua_pushnil(L);
        return 1;
    }

    Color *colors = LoadImageColors(*image);

    if (colors == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    int result = rluaPushColorArray(L, colors, image->width*image->height);
    UnloadImageColors(colors);
    return result;
}

static int rluaLoadImagePalette(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);

    if (!IsImageValid(*image))
    {
        lua_pushnil(L);
        return 1;
    }

    int maxPaletteSize = (int)luaL_checkinteger(L, 2);
    int colorCount = 0;
    Color *colors = LoadImagePalette(*image, maxPaletteSize, &colorCount);

    if (colors == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    int result = rluaPushColorArray(L, colors, colorCount);
    UnloadImagePalette(colors);
    return result;
}

static int rluaGetImageAlphaBorder(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    rluaRectanglePush(L, GetImageAlphaBorder(*image, (float)luaL_checknumber(L, 2)));
    return 1;
}

static int rluaGetImageColor(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    int x = (int)luaL_checkinteger(L, 2);
    int y = (int)luaL_checkinteger(L, 3);
    rluaColorPush(L, GetImageColor(*image, x, y));
    return 1;
}

// }}}

// drawing {{{

static int rluaImageClearBackground(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    ImageClearBackground(dst, rluaColorCheck(L, 2));
    return 0;
}

static int rluaImageDrawPixel(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    int posX = (int)luaL_checkinteger(L, 2);
    int posY = (int)luaL_checkinteger(L, 3);
    Color color = rluaColorCheck(L, 4);
    ImageDrawPixel(dst, posX, posY, color);
    return 0;
}

static int rluaImageDrawPixelV(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 position = rluaVector2Check(L, 2);
    Color color = rluaColorCheck(L, 3);
    ImageDrawPixelV(dst, position, color);
    return 0;
}

static int rluaImageDrawLine(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    int startPosX = (int)luaL_checkinteger(L, 2);
    int startPosY = (int)luaL_checkinteger(L, 3);
    int endPosX = (int)luaL_checkinteger(L, 4);
    int endPosY = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    ImageDrawLine(dst, startPosX, startPosY, endPosX, endPosY, color);
    return 0;
}

static int rluaImageDrawLineV(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 start = rluaVector2Check(L, 2);
    Vector2 end = rluaVector2Check(L, 3);
    Color color = rluaColorCheck(L, 4);
    ImageDrawLineV(dst, start, end, color);
    return 0;
}

static int rluaImageDrawLineEx(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 start = rluaVector2Check(L, 2);
    Vector2 end = rluaVector2Check(L, 3);
    int thick = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    ImageDrawLineEx(dst, start, end, thick, color);
    return 0;
}

static int rluaImageDrawLineStrip(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 2, &count);
    ImageDrawLineStrip(dst, points, count, rluaColorCheck(L, 3));
    return 0;
}

static int rluaImageDrawTriangle(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 v1 = rluaVector2Check(L, 2);
    Vector2 v2 = rluaVector2Check(L, 3);
    Vector2 v3 = rluaVector2Check(L, 4);
    ImageDrawTriangle(dst, v1, v2, v3, rluaColorCheck(L, 5));
    return 0;
}

static int rluaImageDrawTriangleGradient(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 v1 = rluaVector2Check(L, 2);
    Vector2 v2 = rluaVector2Check(L, 3);
    Vector2 v3 = rluaVector2Check(L, 4);
    Color c1 = rluaColorCheck(L, 5);
    Color c2 = rluaColorCheck(L, 6);
    Color c3 = rluaColorCheck(L, 7);
    ImageDrawTriangleGradient(dst, v1, v2, v3, c1, c2, c3);
    return 0;
}

static int rluaImageDrawTriangleLines(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 v1 = rluaVector2Check(L, 2);
    Vector2 v2 = rluaVector2Check(L, 3);
    Vector2 v3 = rluaVector2Check(L, 4);
    ImageDrawTriangleLines(dst, v1, v2, v3, rluaColorCheck(L, 5));
    return 0;
}

static int rluaImageDrawTriangleFan(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 2, &count);
    ImageDrawTriangleFan(dst, points, count, rluaColorCheck(L, 3));
    return 0;
}

static int rluaImageDrawTriangleStrip(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 2, &count);
    ImageDrawTriangleStrip(dst, points, count, rluaColorCheck(L, 3));
    return 0;
}

static int rluaImageDrawRectangle(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    int posX = (int)luaL_checkinteger(L, 2);
    int posY = (int)luaL_checkinteger(L, 3);
    int width = (int)luaL_checkinteger(L, 4);
    int height = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    ImageDrawRectangle(dst, posX, posY, width, height, color);
    return 0;
}

static int rluaImageDrawRectangleV(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 position = rluaVector2Check(L, 2);
    Vector2 size = rluaVector2Check(L, 3);
    Color color = rluaColorCheck(L, 4);
    ImageDrawRectangleV(dst, position, size, color);
    return 0;
}

static int rluaImageDrawRectangleRec(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Rectangle rec = rluaRectangleCheck(L, 2);
    ImageDrawRectangleRec(dst, rec, rluaColorCheck(L, 3));
    return 0;
}

static int rluaImageDrawRectanglePro(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Rectangle rec = rluaRectangleCheck(L, 2);
    Vector2 origin = rluaVector2Check(L, 3);
    float rotation = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    ImageDrawRectanglePro(dst, rec, origin, rotation, color);
    return 0;
}

static int rluaImageDrawRectangleLines(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    int posX = (int)luaL_checkinteger(L, 2);
    int posY = (int)luaL_checkinteger(L, 3);
    int width = (int)luaL_checkinteger(L, 4);
    int height = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    ImageDrawRectangleLines(dst, posX, posY, width, height, color);
    return 0;
}

static int rluaImageDrawRectangleLinesEx(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Rectangle rec = rluaRectangleCheck(L, 2);
    int thick = (int)luaL_checkinteger(L, 3);
    Color color = rluaColorCheck(L, 4);
    ImageDrawRectangleLinesEx(dst, rec, thick, color);
    return 0;
}

static int rluaImageDrawRectangleGradientEx(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Rectangle rec = rluaRectangleCheck(L, 2);
    Color col1 = rluaColorCheck(L, 3);
    Color col2 = rluaColorCheck(L, 4);
    Color col3 = rluaColorCheck(L, 5);
    Color col4 = rluaColorCheck(L, 6);
    ImageDrawRectangleGradientEx(dst, rec, col1, col2, col3, col4);
    return 0;
}

static int rluaImageDrawCircle(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    int centerX = (int)luaL_checkinteger(L, 2);
    int centerY = (int)luaL_checkinteger(L, 3);
    int radius = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    ImageDrawCircle(dst, centerX, centerY, radius, color);
    return 0;
}

static int rluaImageDrawCircleV(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 center = rluaVector2Check(L, 2);
    int radius = (int)luaL_checkinteger(L, 3);
    ImageDrawCircleV(dst, center, radius, rluaColorCheck(L, 4));
    return 0;
}

static int rluaImageDrawCircleLines(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    int centerX = (int)luaL_checkinteger(L, 2);
    int centerY = (int)luaL_checkinteger(L, 3);
    int radius = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    ImageDrawCircleLines(dst, centerX, centerY, radius, color);
    return 0;
}

static int rluaImageDrawCircleLinesV(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 center = rluaVector2Check(L, 2);
    int radius = (int)luaL_checkinteger(L, 3);
    ImageDrawCircleLinesV(dst, center, radius, rluaColorCheck(L, 4));
    return 0;
}

static int rluaImageDrawCircleGradient(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Vector2 center = rluaVector2Check(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    Color inner = rluaColorCheck(L, 4);
    Color outer = rluaColorCheck(L, 5);
    ImageDrawCircleGradient(dst, center, radius, inner, outer);
    return 0;
}

static int rluaImageDrawImage(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Image *src = rluaImage_CheckValid(L, 2);
    int posX = (int)luaL_checkinteger(L, 3);
    int posY = (int)luaL_checkinteger(L, 4);
    Color tint = rluaColorCheck(L, 5);
    ImageDrawImage(dst, *src, posX, posY, tint);
    return 0;
}

static int rluaImageDrawImageEx(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Image *src = rluaImage_CheckValid(L, 2);
    Vector2 position = rluaVector2Check(L, 3);
    float rotation = (float)luaL_checknumber(L, 4);
    float scale = (float)luaL_checknumber(L, 5);
    Color tint = rluaColorCheck(L, 6);
    ImageDrawImageEx(dst, *src, position, rotation, scale, tint);
    return 0;
}

static int rluaImageDrawImageRec(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Image *src = rluaImage_CheckValid(L, 2);
    Rectangle srcRec = rluaRectangleCheck(L, 3);
    Vector2 position = rluaVector2Check(L, 4);
    Color tint = rluaColorCheck(L, 5);
    ImageDrawImageRec(dst, *src, srcRec, position, tint);
    return 0;
}

static int rluaImageDrawImagePro(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Image *src = rluaImage_CheckValid(L, 2);
    Rectangle srcRec = rluaRectangleCheck(L, 3);
    Rectangle dstRec = rluaRectangleCheck(L, 4);
    Vector2 origin = rluaVector2Check(L, 5);
    float rotation = (float)luaL_checknumber(L, 6);
    Color tint = rluaColorCheck(L, 7);
    ImageDrawImagePro(dst, *src, srcRec, dstRec, origin, rotation, tint);
    return 0;
}

static int rluaImageDrawText(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Font font = GetFontDefault();

    if (!IsFontValid(font))
    {
        return luaL_error(L, "default font not available");
    }

    const char *text = luaL_checkstring(L, 2);
    int posX = (int)luaL_checkinteger(L, 3);
    int posY = (int)luaL_checkinteger(L, 4);
    int fontSize = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    ImageDrawText(dst, text, posX, posY, fontSize, color);
    return 0;
}

static int rluaImageDrawTextEx(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Font *font = rluaFont_CheckValid(L, 2);
    const char *text = luaL_checkstring(L, 3);
    Vector2 position = rluaVector2Check(L, 4);
    float fontSize = (float)luaL_checknumber(L, 5);
    float spacing = (float)luaL_checknumber(L, 6);
    Color tint = rluaColorCheck(L, 7);
    ImageDrawTextEx(dst, *font, text, position, fontSize, spacing, tint);
    return 0;
}

static int rluaImageDrawTextPro(lua_State *L)
{
    Image *dst = rluaImage_CheckValid(L, 1);
    Font *font = rluaFont_CheckValid(L, 2);
    const char *text = luaL_checkstring(L, 3);
    Vector2 position = rluaVector2Check(L, 4);
    Vector2 origin = rluaVector2Check(L, 5);
    float rotation = (float)luaL_checknumber(L, 6);
    float fontSize = (float)luaL_checknumber(L, 7);
    float spacing = (float)luaL_checknumber(L, 8);
    Color tint = rluaColorCheck(L, 9);
    ImageDrawTextPro(dst, *font, text, position, origin, rotation, fontSize, spacing, tint);
    return 0;
}

// }}}

// core {{{

static int rluaSetWindowIcon(lua_State *L)
{
    Image *image = rluaImage_CheckValid(L, 1);
    SetWindowIcon(*image);
    return 0;
}

static int rluaSetWindowIcons(lua_State *L)
{
    int count = 0;
    Image *images = rluaImage_ArrayCheck(L, 1, &count);
    SetWindowIcons(images, count);
    return 0;
}

static int rluaGetClipboardImage(lua_State *L)
{
    rluaImage_PushOrNil(L, GetClipboardImage());
    return 1;
}

// }}}

// }}}

static void rluaImageRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_image_mt))
    {
        REG(lua_pushcfunction(L, rluaImageToString), "__tostring");
        REG(lua_pushcfunction(L, rluaImageIndex), "__index");
        REG(lua_pushcfunction(L, rluaImageGc), "__gc");
        REG(lua_pushcfunction(L, rluaImageGc), "__close");
    }

    lua_pushstring(L, "Image");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaLoadImage), "LoadImage");
    REG(lua_pushcfunction(L, rluaLoadImageRaw), "LoadImageRaw");
    REG(lua_pushcfunction(L, rluaLoadImageAnim), "LoadImageAnim");
    REG(lua_pushcfunction(L, rluaLoadImageAnimFromMemory), "LoadImageAnimFromMemory");
    REG(lua_pushcfunction(L, rluaLoadImageFromMemory), "LoadImageFromMemory");
    REG(lua_pushcfunction(L, rluaLoadImageFromTexture), "LoadImageFromTexture");
    REG(lua_pushcfunction(L, rluaLoadImageFromScreen), "LoadImageFromScreen");
    REG(lua_pushcfunction(L, rluaGenImageColor), "GenImageColor");
    REG(lua_pushcfunction(L, rluaGenImageGradientLinear), "GenImageGradientLinear");
    REG(lua_pushcfunction(L, rluaGenImageGradientRadial), "GenImageGradientRadial");
    REG(lua_pushcfunction(L, rluaGenImageGradientSquare), "GenImageGradientSquare");
    REG(lua_pushcfunction(L, rluaGenImageChecked), "GenImageChecked");
    REG(lua_pushcfunction(L, rluaGenImageWhiteNoise), "GenImageWhiteNoise");
    REG(lua_pushcfunction(L, rluaGenImagePerlinNoise), "GenImagePerlinNoise");
    REG(lua_pushcfunction(L, rluaGenImageCellular), "GenImageCellular");
    REG(lua_pushcfunction(L, rluaGenImageText), "GenImageText");
    REG(lua_pushcfunction(L, rluaImageTextEx), "ImageTextEx");
    REG(lua_pushcfunction(L, rluaImageText), "ImageText");
    REG(lua_pushcfunction(L, rluaSetWindowIcon), "SetWindowIcon");
    REG(lua_pushcfunction(L, rluaSetWindowIcons), "SetWindowIcons");
    REG(lua_pushcfunction(L, rluaGetClipboardImage), "GetClipboardImage");
}

// }}}

// Texture, tex data stored in GPU memory (VRAM) [RESOURCE] {{{

static const char rlua_texture_mt[] = "raylib.Texture";


static Texture *rluaTextureCheck(lua_State *L, int index)
{
    return (Texture *)luaL_checkudata(L, index, rlua_texture_mt);
}

static void rluaTexturePush(lua_State *L, Texture texture)
{
    Texture *ud = (Texture *)lua_newuserdatauv(L, sizeof(Texture), 2);
    *ud = texture;
    luaL_setmetatable(L, rlua_texture_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaTexturePushBorrowed(lua_State *L, Texture texture, int parentIndex)
{
    Texture *ud = (Texture *)lua_newuserdatauv(L, sizeof(Texture), 2);
    *ud = texture;
    luaL_setmetatable(L, rlua_texture_mt);
    rluaSetOwnership(L, -1, 0);

    if (parentIndex != 0)
    {
        lua_pushvalue(L, parentIndex);
        lua_setiuservalue(L, -2, 2);
    }
}

static int rluaTexture_ParentValid(lua_State *L, int index)
{
    if (rluaGetOwnership(L, index) != 0) return 1;
    if (lua_getiuservalue(L, index, 2) == LUA_TNIL)
    {
        lua_pop(L, 1);
        return 1;
    }

    RenderTexture *parent = luaL_testudata(L, -1, rlua_rendertexture_mt);
    int valid = (parent != NULL) &&
                (rluaGetOwnership(L, -1) != 0) &&
                IsRenderTextureValid(*parent);

    lua_pop(L, 1);
    return valid;
}

static int rluaTexture_IsBorrowed(lua_State *L, int index)
{
    int type = lua_getiuservalue(L, index, 2);
    lua_pop(L, 1);
    return type != LUA_TNIL;
}

static Texture *rluaTexture_CheckUsable(lua_State *L, int index)
{
    Texture *texture = rluaTextureCheck(L, index);

    if (!rluaTexture_ParentValid(L, index))
    {
        luaL_error(L, "invalid parent RenderTexture");
        return NULL;
    }

    return texture;
}

static void rluaTexture_Release(lua_State *L, int index)
{
    Texture *texture = luaL_testudata(L, index, rlua_texture_mt);
    if (texture == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadTexture(*texture);
    *texture = (Texture){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadTexture(lua_State *L);

// metamethods {{{

static int rluaTextureToString(lua_State *L)
{
    Texture *texture = rluaTexture_CheckUsable(L, 1);
    lua_pushfstring(
        L,
        "Texture(id=%d, width=%d, height=%d, mipmaps=%d, format=%d)",
        (int)texture->id,
        texture->width,
        texture->height,
        texture->mipmaps,
        texture->format
    );
    return 1;
}

static int rluaTextureIndex(lua_State *L)
{
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "isValid") == 0)
    {
        lua_pushcfunction(L, rluaIsTextureValid);
        return 1;
    }

    Texture *texture = rluaTexture_CheckUsable(L, 1);

    if (strcmp(field, "id") == 0) lua_pushinteger(L, (lua_Integer)texture->id);
    else if (strcmp(field, "width") == 0) lua_pushinteger(L, texture->width);
    else if (strcmp(field, "height") == 0) lua_pushinteger(L, texture->height);
    else if (strcmp(field, "mipmaps") == 0) lua_pushinteger(L, texture->mipmaps);
    else if (strcmp(field, "format") == 0) lua_pushinteger(L, texture->format);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadTexture);
    else if (strcmp(field, "update") == 0) lua_pushcfunction(L, rluaUpdateTexture);
    else if (strcmp(field, "updateRec") == 0) lua_pushcfunction(L, rluaUpdateTextureRec);
    else if (strcmp(field, "genMipmaps") == 0) lua_pushcfunction(L, rluaGenTextureMipmaps);
    else if (strcmp(field, "setFilter") == 0) lua_pushcfunction(L, rluaSetTextureFilter);
    else if (strcmp(field, "setWrap") == 0) lua_pushcfunction(L, rluaSetTextureWrap);
    else if (strcmp(field, "draw") == 0) lua_pushcfunction(L, rluaDrawTexture);
    else if (strcmp(field, "drawV") == 0) lua_pushcfunction(L, rluaDrawTextureV);
    else if (strcmp(field, "drawEx") == 0) lua_pushcfunction(L, rluaDrawTextureEx);
    else if (strcmp(field, "drawRec") == 0) lua_pushcfunction(L, rluaDrawTextureRec);
    else if (strcmp(field, "drawPro") == 0) lua_pushcfunction(L, rluaDrawTexturePro);
    else if (strcmp(field, "drawNPatch") == 0) lua_pushcfunction(L, rluaDrawTextureNPatch);
    else lua_pushnil(L);

    return 1;
}

static int rluaTextureGc(lua_State *L)
{
    rluaTexture_Release(L, 1);
    return 0;
}

// }}}

// methods {{{

static int rluaUnloadTexture(lua_State *L)
{
    rluaTextureCheck(L, 1);

    if ((rluaGetOwnership(L, 1) == 0) && rluaTexture_IsBorrowed(L, 1))
    {
        return luaL_error(L, "attempt to unload borrowed Texture");
    }

    rluaTexture_Release(L, 1);
    return 0;
}

// }}}

// functions {{{

static Texture *rluaTexture_CheckValid(lua_State *L, int index)
{
    Texture *texture = rluaTexture_CheckUsable(L, index);

    if (!IsTextureValid(*texture))
    {
        luaL_error(L, "invalid Texture");
        return NULL;
    }

    return texture;
}

static void rluaTexture_PushOrNil(lua_State *L, Texture texture)
{
    if (!IsTextureValid(texture))
    {
        UnloadTexture(texture);
        lua_pushnil(L);
        return;
    }

    rluaTexturePush(L, texture);
}

static NPatchInfo rluaNPatchInfoCheck(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);

    NPatchInfo info = { 0 };

    lua_getfield(L, index, "source");
    info.source = rluaRectangleCheck(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "left");
    info.left = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "top");
    info.top = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "right");
    info.right = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "bottom");
    info.bottom = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "layout");
    info.layout = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    return info;
}

// loading {{{

static int rluaLoadTexture(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    rluaTexture_PushOrNil(L, LoadTexture(fileName));
    return 1;
}

static int rluaLoadTextureFromImage(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);

    if (!IsImageValid(*image))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaTexture_PushOrNil(L, LoadTextureFromImage(*image));
    return 1;
}

static int rluaLoadTextureCubemap(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);
    int layout = (int)luaL_checkinteger(L, 2);

    if (!IsImageValid(*image))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaTexture_PushOrNil(L, LoadTextureCubemap(*image, layout));
    return 1;
}

static int rluaIsTextureValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTextureCheck(L, 1);

    if (!rluaTexture_ParentValid(L, 1))
    {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushboolean(L, IsTextureValid(*texture));
    return 1;
}

static int rluaUpdateTexture(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    size_t dataLen = 0;
    const char *pixels = luaL_checklstring(L, 2, &dataLen);
    int expected = GetPixelDataSize(texture->width, texture->height, texture->format);

    if ((expected > 0) && ((int)dataLen < expected))
    {
        return luaL_error(L, "pixel value is too small");
    }

    UpdateTexture(*texture, pixels);
    return 0;
}

static int rluaUpdateTextureRec(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    Rectangle rec = rluaRectangleCheck(L, 2);
    size_t dataLen = 0;
    const char *pixels = luaL_checklstring(L, 3, &dataLen);
    int expected = GetPixelDataSize((int)rec.width, (int)rec.height, texture->format);

    if ((expected > 0) && ((int)dataLen < expected))
    {
        return luaL_error(L, "pixel value is too small");
    }

    UpdateTextureRec(*texture, rec, pixels);
    return 0;
}

// }}}

// configuration {{{

static int rluaGenTextureMipmaps(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    GenTextureMipmaps(texture);
    return 0;
}

static int rluaSetTextureFilter(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    SetTextureFilter(*texture, (int)luaL_checkinteger(L, 2));
    return 0;
}

static int rluaSetTextureWrap(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    SetTextureWrap(*texture, (int)luaL_checkinteger(L, 2));
    return 0;
}

// }}}

// drawing {{{

static int rluaDrawTexture(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    int posX = (int)luaL_checkinteger(L, 2);
    int posY = (int)luaL_checkinteger(L, 3);
    Color tint = rluaColorCheck(L, 4);
    DrawTexture(*texture, posX, posY, tint);
    return 0;
}

static int rluaDrawTextureV(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    Vector2 position = rluaVector2Check(L, 2);
    Color tint = rluaColorCheck(L, 3);
    DrawTextureV(*texture, position, tint);
    return 0;
}

static int rluaDrawTextureEx(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    Vector2 position = rluaVector2Check(L, 2);
    float rotation = (float)luaL_checknumber(L, 3);
    float scale = (float)luaL_checknumber(L, 4);
    Color tint = rluaColorCheck(L, 5);
    DrawTextureEx(*texture, position, rotation, scale, tint);
    return 0;
}

static int rluaDrawTextureRec(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    Rectangle rec = rluaRectangleCheck(L, 2);
    Vector2 position = rluaVector2Check(L, 3);
    Color tint = rluaColorCheck(L, 4);
    DrawTextureRec(*texture, rec, position, tint);
    return 0;
}

static int rluaDrawTexturePro(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    Rectangle srcRec = rluaRectangleCheck(L, 2);
    Rectangle dstRec = rluaRectangleCheck(L, 3);
    Vector2 origin = rluaVector2Check(L, 4);
    float rotation = (float)luaL_checknumber(L, 5);
    Color tint = rluaColorCheck(L, 6);
    DrawTexturePro(*texture, srcRec, dstRec, origin, rotation, tint);
    return 0;
}

static int rluaDrawTextureNPatch(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Texture *texture = rluaTexture_CheckValid(L, 1);
    NPatchInfo nPatchInfo = rluaNPatchInfoCheck(L, 2);
    Rectangle dstRec = rluaRectangleCheck(L, 3);
    Vector2 origin = rluaVector2Check(L, 4);
    float rotation = (float)luaL_checknumber(L, 5);
    Color tint = rluaColorCheck(L, 6);
    DrawTextureNPatch(*texture, nPatchInfo, dstRec, origin, rotation, tint);
    return 0;
}

// }}}

static int rluaGetPixelDataSize(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    int format = (int)luaL_checkinteger(L, 3);
    lua_pushinteger(L, GetPixelDataSize(width, height, format));
    return 1;
}

// }}}

static void rluaTextureRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_texture_mt))
    {
        REG(lua_pushcfunction(L, rluaTextureToString), "__tostring");
        REG(lua_pushcfunction(L, rluaTextureIndex), "__index");
        REG(lua_pushcfunction(L, rluaTextureGc), "__gc");
        REG(lua_pushcfunction(L, rluaTextureGc), "__close");
    }

    lua_pushstring(L, "Texture");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaLoadTexture), "LoadTexture");
    REG(lua_pushcfunction(L, rluaLoadTextureFromImage), "LoadTextureFromImage");
    REG(lua_pushcfunction(L, rluaLoadTextureCubemap), "LoadTextureCubemap");
    REG(lua_pushcfunction(L, rluaGetPixelDataSize), "GetPixelDataSize");
}

// }}}

// RenderTexture, framebuffer and texture attachments [RESOURCE] {{{


static RenderTexture *rluaRenderTextureCheck(lua_State *L, int index)
{
    return (RenderTexture *)luaL_checkudata(L, index, rlua_rendertexture_mt);
}

static void rluaRenderTexturePush(lua_State *L, RenderTexture target)
{
    RenderTexture *ud = (RenderTexture *)lua_newuserdatauv(L, sizeof(RenderTexture), 1);
    *ud = target;
    luaL_setmetatable(L, rlua_rendertexture_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaRenderTexture_PushOrNil(lua_State *L, RenderTexture target)
{
    if (!IsRenderTextureValid(target))
    {
        UnloadRenderTexture(target);
        lua_pushnil(L);
        return;
    }

    rluaRenderTexturePush(L, target);
}

static void rluaRenderTexture_Release(lua_State *L, int index)
{
    RenderTexture *target = luaL_testudata(L, index, rlua_rendertexture_mt);
    if (target == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadRenderTexture(*target);
    *target = (RenderTexture){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadRenderTexture(lua_State *L);

// metamethods {{{

static int rluaRenderTextureToString(lua_State *L)
{
    RenderTexture *target = rluaRenderTextureCheck(L, 1);
    lua_pushfstring(
        L,
        "RenderTexture(id=%d, width=%d, height=%d)",
        (int)target->id,
        target->texture.width,
        target->texture.height
    );
    return 1;
}

static int rluaRenderTextureIndex(lua_State *L)
{
    RenderTexture *target = rluaRenderTextureCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "id") == 0) lua_pushinteger(L, (lua_Integer)target->id);
    else if (strcmp(field, "texture") == 0) rluaTexturePushBorrowed(L, target->texture, 1);
    else if (strcmp(field, "depth") == 0) rluaTexturePushBorrowed(L, target->depth, 1);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadRenderTexture);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsRenderTextureValid);
    else lua_pushnil(L);

    return 1;
}

static int rluaRenderTextureGc(lua_State *L)
{
    rluaRenderTexture_Release(L, 1);
    return 0;
}

// }}}

// methods {{{

static int rluaUnloadRenderTexture(lua_State *L)
{
    rluaRenderTextureCheck(L, 1);
    rluaRenderTexture_Release(L, 1);
    return 0;
}

// }}}

// functions {{{

static RenderTexture *rluaRenderTexture_CheckValid(lua_State *L, int index)
{
    RenderTexture *target = rluaRenderTextureCheck(L, index);

    if (!IsRenderTextureValid(*target))
    {
        luaL_error(L, "invalid RenderTexture");
        return NULL;
    }

    return target;
}

static int rluaLoadRenderTexture(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);

    if ((width <= 0) || (height <= 0))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaRenderTexture_PushOrNil(L, LoadRenderTexture(width, height));
    return 1;
}

static int rluaLoadRenderTextureEx(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    int format = (int)luaL_checkinteger(L, 3);

    if ((width <= 0) || (height <= 0))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaRenderTexture_PushOrNil(L, LoadRenderTextureEx(width, height, format));
    return 1;
}

static int rluaIsRenderTextureValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    RenderTexture *target = rluaRenderTextureCheck(L, 1);
    lua_pushboolean(L, IsRenderTextureValid(*target));
    return 1;
}

// }}}

static void rluaRenderTextureRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_rendertexture_mt))
    {
        REG(lua_pushcfunction(L, rluaRenderTextureToString), "__tostring");
        REG(lua_pushcfunction(L, rluaRenderTextureIndex), "__index");
        REG(lua_pushcfunction(L, rluaRenderTextureGc), "__gc");
        REG(lua_pushcfunction(L, rluaRenderTextureGc), "__close");
    }

    lua_pushstring(L, "RenderTexture");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaLoadRenderTexture), "LoadRenderTexture");
    REG(lua_pushcfunction(L, rluaLoadRenderTextureEx), "LoadRenderTextureEx");
}

// }}}

// Font, font texture and glyph data [RESOURCE] {{{

static const char rlua_font_mt[] = "raylib.Font";


static Font *rluaFontCheck(lua_State *L, int index)
{
    return (Font *)luaL_checkudata(L, index, rlua_font_mt);
}

static void rluaFontPush(lua_State *L, Font font)
{
    Font *ud = (Font *)lua_newuserdatauv(L, sizeof(Font), 1);
    *ud = font;
    luaL_setmetatable(L, rlua_font_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaFontPushBorrowed(lua_State *L, Font font)
{
    Font *ud = (Font *)lua_newuserdatauv(L, sizeof(Font), 1);
    *ud = font;
    luaL_setmetatable(L, rlua_font_mt);
    rluaSetOwnership(L, -1, 0);
}

static void rluaFont_Release(lua_State *L, int index)
{
    Font *font = luaL_testudata(L, index, rlua_font_mt);
    if (font == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadFont(*font);
    *font = (Font){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadFont(lua_State *L);

// metamethods {{{

static int rluaFontToString(lua_State *L)
{
    Font *font = rluaFontCheck(L, 1);
    lua_pushfstring(
        L,
        "Font(baseSize=%d, glyphCount=%d, glyphPadding=%d)",
        font->baseSize,
        font->glyphCount,
        font->glyphPadding
    );
    return 1;
}

static int rluaFontIndex(lua_State *L)
{
    Font *font = rluaFontCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "baseSize") == 0) lua_pushinteger(L, font->baseSize);
    else if (strcmp(field, "glyphCount") == 0) lua_pushinteger(L, font->glyphCount);
    else if (strcmp(field, "glyphPadding") == 0) lua_pushinteger(L, font->glyphPadding);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadFont);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsFontValid);
    else if (strcmp(field, "exportAsCode") == 0) lua_pushcfunction(L, rluaExportFontAsCode);
    else if (strcmp(field, "drawTextEx") == 0) lua_pushcfunction(L, rluaDrawTextEx);
    else if (strcmp(field, "drawTextPro") == 0) lua_pushcfunction(L, rluaDrawTextPro);
    else if (strcmp(field, "drawTextCodepoint") == 0) lua_pushcfunction(L, rluaDrawTextCodepoint);
    else if (strcmp(field, "drawTextCodepoints") == 0) lua_pushcfunction(L, rluaDrawTextCodepoints);
    else if (strcmp(field, "measureTextEx") == 0) lua_pushcfunction(L, rluaMeasureTextEx);
    else if (strcmp(field, "measureTextCodepoints") == 0) lua_pushcfunction(L, rluaMeasureTextCodepoints);
    else if (strcmp(field, "getGlyphIndex") == 0) lua_pushcfunction(L, rluaGetGlyphIndex);
    else if (strcmp(field, "getGlyphInfo") == 0) lua_pushcfunction(L, rluaGetGlyphInfo);
    else if (strcmp(field, "getGlyphAtlasRec") == 0) lua_pushcfunction(L, rluaGetGlyphAtlasRec);
    else lua_pushnil(L);

    return 1;
}

static int rluaFontGc(lua_State *L)
{
    rluaFont_Release(L, 1);
    return 0;
}

// }}}

// methods {{{

static int rluaUnloadFont(lua_State *L)
{
    rluaFontCheck(L, 1);
    rluaFont_Release(L, 1);
    return 0;
}

// }}}

// functions {{{

static Font *rluaFont_CheckValid(lua_State *L, int index)
{
    Font *font = rluaFontCheck(L, index);

    if (!IsFontValid(*font))
    {
        luaL_error(L, "invalid Font");
        return NULL;
    }

    return font;
}

static int rluaFont_IsDefault(Font font)
{
    Font def = GetFontDefault();
    return (font.texture.id == def.texture.id) && (font.glyphs == def.glyphs);
}

static void rluaFont_PushOrNil(lua_State *L, Font font)
{
    if (!IsFontValid(font) || rluaFont_IsDefault(font))
    {
        lua_pushnil(L);
        return;
    }

    rluaFontPush(L, font);
}

static int rluaGetFontDefault(lua_State *L)
{
    Font font = GetFontDefault();

    if (!IsFontValid(font))
    {
        lua_pushnil(L);
        return 1;
    }

    rluaFontPushBorrowed(L, font);
    return 1;
}

static int rluaLoadFont(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    rluaFont_PushOrNil(L, LoadFont(fileName));
    return 1;
}

static int rluaLoadFontEx(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    int fontSize = (int)luaL_checkinteger(L, 2);
    int codepointCount = 0;
    int *codepoints = NULL;

    if (!lua_isnoneornil(L, 3))
    {
        codepoints = rluaIntArrayCheck(L, 3, &codepointCount);
    }

    rluaFont_PushOrNil(L, LoadFontEx(fileName, fontSize, codepoints, codepointCount));
    return 1;
}

static int rluaLoadFontFromImage(lua_State *L)
{
    Image *image = rluaImageCheck(L, 1);

    if (!IsImageValid(*image))
    {
        lua_pushnil(L);
        return 1;
    }

    Color key = rluaColorCheck(L, 2);
    int firstChar = (int)luaL_checkinteger(L, 3);
    rluaFont_PushOrNil(L, LoadFontFromImage(*image, key, firstChar));
    return 1;
}

static int rluaLoadFontFromMemory(lua_State *L)
{
    const char *fileType = luaL_checkstring(L, 1);
    size_t dataLen = 0;
    const unsigned char *fileData = (const unsigned char *)luaL_checklstring(L, 2, &dataLen);
    int fontSize = (int)luaL_checkinteger(L, 3);
    int codepointCount = 0;
    int *codepoints = NULL;

    if (!lua_isnoneornil(L, 4))
    {
        codepoints = rluaIntArrayCheck(L, 4, &codepointCount);
    }

    rluaFont_PushOrNil(L, LoadFontFromMemory(fileType, fileData, (int)dataLen, fontSize, codepoints, codepointCount));
    return 1;
}

static int rluaIsFontValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFontCheck(L, 1);
    lua_pushboolean(L, IsFontValid(*font));
    return 1;
}

static int rluaExportFontAsCode(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    const char *fileName = luaL_checkstring(L, 2);
    lua_pushboolean(L, ExportFontAsCode(*font, fileName));
    return 1;
}

// }}}

// GlyphInfo array, owned glyph data [RESOURCE] {{{

static const char rlua_glyphinfo_array_mt[] = "raylib.GlyphInfoArray";

typedef struct rluaGlyphInfoArray {
    GlyphInfo *glyphs;
    int count;
} rluaGlyphInfoArray;


static rluaGlyphInfoArray *rluaGlyphInfoArrayCheck(lua_State *L, int index)
{
    return (rluaGlyphInfoArray *)luaL_checkudata(L, index, rlua_glyphinfo_array_mt);
}

static void rluaGlyphInfoArrayPush(lua_State *L, GlyphInfo *glyphs, int count)
{
    rluaGlyphInfoArray *ud = (rluaGlyphInfoArray *)lua_newuserdatauv(L, sizeof(rluaGlyphInfoArray), 1);
    ud->glyphs = glyphs;
    ud->count = count;
    luaL_setmetatable(L, rlua_glyphinfo_array_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaGlyphInfoArray_Release(lua_State *L, int index)
{
    rluaGlyphInfoArray *array = luaL_testudata(L, index, rlua_glyphinfo_array_mt);
    if (array == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadFontData(array->glyphs, array->count);
    array->glyphs = NULL;
    array->count = 0;
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadFontData(lua_State *L);

static int rluaGlyphInfoArrayToString(lua_State *L)
{
    rluaGlyphInfoArray *array = rluaGlyphInfoArrayCheck(L, 1);
    lua_pushfstring(L, "GlyphInfoArray(count=%d)", array->count);
    return 1;
}

static int rluaGlyphInfoArrayIndex(lua_State *L)
{
    rluaGlyphInfoArray *array = rluaGlyphInfoArrayCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "count") == 0) lua_pushinteger(L, array->count);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadFontData);
    else lua_pushnil(L);

    return 1;
}

static int rluaGlyphInfoArrayGc(lua_State *L)
{
    rluaGlyphInfoArray_Release(L, 1);
    return 0;
}

static int rluaUnloadFontData(lua_State *L)
{
    rluaGlyphInfoArrayCheck(L, 1);
    rluaGlyphInfoArray_Release(L, 1);
    return 0;
}

static int rluaLoadFontData(lua_State *L)
{
    size_t dataLen = 0;
    const unsigned char *fileData = (const unsigned char *)luaL_checklstring(L, 1, &dataLen);
    int fontSize = (int)luaL_checkinteger(L, 2);
    int codepointCount = 0;
    int *codepoints = NULL;

    if (!lua_isnoneornil(L, 3))
    {
        codepoints = rluaIntArrayCheck(L, 3, &codepointCount);
    }

    int type = (int)luaL_checkinteger(L, 4);
    int glyphCount = 0;
    GlyphInfo *glyphs = LoadFontData(fileData, (int)dataLen, fontSize, codepoints, codepointCount, type, &glyphCount);

    if (glyphs == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    rluaGlyphInfoArrayPush(L, glyphs, glyphCount);
    return 1;
}

static int rluaGenImageFontAtlas(lua_State *L)
{
    rluaGlyphInfoArray *glyphs = rluaGlyphInfoArrayCheck(L, 1);
    int fontSize = (int)luaL_checkinteger(L, 2);
    int padding = (int)luaL_checkinteger(L, 3);
    int packMethod = (int)luaL_checkinteger(L, 4);
    Rectangle *glyphRecs = NULL;

    Image atlas = GenImageFontAtlas(glyphs->glyphs, &glyphRecs, glyphs->count, fontSize, padding, packMethod);

    if (!IsImageValid(atlas))
    {
        MemFree(glyphRecs);
        lua_pushnil(L);
        return 1;
    }

    rluaImagePush(L, atlas);
    lua_createtable(L, glyphs->count, 0);

    for (int i = 0; i < glyphs->count; i++)
    {
        rluaRectanglePush(L, glyphRecs[i]);
        lua_rawseti(L, -2, i + 1);
    }

    MemFree(glyphRecs);
    return 2;
}

static void rluaGlyphInfoArrayRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_glyphinfo_array_mt))
    {
        REG(lua_pushcfunction(L, rluaGlyphInfoArrayToString), "__tostring");
        REG(lua_pushcfunction(L, rluaGlyphInfoArrayIndex), "__index");
        REG(lua_pushcfunction(L, rluaGlyphInfoArrayGc), "__gc");
        REG(lua_pushcfunction(L, rluaGlyphInfoArrayGc), "__close");
    }

    lua_pushstring(L, "GlyphInfoArray");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaLoadFontData), "LoadFontData");
    REG(lua_pushcfunction(L, rluaGenImageFontAtlas), "GenImageFontAtlas");
}

// }}}

static void rluaFontRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_font_mt))
    {
        REG(lua_pushcfunction(L, rluaFontToString), "__tostring");
        REG(lua_pushcfunction(L, rluaFontIndex), "__index");
        REG(lua_pushcfunction(L, rluaFontGc), "__gc");
        REG(lua_pushcfunction(L, rluaFontGc), "__close");
    }

    lua_pushstring(L, "Font");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaGetFontDefault), "GetFontDefault");
    REG(lua_pushcfunction(L, rluaLoadFont), "LoadFont");
    REG(lua_pushcfunction(L, rluaLoadFontEx), "LoadFontEx");
    REG(lua_pushcfunction(L, rluaLoadFontFromImage), "LoadFontFromImage");
    REG(lua_pushcfunction(L, rluaLoadFontFromMemory), "LoadFontFromMemory");
}

// }}}

// Shader, GPU shader program [RESOURCE] {{{

static const char rlua_shader_mt[] = "raylib.Shader";


static Shader *rluaShaderCheck(lua_State *L, int index)
{
    return (Shader *)luaL_checkudata(L, index, rlua_shader_mt);
}

static void rluaShaderPush(lua_State *L, Shader shader)
{
    Shader *ud = (Shader *)lua_newuserdatauv(L, sizeof(Shader), 1);
    *ud = shader;
    luaL_setmetatable(L, rlua_shader_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaShader_Release(lua_State *L, int index)
{
    Shader *shader = luaL_testudata(L, index, rlua_shader_mt);
    if (shader == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadShader(*shader);
    *shader = (Shader){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadShader(lua_State *L);

static int rluaShaderToString(lua_State *L)
{
    Shader *shader = rluaShaderCheck(L, 1);
    lua_pushfstring(L, "Shader(id=%d)", (int)shader->id);
    return 1;
}

static int rluaShaderIndex(lua_State *L)
{
    Shader *shader = rluaShaderCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "id") == 0) lua_pushinteger(L, (lua_Integer)shader->id);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadShader);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsShaderValid);
    else if (strcmp(field, "getLocation") == 0) lua_pushcfunction(L, rluaGetShaderLocation);
    else if (strcmp(field, "getLocationAttrib") == 0) lua_pushcfunction(L, rluaGetShaderLocationAttrib);
    else if (strcmp(field, "setValue") == 0) lua_pushcfunction(L, rluaSetShaderValue);
    else if (strcmp(field, "setValueV") == 0) lua_pushcfunction(L, rluaSetShaderValueV);
    else if (strcmp(field, "setValueMatrix") == 0) lua_pushcfunction(L, rluaSetShaderValueMatrix);
    else if (strcmp(field, "setValueTexture") == 0) lua_pushcfunction(L, rluaSetShaderValueTexture);
    else lua_pushnil(L);

    return 1;
}

static int rluaShaderGc(lua_State *L)
{
    rluaShader_Release(L, 1);
    return 0;
}

static int rluaUnloadShader(lua_State *L)
{
    rluaShaderCheck(L, 1);
    rluaShader_Release(L, 1);
    return 0;
}

static Shader *rluaShader_CheckValid(lua_State *L, int index)
{
    Shader *shader = rluaShaderCheck(L, index);

    if (!IsShaderValid(*shader))
    {
        luaL_error(L, "invalid Shader");
        return NULL;
    }

    return shader;
}

static void rluaShader_PushOrNil(lua_State *L, Shader shader)
{
    if (!IsShaderValid(shader))
    {
        UnloadShader(shader);
        lua_pushnil(L);
        return;
    }

    rluaShaderPush(L, shader);
}

static int rluaLoadShader(lua_State *L)
{
    const char *vsFileName = luaL_optstring(L, 1, NULL);
    const char *fsFileName = luaL_optstring(L, 2, NULL);
    rluaShader_PushOrNil(L, LoadShader(vsFileName, fsFileName));
    return 1;
}

static int rluaLoadShaderFromMemory(lua_State *L)
{
    const char *vsCode = luaL_optstring(L, 1, NULL);
    const char *fsCode = luaL_optstring(L, 2, NULL);
    rluaShader_PushOrNil(L, LoadShaderFromMemory(vsCode, fsCode));
    return 1;
}

static int rluaIsShaderValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Shader *shader = rluaShaderCheck(L, 1);
    lua_pushboolean(L, IsShaderValid(*shader));
    return 1;
}

static int rluaGetShaderLocation(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Shader *shader = rluaShader_CheckValid(L, 1);
    const char *uniformName = luaL_checkstring(L, 2);
    lua_pushinteger(L, GetShaderLocation(*shader, uniformName));
    return 1;
}

static int rluaGetShaderLocationAttrib(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Shader *shader = rluaShader_CheckValid(L, 1);
    const char *attribName = luaL_checkstring(L, 2);
    lua_pushinteger(L, GetShaderLocationAttrib(*shader, attribName));
    return 1;
}

static int rluaSetShaderValue(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Shader *shader = rluaShader_CheckValid(L, 1);
    int locIndex = (int)luaL_checkinteger(L, 2);
    int uniformType = (int)luaL_checkinteger(L, 4);

    switch (uniformType)
    {
        case SHADER_UNIFORM_FLOAT:
        {
            float v = (float)luaL_checknumber(L, 3);
            SetShaderValue(*shader, locIndex, &v, uniformType);
            break;
        }
        case SHADER_UNIFORM_VEC2:
        {
            Vector2 v = rluaVector2Check(L, 3);
            SetShaderValue(*shader, locIndex, &v, uniformType);
            break;
        }
        case SHADER_UNIFORM_VEC3:
        {
            Vector3 v = rluaVector3Check(L, 3);
            SetShaderValue(*shader, locIndex, &v, uniformType);
            break;
        }
        case SHADER_UNIFORM_VEC4:
        {
            Vector4 v = rluaVector4Check(L, 3);
            SetShaderValue(*shader, locIndex, &v, uniformType);
            break;
        }
        case SHADER_UNIFORM_INT:
        {
            int v = (int)luaL_checkinteger(L, 3);
            SetShaderValue(*shader, locIndex, &v, uniformType);
            break;
        }
        case SHADER_UNIFORM_IVEC2:
        {
            Vector2 v = rluaVector2Check(L, 3);
            int iv[2] = { (int)v.x, (int)v.y };
            SetShaderValue(*shader, locIndex, iv, uniformType);
            break;
        }
        case SHADER_UNIFORM_IVEC3:
        {
            Vector3 v = rluaVector3Check(L, 3);
            int iv[3] = { (int)v.x, (int)v.y, (int)v.z };
            SetShaderValue(*shader, locIndex, iv, uniformType);
            break;
        }
        case SHADER_UNIFORM_IVEC4:
        {
            Vector4 v = rluaVector4Check(L, 3);
            int iv[4] = { (int)v.x, (int)v.y, (int)v.z, (int)v.w };
            SetShaderValue(*shader, locIndex, iv, uniformType);
            break;
        }
        case SHADER_UNIFORM_UINT:
        {
            unsigned int v = (unsigned int)luaL_checkinteger(L, 3);
            SetShaderValue(*shader, locIndex, &v, uniformType);
            break;
        }
        case SHADER_UNIFORM_UIVEC2:
        {
            Vector2 v = rluaVector2Check(L, 3);
            unsigned int iv[2] = { (unsigned int)v.x, (unsigned int)v.y };
            SetShaderValue(*shader, locIndex, iv, uniformType);
            break;
        }
        case SHADER_UNIFORM_UIVEC3:
        {
            Vector3 v = rluaVector3Check(L, 3);
            unsigned int iv[3] = { (unsigned int)v.x, (unsigned int)v.y, (unsigned int)v.z };
            SetShaderValue(*shader, locIndex, iv, uniformType);
            break;
        }
        case SHADER_UNIFORM_UIVEC4:
        {
            Vector4 v = rluaVector4Check(L, 3);
            unsigned int iv[4] = { (unsigned int)v.x, (unsigned int)v.y, (unsigned int)v.z, (unsigned int)v.w };
            SetShaderValue(*shader, locIndex, iv, uniformType);
            break;
        }
        case SHADER_UNIFORM_SAMPLER2D:
        {
            Texture *texture = rluaTexture_CheckValid(L, 3);
            SetShaderValueTexture(*shader, locIndex, *texture);
            break;
        }
        default:
            return luaL_error(L, "unsupported shader uniform type %d", uniformType);
    }

    return 0;
}

static int rluaSetShaderValueV(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Shader *shader = rluaShader_CheckValid(L, 1);
    int locIndex = (int)luaL_checkinteger(L, 2);
    int uniformType = (int)luaL_checkinteger(L, 4);
    int count = (int)luaL_len(L, 3);

    if (count <= 0) return luaL_argerror(L, 3, "non-empty table expected");

    switch (uniformType)
    {
        case SHADER_UNIFORM_FLOAT:
        {
            float *values = (float *)lua_newuserdatauv(L, sizeof(float)*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                values[i] = (float)luaL_checknumber(L, -1);
                lua_pop(L, 1);
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_VEC2:
        {
            Vector2 *values = (Vector2 *)lua_newuserdatauv(L, sizeof(Vector2)*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                values[i] = rluaVector2Check(L, -1);
                lua_pop(L, 1);
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_VEC3:
        {
            Vector3 *values = (Vector3 *)lua_newuserdatauv(L, sizeof(Vector3)*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                values[i] = rluaVector3Check(L, -1);
                lua_pop(L, 1);
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_VEC4:
        {
            Vector4 *values = (Vector4 *)lua_newuserdatauv(L, sizeof(Vector4)*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                values[i] = rluaVector4Check(L, -1);
                lua_pop(L, 1);
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_INT:
        {
            int *values = (int *)lua_newuserdatauv(L, sizeof(int)*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                values[i] = (int)luaL_checkinteger(L, -1);
                lua_pop(L, 1);
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_IVEC2:
        {
            int *values = (int *)lua_newuserdatauv(L, sizeof(int)*2*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                Vector2 v = rluaVector2Check(L, -1);
                lua_pop(L, 1);
                values[i*2 + 0] = (int)v.x;
                values[i*2 + 1] = (int)v.y;
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_IVEC3:
        {
            int *values = (int *)lua_newuserdatauv(L, sizeof(int)*3*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                Vector3 v = rluaVector3Check(L, -1);
                lua_pop(L, 1);
                values[i*3 + 0] = (int)v.x;
                values[i*3 + 1] = (int)v.y;
                values[i*3 + 2] = (int)v.z;
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_IVEC4:
        {
            int *values = (int *)lua_newuserdatauv(L, sizeof(int)*4*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                Vector4 v = rluaVector4Check(L, -1);
                lua_pop(L, 1);
                values[i*4 + 0] = (int)v.x;
                values[i*4 + 1] = (int)v.y;
                values[i*4 + 2] = (int)v.z;
                values[i*4 + 3] = (int)v.w;
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_UINT:
        {
            unsigned int *values = (unsigned int *)lua_newuserdatauv(L, sizeof(unsigned int)*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                values[i] = (unsigned int)luaL_checkinteger(L, -1);
                lua_pop(L, 1);
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_UIVEC2:
        {
            unsigned int *values = (unsigned int *)lua_newuserdatauv(L, sizeof(unsigned int)*2*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                Vector2 v = rluaVector2Check(L, -1);
                lua_pop(L, 1);
                values[i*2 + 0] = (unsigned int)v.x;
                values[i*2 + 1] = (unsigned int)v.y;
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_UIVEC3:
        {
            unsigned int *values = (unsigned int *)lua_newuserdatauv(L, sizeof(unsigned int)*3*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                Vector3 v = rluaVector3Check(L, -1);
                lua_pop(L, 1);
                values[i*3 + 0] = (unsigned int)v.x;
                values[i*3 + 1] = (unsigned int)v.y;
                values[i*3 + 2] = (unsigned int)v.z;
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        case SHADER_UNIFORM_UIVEC4:
        {
            unsigned int *values = (unsigned int *)lua_newuserdatauv(L, sizeof(unsigned int)*4*count, 0);
            for (int i = 0; i < count; i++)
            {
                lua_rawgeti(L, 3, i + 1);
                Vector4 v = rluaVector4Check(L, -1);
                lua_pop(L, 1);
                values[i*4 + 0] = (unsigned int)v.x;
                values[i*4 + 1] = (unsigned int)v.y;
                values[i*4 + 2] = (unsigned int)v.z;
                values[i*4 + 3] = (unsigned int)v.w;
            }
            SetShaderValueV(*shader, locIndex, values, uniformType, count);
            break;
        }
        default:
            return luaL_error(L, "unsupported shader uniform type %d", uniformType);
    }

    return 0;
}

static int rluaSetShaderValueMatrix(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Shader *shader = rluaShader_CheckValid(L, 1);
    int locIndex = (int)luaL_checkinteger(L, 2);
    Matrix mat = rluaMatrixCheck(L, 3);
    SetShaderValueMatrix(*shader, locIndex, mat);
    return 0;
}

static int rluaSetShaderValueTexture(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Shader *shader = rluaShader_CheckValid(L, 1);
    int locIndex = (int)luaL_checkinteger(L, 2);
    Texture *texture = rluaTexture_CheckValid(L, 3);
    SetShaderValueTexture(*shader, locIndex, *texture);
    return 0;
}

static void rluaShaderRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_shader_mt))
    {
        REG(lua_pushcfunction(L, rluaShaderToString), "__tostring");
        REG(lua_pushcfunction(L, rluaShaderIndex), "__index");
        REG(lua_pushcfunction(L, rluaShaderGc), "__gc");
        REG(lua_pushcfunction(L, rluaShaderGc), "__close");
    }

    lua_pushstring(L, "Shader");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaLoadShader), "LoadShader");
    REG(lua_pushcfunction(L, rluaLoadShaderFromMemory), "LoadShaderFromMemory");
}

// }}}

// VrStereoConfig, VR stereo rendering configuration [RESOURCE] {{{

static const char rlua_vrstereoconfig_mt[] = "raylib.VrStereoConfig";


static VrStereoConfig *rluaVrStereoConfigCheck(lua_State *L, int index)
{
    return (VrStereoConfig *)luaL_checkudata(L, index, rlua_vrstereoconfig_mt);
}

static void rluaVrStereoConfigPush(lua_State *L, VrStereoConfig config)
{
    VrStereoConfig *ud = (VrStereoConfig *)lua_newuserdatauv(L, sizeof(VrStereoConfig), 1);
    *ud = config;
    luaL_setmetatable(L, rlua_vrstereoconfig_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaVrStereoConfig_Release(lua_State *L, int index)
{
    VrStereoConfig *config = luaL_testudata(L, index, rlua_vrstereoconfig_mt);
    if (config == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadVrStereoConfig(*config);
    *config = (VrStereoConfig){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadVrStereoConfig(lua_State *L);

static int rluaVrStereoConfigToString(lua_State *L)
{
    (void)rluaVrStereoConfigCheck(L, 1);
    lua_pushliteral(L, "VrStereoConfig");
    return 1;
}

static int rluaVrStereoConfigIndex(lua_State *L)
{
    rluaVrStereoConfigCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadVrStereoConfig);
    else lua_pushnil(L);

    return 1;
}

static int rluaVrStereoConfigGc(lua_State *L)
{
    rluaVrStereoConfig_Release(L, 1);
    return 0;
}

static int rluaUnloadVrStereoConfig(lua_State *L)
{
    rluaVrStereoConfigCheck(L, 1);
    rluaVrStereoConfig_Release(L, 1);
    return 0;
}

static int rluaVrDeviceInfo_IntField(lua_State *L, int index, const char *name)
{
    index = lua_absindex(L, index);
    lua_getfield(L, index, name);
    int value = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    return value;
}

static float rluaVrDeviceInfo_FloatField(lua_State *L, int index, const char *name)
{
    index = lua_absindex(L, index);
    lua_getfield(L, index, name);
    float value = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);
    return value;
}

static void rluaVrDeviceInfo_FloatArray(lua_State *L, int index, const char *name, float *out, int count)
{
    index = lua_absindex(L, index);
    lua_getfield(L, index, name);
    luaL_checktype(L, -1, LUA_TTABLE);

    for (int i = 0; i < count; i++)
    {
        lua_rawgeti(L, -1, i + 1);
        out[i] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

    lua_pop(L, 1);
}

static VrDeviceInfo rluaVrDeviceInfoCheck(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);

    VrDeviceInfo device = { 0 };
    device.hResolution = rluaVrDeviceInfo_IntField(L, index, "hResolution");
    device.vResolution = rluaVrDeviceInfo_IntField(L, index, "vResolution");
    device.hScreenSize = rluaVrDeviceInfo_FloatField(L, index, "hScreenSize");
    device.vScreenSize = rluaVrDeviceInfo_FloatField(L, index, "vScreenSize");
    device.eyeToScreenDistance = rluaVrDeviceInfo_FloatField(L, index, "eyeToScreenDistance");
    device.lensSeparationDistance = rluaVrDeviceInfo_FloatField(L, index, "lensSeparationDistance");
    device.interpupillaryDistance = rluaVrDeviceInfo_FloatField(L, index, "interpupillaryDistance");
    rluaVrDeviceInfo_FloatArray(L, index, "lensDistortionValues", device.lensDistortionValues, 4);
    rluaVrDeviceInfo_FloatArray(L, index, "chromaAbCorrection", device.chromaAbCorrection, 4);
    return device;
}

static int rluaLoadVrStereoConfig(lua_State *L)
{
    VrDeviceInfo device = rluaVrDeviceInfoCheck(L, 1);
    rluaVrStereoConfigPush(L, LoadVrStereoConfig(device));
    return 1;
}

static void rluaVrStereoConfigRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_vrstereoconfig_mt))
    {
        REG(lua_pushcfunction(L, rluaVrStereoConfigToString), "__tostring");
        REG(lua_pushcfunction(L, rluaVrStereoConfigIndex), "__index");
        REG(lua_pushcfunction(L, rluaVrStereoConfigGc), "__gc");
        REG(lua_pushcfunction(L, rluaVrStereoConfigGc), "__close");
    }

    lua_pushstring(L, "VrStereoConfig");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaLoadVrStereoConfig), "LoadVrStereoConfig");
}

// }}}

// Models, meshes, materials and 3d shapes {{{

static BoundingBox rluaBoundingBoxCheck(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);

    BoundingBox box = { 0 };
    lua_getfield(L, index, "min");
    box.min = rluaVector3Check(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, index, "max");
    box.max = rluaVector3Check(L, -1);
    lua_pop(L, 1);
    return box;
}

static void rluaBoundingBoxPush(lua_State *L, BoundingBox box)
{
    lua_createtable(L, 0, 2);
    rluaVector3Push(L, box.min);
    lua_setfield(L, -2, "min");
    rluaVector3Push(L, box.max);
    lua_setfield(L, -2, "max");
}


static void rluaRayCollisionPush(lua_State *L, RayCollision collision)
{
    lua_createtable(L, 0, 4);
    REG(lua_pushboolean(L, collision.hit), "hit");
    REG(lua_pushnumber(L, collision.distance), "distance");
    rluaVector3Push(L, collision.point);
    lua_setfield(L, -2, "point");
    rluaVector3Push(L, collision.normal);
    lua_setfield(L, -2, "normal");
}



static const char rlua_mesh_mt[] = "raylib.Mesh";


static Mesh *rluaMeshCheck(lua_State *L, int index)
{
    return (Mesh *)luaL_checkudata(L, index, rlua_mesh_mt);
}

static void rluaMeshPush(lua_State *L, Mesh mesh)
{
    Mesh *ud = (Mesh *)lua_newuserdatauv(L, sizeof(Mesh), 1);
    *ud = mesh;
    luaL_setmetatable(L, rlua_mesh_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaMesh_Release(lua_State *L, int index)
{
    Mesh *mesh = luaL_testudata(L, index, rlua_mesh_mt);
    if (mesh == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadMesh(*mesh);
    *mesh = (Mesh){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadMesh(lua_State *L);

static int rluaMeshToString(lua_State *L)
{
    Mesh *mesh = rluaMeshCheck(L, 1);
    lua_pushfstring(L, "Mesh(vertexCount=%d, triangleCount=%d)", mesh->vertexCount, mesh->triangleCount);
    return 1;
}

static int rluaMeshIndex(lua_State *L)
{
    Mesh *mesh = rluaMeshCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "vertexCount") == 0) lua_pushinteger(L, mesh->vertexCount);
    else if (strcmp(field, "triangleCount") == 0) lua_pushinteger(L, mesh->triangleCount);
    else if (strcmp(field, "boneCount") == 0) lua_pushinteger(L, mesh->boneCount);
    else if (strcmp(field, "vaoId") == 0) lua_pushinteger(L, (lua_Integer)mesh->vaoId);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadMesh);
    else if (strcmp(field, "upload") == 0) lua_pushcfunction(L, rluaUploadMesh);
    else if (strcmp(field, "updateBuffer") == 0) lua_pushcfunction(L, rluaUpdateMeshBuffer);
    else if (strcmp(field, "draw") == 0) lua_pushcfunction(L, rluaDrawMesh);
    else if (strcmp(field, "drawInstanced") == 0) lua_pushcfunction(L, rluaDrawMeshInstanced);
    else if (strcmp(field, "getBoundingBox") == 0) lua_pushcfunction(L, rluaGetMeshBoundingBox);
    else if (strcmp(field, "genTangents") == 0) lua_pushcfunction(L, rluaGenMeshTangents);
    else if (strcmp(field, "export") == 0) lua_pushcfunction(L, rluaExportMesh);
    else if (strcmp(field, "exportAsCode") == 0) lua_pushcfunction(L, rluaExportMeshAsCode);
    else lua_pushnil(L);

    return 1;
}

static int rluaMeshGc(lua_State *L)
{
    rluaMesh_Release(L, 1);
    return 0;
}

static int rluaUnloadMesh(lua_State *L)
{
    rluaMeshCheck(L, 1);
    rluaMesh_Release(L, 1);
    return 0;
}

static const char rlua_model_mt[] = "raylib.Model";


static Model *rluaModelCheck(lua_State *L, int index)
{
    return (Model *)luaL_checkudata(L, index, rlua_model_mt);
}

static void rluaModelPush(lua_State *L, Model model)
{
    Model *ud = (Model *)lua_newuserdatauv(L, sizeof(Model), 1);
    *ud = model;
    luaL_setmetatable(L, rlua_model_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaModel_Release(lua_State *L, int index)
{
    Model *model = luaL_testudata(L, index, rlua_model_mt);
    if (model == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadModel(*model);
    *model = (Model){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadModel(lua_State *L);

static int rluaModelToString(lua_State *L)
{
    Model *model = rluaModelCheck(L, 1);
    lua_pushfstring(L, "Model(meshCount=%d, materialCount=%d)", model->meshCount, model->materialCount);
    return 1;
}

static int rluaModelIndex(lua_State *L)
{
    Model *model = rluaModelCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "meshCount") == 0) lua_pushinteger(L, model->meshCount);
    else if (strcmp(field, "materialCount") == 0) lua_pushinteger(L, model->materialCount);
    else if (strcmp(field, "transform") == 0) rluaMatrixPush(L, model->transform);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadModel);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsModelValid);
    else if (strcmp(field, "getBoundingBox") == 0) lua_pushcfunction(L, rluaGetModelBoundingBox);
    else if (strcmp(field, "draw") == 0) lua_pushcfunction(L, rluaDrawModel);
    else if (strcmp(field, "drawEx") == 0) lua_pushcfunction(L, rluaDrawModelEx);
    else if (strcmp(field, "drawWires") == 0) lua_pushcfunction(L, rluaDrawModelWires);
    else if (strcmp(field, "drawWiresEx") == 0) lua_pushcfunction(L, rluaDrawModelWiresEx);
    else if (strcmp(field, "setMeshMaterial") == 0) lua_pushcfunction(L, rluaSetModelMeshMaterial);
    else if (strcmp(field, "updateAnimation") == 0) lua_pushcfunction(L, rluaUpdateModelAnimation);
    else if (strcmp(field, "updateAnimationEx") == 0) lua_pushcfunction(L, rluaUpdateModelAnimationEx);
    else if (strcmp(field, "isAnimationValid") == 0) lua_pushcfunction(L, rluaIsModelAnimationValid);
    else lua_pushnil(L);

    return 1;
}

static int rluaModelGc(lua_State *L)
{
    rluaModel_Release(L, 1);
    return 0;
}

static int rluaUnloadModel(lua_State *L)
{
    rluaModelCheck(L, 1);
    rluaModel_Release(L, 1);
    return 0;
}


static const char rlua_material_mt[] = "raylib.Material";


static Material *rluaMaterialCheck(lua_State *L, int index)
{
    return (Material *)luaL_checkudata(L, index, rlua_material_mt);
}

static void rluaMaterialPush(lua_State *L, Material material)
{
    Material *ud = (Material *)lua_newuserdatauv(L, sizeof(Material), 2);
    *ud = material;
    luaL_setmetatable(L, rlua_material_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaMaterialPushBorrowed(lua_State *L, Material material, int parentIndex)
{
    Material *ud = (Material *)lua_newuserdatauv(L, sizeof(Material), 2);
    *ud = material;
    luaL_setmetatable(L, rlua_material_mt);
    rluaSetOwnership(L, -1, 0);

    if (parentIndex != 0)
    {
        lua_pushvalue(L, parentIndex);
        lua_setiuservalue(L, -2, 2);
    }
}

static int rluaMaterial_IsBorrowed(lua_State *L, int index)
{
    int type = lua_getiuservalue(L, index, 2);
    lua_pop(L, 1);
    return type != LUA_TNIL;
}

static void rluaMaterial_Release(lua_State *L, int index)
{
    Material *material = luaL_testudata(L, index, rlua_material_mt);
    if (material == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadMaterial(*material);
    *material = (Material){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadMaterial(lua_State *L);

static int rluaMaterialToString(lua_State *L)
{
    rluaMaterialCheck(L, 1);
    lua_pushliteral(L, "Material");
    return 1;
}

static int rluaMaterialIndex(lua_State *L)
{
    rluaMaterialCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadMaterial);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsMaterialValid);
    else if (strcmp(field, "setTexture") == 0) lua_pushcfunction(L, rluaSetMaterialTexture);
    else lua_pushnil(L);

    return 1;
}

static int rluaMaterialGc(lua_State *L)
{
    rluaMaterial_Release(L, 1);
    return 0;
}

static int rluaUnloadMaterial(lua_State *L)
{
    rluaMaterialCheck(L, 1);

    if ((rluaGetOwnership(L, 1) == 0) && rluaMaterial_IsBorrowed(L, 1))
    {
        return luaL_error(L, "attempt to unload borrowed Material");
    }

    rluaMaterial_Release(L, 1);
    return 0;
}


static const char rlua_materials_mt[] = "raylib.Materials";

typedef struct rluaMaterials {
    Material *materials;
    int count;
} rluaMaterials;

static rluaMaterials *rluaMaterialsCheck(lua_State *L, int index)
{
    return (rluaMaterials *)luaL_checkudata(L, index, rlua_materials_mt);
}

static void rluaMaterialsPush(lua_State *L, Material *materials, int count)
{
    rluaMaterials *ud = (rluaMaterials *)lua_newuserdatauv(L, sizeof(rluaMaterials), 1);
    ud->materials = materials;
    ud->count = count;
    luaL_setmetatable(L, rlua_materials_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaMaterials_Release(lua_State *L, int index)
{
    rluaMaterials *materials = luaL_testudata(L, index, rlua_materials_mt);
    if (materials == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    for (int i = 0; i < materials->count; i++) UnloadMaterial(materials->materials[i]);
    MemFree(materials->materials);
    materials->materials = NULL;
    materials->count = 0;
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadMaterials(lua_State *L);

static int rluaMaterialsToString(lua_State *L)
{
    rluaMaterials *materials = rluaMaterialsCheck(L, 1);
    lua_pushfstring(L, "Materials(count=%d)", materials->count);
    return 1;
}

static int rluaMaterialsIndex(lua_State *L)
{
    rluaMaterials *materials = rluaMaterialsCheck(L, 1);

    if (lua_type(L, 2) == LUA_TNUMBER)
    {
        int index = (int)luaL_checkinteger(L, 2);

        if ((index < 1) || (index > materials->count))
        {
            lua_pushnil(L);
            return 1;
        }

        rluaMaterialPushBorrowed(L, materials->materials[index - 1], 1);
        return 1;
    }

    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "count") == 0) lua_pushinteger(L, materials->count);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadMaterials);
    else lua_pushnil(L);

    return 1;
}

static int rluaMaterialsLen(lua_State *L)
{
    lua_pushinteger(L, rluaMaterialsCheck(L, 1)->count);
    return 1;
}

static int rluaMaterialsGc(lua_State *L)
{
    rluaMaterials_Release(L, 1);
    return 0;
}

static int rluaUnloadMaterials(lua_State *L)
{
    rluaMaterialsCheck(L, 1);
    rluaMaterials_Release(L, 1);
    return 0;
}

static const char rlua_modelanimation_mt[] = "raylib.ModelAnimation";

static int rluaModelAnimationIs(lua_State *L, int index)
{
    return luaL_testudata(L, index, rlua_modelanimation_mt) != NULL;
}

static ModelAnimation *rluaModelAnimationCheck(lua_State *L, int index)
{
    return (ModelAnimation *)luaL_checkudata(L, index, rlua_modelanimation_mt);
}

static void rluaModelAnimationPushBorrowed(lua_State *L, ModelAnimation anim, int parentIndex)
{
    ModelAnimation *ud = (ModelAnimation *)lua_newuserdatauv(L, sizeof(ModelAnimation), 2);
    *ud = anim;
    luaL_setmetatable(L, rlua_modelanimation_mt);
    rluaSetOwnership(L, -1, 0);

    if (parentIndex != 0)
    {
        lua_pushvalue(L, parentIndex);
        lua_setiuservalue(L, -2, 2);
    }
}

static int rluaModelAnimationToString(lua_State *L)
{
    ModelAnimation *anim = rluaModelAnimationCheck(L, 1);
    lua_pushfstring(L, "ModelAnimation(name=%s, boneCount=%u, keyframeCount=%d)", anim->name, anim->boneCount, anim->keyframeCount);
    return 1;
}

static int rluaModelAnimationIndex(lua_State *L)
{
    ModelAnimation *anim = rluaModelAnimationCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "name") == 0) lua_pushstring(L, anim->name);
    else if (strcmp(field, "boneCount") == 0) lua_pushinteger(L, (lua_Integer)anim->boneCount);
    else if (strcmp(field, "keyframeCount") == 0) lua_pushinteger(L, anim->keyframeCount);
    else lua_pushnil(L);

    return 1;
}

static const char rlua_modelanimations_mt[] = "raylib.ModelAnimations";

typedef struct rluaModelAnimations {
    ModelAnimation *animations;
    int count;
} rluaModelAnimations;

static rluaModelAnimations *rluaModelAnimationsCheck(lua_State *L, int index)
{
    return (rluaModelAnimations *)luaL_checkudata(L, index, rlua_modelanimations_mt);
}

static void rluaModelAnimationsPush(lua_State *L, ModelAnimation *animations, int count)
{
    rluaModelAnimations *ud = (rluaModelAnimations *)lua_newuserdatauv(L, sizeof(rluaModelAnimations), 1);
    ud->animations = animations;
    ud->count = count;
    luaL_setmetatable(L, rlua_modelanimations_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaModelAnimations_Release(lua_State *L, int index)
{
    rluaModelAnimations *animations = luaL_testudata(L, index, rlua_modelanimations_mt);
    if (animations == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadModelAnimations(animations->animations, animations->count);
    animations->animations = NULL;
    animations->count = 0;
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadModelAnimations(lua_State *L);

static int rluaModelAnimationsToString(lua_State *L)
{
    rluaModelAnimations *animations = rluaModelAnimationsCheck(L, 1);
    lua_pushfstring(L, "ModelAnimations(count=%d)", animations->count);
    return 1;
}

static int rluaModelAnimationsIndex(lua_State *L)
{
    rluaModelAnimations *animations = rluaModelAnimationsCheck(L, 1);

    if (lua_type(L, 2) == LUA_TNUMBER)
    {
        int index = (int)luaL_checkinteger(L, 2);

        if ((index < 1) || (index > animations->count))
        {
            lua_pushnil(L);
            return 1;
        }

        rluaModelAnimationPushBorrowed(L, animations->animations[index - 1], 1);
        return 1;
    }

    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "count") == 0) lua_pushinteger(L, animations->count);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadModelAnimations);
    else lua_pushnil(L);

    return 1;
}

static int rluaModelAnimationsLen(lua_State *L)
{
    lua_pushinteger(L, rluaModelAnimationsCheck(L, 1)->count);
    return 1;
}

static int rluaModelAnimationsGc(lua_State *L)
{
    rluaModelAnimations_Release(L, 1);
    return 0;
}

static int rluaUnloadModelAnimations(lua_State *L)
{
    rluaModelAnimationsCheck(L, 1);
    rluaModelAnimations_Release(L, 1);
    return 0;
}


static int rluaLoadModel(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    Model model = LoadModel(fileName);

    if (!IsModelValid(model))
    {
        UnloadModel(model);
        lua_pushnil(L);
        return 1;
    }

    rluaModelPush(L, model);
    return 1;
}

static int rluaLoadModelFromMesh(lua_State *L)
{
    Mesh *mesh = rluaMeshCheck(L, 1);
    Model model = LoadModelFromMesh(*mesh);
    *mesh = (Mesh){ 0 };
    rluaSetOwnership(L, 1, 0);
    rluaModelPush(L, model);
    return 1;
}

static int rluaUploadMesh(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Mesh *mesh = rluaMeshCheck(L, 1);
    bool dynamic = lua_toboolean(L, 2);
    UploadMesh(mesh, dynamic);
    return 0;
}

static int rluaGenMeshTangents(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Mesh *mesh = rluaMeshCheck(L, 1);
    GenMeshTangents(mesh);
    return 0;
}

static int rluaUpdateMeshBuffer(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Mesh *mesh = rluaMeshCheck(L, 1);
    int index = (int)luaL_checkinteger(L, 2);
    size_t dataSize = 0;
    const char *data = luaL_checklstring(L, 3, &dataSize);
    int offset = (int)luaL_checkinteger(L, 4);
    UpdateMeshBuffer(*mesh, index, data, (int)dataSize, offset);
    return 0;
}

static int rluaDrawMeshInstanced(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Mesh *mesh = rluaMeshCheck(L, 1);
    Material *material = rluaMaterialCheck(L, 2);
    int count = 0;
    Matrix *transforms = rluaMatrixArrayCheck(L, 3, &count);
    DrawMeshInstanced(*mesh, *material, transforms, count);
    return 0;
}

static int rluaDrawTriangleStrip3D(lua_State *L)
{
    int count = 0;
    Vector3 *points = rluaVector3ArrayCheck(L, 1, &count);
    Color color = rluaColorCheck(L, 2);
    DrawTriangleStrip3D(points, count, color);
    return 0;
}

static int rluaSetMaterialTexture(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Material *material = rluaMaterialCheck(L, 1);
    int mapType = (int)luaL_checkinteger(L, 2);
    Texture *texture = rluaTexture_CheckValid(L, 3);
    SetMaterialTexture(material, mapType, *texture);
    return 0;
}

static int rluaSetModelMeshMaterial(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model *model = rluaModelCheck(L, 1);
    int meshId = (int)luaL_checkinteger(L, 2);
    int materialId = (int)luaL_checkinteger(L, 3);
    SetModelMeshMaterial(model, meshId, materialId);
    return 0;
}

static int rluaUpdateModelAnimation(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model *model = rluaModelCheck(L, 1);
    ModelAnimation *anim = rluaModelAnimationCheck(L, 2);
    float frame = (float)luaL_checknumber(L, 3);
    UpdateModelAnimation(*model, *anim, frame);
    return 0;
}

static int rluaUpdateModelAnimationEx(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model *model = rluaModelCheck(L, 1);
    ModelAnimation *animA = rluaModelAnimationCheck(L, 2);
    float frameA = (float)luaL_checknumber(L, 3);
    ModelAnimation *animB = rluaModelAnimationCheck(L, 4);
    float frameB = (float)luaL_checknumber(L, 5);
    float blend = (float)luaL_checknumber(L, 6);
    UpdateModelAnimationEx(*model, *animA, frameA, *animB, frameB, blend);
    return 0;
}

static int rluaLoadMaterialDefault(lua_State *L)
{
    rluaMaterialPush(L, LoadMaterialDefault());
    return 1;
}

static int rluaLoadMaterials(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    int count = 0;
    Material *materials = LoadMaterials(fileName, &count);

    if ((materials == NULL) || (count <= 0))
    {
        MemFree(materials);
        lua_pushnil(L);
        return 1;
    }

    rluaMaterialsPush(L, materials, count);
    return 1;
}

static int rluaLoadModelAnimations(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    int count = 0;
    ModelAnimation *animations = LoadModelAnimations(fileName, &count);

    if ((animations == NULL) || (count <= 0))
    {
        MemFree(animations);
        lua_pushnil(L);
        return 1;
    }

    rluaModelAnimationsPush(L, animations, count);
    return 1;
}

static int rluaDrawLine3D(lua_State *L)
{
    Vector3 startPos = rluaVector3Check(L, 1);
    Vector3 endPos = rluaVector3Check(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawLine3D(startPos, endPos, color);
    return 0;
}

static int rluaDrawPoint3D(lua_State *L)
{
    Vector3 position = rluaVector3Check(L, 1);
    Color color = rluaColorCheck(L, 2);
    DrawPoint3D(position, color);
    return 0;
}

static int rluaDrawCircle3D(lua_State *L)
{
    Vector3 center = rluaVector3Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    Vector3 rotationAxis = rluaVector3Check(L, 3);
    float rotationAngle = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawCircle3D(center, radius, rotationAxis, rotationAngle, color);
    return 0;
}

static int rluaDrawTriangle3D(lua_State *L)
{
    Vector3 v1 = rluaVector3Check(L, 1);
    Vector3 v2 = rluaVector3Check(L, 2);
    Vector3 v3 = rluaVector3Check(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawTriangle3D(v1, v2, v3, color);
    return 0;
}

static int rluaDrawCube(lua_State *L)
{
    Vector3 position = rluaVector3Check(L, 1);
    float width = (float)luaL_checknumber(L, 2);
    float height = (float)luaL_checknumber(L, 3);
    float length = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawCube(position, width, height, length, color);
    return 0;
}

static int rluaDrawCubeV(lua_State *L)
{
    Vector3 position = rluaVector3Check(L, 1);
    Vector3 size = rluaVector3Check(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawCubeV(position, size, color);
    return 0;
}

static int rluaDrawCubeWires(lua_State *L)
{
    Vector3 position = rluaVector3Check(L, 1);
    float width = (float)luaL_checknumber(L, 2);
    float height = (float)luaL_checknumber(L, 3);
    float length = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawCubeWires(position, width, height, length, color);
    return 0;
}

static int rluaDrawCubeWiresV(lua_State *L)
{
    Vector3 position = rluaVector3Check(L, 1);
    Vector3 size = rluaVector3Check(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawCubeWiresV(position, size, color);
    return 0;
}

static int rluaDrawSphere(lua_State *L)
{
    Vector3 centerPos = rluaVector3Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawSphere(centerPos, radius, color);
    return 0;
}

static int rluaDrawSphereEx(lua_State *L)
{
    Vector3 centerPos = rluaVector3Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    int rings = (int)luaL_checkinteger(L, 3);
    int slices = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawSphereEx(centerPos, radius, rings, slices, color);
    return 0;
}

static int rluaDrawSphereWires(lua_State *L)
{
    Vector3 centerPos = rluaVector3Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    int rings = (int)luaL_checkinteger(L, 3);
    int slices = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawSphereWires(centerPos, radius, rings, slices, color);
    return 0;
}

static int rluaDrawCylinder(lua_State *L)
{
    Vector3 position = rluaVector3Check(L, 1);
    float radiusTop = (float)luaL_checknumber(L, 2);
    float radiusBottom = (float)luaL_checknumber(L, 3);
    float height = (float)luaL_checknumber(L, 4);
    int sides = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawCylinder(position, radiusTop, radiusBottom, height, sides, color);
    return 0;
}

static int rluaDrawCylinderEx(lua_State *L)
{
    Vector3 startPos = rluaVector3Check(L, 1);
    Vector3 endPos = rluaVector3Check(L, 2);
    float startRadius = (float)luaL_checknumber(L, 3);
    float endRadius = (float)luaL_checknumber(L, 4);
    int sides = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawCylinderEx(startPos, endPos, startRadius, endRadius, sides, color);
    return 0;
}

static int rluaDrawCylinderWires(lua_State *L)
{
    Vector3 position = rluaVector3Check(L, 1);
    float radiusTop = (float)luaL_checknumber(L, 2);
    float radiusBottom = (float)luaL_checknumber(L, 3);
    float height = (float)luaL_checknumber(L, 4);
    int sides = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawCylinderWires(position, radiusTop, radiusBottom, height, sides, color);
    return 0;
}

static int rluaDrawCylinderWiresEx(lua_State *L)
{
    Vector3 startPos = rluaVector3Check(L, 1);
    Vector3 endPos = rluaVector3Check(L, 2);
    float startRadius = (float)luaL_checknumber(L, 3);
    float endRadius = (float)luaL_checknumber(L, 4);
    int sides = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawCylinderWiresEx(startPos, endPos, startRadius, endRadius, sides, color);
    return 0;
}

static int rluaDrawCapsule(lua_State *L)
{
    Vector3 startPos = rluaVector3Check(L, 1);
    Vector3 endPos = rluaVector3Check(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    int rings = (int)luaL_checkinteger(L, 4);
    int slices = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawCapsule(startPos, endPos, radius, rings, slices, color);
    return 0;
}

static int rluaDrawCapsuleWires(lua_State *L)
{
    Vector3 startPos = rluaVector3Check(L, 1);
    Vector3 endPos = rluaVector3Check(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    int rings = (int)luaL_checkinteger(L, 4);
    int slices = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawCapsuleWires(startPos, endPos, radius, rings, slices, color);
    return 0;
}

static int rluaDrawPlane(lua_State *L)
{
    Vector3 centerPos = rluaVector3Check(L, 1);
    Vector2 size = rluaVector2Check(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawPlane(centerPos, size, color);
    return 0;
}

static int rluaDrawRay(lua_State *L)
{
    Ray ray = rluaRayCheck(L, 1);
    Color color = rluaColorCheck(L, 2);
    DrawRay(ray, color);
    return 0;
}

static int rluaDrawGrid(lua_State *L)
{
    int slices = (int)luaL_checkinteger(L, 1);
    float spacing = (float)luaL_checknumber(L, 2);
    DrawGrid(slices, spacing);
    return 0;
}

static int rluaIsModelValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model model = *rluaModelCheck(L, 1);
    lua_pushboolean(L, IsModelValid(model));
    return 1;
}

static int rluaGetModelBoundingBox(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model model = *rluaModelCheck(L, 1);
    rluaBoundingBoxPush(L, GetModelBoundingBox(model));
    return 1;
}

static int rluaDrawModel(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model model = *rluaModelCheck(L, 1);
    Vector3 position = rluaVector3Check(L, 2);
    float scale = (float)luaL_checknumber(L, 3);
    Color tint = rluaColorCheck(L, 4);
    DrawModel(model, position, scale, tint);
    return 0;
}

static int rluaDrawModelEx(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model model = *rluaModelCheck(L, 1);
    Vector3 position = rluaVector3Check(L, 2);
    Vector3 rotationAxis = rluaVector3Check(L, 3);
    float rotationAngle = (float)luaL_checknumber(L, 4);
    Vector3 scale = rluaVector3Check(L, 5);
    Color tint = rluaColorCheck(L, 6);
    DrawModelEx(model, position, rotationAxis, rotationAngle, scale, tint);
    return 0;
}

static int rluaDrawModelWires(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model model = *rluaModelCheck(L, 1);
    Vector3 position = rluaVector3Check(L, 2);
    float scale = (float)luaL_checknumber(L, 3);
    Color tint = rluaColorCheck(L, 4);
    DrawModelWires(model, position, scale, tint);
    return 0;
}

static int rluaDrawModelWiresEx(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model model = *rluaModelCheck(L, 1);
    Vector3 position = rluaVector3Check(L, 2);
    Vector3 rotationAxis = rluaVector3Check(L, 3);
    float rotationAngle = (float)luaL_checknumber(L, 4);
    Vector3 scale = rluaVector3Check(L, 5);
    Color tint = rluaColorCheck(L, 6);
    DrawModelWiresEx(model, position, rotationAxis, rotationAngle, scale, tint);
    return 0;
}

static int rluaDrawBoundingBox(lua_State *L)
{
    BoundingBox box = rluaBoundingBoxCheck(L, 1);
    Color color = rluaColorCheck(L, 2);
    DrawBoundingBox(box, color);
    return 0;
}

static int rluaDrawBillboard(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Camera3D camera = rluaCamera3DCheck(L, 1);
    Texture *texture = rluaTexture_CheckValid(L, 2);
    Vector3 position = rluaVector3Check(L, 3);
    float scale = (float)luaL_checknumber(L, 4);
    Color tint = rluaColorCheck(L, 5);
    DrawBillboard(camera, *texture, position, scale, tint);
    return 0;
}

static int rluaDrawBillboardRec(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Camera3D camera = rluaCamera3DCheck(L, 1);
    Texture *texture = rluaTexture_CheckValid(L, 2);
    Rectangle rec = rluaRectangleCheck(L, 3);
    Vector3 position = rluaVector3Check(L, 4);
    Vector2 size = rluaVector2Check(L, 5);
    Color tint = rluaColorCheck(L, 6);
    DrawBillboardRec(camera, *texture, rec, position, size, tint);
    return 0;
}

static int rluaDrawBillboardPro(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Camera3D camera = rluaCamera3DCheck(L, 1);
    Texture *texture = rluaTexture_CheckValid(L, 2);
    Rectangle rec = rluaRectangleCheck(L, 3);
    Vector3 position = rluaVector3Check(L, 4);
    Vector3 up = rluaVector3Check(L, 5);
    Vector2 size = rluaVector2Check(L, 6);
    Vector2 origin = rluaVector2Check(L, 7);
    float rotation = (float)luaL_checknumber(L, 8);
    Color tint = rluaColorCheck(L, 9);
    DrawBillboardPro(camera, *texture, rec, position, up, size, origin, rotation, tint);
    return 0;
}

static int rluaDrawMesh(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Mesh mesh = *rluaMeshCheck(L, 1);
    Material material = *rluaMaterialCheck(L, 2);
    Matrix transform = rluaMatrixCheck(L, 3);
    DrawMesh(mesh, material, transform);
    return 0;
}

static int rluaGetMeshBoundingBox(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Mesh mesh = *rluaMeshCheck(L, 1);
    rluaBoundingBoxPush(L, GetMeshBoundingBox(mesh));
    return 1;
}

static int rluaExportMesh(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Mesh mesh = *rluaMeshCheck(L, 1);
    const char *fileName = luaL_checkstring(L, 2);
    lua_pushboolean(L, ExportMesh(mesh, fileName));
    return 1;
}

static int rluaExportMeshAsCode(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Mesh mesh = *rluaMeshCheck(L, 1);
    const char *fileName = luaL_checkstring(L, 2);
    lua_pushboolean(L, ExportMeshAsCode(mesh, fileName));
    return 1;
}

static int rluaGenMeshPoly(lua_State *L)
{
    int sides = (int)luaL_checkinteger(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    rluaMeshPush(L, GenMeshPoly(sides, radius));
    return 1;
}

static int rluaGenMeshPlane(lua_State *L)
{
    float width = (float)luaL_checknumber(L, 1);
    float length = (float)luaL_checknumber(L, 2);
    int resX = (int)luaL_checkinteger(L, 3);
    int resZ = (int)luaL_checkinteger(L, 4);
    rluaMeshPush(L, GenMeshPlane(width, length, resX, resZ));
    return 1;
}

static int rluaGenMeshCube(lua_State *L)
{
    float width = (float)luaL_checknumber(L, 1);
    float height = (float)luaL_checknumber(L, 2);
    float length = (float)luaL_checknumber(L, 3);
    rluaMeshPush(L, GenMeshCube(width, height, length));
    return 1;
}

static int rluaGenMeshSphere(lua_State *L)
{
    float radius = (float)luaL_checknumber(L, 1);
    int rings = (int)luaL_checkinteger(L, 2);
    int slices = (int)luaL_checkinteger(L, 3);
    rluaMeshPush(L, GenMeshSphere(radius, rings, slices));
    return 1;
}

static int rluaGenMeshHemiSphere(lua_State *L)
{
    float radius = (float)luaL_checknumber(L, 1);
    int rings = (int)luaL_checkinteger(L, 2);
    int slices = (int)luaL_checkinteger(L, 3);
    rluaMeshPush(L, GenMeshHemiSphere(radius, rings, slices));
    return 1;
}

static int rluaGenMeshCylinder(lua_State *L)
{
    float radius = (float)luaL_checknumber(L, 1);
    float height = (float)luaL_checknumber(L, 2);
    int slices = (int)luaL_checkinteger(L, 3);
    rluaMeshPush(L, GenMeshCylinder(radius, height, slices));
    return 1;
}

static int rluaGenMeshCone(lua_State *L)
{
    float radius = (float)luaL_checknumber(L, 1);
    float height = (float)luaL_checknumber(L, 2);
    int slices = (int)luaL_checkinteger(L, 3);
    rluaMeshPush(L, GenMeshCone(radius, height, slices));
    return 1;
}

static int rluaGenMeshTorus(lua_State *L)
{
    float radius = (float)luaL_checknumber(L, 1);
    float size = (float)luaL_checknumber(L, 2);
    int radSeg = (int)luaL_checkinteger(L, 3);
    int sides = (int)luaL_checkinteger(L, 4);
    rluaMeshPush(L, GenMeshTorus(radius, size, radSeg, sides));
    return 1;
}

static int rluaGenMeshKnot(lua_State *L)
{
    float radius = (float)luaL_checknumber(L, 1);
    float size = (float)luaL_checknumber(L, 2);
    int radSeg = (int)luaL_checkinteger(L, 3);
    int sides = (int)luaL_checkinteger(L, 4);
    rluaMeshPush(L, GenMeshKnot(radius, size, radSeg, sides));
    return 1;
}

static int rluaGenMeshHeightmap(lua_State *L)
{
    Image *heightmap = rluaImageCheck(L, 1);
    Vector3 size = rluaVector3Check(L, 2);
    rluaMeshPush(L, GenMeshHeightmap(*heightmap, size));
    return 1;
}

static int rluaGenMeshCubicmap(lua_State *L)
{
    Image *cubicmap = rluaImageCheck(L, 1);
    Vector3 cubeSize = rluaVector3Check(L, 2);
    rluaMeshPush(L, GenMeshCubicmap(*cubicmap, cubeSize));
    return 1;
}

static int rluaIsMaterialValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Material material = *rluaMaterialCheck(L, 1);
    lua_pushboolean(L, IsMaterialValid(material));
    return 1;
}

static int rluaIsModelAnimationValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Model model = *rluaModelCheck(L, 1);
    ModelAnimation anim = *rluaModelAnimationCheck(L, 2);
    lua_pushboolean(L, IsModelAnimationValid(model, anim));
    return 1;
}

static int rluaCheckCollisionSpheres(lua_State *L)
{
    Vector3 center1 = rluaVector3Check(L, 1);
    float radius1 = (float)luaL_checknumber(L, 2);
    Vector3 center2 = rluaVector3Check(L, 3);
    float radius2 = (float)luaL_checknumber(L, 4);
    lua_pushboolean(L, CheckCollisionSpheres(center1, radius1, center2, radius2));
    return 1;
}

static int rluaCheckCollisionBoxes(lua_State *L)
{
    BoundingBox box1 = rluaBoundingBoxCheck(L, 1);
    BoundingBox box2 = rluaBoundingBoxCheck(L, 2);
    lua_pushboolean(L, CheckCollisionBoxes(box1, box2));
    return 1;
}

static int rluaCheckCollisionBoxSphere(lua_State *L)
{
    BoundingBox box = rluaBoundingBoxCheck(L, 1);
    Vector3 center = rluaVector3Check(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    lua_pushboolean(L, CheckCollisionBoxSphere(box, center, radius));
    return 1;
}

static int rluaGetRayCollisionSphere(lua_State *L)
{
    Ray ray = rluaRayCheck(L, 1);
    Vector3 center = rluaVector3Check(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    rluaRayCollisionPush(L, GetRayCollisionSphere(ray, center, radius));
    return 1;
}

static int rluaGetRayCollisionBox(lua_State *L)
{
    Ray ray = rluaRayCheck(L, 1);
    BoundingBox box = rluaBoundingBoxCheck(L, 2);
    rluaRayCollisionPush(L, GetRayCollisionBox(ray, box));
    return 1;
}

static int rluaGetRayCollisionMesh(lua_State *L)
{
    Ray ray = rluaRayCheck(L, 1);
    Mesh mesh = *rluaMeshCheck(L, 2);
    Matrix transform = rluaMatrixCheck(L, 3);
    rluaRayCollisionPush(L, GetRayCollisionMesh(ray, mesh, transform));
    return 1;
}

static int rluaGetRayCollisionTriangle(lua_State *L)
{
    Ray ray = rluaRayCheck(L, 1);
    Vector3 p1 = rluaVector3Check(L, 2);
    Vector3 p2 = rluaVector3Check(L, 3);
    Vector3 p3 = rluaVector3Check(L, 4);
    rluaRayCollisionPush(L, GetRayCollisionTriangle(ray, p1, p2, p3));
    return 1;
}

static int rluaGetRayCollisionQuad(lua_State *L)
{
    Ray ray = rluaRayCheck(L, 1);
    Vector3 p1 = rluaVector3Check(L, 2);
    Vector3 p2 = rluaVector3Check(L, 3);
    Vector3 p3 = rluaVector3Check(L, 4);
    Vector3 p4 = rluaVector3Check(L, 5);
    rluaRayCollisionPush(L, GetRayCollisionQuad(ray, p1, p2, p3, p4));
    return 1;
}

static void rluaModelsRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_mesh_mt))
    {
        REG(lua_pushcfunction(L, rluaMeshToString), "__tostring");
        REG(lua_pushcfunction(L, rluaMeshIndex), "__index");
        REG(lua_pushcfunction(L, rluaMeshGc), "__gc");
        REG(lua_pushcfunction(L, rluaMeshGc), "__close");
    }
    lua_pushstring(L, "Mesh");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    if (luaL_newmetatable(L, rlua_model_mt))
    {
        REG(lua_pushcfunction(L, rluaModelToString), "__tostring");
        REG(lua_pushcfunction(L, rluaModelIndex), "__index");
        REG(lua_pushcfunction(L, rluaModelGc), "__gc");
        REG(lua_pushcfunction(L, rluaModelGc), "__close");
    }
    lua_pushstring(L, "Model");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    if (luaL_newmetatable(L, rlua_material_mt))
    {
        REG(lua_pushcfunction(L, rluaMaterialToString), "__tostring");
        REG(lua_pushcfunction(L, rluaMaterialIndex), "__index");
        REG(lua_pushcfunction(L, rluaMaterialGc), "__gc");
        REG(lua_pushcfunction(L, rluaMaterialGc), "__close");
    }
    lua_pushstring(L, "Material");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    if (luaL_newmetatable(L, rlua_materials_mt))
    {
        REG(lua_pushcfunction(L, rluaMaterialsToString), "__tostring");
        REG(lua_pushcfunction(L, rluaMaterialsIndex), "__index");
        REG(lua_pushcfunction(L, rluaMaterialsLen), "__len");
        REG(lua_pushcfunction(L, rluaMaterialsGc), "__gc");
        REG(lua_pushcfunction(L, rluaMaterialsGc), "__close");
    }
    lua_pushstring(L, "Materials");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    if (luaL_newmetatable(L, rlua_modelanimation_mt))
    {
        REG(lua_pushcfunction(L, rluaModelAnimationToString), "__tostring");
        REG(lua_pushcfunction(L, rluaModelAnimationIndex), "__index");
    }
    lua_pushstring(L, "ModelAnimation");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    if (luaL_newmetatable(L, rlua_modelanimations_mt))
    {
        REG(lua_pushcfunction(L, rluaModelAnimationsToString), "__tostring");
        REG(lua_pushcfunction(L, rluaModelAnimationsIndex), "__index");
        REG(lua_pushcfunction(L, rluaModelAnimationsLen), "__len");
        REG(lua_pushcfunction(L, rluaModelAnimationsGc), "__gc");
        REG(lua_pushcfunction(L, rluaModelAnimationsGc), "__close");
    }
    lua_pushstring(L, "ModelAnimations");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaDrawLine3D), "DrawLine3D");
    REG(lua_pushcfunction(L, rluaDrawPoint3D), "DrawPoint3D");
    REG(lua_pushcfunction(L, rluaDrawCircle3D), "DrawCircle3D");
    REG(lua_pushcfunction(L, rluaDrawTriangle3D), "DrawTriangle3D");
    REG(lua_pushcfunction(L, rluaDrawTriangleStrip3D), "DrawTriangleStrip3D");
    REG(lua_pushcfunction(L, rluaDrawCube), "DrawCube");
    REG(lua_pushcfunction(L, rluaDrawCubeV), "DrawCubeV");
    REG(lua_pushcfunction(L, rluaDrawCubeWires), "DrawCubeWires");
    REG(lua_pushcfunction(L, rluaDrawCubeWiresV), "DrawCubeWiresV");
    REG(lua_pushcfunction(L, rluaDrawSphere), "DrawSphere");
    REG(lua_pushcfunction(L, rluaDrawSphereEx), "DrawSphereEx");
    REG(lua_pushcfunction(L, rluaDrawSphereWires), "DrawSphereWires");
    REG(lua_pushcfunction(L, rluaDrawCylinder), "DrawCylinder");
    REG(lua_pushcfunction(L, rluaDrawCylinderEx), "DrawCylinderEx");
    REG(lua_pushcfunction(L, rluaDrawCylinderWires), "DrawCylinderWires");
    REG(lua_pushcfunction(L, rluaDrawCylinderWiresEx), "DrawCylinderWiresEx");
    REG(lua_pushcfunction(L, rluaDrawCapsule), "DrawCapsule");
    REG(lua_pushcfunction(L, rluaDrawCapsuleWires), "DrawCapsuleWires");
    REG(lua_pushcfunction(L, rluaDrawPlane), "DrawPlane");
    REG(lua_pushcfunction(L, rluaDrawRay), "DrawRay");
    REG(lua_pushcfunction(L, rluaDrawGrid), "DrawGrid");
    REG(lua_pushcfunction(L, rluaLoadModel), "LoadModel");
    REG(lua_pushcfunction(L, rluaLoadModelFromMesh), "LoadModelFromMesh");
    REG(lua_pushcfunction(L, rluaDrawBoundingBox), "DrawBoundingBox");
    REG(lua_pushcfunction(L, rluaGenMeshPoly), "GenMeshPoly");
    REG(lua_pushcfunction(L, rluaGenMeshPlane), "GenMeshPlane");
    REG(lua_pushcfunction(L, rluaGenMeshCube), "GenMeshCube");
    REG(lua_pushcfunction(L, rluaGenMeshSphere), "GenMeshSphere");
    REG(lua_pushcfunction(L, rluaGenMeshHemiSphere), "GenMeshHemiSphere");
    REG(lua_pushcfunction(L, rluaGenMeshCylinder), "GenMeshCylinder");
    REG(lua_pushcfunction(L, rluaGenMeshCone), "GenMeshCone");
    REG(lua_pushcfunction(L, rluaGenMeshTorus), "GenMeshTorus");
    REG(lua_pushcfunction(L, rluaGenMeshKnot), "GenMeshKnot");
    REG(lua_pushcfunction(L, rluaGenMeshHeightmap), "GenMeshHeightmap");
    REG(lua_pushcfunction(L, rluaGenMeshCubicmap), "GenMeshCubicmap");
    REG(lua_pushcfunction(L, rluaLoadMaterialDefault), "LoadMaterialDefault");
    REG(lua_pushcfunction(L, rluaLoadMaterials), "LoadMaterials");
    REG(lua_pushcfunction(L, rluaLoadModelAnimations), "LoadModelAnimations");
    REG(lua_pushcfunction(L, rluaCheckCollisionSpheres), "CheckCollisionSpheres");
    REG(lua_pushcfunction(L, rluaCheckCollisionBoxes), "CheckCollisionBoxes");
    REG(lua_pushcfunction(L, rluaCheckCollisionBoxSphere), "CheckCollisionBoxSphere");
    REG(lua_pushcfunction(L, rluaGetRayCollisionSphere), "GetRayCollisionSphere");
    REG(lua_pushcfunction(L, rluaGetRayCollisionBox), "GetRayCollisionBox");
    REG(lua_pushcfunction(L, rluaGetRayCollisionMesh), "GetRayCollisionMesh");
    REG(lua_pushcfunction(L, rluaGetRayCollisionTriangle), "GetRayCollisionTriangle");
    REG(lua_pushcfunction(L, rluaGetRayCollisionQuad), "GetRayCollisionQuad");
}

// }}}

// Audio, wave/sound/music/stream resources {{{

static const char rlua_wave_mt[] = "raylib.Wave";

static Wave *rluaWaveCheck(lua_State *L, int index)
{
    return (Wave *)luaL_checkudata(L, index, rlua_wave_mt);
}

static void rluaWavePush(lua_State *L, Wave wave)
{
    Wave *ud = (Wave *)lua_newuserdatauv(L, sizeof(Wave), 1);
    *ud = wave;
    luaL_setmetatable(L, rlua_wave_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaWave_Release(lua_State *L, int index)
{
    Wave *wave = luaL_testudata(L, index, rlua_wave_mt);
    if (wave == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadWave(*wave);
    *wave = (Wave){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadWave(lua_State *L);

static int rluaWaveToString(lua_State *L)
{
    Wave *wave = rluaWaveCheck(L, 1);
    lua_pushfstring(L, "Wave(frameCount=%u, sampleRate=%u, sampleSize=%u, channels=%u)",
                    wave->frameCount, wave->sampleRate, wave->sampleSize, wave->channels);
    return 1;
}

static int rluaWaveIndex(lua_State *L)
{
    Wave *wave = rluaWaveCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "frameCount") == 0) lua_pushinteger(L, (lua_Integer)wave->frameCount);
    else if (strcmp(field, "sampleRate") == 0) lua_pushinteger(L, (lua_Integer)wave->sampleRate);
    else if (strcmp(field, "sampleSize") == 0) lua_pushinteger(L, (lua_Integer)wave->sampleSize);
    else if (strcmp(field, "channels") == 0) lua_pushinteger(L, (lua_Integer)wave->channels);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadWave);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsWaveValid);
    else if (strcmp(field, "copy") == 0) lua_pushcfunction(L, rluaWaveCopy);
    else if (strcmp(field, "crop") == 0) lua_pushcfunction(L, rluaWaveCrop);
    else if (strcmp(field, "format") == 0) lua_pushcfunction(L, rluaWaveFormat);
    else if (strcmp(field, "export") == 0) lua_pushcfunction(L, rluaExportWave);
    else if (strcmp(field, "exportAsCode") == 0) lua_pushcfunction(L, rluaExportWaveAsCode);
    else if (strcmp(field, "loadSamples") == 0) lua_pushcfunction(L, rluaLoadWaveSamples);
    else lua_pushnil(L);

    return 1;
}

static int rluaWaveGc(lua_State *L)
{
    rluaWave_Release(L, 1);
    return 0;
}

static int rluaUnloadWave(lua_State *L)
{
    rluaWaveCheck(L, 1);
    rluaWave_Release(L, 1);
    return 0;
}

static const char rlua_sound_mt[] = "raylib.Sound";

static Sound *rluaSoundCheck(lua_State *L, int index)
{
    return (Sound *)luaL_checkudata(L, index, rlua_sound_mt);
}

static int rluaSound_IsAlias(lua_State *L, int index)
{
    int alias = 0;
    if (lua_getiuservalue(L, index, 2) == LUA_TNUMBER) alias = (int)lua_tointeger(L, -1);
    lua_pop(L, 1);
    return alias;
}

static void rluaSoundPush(lua_State *L, Sound sound, int alias)
{
    Sound *ud = (Sound *)lua_newuserdatauv(L, sizeof(Sound), 2);
    *ud = sound;
    luaL_setmetatable(L, rlua_sound_mt);
    rluaSetOwnership(L, -1, 1);
    lua_pushinteger(L, alias);
    lua_setiuservalue(L, -2, 2);
}

static void rluaSound_Release(lua_State *L, int index)
{
    Sound *sound = luaL_testudata(L, index, rlua_sound_mt);
    if (sound == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    if (rluaSound_IsAlias(L, index)) UnloadSoundAlias(*sound);
    else UnloadSound(*sound);

    *sound = (Sound){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadSound(lua_State *L);

static int rluaSoundToString(lua_State *L)
{
    Sound *sound = rluaSoundCheck(L, 1);
    lua_pushfstring(L, "Sound(frameCount=%u, sampleRate=%u, channels=%u)",
                    sound->frameCount, sound->stream.sampleRate, sound->stream.channels);
    return 1;
}

static int rluaSoundIndex(lua_State *L)
{
    Sound *sound = rluaSoundCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "frameCount") == 0) lua_pushinteger(L, (lua_Integer)sound->frameCount);
    else if (strcmp(field, "sampleRate") == 0) lua_pushinteger(L, (lua_Integer)sound->stream.sampleRate);
    else if (strcmp(field, "sampleSize") == 0) lua_pushinteger(L, (lua_Integer)sound->stream.sampleSize);
    else if (strcmp(field, "channels") == 0) lua_pushinteger(L, (lua_Integer)sound->stream.channels);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadSound);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsSoundValid);
    else if (strcmp(field, "update") == 0) lua_pushcfunction(L, rluaUpdateSound);
    else if (strcmp(field, "play") == 0) lua_pushcfunction(L, rluaPlaySound);
    else if (strcmp(field, "stop") == 0) lua_pushcfunction(L, rluaStopSound);
    else if (strcmp(field, "pause") == 0) lua_pushcfunction(L, rluaPauseSound);
    else if (strcmp(field, "resume") == 0) lua_pushcfunction(L, rluaResumeSound);
    else if (strcmp(field, "isPlaying") == 0) lua_pushcfunction(L, rluaIsSoundPlaying);
    else if (strcmp(field, "setVolume") == 0) lua_pushcfunction(L, rluaSetSoundVolume);
    else if (strcmp(field, "setPitch") == 0) lua_pushcfunction(L, rluaSetSoundPitch);
    else if (strcmp(field, "setPan") == 0) lua_pushcfunction(L, rluaSetSoundPan);
    else lua_pushnil(L);

    return 1;
}

static int rluaSoundGc(lua_State *L)
{
    rluaSound_Release(L, 1);
    return 0;
}

static int rluaUnloadSound(lua_State *L)
{
    rluaSoundCheck(L, 1);
    rluaSound_Release(L, 1);
    return 0;
}

static const char rlua_music_mt[] = "raylib.Music";

static Music *rluaMusicCheck(lua_State *L, int index)
{
    return (Music *)luaL_checkudata(L, index, rlua_music_mt);
}

static void rluaMusicPush(lua_State *L, Music music)
{
    Music *ud = (Music *)lua_newuserdatauv(L, sizeof(Music), 1);
    *ud = music;
    luaL_setmetatable(L, rlua_music_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaMusic_Release(lua_State *L, int index)
{
    Music *music = luaL_testudata(L, index, rlua_music_mt);
    if (music == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadMusicStream(*music);
    *music = (Music){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadMusicStream(lua_State *L);

static int rluaMusicToString(lua_State *L)
{
    Music *music = rluaMusicCheck(L, 1);
    lua_pushfstring(L, "Music(frameCount=%u, sampleRate=%u, channels=%u)",
                    music->frameCount, music->stream.sampleRate, music->stream.channels);
    return 1;
}

static int rluaMusicIndex(lua_State *L)
{
    Music *music = rluaMusicCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "frameCount") == 0) lua_pushinteger(L, (lua_Integer)music->frameCount);
    else if (strcmp(field, "sampleRate") == 0) lua_pushinteger(L, (lua_Integer)music->stream.sampleRate);
    else if (strcmp(field, "channels") == 0) lua_pushinteger(L, (lua_Integer)music->stream.channels);
    else if (strcmp(field, "looping") == 0) lua_pushboolean(L, music->looping);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadMusicStream);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsMusicValid);
    else if (strcmp(field, "playStream") == 0) lua_pushcfunction(L, rluaPlayMusicStream);
    else if (strcmp(field, "isStreamPlaying") == 0) lua_pushcfunction(L, rluaIsMusicStreamPlaying);
    else if (strcmp(field, "updateStream") == 0) lua_pushcfunction(L, rluaUpdateMusicStream);
    else if (strcmp(field, "stopStream") == 0) lua_pushcfunction(L, rluaStopMusicStream);
    else if (strcmp(field, "pauseStream") == 0) lua_pushcfunction(L, rluaPauseMusicStream);
    else if (strcmp(field, "resumeStream") == 0) lua_pushcfunction(L, rluaResumeMusicStream);
    else if (strcmp(field, "seekStream") == 0) lua_pushcfunction(L, rluaSeekMusicStream);
    else if (strcmp(field, "setVolume") == 0) lua_pushcfunction(L, rluaSetMusicVolume);
    else if (strcmp(field, "setPitch") == 0) lua_pushcfunction(L, rluaSetMusicPitch);
    else if (strcmp(field, "setPan") == 0) lua_pushcfunction(L, rluaSetMusicPan);
    else if (strcmp(field, "getTimeLength") == 0) lua_pushcfunction(L, rluaGetMusicTimeLength);
    else if (strcmp(field, "getTimePlayed") == 0) lua_pushcfunction(L, rluaGetMusicTimePlayed);
    else lua_pushnil(L);

    return 1;
}

static int rluaMusicNewIndex(lua_State *L)
{
    Music *music = rluaMusicCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "looping") == 0)
    {
        music->looping = lua_toboolean(L, 3);
        return 0;
    }

    return luaL_error(L, "unknown Music field '%s'", field);
}

static int rluaMusicGc(lua_State *L)
{
    rluaMusic_Release(L, 1);
    return 0;
}

static int rluaUnloadMusicStream(lua_State *L)
{
    rluaMusicCheck(L, 1);
    rluaMusic_Release(L, 1);
    return 0;
}

static const char rlua_audiostream_mt[] = "raylib.AudioStream";

static AudioStream *rluaAudioStreamCheck(lua_State *L, int index)
{
    return (AudioStream *)luaL_checkudata(L, index, rlua_audiostream_mt);
}

static void rluaAudioStreamPush(lua_State *L, AudioStream stream)
{
    AudioStream *ud = (AudioStream *)lua_newuserdatauv(L, sizeof(AudioStream), 1);
    *ud = stream;
    luaL_setmetatable(L, rlua_audiostream_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaAudioStream_Release(lua_State *L, int index)
{
    AudioStream *stream = luaL_testudata(L, index, rlua_audiostream_mt);
    if (stream == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadAudioStream(*stream);
    *stream = (AudioStream){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadAudioStream(lua_State *L);

static int rluaAudioStreamToString(lua_State *L)
{
    AudioStream *stream = rluaAudioStreamCheck(L, 1);
    lua_pushfstring(L, "AudioStream(sampleRate=%u, sampleSize=%u, channels=%u)",
                    stream->sampleRate, stream->sampleSize, stream->channels);
    return 1;
}

static int rluaAudioStreamIndex(lua_State *L)
{
    AudioStream *stream = rluaAudioStreamCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "sampleRate") == 0) lua_pushinteger(L, (lua_Integer)stream->sampleRate);
    else if (strcmp(field, "sampleSize") == 0) lua_pushinteger(L, (lua_Integer)stream->sampleSize);
    else if (strcmp(field, "channels") == 0) lua_pushinteger(L, (lua_Integer)stream->channels);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadAudioStream);
    else if (strcmp(field, "isValid") == 0) lua_pushcfunction(L, rluaIsAudioStreamValid);
    else if (strcmp(field, "update") == 0) lua_pushcfunction(L, rluaUpdateAudioStream);
    else if (strcmp(field, "isProcessed") == 0) lua_pushcfunction(L, rluaIsAudioStreamProcessed);
    else if (strcmp(field, "play") == 0) lua_pushcfunction(L, rluaPlayAudioStream);
    else if (strcmp(field, "pause") == 0) lua_pushcfunction(L, rluaPauseAudioStream);
    else if (strcmp(field, "resume") == 0) lua_pushcfunction(L, rluaResumeAudioStream);
    else if (strcmp(field, "isPlaying") == 0) lua_pushcfunction(L, rluaIsAudioStreamPlaying);
    else if (strcmp(field, "stop") == 0) lua_pushcfunction(L, rluaStopAudioStream);
    else if (strcmp(field, "setVolume") == 0) lua_pushcfunction(L, rluaSetAudioStreamVolume);
    else if (strcmp(field, "setPitch") == 0) lua_pushcfunction(L, rluaSetAudioStreamPitch);
    else if (strcmp(field, "setPan") == 0) lua_pushcfunction(L, rluaSetAudioStreamPan);
    else if (strcmp(field, "setCallback") == 0) lua_pushcfunction(L, rluaSetAudioStreamCallback);
    else if (strcmp(field, "attachProcessor") == 0) lua_pushcfunction(L, rluaAttachAudioStreamProcessor);
    else if (strcmp(field, "detachProcessor") == 0) lua_pushcfunction(L, rluaDetachAudioStreamProcessor);
    else lua_pushnil(L);

    return 1;
}

static int rluaAudioStreamGc(lua_State *L)
{
    rluaAudioStream_Release(L, 1);
    return 0;
}

static int rluaUnloadAudioStream(lua_State *L)
{
    rluaAudioStreamCheck(L, 1);
    rluaAudioStream_Release(L, 1);
    return 0;
}


static int rluaInitAudioDevice(lua_State *L)
{
    InitAudioDevice();
    return 0;
}

static int rluaCloseAudioDevice(lua_State *L)
{
    CloseAudioDevice();
    return 0;
}

static int rluaIsAudioDeviceReady(lua_State *L)
{
    lua_pushboolean(L, IsAudioDeviceReady());
    return 1;
}

static int rluaSetMasterVolume(lua_State *L)
{
    SetMasterVolume((float)luaL_checknumber(L, 1));
    return 0;
}

static int rluaGetMasterVolume(lua_State *L)
{
    lua_pushnumber(L, GetMasterVolume());
    return 1;
}

static int rluaLoadWave(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    Wave wave = LoadWave(fileName);

    if (!IsWaveValid(wave))
    {
        UnloadWave(wave);
        lua_pushnil(L);
        return 1;
    }

    rluaWavePush(L, wave);
    return 1;
}

static int rluaLoadWaveFromMemory(lua_State *L)
{
    const char *fileType = luaL_checkstring(L, 1);
    size_t dataSize = 0;
    const unsigned char *data = (const unsigned char *)luaL_checklstring(L, 2, &dataSize);
    Wave wave = LoadWaveFromMemory(fileType, data, (int)dataSize);

    if (!IsWaveValid(wave))
    {
        UnloadWave(wave);
        lua_pushnil(L);
        return 1;
    }

    rluaWavePush(L, wave);
    return 1;
}

static int rluaLoadSound(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    Sound sound = LoadSound(fileName);

    if (!IsSoundValid(sound))
    {
        UnloadSound(sound);
        lua_pushnil(L);
        return 1;
    }

    rluaSoundPush(L, sound, 0);
    return 1;
}

static int rluaLoadSoundFromWave(lua_State *L)
{
    Wave *wave = rluaWaveCheck(L, 1);
    Sound sound = LoadSoundFromWave(*wave);

    if (!IsSoundValid(sound))
    {
        UnloadSound(sound);
        lua_pushnil(L);
        return 1;
    }

    rluaSoundPush(L, sound, 0);
    return 1;
}

static int rluaLoadSoundAlias(lua_State *L)
{
    Sound *source = rluaSoundCheck(L, 1);
    Sound sound = LoadSoundAlias(*source);

    if (!IsSoundValid(sound))
    {
        UnloadSoundAlias(sound);
        lua_pushnil(L);
        return 1;
    }

    rluaSoundPush(L, sound, 1);
    return 1;
}

static int rluaLoadMusicStream(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    Music music = LoadMusicStream(fileName);

    if (!IsMusicValid(music))
    {
        UnloadMusicStream(music);
        lua_pushnil(L);
        return 1;
    }

    rluaMusicPush(L, music);
    return 1;
}

static int rluaLoadMusicStreamFromMemory(lua_State *L)
{
    const char *fileType = luaL_checkstring(L, 1);
    size_t dataSize = 0;
    const unsigned char *data = (const unsigned char *)luaL_checklstring(L, 2, &dataSize);
    Music music = LoadMusicStreamFromMemory(fileType, data, (int)dataSize);

    if (!IsMusicValid(music))
    {
        UnloadMusicStream(music);
        lua_pushnil(L);
        return 1;
    }

    rluaMusicPush(L, music);
    return 1;
}

static int rluaLoadAudioStream(lua_State *L)
{
    unsigned int sampleRate = (unsigned int)luaL_checkinteger(L, 1);
    unsigned int sampleSize = (unsigned int)luaL_checkinteger(L, 2);
    unsigned int channels = (unsigned int)luaL_checkinteger(L, 3);
    AudioStream stream = LoadAudioStream(sampleRate, sampleSize, channels);

    if (!IsAudioStreamValid(stream))
    {
        UnloadAudioStream(stream);
        lua_pushnil(L);
        return 1;
    }

    rluaAudioStreamPush(L, stream);
    return 1;
}

static int rluaIsWaveValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Wave *wave = rluaWaveCheck(L, 1);
    lua_pushboolean(L, IsWaveValid(*wave));
    return 1;
}

static int rluaIsSoundValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound *sound = rluaSoundCheck(L, 1);
    lua_pushboolean(L, IsSoundValid(*sound));
    return 1;
}

static int rluaIsMusicValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music *music = rluaMusicCheck(L, 1);
    lua_pushboolean(L, IsMusicValid(*music));
    return 1;
}

static int rluaIsAudioStreamValid(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream *stream = rluaAudioStreamCheck(L, 1);
    lua_pushboolean(L, IsAudioStreamValid(*stream));
    return 1;
}

static int rluaWaveCopy(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Wave *wave = rluaWaveCheck(L, 1);
    rluaWavePush(L, WaveCopy(*wave));
    return 1;
}

static int rluaWaveCrop(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Wave *wave = rluaWaveCheck(L, 1);
    int initFrame = (int)luaL_checkinteger(L, 2);
    int finalFrame = (int)luaL_checkinteger(L, 3);
    WaveCrop(wave, initFrame, finalFrame);
    return 0;
}

static int rluaWaveFormat(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Wave *wave = rluaWaveCheck(L, 1);
    int sampleRate = (int)luaL_checkinteger(L, 2);
    int sampleSize = (int)luaL_checkinteger(L, 3);
    int channels = (int)luaL_checkinteger(L, 4);
    WaveFormat(wave, sampleRate, sampleSize, channels);
    return 0;
}

static int rluaLoadWaveSamples(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Wave *wave = rluaWaveCheck(L, 1);
    float *samples = LoadWaveSamples(*wave);
    int count = (int)(wave->frameCount*wave->channels);

    lua_createtable(L, count, 0);

    for (int i = 0; i < count; i++)
    {
        lua_pushnumber(L, samples[i]);
        lua_rawseti(L, -2, i + 1);
    }

    UnloadWaveSamples(samples);
    return 1;
}

static int rluaUpdateSound(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound *sound = rluaSoundCheck(L, 1);
    size_t dataSize = 0;
    const char *data = luaL_checklstring(L, 2, &dataSize);
    int frameCount = (int)luaL_checkinteger(L, 3);
    UpdateSound(*sound, data, frameCount);
    return 0;
}

static int rluaUpdateAudioStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream *stream = rluaAudioStreamCheck(L, 1);
    size_t dataSize = 0;
    const char *data = luaL_checklstring(L, 2, &dataSize);
    int frameCount = (int)luaL_checkinteger(L, 3);
    UpdateAudioStream(*stream, data, frameCount);
    return 0;
}

static int rluaExportWave(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Wave wave = *rluaWaveCheck(L, 1);
    const char *fileName = luaL_checkstring(L, 2);
    lua_pushboolean(L, ExportWave(wave, fileName));
    return 1;
}

static int rluaExportWaveAsCode(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Wave wave = *rluaWaveCheck(L, 1);
    const char *fileName = luaL_checkstring(L, 2);
    lua_pushboolean(L, ExportWaveAsCode(wave, fileName));
    return 1;
}

static int rluaPlaySound(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound sound = *rluaSoundCheck(L, 1);
    PlaySound(sound);
    return 0;
}

static int rluaStopSound(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound sound = *rluaSoundCheck(L, 1);
    StopSound(sound);
    return 0;
}

static int rluaPauseSound(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound sound = *rluaSoundCheck(L, 1);
    PauseSound(sound);
    return 0;
}

static int rluaResumeSound(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound sound = *rluaSoundCheck(L, 1);
    ResumeSound(sound);
    return 0;
}

static int rluaIsSoundPlaying(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound sound = *rluaSoundCheck(L, 1);
    lua_pushboolean(L, IsSoundPlaying(sound));
    return 1;
}

static int rluaSetSoundVolume(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound sound = *rluaSoundCheck(L, 1);
    float volume = (float)luaL_checknumber(L, 2);
    SetSoundVolume(sound, volume);
    return 0;
}

static int rluaSetSoundPitch(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound sound = *rluaSoundCheck(L, 1);
    float pitch = (float)luaL_checknumber(L, 2);
    SetSoundPitch(sound, pitch);
    return 0;
}

static int rluaSetSoundPan(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Sound sound = *rluaSoundCheck(L, 1);
    float pan = (float)luaL_checknumber(L, 2);
    SetSoundPan(sound, pan);
    return 0;
}

static int rluaPlayMusicStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    PlayMusicStream(music);
    return 0;
}

static int rluaIsMusicStreamPlaying(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    lua_pushboolean(L, IsMusicStreamPlaying(music));
    return 1;
}

static int rluaUpdateMusicStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    UpdateMusicStream(music);
    return 0;
}

static int rluaStopMusicStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    StopMusicStream(music);
    return 0;
}

static int rluaPauseMusicStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    PauseMusicStream(music);
    return 0;
}

static int rluaResumeMusicStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    ResumeMusicStream(music);
    return 0;
}

static int rluaSeekMusicStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    float position = (float)luaL_checknumber(L, 2);
    SeekMusicStream(music, position);
    return 0;
}

static int rluaSetMusicVolume(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    float volume = (float)luaL_checknumber(L, 2);
    SetMusicVolume(music, volume);
    return 0;
}

static int rluaSetMusicPitch(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    float pitch = (float)luaL_checknumber(L, 2);
    SetMusicPitch(music, pitch);
    return 0;
}

static int rluaSetMusicPan(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    float pan = (float)luaL_checknumber(L, 2);
    SetMusicPan(music, pan);
    return 0;
}

static int rluaGetMusicTimeLength(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    lua_pushnumber(L, GetMusicTimeLength(music));
    return 1;
}

static int rluaGetMusicTimePlayed(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Music music = *rluaMusicCheck(L, 1);
    lua_pushnumber(L, GetMusicTimePlayed(music));
    return 1;
}

static int rluaIsAudioStreamProcessed(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream stream = *rluaAudioStreamCheck(L, 1);
    lua_pushboolean(L, IsAudioStreamProcessed(stream));
    return 1;
}

static int rluaPlayAudioStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream stream = *rluaAudioStreamCheck(L, 1);
    PlayAudioStream(stream);
    return 0;
}

static int rluaPauseAudioStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream stream = *rluaAudioStreamCheck(L, 1);
    PauseAudioStream(stream);
    return 0;
}

static int rluaResumeAudioStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream stream = *rluaAudioStreamCheck(L, 1);
    ResumeAudioStream(stream);
    return 0;
}

static int rluaIsAudioStreamPlaying(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream stream = *rluaAudioStreamCheck(L, 1);
    lua_pushboolean(L, IsAudioStreamPlaying(stream));
    return 1;
}

static int rluaStopAudioStream(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream stream = *rluaAudioStreamCheck(L, 1);
    StopAudioStream(stream);
    return 0;
}

static int rluaSetAudioStreamVolume(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream stream = *rluaAudioStreamCheck(L, 1);
    float volume = (float)luaL_checknumber(L, 2);
    SetAudioStreamVolume(stream, volume);
    return 0;
}

static int rluaSetAudioStreamPitch(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream stream = *rluaAudioStreamCheck(L, 1);
    float pitch = (float)luaL_checknumber(L, 2);
    SetAudioStreamPitch(stream, pitch);
    return 0;
}

static int rluaSetAudioStreamPan(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream stream = *rluaAudioStreamCheck(L, 1);
    float pan = (float)luaL_checknumber(L, 2);
    SetAudioStreamPan(stream, pan);
    return 0;
}

static int rluaSetAudioStreamBufferSizeDefault(lua_State *L)
{
    int size = (int)luaL_checkinteger(L, 1);
    SetAudioStreamBufferSizeDefault(size);
    return 0;
}

#define RLUA_AUDIO_SLOTS 16

typedef struct rluaAudioSlot {
    int active;
    int kind;
    int ref;
    AudioStream stream;
    unsigned int sampleSize;
    unsigned int channels;
} rluaAudioSlot;

static rluaAudioSlot rlua_audio_slots[RLUA_AUDIO_SLOTS];
static pthread_mutex_t rlua_lua_mutex = PTHREAD_MUTEX_INITIALIZER;
static int rlua_audio_shutdown = 0;
static __thread int rlua_audio_in_callback = 0;
static lua_State *rlua_audio_L = NULL;
static pthread_t rlua_main_thread;

void rLuaLock(void)
{
    pthread_mutex_lock(&rlua_lua_mutex);
}

void rLuaUnlock(void)
{
    pthread_mutex_unlock(&rlua_lua_mutex);
}

void rLuaShutdown(void)
{
    rLuaLock();
    rlua_audio_shutdown = 1;
    rlua_audio_L = NULL;
    rLuaUnlock();

    if (IsAudioDeviceReady()) CloseAudioDevice();
}

static void rluaAudioSlotCallback(int index, void *bufferData, unsigned int frames)
{
    if (rlua_audio_shutdown || (rlua_audio_L == NULL)) return;

    rluaAudioSlot *slot = &rlua_audio_slots[index];
    if (!slot->active || (slot->ref == LUA_NOREF)) return;

    int expected = (slot->kind == 0)
        ? (int)frames*(int)(slot->sampleSize/8)*(int)slot->channels
        : (int)frames*2*(int)sizeof(float);

    if (pthread_mutex_trylock(&rlua_lua_mutex) != 0)
    {
        if (slot->kind == 0) memset(bufferData, 0, (size_t)expected);
        return;
    }

    if (rlua_audio_shutdown || (rlua_audio_L == NULL))
    {
        pthread_mutex_unlock(&rlua_lua_mutex);
        if (slot->kind == 0) memset(bufferData, 0, (size_t)expected);
        return;
    }

    lua_State *L = rlua_audio_L;
    int top = lua_gettop(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, slot->ref);

    if (slot->kind == 0)
    {
        lua_pushinteger(L, (lua_Integer)frames);
    }
    else
    {
        lua_pushlstring(L, (const char *)bufferData, (size_t)expected);
        lua_pushinteger(L, (lua_Integer)frames);
    }

    rlua_audio_in_callback = 1;
    int status = lua_pcall(L, (slot->kind == 0) ? 1 : 2, 1, 0);
    rlua_audio_in_callback = 0;

    if (status != LUA_OK)
    {
        lua_settop(L, top);
        pthread_mutex_unlock(&rlua_lua_mutex);
        return;
    }

    size_t length = 0;
    const char *data = lua_tolstring(L, -1, &length);

    if ((data != NULL) && (expected > 0))
    {
        size_t amount = (length < (size_t)expected) ? length : (size_t)expected;
        memcpy(bufferData, data, amount);

        if ((slot->kind == 0) && (amount < (size_t)expected))
        {
            memset((char *)bufferData + amount, 0, (size_t)expected - amount);
        }
    }

    lua_settop(L, top);
    pthread_mutex_unlock(&rlua_lua_mutex);
}

#define RLUA_AUDIO_TRAMPOLINE(name, index) \
    static void name(void *bufferData, unsigned int frames) \
    { \
        rluaAudioSlotCallback(index, bufferData, frames); \
    }

RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline0, 0)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline1, 1)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline2, 2)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline3, 3)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline4, 4)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline5, 5)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline6, 6)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline7, 7)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline8, 8)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline9, 9)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline10, 10)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline11, 11)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline12, 12)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline13, 13)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline14, 14)
RLUA_AUDIO_TRAMPOLINE(rluaAudioTrampoline15, 15)

static AudioCallback rlua_audio_trampolines[RLUA_AUDIO_SLOTS] = {
    rluaAudioTrampoline0, rluaAudioTrampoline1, rluaAudioTrampoline2, rluaAudioTrampoline3,
    rluaAudioTrampoline4, rluaAudioTrampoline5, rluaAudioTrampoline6, rluaAudioTrampoline7,
    rluaAudioTrampoline8, rluaAudioTrampoline9, rluaAudioTrampoline10, rluaAudioTrampoline11,
    rluaAudioTrampoline12, rluaAudioTrampoline13, rluaAudioTrampoline14, rluaAudioTrampoline15,
};

static int rluaAudioSlotFind(lua_State *L, int kind, AudioStream stream, int functionIndex)
{
    for (int i = 0; i < RLUA_AUDIO_SLOTS; i++)
    {
        if (!rlua_audio_slots[i].active || (rlua_audio_slots[i].kind != kind)) continue;

        if ((kind != 2) && (rlua_audio_slots[i].stream.buffer != stream.buffer)) continue;

        if (functionIndex != 0)
        {
            lua_rawgeti(L, LUA_REGISTRYINDEX, rlua_audio_slots[i].ref);
            int equal = lua_rawequal(L, -1, functionIndex);
            lua_pop(L, 1);
            if (!equal) continue;
        }

        return i;
    }

    return -1;
}

static int rluaAudioSlotAlloc(int kind, AudioStream stream)
{
    for (int i = 0; i < RLUA_AUDIO_SLOTS; i++)
    {
        if (rlua_audio_slots[i].active) continue;

        rlua_audio_slots[i] = (rluaAudioSlot){ 0 };
        rlua_audio_slots[i].active = 1;
        rlua_audio_slots[i].kind = kind;
        rlua_audio_slots[i].ref = LUA_NOREF;
        rlua_audio_slots[i].stream = stream;
        rlua_audio_slots[i].sampleSize = stream.sampleSize;
        rlua_audio_slots[i].channels = stream.channels;
        return i;
    }

    return -1;
}

static void rluaAudioSlotClear(lua_State *L, int index)
{
    if (rlua_audio_slots[index].ref != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, rlua_audio_slots[index].ref);
    }

    rlua_audio_slots[index] = (rluaAudioSlot){ 0 };
    rlua_audio_slots[index].ref = LUA_NOREF;
}

static int rluaSetAudioStreamCallback(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream *stream = rluaAudioStreamCheck(L, 1);
    int existing = rluaAudioSlotFind(L, 0, *stream, 0);

    if (lua_isnoneornil(L, 2))
    {
        if (existing >= 0)
        {
            SetAudioStreamCallback(*stream, NULL);
            rluaAudioSlotClear(L, existing);
        }

        return 0;
    }

    luaL_checktype(L, 2, LUA_TFUNCTION);

    int index = existing;
    if (index < 0) index = rluaAudioSlotAlloc(0, *stream);
    if (index < 0) return luaL_error(L, "too many audio callbacks");

    if (rlua_audio_slots[index].ref != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, rlua_audio_slots[index].ref);
    }

    lua_pushvalue(L, 2);
    rlua_audio_slots[index].ref = luaL_ref(L, LUA_REGISTRYINDEX);
    rlua_audio_slots[index].sampleSize = stream->sampleSize;
    rlua_audio_slots[index].channels = stream->channels;

    SetAudioStreamCallback(*stream, rlua_audio_trampolines[index]);
    return 0;
}

static int rluaAttachAudioStreamProcessor(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream *stream = rluaAudioStreamCheck(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    int index = rluaAudioSlotAlloc(1, *stream);
    if (index < 0) return luaL_error(L, "too many audio processor callbacks");

    lua_pushvalue(L, 2);
    rlua_audio_slots[index].ref = luaL_ref(L, LUA_REGISTRYINDEX);

    AttachAudioStreamProcessor(*stream, rlua_audio_trampolines[index]);
    return 0;
}

static int rluaDetachAudioStreamProcessor(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AudioStream *stream = rluaAudioStreamCheck(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    int index = rluaAudioSlotFind(L, 1, *stream, 2);

    if (index >= 0)
    {
        DetachAudioStreamProcessor(*stream, rlua_audio_trampolines[index]);
        rluaAudioSlotClear(L, index);
    }

    return 0;
}

static int rluaAttachAudioMixedProcessor(lua_State *L)
{
    AudioStream stream = { 0 };
    luaL_checktype(L, 1, LUA_TFUNCTION);

    int index = rluaAudioSlotAlloc(2, stream);
    if (index < 0) return luaL_error(L, "too many audio processor callbacks");

    lua_pushvalue(L, 1);
    rlua_audio_slots[index].ref = luaL_ref(L, LUA_REGISTRYINDEX);

    AttachAudioMixedProcessor(rlua_audio_trampolines[index]);
    return 0;
}

static int rluaDetachAudioMixedProcessor(lua_State *L)
{
    AudioStream stream = { 0 };
    luaL_checktype(L, 1, LUA_TFUNCTION);

    int index = rluaAudioSlotFind(L, 2, stream, 1);

    if (index >= 0)
    {
        DetachAudioMixedProcessor(rlua_audio_trampolines[index]);
        rluaAudioSlotClear(L, index);
    }

    return 0;
}

static int rluaGuardedCall(lua_State *L)
{
    if (rlua_audio_in_callback)
    {
        return luaL_error(L, "raylib functions cannot be called from an audio callback");
    }

    lua_CFunction fn = (lua_CFunction)lua_touserdata(L, lua_upvalueindex(1));
    return fn(L);
}

static void rluaGuardFunctions(lua_State *L)
{
    int table = lua_gettop(L);
    lua_pushnil(L);

    while (lua_next(L, table) != 0)
    {
        if (lua_isfunction(L, -1))
        {
            const char *name = lua_tostring(L, -2);
            int allowed = (name != NULL) &&
                ((strcmp(name, "Vector2") == 0) || (strcmp(name, "Vector3") == 0) ||
                 (strcmp(name, "Vector4") == 0) || (strcmp(name, "Quaternion") == 0) ||
                 (strcmp(name, "Matrix") == 0) || (strcmp(name, "Color") == 0));

            if (!allowed)
            {
                lua_CFunction fn = lua_tocfunction(L, -1);

                if (fn != NULL)
                {
                    lua_pushvalue(L, -2);
                    lua_pushlightuserdata(L, (void *)fn);
                    lua_pushcclosure(L, rluaGuardedCall, 1);
                    lua_rawset(L, table);
                }
            }
        }

        lua_pop(L, 1);
    }
}

static void rluaAudioRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_wave_mt))
    {
        REG(lua_pushcfunction(L, rluaWaveToString), "__tostring");
        REG(lua_pushcfunction(L, rluaWaveIndex), "__index");
        REG(lua_pushcfunction(L, rluaWaveGc), "__gc");
        REG(lua_pushcfunction(L, rluaWaveGc), "__close");
    }
    lua_pushstring(L, "Wave");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    if (luaL_newmetatable(L, rlua_sound_mt))
    {
        REG(lua_pushcfunction(L, rluaSoundToString), "__tostring");
        REG(lua_pushcfunction(L, rluaSoundIndex), "__index");
        REG(lua_pushcfunction(L, rluaSoundGc), "__gc");
        REG(lua_pushcfunction(L, rluaSoundGc), "__close");
    }
    lua_pushstring(L, "Sound");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    if (luaL_newmetatable(L, rlua_music_mt))
    {
        REG(lua_pushcfunction(L, rluaMusicToString), "__tostring");
        REG(lua_pushcfunction(L, rluaMusicIndex), "__index");
        REG(lua_pushcfunction(L, rluaMusicNewIndex), "__newindex");
        REG(lua_pushcfunction(L, rluaMusicGc), "__gc");
        REG(lua_pushcfunction(L, rluaMusicGc), "__close");
    }
    lua_pushstring(L, "Music");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    if (luaL_newmetatable(L, rlua_audiostream_mt))
    {
        REG(lua_pushcfunction(L, rluaAudioStreamToString), "__tostring");
        REG(lua_pushcfunction(L, rluaAudioStreamIndex), "__index");
        REG(lua_pushcfunction(L, rluaAudioStreamGc), "__gc");
        REG(lua_pushcfunction(L, rluaAudioStreamGc), "__close");
    }
    lua_pushstring(L, "AudioStream");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaInitAudioDevice), "InitAudioDevice");
    REG(lua_pushcfunction(L, rluaCloseAudioDevice), "CloseAudioDevice");
    REG(lua_pushcfunction(L, rluaIsAudioDeviceReady), "IsAudioDeviceReady");
    REG(lua_pushcfunction(L, rluaSetMasterVolume), "SetMasterVolume");
    REG(lua_pushcfunction(L, rluaGetMasterVolume), "GetMasterVolume");
    REG(lua_pushcfunction(L, rluaLoadWave), "LoadWave");
    REG(lua_pushcfunction(L, rluaLoadWaveFromMemory), "LoadWaveFromMemory");
    REG(lua_pushcfunction(L, rluaLoadSound), "LoadSound");
    REG(lua_pushcfunction(L, rluaLoadSoundFromWave), "LoadSoundFromWave");
    REG(lua_pushcfunction(L, rluaLoadSoundAlias), "LoadSoundAlias");
    REG(lua_pushcfunction(L, rluaLoadMusicStream), "LoadMusicStream");
    REG(lua_pushcfunction(L, rluaLoadMusicStreamFromMemory), "LoadMusicStreamFromMemory");
    REG(lua_pushcfunction(L, rluaLoadAudioStream), "LoadAudioStream");
    REG(lua_pushcfunction(L, rluaSetAudioStreamBufferSizeDefault), "SetAudioStreamBufferSizeDefault");
    REG(lua_pushcfunction(L, rluaAttachAudioMixedProcessor), "AttachAudioMixedProcessor");
    REG(lua_pushcfunction(L, rluaDetachAudioMixedProcessor), "DetachAudioMixedProcessor");
}

// }}}

// [CORE] {{{

static int rluaInitWindow(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    const char *title = luaL_checkstring(L, 3);
    InitWindow(width, height, title);
    return 0;
}

static int rluaCloseWindow(lua_State *L)
{
    CloseWindow();
    return 0;
}

static int rluaWindowShouldClose(lua_State *L)
{
    lua_pushboolean(L, WindowShouldClose());
    return 1;
}

static int rluaIsWindowReady(lua_State *L)
{
    lua_pushboolean(L, IsWindowReady());
    return 1;
}

static int rluaIsWindowFullscreen(lua_State *L)
{
    lua_pushboolean(L, IsWindowFullscreen());
    return 1;
}

static int rluaIsWindowHidden(lua_State *L)
{
    lua_pushboolean(L, IsWindowHidden());
    return 1;
}

static int rluaIsWindowMinimized(lua_State *L)
{
    lua_pushboolean(L, IsWindowMinimized());
    return 1;
}

static int rluaIsWindowMaximized(lua_State *L)
{
    lua_pushboolean(L, IsWindowMaximized());
    return 1;
}

static int rluaIsWindowFocused(lua_State *L)
{
    lua_pushboolean(L, IsWindowFocused());
    return 1;
}

static int rluaIsWindowResized(lua_State *L)
{
    lua_pushboolean(L, IsWindowResized());
    return 1;
}

static int rluaIsWindowState(lua_State *L)
{
    unsigned int flag = (unsigned int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsWindowState(flag));
    return 1;
}

static int rluaSetWindowState(lua_State *L)
{
    unsigned int flags = (unsigned int)luaL_checkinteger(L, 1);
    SetWindowState(flags);
    return 0;
}

static int rluaClearWindowState(lua_State *L)
{
    unsigned int flags = (unsigned int)luaL_checkinteger(L, 1);
    ClearWindowState(flags);
    return 0;
}

static int rluaToggleFullscreen(lua_State *L)
{
    ToggleFullscreen();
    return 0;
}

static int rluaToggleBorderlessWindowed(lua_State *L)
{
    ToggleBorderlessWindowed();
    return 0;
}

static int rluaMaximizeWindow(lua_State *L)
{
    MaximizeWindow();
    return 0;
}

static int rluaMinimizeWindow(lua_State *L)
{
    MinimizeWindow();
    return 0;
}

static int rluaRestoreWindow(lua_State *L)
{
    RestoreWindow();
    return 0;
}

static int rluaSetWindowTitle(lua_State *L)
{
    const char *title = luaL_checkstring(L, 1);
    SetWindowTitle(title);
    return 0;
}

static int rluaSetWindowPosition(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    SetWindowPosition(x, y);
    return 0;
}

static int rluaSetWindowMonitor(lua_State *L)
{
    int monitor = (int)luaL_checkinteger(L, 1);
    SetWindowMonitor(monitor);
    return 0;
}

static int rluaSetWindowMinSize(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    SetWindowMinSize(width, height);
    return 0;
}

static int rluaSetWindowMaxSize(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    SetWindowMaxSize(width, height);
    return 0;
}

static int rluaSetWindowSize(lua_State *L)
{
    int width = (int)luaL_checkinteger(L, 1);
    int height = (int)luaL_checkinteger(L, 2);
    SetWindowSize(width, height);
    return 0;
}

static int rluaSetWindowOpacity(lua_State *L)
{
    float opacity = (float)luaL_checknumber(L, 1);
    SetWindowOpacity(opacity);
    return 0;
}

static int rluaSetWindowFocused(lua_State *L)
{
    SetWindowFocused();
    return 0;
}

static int rluaGetWindowHandle(lua_State *L)
{
    lua_pushlightuserdata(L, GetWindowHandle());
    return 1;
}

static int rluaGetScreenWidth(lua_State *L)
{
    lua_pushinteger(L, GetScreenWidth());
    return 1;
}

static int rluaGetScreenHeight(lua_State *L)
{
    lua_pushinteger(L, GetScreenHeight());
    return 1;
}

static int rluaGetRenderWidth(lua_State *L)
{
    lua_pushinteger(L, GetRenderWidth());
    return 1;
}

static int rluaGetRenderHeight(lua_State *L)
{
    lua_pushinteger(L, GetRenderHeight());
    return 1;
}

static int rluaGetMonitorCount(lua_State *L)
{
    lua_pushinteger(L, GetMonitorCount());
    return 1;
}

static int rluaGetCurrentMonitor(lua_State *L)
{
    lua_pushinteger(L, GetCurrentMonitor());
    return 1;
}

static int rluaGetMonitorPosition(lua_State *L)
{
    int monitor = (int)luaL_checkinteger(L, 1);
    rluaVector2Push(L, GetMonitorPosition(monitor));
    return 1;
}

static int rluaGetMonitorWidth(lua_State *L)
{
    int monitor = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, GetMonitorWidth(monitor));
    return 1;
}

static int rluaGetMonitorHeight(lua_State *L)
{
    int monitor = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, GetMonitorHeight(monitor));
    return 1;
}

static int rluaGetMonitorPhysicalWidth(lua_State *L)
{
    int monitor = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, GetMonitorPhysicalWidth(monitor));
    return 1;
}

static int rluaGetMonitorPhysicalHeight(lua_State *L)
{
    int monitor = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, GetMonitorPhysicalHeight(monitor));
    return 1;
}

static int rluaGetMonitorRefreshRate(lua_State *L)
{
    int monitor = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, GetMonitorRefreshRate(monitor));
    return 1;
}

static int rluaGetWindowPosition(lua_State *L)
{
    rluaVector2Push(L, GetWindowPosition());
    return 1;
}

static int rluaGetWindowScaleDPI(lua_State *L)
{
    rluaVector2Push(L, GetWindowScaleDPI());
    return 1;
}

static int rluaGetMonitorName(lua_State *L)
{
    int monitor = (int)luaL_checkinteger(L, 1);
    const char *rlua_string_result = GetMonitorName(monitor);
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaSetClipboardText(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    SetClipboardText(text);
    return 0;
}

static int rluaGetClipboardText(lua_State *L)
{
    const char *rlua_string_result = GetClipboardText();
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaEnableEventWaiting(lua_State *L)
{
    EnableEventWaiting();
    return 0;
}

static int rluaDisableEventWaiting(lua_State *L)
{
    DisableEventWaiting();
    return 0;
}

static int rluaShowCursor(lua_State *L)
{
    ShowCursor();
    return 0;
}

static int rluaHideCursor(lua_State *L)
{
    HideCursor();
    return 0;
}

static int rluaIsCursorHidden(lua_State *L)
{
    lua_pushboolean(L, IsCursorHidden());
    return 1;
}

static int rluaEnableCursor(lua_State *L)
{
    EnableCursor();
    return 0;
}

static int rluaDisableCursor(lua_State *L)
{
    DisableCursor();
    return 0;
}

static int rluaIsCursorOnScreen(lua_State *L)
{
    lua_pushboolean(L, IsCursorOnScreen());
    return 1;
}

static int rluaClearBackground(lua_State *L)
{
    Color color = rluaColorCheck(L, 1);
    ClearBackground(color);
    return 0;
}

static int rluaBeginDrawing(lua_State *L)
{
    BeginDrawing();
    return 0;
}

static int rluaEndDrawing(lua_State *L)
{
    rLuaUnlock();
    EndDrawing();
    rLuaLock();
    return 0;
}

static int rluaBeginMode2D(lua_State *L)
{
    Camera2D camera = rluaCamera2DCheck(L, 1);
    BeginMode2D(camera);
    return 0;
}

static int rluaEndMode2D(lua_State *L)
{
    EndMode2D();
    return 0;
}

static int rluaBeginMode3D(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    BeginMode3D(camera);
    return 0;
}

static int rluaEndMode3D(lua_State *L)
{
    EndMode3D();
    return 0;
}

static int rluaBeginTextureMode(lua_State *L)
{
    RenderTexture *target = rluaRenderTexture_CheckValid(L, 1);
    BeginTextureMode(*target);
    return 0;
}

static int rluaEndTextureMode(lua_State *L)
{
    EndTextureMode();
    return 0;
}

static int rluaBeginShaderMode(lua_State *L)
{
    Shader *shader = rluaShader_CheckValid(L, 1);
    BeginShaderMode(*shader);
    return 0;
}

static int rluaEndShaderMode(lua_State *L)
{
    EndShaderMode();
    return 0;
}

static int rluaBeginBlendMode(lua_State *L)
{
    int mode = (int)luaL_checkinteger(L, 1);
    BeginBlendMode(mode);
    return 0;
}

static int rluaEndBlendMode(lua_State *L)
{
    EndBlendMode();
    return 0;
}

static int rluaBeginScissorMode(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    int width = (int)luaL_checkinteger(L, 3);
    int height = (int)luaL_checkinteger(L, 4);
    BeginScissorMode(x, y, width, height);
    return 0;
}

static int rluaEndScissorMode(lua_State *L)
{
    EndScissorMode();
    return 0;
}

static int rluaBeginVrStereoMode(lua_State *L)
{
    VrStereoConfig *config = rluaVrStereoConfigCheck(L, 1);
    BeginVrStereoMode(*config);
    return 0;
}

static int rluaEndVrStereoMode(lua_State *L)
{
    EndVrStereoMode();
    return 0;
}

static int rluaGetScreenToWorldRay(lua_State *L)
{
    Vector2 position = rluaVector2Check(L, 1);
    Camera3D camera = rluaCamera3DCheck(L, 2);
    rluaRayPush(L, GetScreenToWorldRay(position, camera));
    return 1;
}

static int rluaGetScreenToWorldRayEx(lua_State *L)
{
    Vector2 position = rluaVector2Check(L, 1);
    Camera3D camera = rluaCamera3DCheck(L, 2);
    int width = (int)luaL_checkinteger(L, 3);
    int height = (int)luaL_checkinteger(L, 4);
    rluaRayPush(L, GetScreenToWorldRayEx(position, camera, width, height));
    return 1;
}

static int rluaGetWorldToScreen(lua_State *L)
{
    Vector3 position = rluaVector3Check(L, 1);
    Camera3D camera = rluaCamera3DCheck(L, 2);
    rluaVector2Push(L, GetWorldToScreen(position, camera));
    return 1;
}

static int rluaGetWorldToScreenEx(lua_State *L)
{
    Vector3 position = rluaVector3Check(L, 1);
    Camera3D camera = rluaCamera3DCheck(L, 2);
    int width = (int)luaL_checkinteger(L, 3);
    int height = (int)luaL_checkinteger(L, 4);
    rluaVector2Push(L, GetWorldToScreenEx(position, camera, width, height));
    return 1;
}

static int rluaGetWorldToScreen2D(lua_State *L)
{
    Vector2 position = rluaVector2Check(L, 1);
    Camera2D camera = rluaCamera2DCheck(L, 2);
    rluaVector2Push(L, GetWorldToScreen2D(position, camera));
    return 1;
}

static int rluaGetScreenToWorld2D(lua_State *L)
{
    Vector2 position = rluaVector2Check(L, 1);
    Camera2D camera = rluaCamera2DCheck(L, 2);
    rluaVector2Push(L, GetScreenToWorld2D(position, camera));
    return 1;
}

static int rluaGetCameraMatrix(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Camera3D camera = rluaCamera3DCheck(L, 1);
    rluaMatrixPush(L, GetCameraMatrix(camera));
    return 1;
}

static int rluaGetCameraMatrix2D(lua_State *L)
{
    Camera2D camera = rluaCamera2DCheck(L, 1);
    rluaMatrixPush(L, GetCameraMatrix2D(camera));
    return 1;
}

static int rluaSetTargetFPS(lua_State *L)
{
    int fps = (int)luaL_checkinteger(L, 1);
    SetTargetFPS(fps);
    return 0;
}

static int rluaGetFrameTime(lua_State *L)
{
    lua_pushnumber(L, GetFrameTime());
    return 1;
}

static int rluaGetTime(lua_State *L)
{
    lua_pushnumber(L, GetTime());
    return 1;
}

static int rluaGetFPS(lua_State *L)
{
    lua_pushinteger(L, GetFPS());
    return 1;
}

static int rluaSwapScreenBuffer(lua_State *L)
{
    SwapScreenBuffer();
    return 0;
}

static int rluaPollInputEvents(lua_State *L)
{
    PollInputEvents();
    return 0;
}

static int rluaWaitTime(lua_State *L)
{
    double seconds = luaL_checknumber(L, 1);
    WaitTime(seconds);
    return 0;
}

static int rluaSetRandomSeed(lua_State *L)
{
    unsigned int seed = (unsigned int)luaL_checkinteger(L, 1);
    SetRandomSeed(seed);
    return 0;
}

static int rluaGetRandomValue(lua_State *L)
{
    int min = (int)luaL_checkinteger(L, 1);
    int max = (int)luaL_checkinteger(L, 2);
    lua_pushinteger(L, GetRandomValue(min, max));
    return 1;
}

static int rluaLoadRandomSequence(lua_State *L)
{
    unsigned int count = (unsigned int)luaL_checkinteger(L, 1);
    int min = (int)luaL_checkinteger(L, 2);
    int max = (int)luaL_checkinteger(L, 3);
    int *values = LoadRandomSequence(count, min, max);

    if (values == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    int result = rluaPushIntArray(L, values, (int)count);
    UnloadRandomSequence(values);
    return result;
}

static int rluaTakeScreenshot(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    TakeScreenshot(fileName);
    return 0;
}

static int rluaSetConfigFlags(lua_State *L)
{
    unsigned int flags = (unsigned int)luaL_checkinteger(L, 1);
    SetConfigFlags(flags);
    return 0;
}

static int rluaOpenURL(lua_State *L)
{
    const char *url = luaL_checkstring(L, 1);
    OpenURL(url);
    return 0;
}

static int rluaSetTraceLogLevel(lua_State *L)
{
    int logLevel = (int)luaL_checkinteger(L, 1);
    SetTraceLogLevel(logLevel);
    return 0;
}

static int rluaTraceLog(lua_State *L)
{
    int level = (int)luaL_checkinteger(L, 1);
    int top = lua_gettop(L);

    if (top > 2)
    {
        lua_getglobal(L, "string");
        lua_getfield(L, -1, "format");
        lua_remove(L, -2);
        lua_insert(L, 2);
        lua_call(L, top - 1, 1);
    }

    const char *message = luaL_checkstring(L, 2);
    TraceLog(level, "%s", message);
    return 0;
}

static int rluaLoadFileData(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    int rlua_out_dataSize = 0;
    unsigned char * rlua_result = LoadFileData(fileName, &rlua_out_dataSize);
    if (rlua_result != NULL)
    {
        lua_pushlstring(L, (const char *)rlua_result, (size_t)rlua_out_dataSize);
        MemFree(rlua_result);
    }
    else lua_pushnil(L);
    return 1;
}

static int rluaSaveFileData(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    size_t rlua_len_data = 0;
    const void * data = (const void *)luaL_checklstring(L, 2, &rlua_len_data);
    lua_pushboolean(L, SaveFileData(fileName, data, (int)rlua_len_data));
    return 1;
}

static int rluaExportDataAsCode(lua_State *L)
{
    size_t rlua_len_data = 0;
    const unsigned char * data = (const unsigned char *)luaL_checklstring(L, 1, &rlua_len_data);
    const char *fileName = luaL_checkstring(L, 2);
    lua_pushboolean(L, ExportDataAsCode(data, (int)rlua_len_data, fileName));
    return 1;
}

static int rluaLoadFileText(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    char *rlua_result = LoadFileText(fileName);
    lua_pushstring(L, rlua_result);
    MemFree(rlua_result);
    return 1;
}

static int rluaSaveFileText(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    const char *text = luaL_checkstring(L, 2);
    lua_pushboolean(L, SaveFileText(fileName, text));
    return 1;
}

static int rluaFileRename(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    const char *fileRename = luaL_checkstring(L, 2);
    lua_pushinteger(L, FileRename(fileName, fileRename));
    return 1;
}

static int rluaFileRemove(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    lua_pushinteger(L, FileRemove(fileName));
    return 1;
}

static int rluaFileCopy(lua_State *L)
{
    const char *srcPath = luaL_checkstring(L, 1);
    const char *dstPath = luaL_checkstring(L, 2);
    lua_pushinteger(L, FileCopy(srcPath, dstPath));
    return 1;
}

static int rluaFileMove(lua_State *L)
{
    const char *srcPath = luaL_checkstring(L, 1);
    const char *dstPath = luaL_checkstring(L, 2);
    lua_pushinteger(L, FileMove(srcPath, dstPath));
    return 1;
}

static int rluaFileTextReplace(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    const char *search = luaL_checkstring(L, 2);
    const char *replacement = luaL_checkstring(L, 3);
    lua_pushinteger(L, FileTextReplace(fileName, search, replacement));
    return 1;
}

static int rluaFileTextFindIndex(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    const char *search = luaL_checkstring(L, 2);
    lua_pushinteger(L, FileTextFindIndex(fileName, search));
    return 1;
}

static int rluaFileExists(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    lua_pushboolean(L, FileExists(fileName));
    return 1;
}

static int rluaDirectoryExists(lua_State *L)
{
    const char *dirPath = luaL_checkstring(L, 1);
    lua_pushboolean(L, DirectoryExists(dirPath));
    return 1;
}

static int rluaIsFileExtension(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    const char *ext = luaL_checkstring(L, 2);
    lua_pushboolean(L, IsFileExtension(fileName, ext));
    return 1;
}

static int rluaIsFileHidden(lua_State *L)
{
    const char *filePath = luaL_checkstring(L, 1);
    lua_pushboolean(L, IsFileHidden(filePath));
    return 1;
}

static int rluaGetFileLength(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    lua_pushinteger(L, GetFileLength(fileName));
    return 1;
}

static int rluaGetFileModTime(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    lua_pushinteger(L, GetFileModTime(fileName));
    return 1;
}

static int rluaGetFileExtension(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    const char *rlua_string_result = GetFileExtension(fileName);
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaGetFileName(lua_State *L)
{
    const char *filePath = luaL_checkstring(L, 1);
    const char *rlua_string_result = GetFileName(filePath);
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaGetFileNameWithoutExt(lua_State *L)
{
    const char *filePath = luaL_checkstring(L, 1);
    const char *rlua_string_result = GetFileNameWithoutExt(filePath);
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaGetDirectoryPath(lua_State *L)
{
    const char *filePath = luaL_checkstring(L, 1);
    const char *rlua_string_result = GetDirectoryPath(filePath);
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaGetPrevDirectoryPath(lua_State *L)
{
    const char *dirPath = luaL_checkstring(L, 1);
    const char *rlua_string_result = GetPrevDirectoryPath(dirPath);
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaGetWorkingDirectory(lua_State *L)
{
    const char *rlua_string_result = GetWorkingDirectory();
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaGetApplicationDirectory(lua_State *L)
{
    const char *rlua_string_result = GetApplicationDirectory();
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaMakeDirectory(lua_State *L)
{
    const char *dirPath = luaL_checkstring(L, 1);
    lua_pushinteger(L, MakeDirectory(dirPath));
    return 1;
}

static int rluaChangeDirectory(lua_State *L)
{
    const char *dirPath = luaL_checkstring(L, 1);
    lua_pushinteger(L, ChangeDirectory(dirPath));
    return 1;
}

static int rluaIsPathFile(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, IsPathFile(path));
    return 1;
}

static int rluaIsPathDirectory(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, IsPathDirectory(path));
    return 1;
}

static int rluaIsPathAbsolute(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, IsPathAbsolute(path));
    return 1;
}

static int rluaIsFileNameValid(lua_State *L)
{
    const char *fileName = luaL_checkstring(L, 1);
    lua_pushboolean(L, IsFileNameValid(fileName));
    return 1;
}

static int rluaLoadDirectoryFiles(lua_State *L)
{
    const char *dirPath = luaL_checkstring(L, 1);
    FilePathList files = LoadDirectoryFiles(dirPath);

    int result = rluaPushStringArray(L, files.paths, (int)files.count);
    UnloadDirectoryFiles(files);
    return result;
}

static int rluaLoadDirectoryFilesEx(lua_State *L)
{
    const char *basePath = luaL_checkstring(L, 1);
    const char *filter = luaL_checkstring(L, 2);
    bool scanSubdirs = lua_toboolean(L, 3);
    FilePathList files = LoadDirectoryFilesEx(basePath, filter, scanSubdirs);

    int result = rluaPushStringArray(L, files.paths, (int)files.count);
    UnloadDirectoryFiles(files);
    return result;
}

static int rluaIsFileDropped(lua_State *L)
{
    lua_pushboolean(L, IsFileDropped());
    return 1;
}

static int rluaLoadDroppedFiles(lua_State *L)
{
    FilePathList files = LoadDroppedFiles();

    int result = rluaPushStringArray(L, files.paths, (int)files.count);
    UnloadDroppedFiles(files);
    return result;
}

static int rluaGetDirectoryFileCount(lua_State *L)
{
    const char *dirPath = luaL_checkstring(L, 1);
    lua_pushinteger(L, GetDirectoryFileCount(dirPath));
    return 1;
}

static int rluaGetDirectoryFileCountEx(lua_State *L)
{
    const char *basePath = luaL_checkstring(L, 1);
    const char *filter = luaL_checkstring(L, 2);
    bool scanSubdirs = lua_toboolean(L, 3);
    lua_pushinteger(L, GetDirectoryFileCountEx(basePath, filter, scanSubdirs));
    return 1;
}

static int rluaCompressData(lua_State *L)
{
    size_t rlua_len_data = 0;
    const unsigned char * data = (const unsigned char *)luaL_checklstring(L, 1, &rlua_len_data);
    int rlua_out_compDataSize = 0;
    unsigned char * rlua_result = CompressData(data, (int)rlua_len_data, &rlua_out_compDataSize);
    if (rlua_result != NULL)
    {
        lua_pushlstring(L, (const char *)rlua_result, (size_t)rlua_out_compDataSize);
        MemFree(rlua_result);
    }
    else lua_pushnil(L);
    return 1;
}

static int rluaDecompressData(lua_State *L)
{
    size_t rlua_len_compData = 0;
    const unsigned char * compData = (const unsigned char *)luaL_checklstring(L, 1, &rlua_len_compData);
    int rlua_out_dataSize = 0;
    unsigned char * rlua_result = DecompressData(compData, (int)rlua_len_compData, &rlua_out_dataSize);
    if (rlua_result != NULL)
    {
        lua_pushlstring(L, (const char *)rlua_result, (size_t)rlua_out_dataSize);
        MemFree(rlua_result);
    }
    else lua_pushnil(L);
    return 1;
}

static int rluaEncodeDataBase64(lua_State *L)
{
    size_t rlua_len_data = 0;
    const unsigned char * data = (const unsigned char *)luaL_checklstring(L, 1, &rlua_len_data);
    int rlua_out_outputSize = 0;
    char * rlua_result = EncodeDataBase64(data, (int)rlua_len_data, &rlua_out_outputSize);
    if (rlua_result != NULL)
    {
        lua_pushlstring(L, rlua_result, (size_t)(rlua_out_outputSize > 0 ? rlua_out_outputSize - 1 : 0));
        MemFree(rlua_result);
    }
    else lua_pushnil(L);
    return 1;
}

static int rluaDecodeDataBase64(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int rlua_out_outputSize = 0;
    unsigned char * rlua_result = DecodeDataBase64(text, &rlua_out_outputSize);
    if (rlua_result != NULL)
    {
        lua_pushlstring(L, (const char *)rlua_result, (size_t)rlua_out_outputSize);
        MemFree(rlua_result);
    }
    else lua_pushnil(L);
    return 1;
}

static int rluaComputeCRC32(lua_State *L)
{
    size_t rlua_len_data = 0;
    const unsigned char * data = (const unsigned char *)luaL_checklstring(L, 1, &rlua_len_data);
    lua_pushinteger(L, ComputeCRC32(data, (int)rlua_len_data));
    return 1;
}

static int rluaComputeMD5(lua_State *L)
{
    size_t size = 0;
    const unsigned char *data = (const unsigned char *)luaL_checklstring(L, 1, &size);
    return rluaPushUIntArray(L, ComputeMD5(data, (int)size), 4);
}

static int rluaComputeSHA1(lua_State *L)
{
    size_t size = 0;
    const unsigned char *data = (const unsigned char *)luaL_checklstring(L, 1, &size);
    return rluaPushUIntArray(L, ComputeSHA1(data, (int)size), 5);
}

static int rluaComputeSHA256(lua_State *L)
{
    size_t size = 0;
    const unsigned char *data = (const unsigned char *)luaL_checklstring(L, 1, &size);
    return rluaPushUIntArray(L, ComputeSHA256(data, (int)size), 8);
}

static int rluaIsKeyPressed(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsKeyPressed(key));
    return 1;
}

static int rluaIsKeyPressedRepeat(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsKeyPressedRepeat(key));
    return 1;
}

static int rluaIsKeyDown(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsKeyDown(key));
    return 1;
}

static int rluaIsKeyReleased(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsKeyReleased(key));
    return 1;
}

static int rluaIsKeyUp(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsKeyUp(key));
    return 1;
}

static int rluaGetKeyPressed(lua_State *L)
{
    lua_pushinteger(L, GetKeyPressed());
    return 1;
}

static int rluaGetCharPressed(lua_State *L)
{
    lua_pushinteger(L, GetCharPressed());
    return 1;
}

static int rluaGetKeyName(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    const char *rlua_string_result = GetKeyName(key);
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaSetExitKey(lua_State *L)
{
    int key = (int)luaL_checkinteger(L, 1);
    SetExitKey(key);
    return 0;
}

static int rluaIsGamepadAvailable(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsGamepadAvailable(gamepad));
    return 1;
}

static int rluaGetGamepadName(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    const char *rlua_string_result = GetGamepadName(gamepad);
    if (rlua_string_result != NULL) lua_pushstring(L, rlua_string_result);
    else lua_pushnil(L);
    return 1;
}

static int rluaIsGamepadButtonPressed(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, IsGamepadButtonPressed(gamepad, button));
    return 1;
}

static int rluaIsGamepadButtonDown(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, IsGamepadButtonDown(gamepad, button));
    return 1;
}

static int rluaIsGamepadButtonReleased(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, IsGamepadButtonReleased(gamepad, button));
    return 1;
}

static int rluaIsGamepadButtonUp(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int button = (int)luaL_checkinteger(L, 2);
    lua_pushboolean(L, IsGamepadButtonUp(gamepad, button));
    return 1;
}

static int rluaGetGamepadButtonPressed(lua_State *L)
{
    lua_pushinteger(L, GetGamepadButtonPressed());
    return 1;
}

static int rluaGetGamepadAxisCount(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, GetGamepadAxisCount(gamepad));
    return 1;
}

static int rluaGetGamepadAxisMovement(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    int axis = (int)luaL_checkinteger(L, 2);
    lua_pushnumber(L, GetGamepadAxisMovement(gamepad, axis));
    return 1;
}

static int rluaSetGamepadMappings(lua_State *L)
{
    const char *mappings = luaL_checkstring(L, 1);
    lua_pushinteger(L, SetGamepadMappings(mappings));
    return 1;
}

static int rluaSetGamepadVibration(lua_State *L)
{
    int gamepad = (int)luaL_checkinteger(L, 1);
    float leftMotor = (float)luaL_checknumber(L, 2);
    float rightMotor = (float)luaL_checknumber(L, 3);
    float duration = (float)luaL_checknumber(L, 4);
    SetGamepadVibration(gamepad, leftMotor, rightMotor, duration);
    return 0;
}

static int rluaIsMouseButtonPressed(lua_State *L)
{
    int button = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsMouseButtonPressed(button));
    return 1;
}

static int rluaIsMouseButtonDown(lua_State *L)
{
    int button = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsMouseButtonDown(button));
    return 1;
}

static int rluaIsMouseButtonReleased(lua_State *L)
{
    int button = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsMouseButtonReleased(button));
    return 1;
}

static int rluaIsMouseButtonUp(lua_State *L)
{
    int button = (int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsMouseButtonUp(button));
    return 1;
}

static int rluaGetMouseX(lua_State *L)
{
    lua_pushinteger(L, GetMouseX());
    return 1;
}

static int rluaGetMouseY(lua_State *L)
{
    lua_pushinteger(L, GetMouseY());
    return 1;
}

static int rluaGetMousePosition(lua_State *L)
{
    rluaVector2Push(L, GetMousePosition());
    return 1;
}

static int rluaGetMouseDelta(lua_State *L)
{
    rluaVector2Push(L, GetMouseDelta());
    return 1;
}

static int rluaSetMousePosition(lua_State *L)
{
    int x = (int)luaL_checkinteger(L, 1);
    int y = (int)luaL_checkinteger(L, 2);
    SetMousePosition(x, y);
    return 0;
}

static int rluaSetMouseOffset(lua_State *L)
{
    int offsetX = (int)luaL_checkinteger(L, 1);
    int offsetY = (int)luaL_checkinteger(L, 2);
    SetMouseOffset(offsetX, offsetY);
    return 0;
}

static int rluaSetMouseScale(lua_State *L)
{
    float scaleX = (float)luaL_checknumber(L, 1);
    float scaleY = (float)luaL_checknumber(L, 2);
    SetMouseScale(scaleX, scaleY);
    return 0;
}

static int rluaGetMouseWheelMove(lua_State *L)
{
    lua_pushnumber(L, GetMouseWheelMove());
    return 1;
}

static int rluaGetMouseWheelMoveV(lua_State *L)
{
    rluaVector2Push(L, GetMouseWheelMoveV());
    return 1;
}

static int rluaSetMouseCursor(lua_State *L)
{
    int cursor = (int)luaL_checkinteger(L, 1);
    SetMouseCursor(cursor);
    return 0;
}

static int rluaGetTouchX(lua_State *L)
{
    lua_pushinteger(L, GetTouchX());
    return 1;
}

static int rluaGetTouchY(lua_State *L)
{
    lua_pushinteger(L, GetTouchY());
    return 1;
}

static int rluaGetTouchPosition(lua_State *L)
{
    int index = (int)luaL_checkinteger(L, 1);
    rluaVector2Push(L, GetTouchPosition(index));
    return 1;
}

static int rluaGetTouchPointId(lua_State *L)
{
    int index = (int)luaL_checkinteger(L, 1);
    lua_pushinteger(L, GetTouchPointId(index));
    return 1;
}

static int rluaGetTouchPointCount(lua_State *L)
{
    lua_pushinteger(L, GetTouchPointCount());
    return 1;
}

// Callbacks and automation events {{{

static lua_State *rlua_callback_L = NULL;
static int rlua_trace_log_ref = LUA_NOREF;
static int rlua_load_file_data_ref = LUA_NOREF;
static int rlua_save_file_data_ref = LUA_NOREF;
static int rlua_load_file_text_ref = LUA_NOREF;
static int rlua_save_file_text_ref = LUA_NOREF;

static void rluaSetCallbackRef(lua_State *L, int *ref)
{
    if (*ref != LUA_NOREF) luaL_unref(L, LUA_REGISTRYINDEX, *ref);

    if (lua_isnoneornil(L, 1))
    {
        *ref = LUA_NOREF;
        return;
    }

    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_pushvalue(L, 1);
    *ref = luaL_ref(L, LUA_REGISTRYINDEX);
    rlua_callback_L = L;
}

static void rluaTraceLogBridge(int logLevel, const char *text, va_list args)
{
    lua_State *L = rlua_callback_L;

    if ((L == NULL) || (rlua_trace_log_ref == LUA_NOREF)) return;
    if (!pthread_equal(pthread_self(), rlua_main_thread)) return;

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);

    lua_rawgeti(L, LUA_REGISTRYINDEX, rlua_trace_log_ref);
    lua_pushinteger(L, logLevel);
    lua_pushstring(L, buffer);

    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        lua_pop(L, 1);
    }
}

static unsigned char *rluaLoadFileDataBridge(const char *fileName, int *dataSize)
{
    lua_State *L = rlua_callback_L;
    *dataSize = 0;

    if ((L == NULL) || (rlua_load_file_data_ref == LUA_NOREF)) return NULL;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rlua_load_file_data_ref);
    lua_pushstring(L, fileName);

    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
    {
        lua_pop(L, 1);
        return NULL;
    }

    size_t length = 0;
    const char *data = lua_tolstring(L, -1, &length);
    unsigned char *result = NULL;

    if ((data != NULL) && (length > 0))
    {
        result = (unsigned char *)MemAlloc((unsigned int)length);
        memcpy(result, data, length);
        *dataSize = (int)length;
    }

    lua_pop(L, 1);
    return result;
}

static bool rluaSaveFileDataBridge(const char *fileName, const void *data, int dataSize)
{
    lua_State *L = rlua_callback_L;

    if ((L == NULL) || (rlua_save_file_data_ref == LUA_NOREF)) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rlua_save_file_data_ref);
    lua_pushstring(L, fileName);
    lua_pushlstring(L, (const char *)data, dataSize);

    if (lua_pcall(L, 2, 1, 0) != LUA_OK)
    {
        lua_pop(L, 1);
        return false;
    }

    bool result = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return result;
}

static char *rluaLoadFileTextBridge(const char *fileName)
{
    lua_State *L = rlua_callback_L;

    if ((L == NULL) || (rlua_load_file_text_ref == LUA_NOREF)) return NULL;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rlua_load_file_text_ref);
    lua_pushstring(L, fileName);

    if (lua_pcall(L, 1, 1, 0) != LUA_OK)
    {
        lua_pop(L, 1);
        return NULL;
    }

    size_t length = 0;
    const char *text = lua_tolstring(L, -1, &length);
    char *result = NULL;

    if (text != NULL)
    {
        result = (char *)MemAlloc((unsigned int)length + 1);
        memcpy(result, text, length);
        result[length] = '\0';
    }

    lua_pop(L, 1);
    return result;
}

static bool rluaSaveFileTextBridge(const char *fileName, const char *text)
{
    lua_State *L = rlua_callback_L;

    if ((L == NULL) || (rlua_save_file_text_ref == LUA_NOREF)) return false;

    lua_rawgeti(L, LUA_REGISTRYINDEX, rlua_save_file_text_ref);
    lua_pushstring(L, fileName);
    lua_pushstring(L, text);

    if (lua_pcall(L, 2, 1, 0) != LUA_OK)
    {
        lua_pop(L, 1);
        return false;
    }

    bool result = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return result;
}

static int rluaSetTraceLogCallback(lua_State *L)
{
    rluaSetCallbackRef(L, &rlua_trace_log_ref);
    SetTraceLogCallback((rlua_trace_log_ref == LUA_NOREF) ? NULL : rluaTraceLogBridge);
    return 0;
}

static int rluaSetLoadFileDataCallback(lua_State *L)
{
    rluaSetCallbackRef(L, &rlua_load_file_data_ref);
    SetLoadFileDataCallback((rlua_load_file_data_ref == LUA_NOREF) ? NULL : rluaLoadFileDataBridge);
    return 0;
}

static int rluaSetSaveFileDataCallback(lua_State *L)
{
    rluaSetCallbackRef(L, &rlua_save_file_data_ref);
    SetSaveFileDataCallback((rlua_save_file_data_ref == LUA_NOREF) ? NULL : rluaSaveFileDataBridge);
    return 0;
}

static int rluaSetLoadFileTextCallback(lua_State *L)
{
    rluaSetCallbackRef(L, &rlua_load_file_text_ref);
    SetLoadFileTextCallback((rlua_load_file_text_ref == LUA_NOREF) ? NULL : rluaLoadFileTextBridge);
    return 0;
}

static int rluaSetSaveFileTextCallback(lua_State *L)
{
    rluaSetCallbackRef(L, &rlua_save_file_text_ref);
    SetSaveFileTextCallback((rlua_save_file_text_ref == LUA_NOREF) ? NULL : rluaSaveFileTextBridge);
    return 0;
}

static const char rlua_automationeventlist_mt[] = "raylib.AutomationEventList";

static AutomationEventList *rluaAutomationEventListCheck(lua_State *L, int index)
{
    return (AutomationEventList *)luaL_checkudata(L, index, rlua_automationeventlist_mt);
}

static void rluaAutomationEventListPush(lua_State *L, AutomationEventList list)
{
    AutomationEventList *ud = (AutomationEventList *)lua_newuserdatauv(L, sizeof(AutomationEventList), 1);
    *ud = list;
    luaL_setmetatable(L, rlua_automationeventlist_mt);
    rluaSetOwnership(L, -1, 1);
}

static void rluaAutomationEventList_Release(lua_State *L, int index)
{
    AutomationEventList *list = luaL_testudata(L, index, rlua_automationeventlist_mt);
    if (list == NULL) return;
    if (rluaGetOwnership(L, index) == 0) return;

    UnloadAutomationEventList(*list);
    *list = (AutomationEventList){ 0 };
    rluaSetOwnership(L, index, 0);
}

static int rluaUnloadAutomationEventList(lua_State *L);

static int rluaAutomationEventListToString(lua_State *L)
{
    AutomationEventList *list = rluaAutomationEventListCheck(L, 1);
    lua_pushfstring(L, "AutomationEventList(capacity=%u, count=%u)", list->capacity, list->count);
    return 1;
}

static int rluaAutomationEventListIndex(lua_State *L)
{
    AutomationEventList *list = rluaAutomationEventListCheck(L, 1);
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "capacity") == 0) lua_pushinteger(L, (lua_Integer)list->capacity);
    else if (strcmp(field, "count") == 0) lua_pushinteger(L, (lua_Integer)list->count);
    else if (strcmp(field, "unload") == 0) lua_pushcfunction(L, rluaUnloadAutomationEventList);
    else if (strcmp(field, "export") == 0) lua_pushcfunction(L, rluaExportAutomationEventList);
    else lua_pushnil(L);

    return 1;
}

static int rluaAutomationEventListGc(lua_State *L)
{
    rluaAutomationEventList_Release(L, 1);
    return 0;
}

static int rluaUnloadAutomationEventList(lua_State *L)
{
    rluaAutomationEventListCheck(L, 1);
    rluaAutomationEventList_Release(L, 1);
    return 0;
}

static AutomationEvent rluaAutomationEventCheck(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);

    AutomationEvent event = { 0 };
    lua_getfield(L, index, "frame");
    event.frame = (unsigned int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, index, "type");
    event.type = (unsigned int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "params");
    luaL_checktype(L, -1, LUA_TTABLE);

    for (int i = 0; i < 4; i++)
    {
        lua_rawgeti(L, -1, i + 1);
        event.params[i] = (int)luaL_checkinteger(L, -1);
        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    return event;
}

static int rluaLoadAutomationEventList(lua_State *L)
{
    const char *fileName = luaL_optstring(L, 1, NULL);
    rluaAutomationEventListPush(L, LoadAutomationEventList(fileName));
    return 1;
}

static int rluaExportAutomationEventList(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    AutomationEventList *list = rluaAutomationEventListCheck(L, 1);
    const char *fileName = luaL_checkstring(L, 2);
    lua_pushboolean(L, ExportAutomationEventList(*list, fileName));
    return 1;
}

static int rluaSetAutomationEventList(lua_State *L)
{
    if (lua_isnoneornil(L, 1))
    {
        SetAutomationEventList(NULL);
        return 0;
    }

    SetAutomationEventList(rluaAutomationEventListCheck(L, 1));
    return 0;
}

static int rluaSetAutomationEventBaseFrame(lua_State *L)
{
    SetAutomationEventBaseFrame((int)luaL_checkinteger(L, 1));
    return 0;
}

static int rluaStartAutomationEventRecording(lua_State *L)
{
    StartAutomationEventRecording();
    return 0;
}

static int rluaStopAutomationEventRecording(lua_State *L)
{
    StopAutomationEventRecording();
    return 0;
}

static int rluaPlayAutomationEvent(lua_State *L)
{
    PlayAutomationEvent(rluaAutomationEventCheck(L, 1));
    return 0;
}

static void rluaAutomationEventListRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_automationeventlist_mt))
    {
        REG(lua_pushcfunction(L, rluaAutomationEventListToString), "__tostring");
        REG(lua_pushcfunction(L, rluaAutomationEventListIndex), "__index");
        REG(lua_pushcfunction(L, rluaAutomationEventListGc), "__gc");
        REG(lua_pushcfunction(L, rluaAutomationEventListGc), "__close");
    }

    lua_pushstring(L, "AutomationEventList");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaLoadAutomationEventList), "LoadAutomationEventList");
    REG(lua_pushcfunction(L, rluaSetAutomationEventList), "SetAutomationEventList");
    REG(lua_pushcfunction(L, rluaSetAutomationEventBaseFrame), "SetAutomationEventBaseFrame");
    REG(lua_pushcfunction(L, rluaStartAutomationEventRecording), "StartAutomationEventRecording");
    REG(lua_pushcfunction(L, rluaStopAutomationEventRecording), "StopAutomationEventRecording");
    REG(lua_pushcfunction(L, rluaPlayAutomationEvent), "PlayAutomationEvent");
}

static void rluaCoreRegister(lua_State *L)
{
    REG(lua_pushcfunction(L, rluaInitWindow), "InitWindow");
    REG(lua_pushcfunction(L, rluaCloseWindow), "CloseWindow");
    REG(lua_pushcfunction(L, rluaWindowShouldClose), "WindowShouldClose");
    REG(lua_pushcfunction(L, rluaIsWindowReady), "IsWindowReady");
    REG(lua_pushcfunction(L, rluaIsWindowFullscreen), "IsWindowFullscreen");
    REG(lua_pushcfunction(L, rluaIsWindowHidden), "IsWindowHidden");
    REG(lua_pushcfunction(L, rluaIsWindowMinimized), "IsWindowMinimized");
    REG(lua_pushcfunction(L, rluaIsWindowMaximized), "IsWindowMaximized");
    REG(lua_pushcfunction(L, rluaIsWindowFocused), "IsWindowFocused");
    REG(lua_pushcfunction(L, rluaIsWindowResized), "IsWindowResized");
    REG(lua_pushcfunction(L, rluaIsWindowState), "IsWindowState");
    REG(lua_pushcfunction(L, rluaSetWindowState), "SetWindowState");
    REG(lua_pushcfunction(L, rluaClearWindowState), "ClearWindowState");
    REG(lua_pushcfunction(L, rluaToggleFullscreen), "ToggleFullscreen");
    REG(lua_pushcfunction(L, rluaToggleBorderlessWindowed), "ToggleBorderlessWindowed");
    REG(lua_pushcfunction(L, rluaMaximizeWindow), "MaximizeWindow");
    REG(lua_pushcfunction(L, rluaMinimizeWindow), "MinimizeWindow");
    REG(lua_pushcfunction(L, rluaRestoreWindow), "RestoreWindow");
    REG(lua_pushcfunction(L, rluaSetWindowTitle), "SetWindowTitle");
    REG(lua_pushcfunction(L, rluaSetWindowPosition), "SetWindowPosition");
    REG(lua_pushcfunction(L, rluaSetWindowMonitor), "SetWindowMonitor");
    REG(lua_pushcfunction(L, rluaSetWindowMinSize), "SetWindowMinSize");
    REG(lua_pushcfunction(L, rluaSetWindowMaxSize), "SetWindowMaxSize");
    REG(lua_pushcfunction(L, rluaSetWindowSize), "SetWindowSize");
    REG(lua_pushcfunction(L, rluaSetWindowOpacity), "SetWindowOpacity");
    REG(lua_pushcfunction(L, rluaSetWindowFocused), "SetWindowFocused");
    REG(lua_pushcfunction(L, rluaGetWindowHandle), "GetWindowHandle");
    REG(lua_pushcfunction(L, rluaGetScreenWidth), "GetScreenWidth");
    REG(lua_pushcfunction(L, rluaGetScreenHeight), "GetScreenHeight");
    REG(lua_pushcfunction(L, rluaGetRenderWidth), "GetRenderWidth");
    REG(lua_pushcfunction(L, rluaGetRenderHeight), "GetRenderHeight");
    REG(lua_pushcfunction(L, rluaGetMonitorCount), "GetMonitorCount");
    REG(lua_pushcfunction(L, rluaGetCurrentMonitor), "GetCurrentMonitor");
    REG(lua_pushcfunction(L, rluaGetMonitorPosition), "GetMonitorPosition");
    REG(lua_pushcfunction(L, rluaGetMonitorWidth), "GetMonitorWidth");
    REG(lua_pushcfunction(L, rluaGetMonitorHeight), "GetMonitorHeight");
    REG(lua_pushcfunction(L, rluaGetMonitorPhysicalWidth), "GetMonitorPhysicalWidth");
    REG(lua_pushcfunction(L, rluaGetMonitorPhysicalHeight), "GetMonitorPhysicalHeight");
    REG(lua_pushcfunction(L, rluaGetMonitorRefreshRate), "GetMonitorRefreshRate");
    REG(lua_pushcfunction(L, rluaGetWindowPosition), "GetWindowPosition");
    REG(lua_pushcfunction(L, rluaGetWindowScaleDPI), "GetWindowScaleDPI");
    REG(lua_pushcfunction(L, rluaGetMonitorName), "GetMonitorName");
    REG(lua_pushcfunction(L, rluaSetClipboardText), "SetClipboardText");
    REG(lua_pushcfunction(L, rluaGetClipboardText), "GetClipboardText");
    REG(lua_pushcfunction(L, rluaEnableEventWaiting), "EnableEventWaiting");
    REG(lua_pushcfunction(L, rluaDisableEventWaiting), "DisableEventWaiting");
    REG(lua_pushcfunction(L, rluaShowCursor), "ShowCursor");
    REG(lua_pushcfunction(L, rluaHideCursor), "HideCursor");
    REG(lua_pushcfunction(L, rluaIsCursorHidden), "IsCursorHidden");
    REG(lua_pushcfunction(L, rluaEnableCursor), "EnableCursor");
    REG(lua_pushcfunction(L, rluaDisableCursor), "DisableCursor");
    REG(lua_pushcfunction(L, rluaIsCursorOnScreen), "IsCursorOnScreen");
    REG(lua_pushcfunction(L, rluaClearBackground), "ClearBackground");
    REG(lua_pushcfunction(L, rluaBeginDrawing), "BeginDrawing");
    REG(lua_pushcfunction(L, rluaEndDrawing), "EndDrawing");
    REG(lua_pushcfunction(L, rluaBeginMode2D), "BeginMode2D");
    REG(lua_pushcfunction(L, rluaEndMode2D), "EndMode2D");
    REG(lua_pushcfunction(L, rluaBeginMode3D), "BeginMode3D");
    REG(lua_pushcfunction(L, rluaEndMode3D), "EndMode3D");
    REG(lua_pushcfunction(L, rluaBeginTextureMode), "BeginTextureMode");
    REG(lua_pushcfunction(L, rluaEndTextureMode), "EndTextureMode");
    REG(lua_pushcfunction(L, rluaBeginShaderMode), "BeginShaderMode");
    REG(lua_pushcfunction(L, rluaEndShaderMode), "EndShaderMode");
    REG(lua_pushcfunction(L, rluaBeginBlendMode), "BeginBlendMode");
    REG(lua_pushcfunction(L, rluaEndBlendMode), "EndBlendMode");
    REG(lua_pushcfunction(L, rluaBeginScissorMode), "BeginScissorMode");
    REG(lua_pushcfunction(L, rluaEndScissorMode), "EndScissorMode");
    REG(lua_pushcfunction(L, rluaBeginVrStereoMode), "BeginVrStereoMode");
    REG(lua_pushcfunction(L, rluaEndVrStereoMode), "EndVrStereoMode");
    REG(lua_pushcfunction(L, rluaGetScreenToWorldRay), "GetScreenToWorldRay");
    REG(lua_pushcfunction(L, rluaGetScreenToWorldRayEx), "GetScreenToWorldRayEx");
    REG(lua_pushcfunction(L, rluaGetWorldToScreen), "GetWorldToScreen");
    REG(lua_pushcfunction(L, rluaGetWorldToScreenEx), "GetWorldToScreenEx");
    REG(lua_pushcfunction(L, rluaGetWorldToScreen2D), "GetWorldToScreen2D");
    REG(lua_pushcfunction(L, rluaGetScreenToWorld2D), "GetScreenToWorld2D");
    REG(lua_pushcfunction(L, rluaGetCameraMatrix2D), "GetCameraMatrix2D");
    REG(lua_pushcfunction(L, rluaSetTargetFPS), "SetTargetFPS");
    REG(lua_pushcfunction(L, rluaGetFrameTime), "GetFrameTime");
    REG(lua_pushcfunction(L, rluaGetTime), "GetTime");
    REG(lua_pushcfunction(L, rluaGetFPS), "GetFPS");
    REG(lua_pushcfunction(L, rluaSwapScreenBuffer), "SwapScreenBuffer");
    REG(lua_pushcfunction(L, rluaPollInputEvents), "PollInputEvents");
    REG(lua_pushcfunction(L, rluaWaitTime), "WaitTime");
    REG(lua_pushcfunction(L, rluaSetRandomSeed), "SetRandomSeed");
    REG(lua_pushcfunction(L, rluaGetRandomValue), "GetRandomValue");
    REG(lua_pushcfunction(L, rluaLoadRandomSequence), "LoadRandomSequence");
    REG(lua_pushcfunction(L, rluaTakeScreenshot), "TakeScreenshot");
    REG(lua_pushcfunction(L, rluaSetConfigFlags), "SetConfigFlags");
    REG(lua_pushcfunction(L, rluaOpenURL), "OpenURL");
    REG(lua_pushcfunction(L, rluaSetTraceLogLevel), "SetTraceLogLevel");
    REG(lua_pushcfunction(L, rluaSetTraceLogCallback), "SetTraceLogCallback");
    REG(lua_pushcfunction(L, rluaSetLoadFileDataCallback), "SetLoadFileDataCallback");
    REG(lua_pushcfunction(L, rluaSetSaveFileDataCallback), "SetSaveFileDataCallback");
    REG(lua_pushcfunction(L, rluaSetLoadFileTextCallback), "SetLoadFileTextCallback");
    REG(lua_pushcfunction(L, rluaSetSaveFileTextCallback), "SetSaveFileTextCallback");
    REG(lua_pushcfunction(L, rluaTraceLog), "TraceLog");
    REG(lua_pushcfunction(L, rluaLoadFileData), "LoadFileData");
    REG(lua_pushcfunction(L, rluaSaveFileData), "SaveFileData");
    REG(lua_pushcfunction(L, rluaExportDataAsCode), "ExportDataAsCode");
    REG(lua_pushcfunction(L, rluaLoadFileText), "LoadFileText");
    REG(lua_pushcfunction(L, rluaSaveFileText), "SaveFileText");
    REG(lua_pushcfunction(L, rluaFileRename), "FileRename");
    REG(lua_pushcfunction(L, rluaFileRemove), "FileRemove");
    REG(lua_pushcfunction(L, rluaFileCopy), "FileCopy");
    REG(lua_pushcfunction(L, rluaFileMove), "FileMove");
    REG(lua_pushcfunction(L, rluaFileTextReplace), "FileTextReplace");
    REG(lua_pushcfunction(L, rluaFileTextFindIndex), "FileTextFindIndex");
    REG(lua_pushcfunction(L, rluaFileExists), "FileExists");
    REG(lua_pushcfunction(L, rluaDirectoryExists), "DirectoryExists");
    REG(lua_pushcfunction(L, rluaIsFileExtension), "IsFileExtension");
    REG(lua_pushcfunction(L, rluaIsFileHidden), "IsFileHidden");
    REG(lua_pushcfunction(L, rluaGetFileLength), "GetFileLength");
    REG(lua_pushcfunction(L, rluaGetFileModTime), "GetFileModTime");
    REG(lua_pushcfunction(L, rluaGetFileExtension), "GetFileExtension");
    REG(lua_pushcfunction(L, rluaGetFileName), "GetFileName");
    REG(lua_pushcfunction(L, rluaGetFileNameWithoutExt), "GetFileNameWithoutExt");
    REG(lua_pushcfunction(L, rluaGetDirectoryPath), "GetDirectoryPath");
    REG(lua_pushcfunction(L, rluaGetPrevDirectoryPath), "GetPrevDirectoryPath");
    REG(lua_pushcfunction(L, rluaGetWorkingDirectory), "GetWorkingDirectory");
    REG(lua_pushcfunction(L, rluaGetApplicationDirectory), "GetApplicationDirectory");
    REG(lua_pushcfunction(L, rluaMakeDirectory), "MakeDirectory");
    REG(lua_pushcfunction(L, rluaChangeDirectory), "ChangeDirectory");
    REG(lua_pushcfunction(L, rluaIsPathFile), "IsPathFile");
    REG(lua_pushcfunction(L, rluaIsPathDirectory), "IsPathDirectory");
    REG(lua_pushcfunction(L, rluaIsPathAbsolute), "IsPathAbsolute");
    REG(lua_pushcfunction(L, rluaIsFileNameValid), "IsFileNameValid");
    REG(lua_pushcfunction(L, rluaLoadDirectoryFiles), "LoadDirectoryFiles");
    REG(lua_pushcfunction(L, rluaLoadDirectoryFilesEx), "LoadDirectoryFilesEx");
    REG(lua_pushcfunction(L, rluaIsFileDropped), "IsFileDropped");
    REG(lua_pushcfunction(L, rluaLoadDroppedFiles), "LoadDroppedFiles");
    REG(lua_pushcfunction(L, rluaGetDirectoryFileCount), "GetDirectoryFileCount");
    REG(lua_pushcfunction(L, rluaGetDirectoryFileCountEx), "GetDirectoryFileCountEx");
    REG(lua_pushcfunction(L, rluaCompressData), "CompressData");
    REG(lua_pushcfunction(L, rluaDecompressData), "DecompressData");
    REG(lua_pushcfunction(L, rluaEncodeDataBase64), "EncodeDataBase64");
    REG(lua_pushcfunction(L, rluaDecodeDataBase64), "DecodeDataBase64");
    REG(lua_pushcfunction(L, rluaComputeCRC32), "ComputeCRC32");
    REG(lua_pushcfunction(L, rluaComputeMD5), "ComputeMD5");
    REG(lua_pushcfunction(L, rluaComputeSHA1), "ComputeSHA1");
    REG(lua_pushcfunction(L, rluaComputeSHA256), "ComputeSHA256");
    REG(lua_pushcfunction(L, rluaIsKeyPressed), "IsKeyPressed");
    REG(lua_pushcfunction(L, rluaIsKeyPressedRepeat), "IsKeyPressedRepeat");
    REG(lua_pushcfunction(L, rluaIsKeyDown), "IsKeyDown");
    REG(lua_pushcfunction(L, rluaIsKeyReleased), "IsKeyReleased");
    REG(lua_pushcfunction(L, rluaIsKeyUp), "IsKeyUp");
    REG(lua_pushcfunction(L, rluaGetKeyPressed), "GetKeyPressed");
    REG(lua_pushcfunction(L, rluaGetCharPressed), "GetCharPressed");
    REG(lua_pushcfunction(L, rluaGetKeyName), "GetKeyName");
    REG(lua_pushcfunction(L, rluaSetExitKey), "SetExitKey");
    REG(lua_pushcfunction(L, rluaIsGamepadAvailable), "IsGamepadAvailable");
    REG(lua_pushcfunction(L, rluaGetGamepadName), "GetGamepadName");
    REG(lua_pushcfunction(L, rluaIsGamepadButtonPressed), "IsGamepadButtonPressed");
    REG(lua_pushcfunction(L, rluaIsGamepadButtonDown), "IsGamepadButtonDown");
    REG(lua_pushcfunction(L, rluaIsGamepadButtonReleased), "IsGamepadButtonReleased");
    REG(lua_pushcfunction(L, rluaIsGamepadButtonUp), "IsGamepadButtonUp");
    REG(lua_pushcfunction(L, rluaGetGamepadButtonPressed), "GetGamepadButtonPressed");
    REG(lua_pushcfunction(L, rluaGetGamepadAxisCount), "GetGamepadAxisCount");
    REG(lua_pushcfunction(L, rluaGetGamepadAxisMovement), "GetGamepadAxisMovement");
    REG(lua_pushcfunction(L, rluaSetGamepadMappings), "SetGamepadMappings");
    REG(lua_pushcfunction(L, rluaSetGamepadVibration), "SetGamepadVibration");
    REG(lua_pushcfunction(L, rluaIsMouseButtonPressed), "IsMouseButtonPressed");
    REG(lua_pushcfunction(L, rluaIsMouseButtonDown), "IsMouseButtonDown");
    REG(lua_pushcfunction(L, rluaIsMouseButtonReleased), "IsMouseButtonReleased");
    REG(lua_pushcfunction(L, rluaIsMouseButtonUp), "IsMouseButtonUp");
    REG(lua_pushcfunction(L, rluaGetMouseX), "GetMouseX");
    REG(lua_pushcfunction(L, rluaGetMouseY), "GetMouseY");
    REG(lua_pushcfunction(L, rluaGetMousePosition), "GetMousePosition");
    REG(lua_pushcfunction(L, rluaGetMouseDelta), "GetMouseDelta");
    REG(lua_pushcfunction(L, rluaSetMousePosition), "SetMousePosition");
    REG(lua_pushcfunction(L, rluaSetMouseOffset), "SetMouseOffset");
    REG(lua_pushcfunction(L, rluaSetMouseScale), "SetMouseScale");
    REG(lua_pushcfunction(L, rluaGetMouseWheelMove), "GetMouseWheelMove");
    REG(lua_pushcfunction(L, rluaGetMouseWheelMoveV), "GetMouseWheelMoveV");
    REG(lua_pushcfunction(L, rluaSetMouseCursor), "SetMouseCursor");
    REG(lua_pushcfunction(L, rluaGetTouchX), "GetTouchX");
    REG(lua_pushcfunction(L, rluaGetTouchY), "GetTouchY");
    REG(lua_pushcfunction(L, rluaGetTouchPosition), "GetTouchPosition");
    REG(lua_pushcfunction(L, rluaGetTouchPointId), "GetTouchPointId");
    REG(lua_pushcfunction(L, rluaGetTouchPointCount), "GetTouchPointCount");
}

// Gestures

static int rluaSetGesturesEnabled(lua_State *L)
{
    unsigned int flags = (unsigned int)luaL_checkinteger(L, 1);
    SetGesturesEnabled(flags);
    return 0;
}

static int rluaIsGestureDetected(lua_State *L)
{
    unsigned int gesture = (unsigned int)luaL_checkinteger(L, 1);
    lua_pushboolean(L, IsGestureDetected(gesture));
    return 1;
}

static int rluaGetGestureDetected(lua_State *L)
{
    lua_pushinteger(L, GetGestureDetected());
    return 1;
}

static int rluaGetGestureHoldDuration(lua_State *L)
{
    lua_pushnumber(L, GetGestureHoldDuration());
    return 1;
}

static int rluaGetGestureDragVector(lua_State *L)
{
    rluaVector2Push(L, GetGestureDragVector());
    return 1;
}

static int rluaGetGestureDragAngle(lua_State *L)
{
    lua_pushnumber(L, GetGestureDragAngle());
    return 1;
}

static int rluaGetGesturePinchVector(lua_State *L)
{
    rluaVector2Push(L, GetGesturePinchVector());
    return 1;
}

static int rluaGetGesturePinchAngle(lua_State *L)
{
    lua_pushnumber(L, GetGesturePinchAngle());
    return 1;
}

static void rluaGesturesRegister(lua_State *L)
{
    REG(lua_pushcfunction(L, rluaSetGesturesEnabled), "SetGesturesEnabled");
    REG(lua_pushcfunction(L, rluaIsGestureDetected), "IsGestureDetected");
    REG(lua_pushcfunction(L, rluaGetGestureDetected), "GetGestureDetected");
    REG(lua_pushcfunction(L, rluaGetGestureHoldDuration), "GetGestureHoldDuration");
    REG(lua_pushcfunction(L, rluaGetGestureDragVector), "GetGestureDragVector");
    REG(lua_pushcfunction(L, rluaGetGestureDragAngle), "GetGestureDragAngle");
    REG(lua_pushcfunction(L, rluaGetGesturePinchVector), "GetGesturePinchVector");
    REG(lua_pushcfunction(L, rluaGetGesturePinchAngle), "GetGesturePinchAngle");
}

// Camera

static Camera2D rluaCamera2DCheck(lua_State *L, int index)
{
    index = lua_absindex(L, index);
    luaL_checktype(L, index, LUA_TTABLE);

    Camera2D camera = { 0 };

    lua_getfield(L, index, "offset");
    camera.offset = rluaVector2Check(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "target");
    camera.target = rluaVector2Check(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "rotation");
    camera.rotation = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "zoom");
    camera.zoom = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    return camera;
}

static Camera3D rluaCamera3DCheck(lua_State *L, int index)
{
    index = lua_absindex(L, index);
    luaL_checktype(L, index, LUA_TTABLE);

    Camera3D camera = { 0 };

    lua_getfield(L, index, "position");
    camera.position = rluaVector3Check(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "target");
    camera.target = rluaVector3Check(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "up");
    camera.up = rluaVector3Check(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "fovy");
    camera.fovy = (float)luaL_checknumber(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, index, "projection");
    camera.projection = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);

    return camera;
}

static void rluaCamera3D_SetFields(lua_State *L, int index, Camera3D camera)
{
    index = lua_absindex(L, index);

    lua_getfield(L, index, "position");
    rluaVector3_SetFields(L, -1, camera.position);
    lua_pop(L, 1);

    lua_getfield(L, index, "target");
    rluaVector3_SetFields(L, -1, camera.target);
    lua_pop(L, 1);

    lua_getfield(L, index, "up");
    rluaVector3_SetFields(L, -1, camera.up);
    lua_pop(L, 1);

    lua_pushnumber(L, camera.fovy);
    lua_setfield(L, index, "fovy");

    lua_pushinteger(L, camera.projection);
    lua_setfield(L, index, "projection");
}


static const char rlua_camera3d_mt[] = "raylib.Camera3D";

static int rluaCamera3D(lua_State *L)
{
    lua_createtable(L, 0, 5);

    rluaVector3Push(L, rluaVector3Check(L, 1));
    lua_setfield(L, -2, "position");

    rluaVector3Push(L, rluaVector3Check(L, 2));
    lua_setfield(L, -2, "target");

    rluaVector3Push(L, rluaVector3Check(L, 3));
    lua_setfield(L, -2, "up");

    lua_pushnumber(L, (lua_Number)luaL_checknumber(L, 4));
    lua_setfield(L, -2, "fovy");

    lua_pushinteger(L, (lua_Integer)luaL_checkinteger(L, 5));
    lua_setfield(L, -2, "projection");

    luaL_setmetatable(L, rlua_camera3d_mt);
    return 1;
}

static int rluaCamera3DMoveForward(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    float distance = (float)luaL_checknumber(L, 2);
    bool moveInWorldPlane = lua_toboolean(L, 3);
    CameraMoveForward(&camera, distance, moveInWorldPlane);
    rluaCamera3D_SetFields(L, 1, camera);
    return 0;
}

static int rluaCamera3DMoveUp(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    float distance = (float)luaL_checknumber(L, 2);
    CameraMoveUp(&camera, distance);
    rluaCamera3D_SetFields(L, 1, camera);
    return 0;
}

static int rluaCamera3DMoveRight(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    float distance = (float)luaL_checknumber(L, 2);
    bool moveInWorldPlane = lua_toboolean(L, 3);
    CameraMoveRight(&camera, distance, moveInWorldPlane);
    rluaCamera3D_SetFields(L, 1, camera);
    return 0;
}

static int rluaCamera3DMoveToTarget(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    float delta = (float)luaL_checknumber(L, 2);
    CameraMoveToTarget(&camera, delta);
    rluaCamera3D_SetFields(L, 1, camera);
    return 0;
}

static int rluaCamera3DYaw(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    float angle = (float)luaL_checknumber(L, 2);
    bool rotateAroundTarget = lua_toboolean(L, 3);
    CameraYaw(&camera, angle, rotateAroundTarget);
    rluaCamera3D_SetFields(L, 1, camera);
    return 0;
}

static int rluaCamera3DPitch(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    float angle = (float)luaL_checknumber(L, 2);
    bool lockView = lua_toboolean(L, 3);
    bool rotateAroundTarget = lua_toboolean(L, 4);
    bool rotateUp = lua_toboolean(L, 5);
    CameraPitch(&camera, angle, lockView, rotateAroundTarget, rotateUp);
    rluaCamera3D_SetFields(L, 1, camera);
    return 0;
}

static int rluaCamera3DRoll(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    float angle = (float)luaL_checknumber(L, 2);
    CameraRoll(&camera, angle);
    rluaCamera3D_SetFields(L, 1, camera);
    return 0;
}

static int rluaCamera3DForward(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    rluaVector3Push(L, GetCameraForward(&camera));
    return 1;
}

static int rluaCamera3DUp(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    rluaVector3Push(L, GetCameraUp(&camera));
    return 1;
}

static int rluaCamera3DRight(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    rluaVector3Push(L, GetCameraRight(&camera));
    return 1;
}

static int rluaCamera3DViewMatrix(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    rluaMatrixPush(L, GetCameraViewMatrix(&camera));
    return 1;
}

static int rluaCamera3DProjectionMatrix(lua_State *L)
{
    Camera3D camera = rluaCamera3DCheck(L, 1);
    float aspect = (float)luaL_checknumber(L, 2);
    rluaMatrixPush(L, GetCameraProjectionMatrix(&camera, aspect));
    return 1;
}

static int rluaCamera3DIndex(lua_State *L)
{
    const char *field = luaL_checkstring(L, 2);

    if (strcmp(field, "forward") == 0) lua_pushcfunction(L, rluaCamera3DForward);
    else if (strcmp(field, "upVector") == 0) lua_pushcfunction(L, rluaCamera3DUp);
    else if (strcmp(field, "right") == 0) lua_pushcfunction(L, rluaCamera3DRight);
    else if (strcmp(field, "moveForward") == 0) lua_pushcfunction(L, rluaCamera3DMoveForward);
    else if (strcmp(field, "moveUp") == 0) lua_pushcfunction(L, rluaCamera3DMoveUp);
    else if (strcmp(field, "moveRight") == 0) lua_pushcfunction(L, rluaCamera3DMoveRight);
    else if (strcmp(field, "moveToTarget") == 0) lua_pushcfunction(L, rluaCamera3DMoveToTarget);
    else if (strcmp(field, "yaw") == 0) lua_pushcfunction(L, rluaCamera3DYaw);
    else if (strcmp(field, "pitch") == 0) lua_pushcfunction(L, rluaCamera3DPitch);
    else if (strcmp(field, "roll") == 0) lua_pushcfunction(L, rluaCamera3DRoll);
    else if (strcmp(field, "viewMatrix") == 0) lua_pushcfunction(L, rluaCamera3DViewMatrix);
    else if (strcmp(field, "projectionMatrix") == 0) lua_pushcfunction(L, rluaCamera3DProjectionMatrix);
    else if (strcmp(field, "getMatrix") == 0) lua_pushcfunction(L, rluaGetCameraMatrix);
    else if (strcmp(field, "update") == 0) lua_pushcfunction(L, rluaUpdateCamera);
    else if (strcmp(field, "updatePro") == 0) lua_pushcfunction(L, rluaUpdateCameraPro);
    else if (strcmp(field, "drawBillboard") == 0) lua_pushcfunction(L, rluaDrawBillboard);
    else if (strcmp(field, "drawBillboardRec") == 0) lua_pushcfunction(L, rluaDrawBillboardRec);
    else if (strcmp(field, "drawBillboardPro") == 0) lua_pushcfunction(L, rluaDrawBillboardPro);
    else lua_pushnil(L);

    return 1;
}

static int rluaUpdateCamera(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Camera3D camera = rluaCamera3DCheck(L, 1);
    int mode = (int)luaL_checkinteger(L, 2);
    UpdateCamera(&camera, mode);
    rluaCamera3D_SetFields(L, 1, camera);
    return 0;
}

static int rluaUpdateCameraPro(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Camera3D camera = rluaCamera3DCheck(L, 1);
    Vector3 movement = rluaVector3Check(L, 2);
    Vector3 rotation = rluaVector3Check(L, 3);
    float zoom = (float)luaL_checknumber(L, 4);
    UpdateCameraPro(&camera, movement, rotation, zoom);
    rluaCamera3D_SetFields(L, 1, camera);
    return 0;
}

static void rluaCameraRegister(lua_State *L)
{
    if (luaL_newmetatable(L, rlua_camera3d_mt))
    {
        REG(lua_pushcfunction(L, rluaCamera3DIndex), "__index");
    }

    lua_pushstring(L, "Camera3D");
    lua_setfield(L, -2, "__name");
    lua_pop(L, 1);

    REG(lua_pushcfunction(L, rluaCamera3D), "Camera3D");
}

// Shapes

static int rluaSetShapesTexture(lua_State *L)
{
    Texture *texture = rluaTexture_CheckValid(L, 1);
    Rectangle rec = rluaRectangleCheck(L, 2);
    SetShapesTexture(*texture, rec);
    return 0;
}

static int rluaGetShapesTexture(lua_State *L)
{
    rluaTexturePushBorrowed(L, GetShapesTexture(), 0);
    return 1;
}

static int rluaGetShapesTextureRectangle(lua_State *L)
{
    rluaRectanglePush(L, GetShapesTextureRectangle());
    return 1;
}

static int rluaDrawPixel(lua_State *L)
{
    int posX = (int)luaL_checkinteger(L, 1);
    int posY = (int)luaL_checkinteger(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawPixel(posX, posY, color);
    return 0;
}

static int rluaDrawPixelV(lua_State *L)
{
    Vector2 position = rluaVector2Check(L, 1);
    Color color = rluaColorCheck(L, 2);
    DrawPixelV(position, color);
    return 0;
}

static int rluaDrawLine(lua_State *L)
{
    int startPosX = (int)luaL_checkinteger(L, 1);
    int startPosY = (int)luaL_checkinteger(L, 2);
    int endPosX = (int)luaL_checkinteger(L, 3);
    int endPosY = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawLine(startPosX, startPosY, endPosX, endPosY, color);
    return 0;
}

static int rluaDrawLineV(lua_State *L)
{
    Vector2 startPos = rluaVector2Check(L, 1);
    Vector2 endPos = rluaVector2Check(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawLineV(startPos, endPos, color);
    return 0;
}

static int rluaDrawLineEx(lua_State *L)
{
    Vector2 startPos = rluaVector2Check(L, 1);
    Vector2 endPos = rluaVector2Check(L, 2);
    float thick = (float)luaL_checknumber(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawLineEx(startPos, endPos, thick, color);
    return 0;
}

static int rluaDrawLineStrip(lua_State *L)
{
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 1, &count);
    Color color = rluaColorCheck(L, 2);
    DrawLineStrip(points, count, color);
    return 0;
}

static int rluaDrawLineBezier(lua_State *L)
{
    Vector2 startPos = rluaVector2Check(L, 1);
    Vector2 endPos = rluaVector2Check(L, 2);
    float thick = (float)luaL_checknumber(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawLineBezier(startPos, endPos, thick, color);
    return 0;
}

static int rluaDrawLineDashed(lua_State *L)
{
    Vector2 startPos = rluaVector2Check(L, 1);
    Vector2 endPos = rluaVector2Check(L, 2);
    int dashSize = (int)luaL_checkinteger(L, 3);
    int spaceSize = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawLineDashed(startPos, endPos, dashSize, spaceSize, color);
    return 0;
}

static int rluaDrawTriangle(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    Vector2 v3 = rluaVector2Check(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawTriangle(v1, v2, v3, color);
    return 0;
}

static int rluaDrawTriangleGradient(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    Vector2 v3 = rluaVector2Check(L, 3);
    Color c1 = rluaColorCheck(L, 4);
    Color c2 = rluaColorCheck(L, 5);
    Color c3 = rluaColorCheck(L, 6);
    DrawTriangleGradient(v1, v2, v3, c1, c2, c3);
    return 0;
}

static int rluaDrawTriangleLines(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    Vector2 v3 = rluaVector2Check(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawTriangleLines(v1, v2, v3, color);
    return 0;
}

static int rluaDrawTriangleLinesEx(lua_State *L)
{
    Vector2 v1 = rluaVector2Check(L, 1);
    Vector2 v2 = rluaVector2Check(L, 2);
    Vector2 v3 = rluaVector2Check(L, 3);
    float thick = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawTriangleLinesEx(v1, v2, v3, thick, color);
    return 0;
}

static int rluaDrawTriangleFan(lua_State *L)
{
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 1, &count);
    Color color = rluaColorCheck(L, 2);
    DrawTriangleFan(points, count, color);
    return 0;
}

static int rluaDrawTriangleStrip(lua_State *L)
{
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 1, &count);
    Color color = rluaColorCheck(L, 2);
    DrawTriangleStrip(points, count, color);
    return 0;
}

static int rluaDrawRectangle(lua_State *L)
{
    int posX = (int)luaL_checkinteger(L, 1);
    int posY = (int)luaL_checkinteger(L, 2);
    int width = (int)luaL_checkinteger(L, 3);
    int height = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawRectangle(posX, posY, width, height, color);
    return 0;
}

static int rluaDrawRectangleV(lua_State *L)
{
    Vector2 position = rluaVector2Check(L, 1);
    Vector2 size = rluaVector2Check(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawRectangleV(position, size, color);
    return 0;
}

static int rluaDrawRectangleRec(lua_State *L)
{
    Rectangle rec = rluaRectangleCheck(L, 1);
    Color color = rluaColorCheck(L, 2);
    DrawRectangleRec(rec, color);
    return 0;
}

static int rluaDrawRectanglePro(lua_State *L)
{
    Rectangle rec = rluaRectangleCheck(L, 1);
    Vector2 origin = rluaVector2Check(L, 2);
    float rotation = (float)luaL_checknumber(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawRectanglePro(rec, origin, rotation, color);
    return 0;
}

static int rluaDrawRectangleGradientV(lua_State *L)
{
    int posX = (int)luaL_checkinteger(L, 1);
    int posY = (int)luaL_checkinteger(L, 2);
    int width = (int)luaL_checkinteger(L, 3);
    int height = (int)luaL_checkinteger(L, 4);
    Color top = rluaColorCheck(L, 5);
    Color bottom = rluaColorCheck(L, 6);
    DrawRectangleGradientV(posX, posY, width, height, top, bottom);
    return 0;
}

static int rluaDrawRectangleGradientH(lua_State *L)
{
    int posX = (int)luaL_checkinteger(L, 1);
    int posY = (int)luaL_checkinteger(L, 2);
    int width = (int)luaL_checkinteger(L, 3);
    int height = (int)luaL_checkinteger(L, 4);
    Color left = rluaColorCheck(L, 5);
    Color right = rluaColorCheck(L, 6);
    DrawRectangleGradientH(posX, posY, width, height, left, right);
    return 0;
}

static int rluaDrawRectangleGradientEx(lua_State *L)
{
    Rectangle rec = rluaRectangleCheck(L, 1);
    Color col1 = rluaColorCheck(L, 2);
    Color col2 = rluaColorCheck(L, 3);
    Color col3 = rluaColorCheck(L, 4);
    Color col4 = rluaColorCheck(L, 5);
    DrawRectangleGradientEx(rec, col1, col2, col3, col4);
    return 0;
}

static int rluaDrawRectangleLines(lua_State *L)
{
    int posX = (int)luaL_checkinteger(L, 1);
    int posY = (int)luaL_checkinteger(L, 2);
    int width = (int)luaL_checkinteger(L, 3);
    int height = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawRectangleLines(posX, posY, width, height, color);
    return 0;
}

static int rluaDrawRectangleLinesEx(lua_State *L)
{
    Rectangle rec = rluaRectangleCheck(L, 1);
    float thick = (float)luaL_checknumber(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawRectangleLinesEx(rec, thick, color);
    return 0;
}

static int rluaDrawRectangleRounded(lua_State *L)
{
    Rectangle rec = rluaRectangleCheck(L, 1);
    float roundness = (float)luaL_checknumber(L, 2);
    int segments = (int)luaL_checkinteger(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawRectangleRounded(rec, roundness, segments, color);
    return 0;
}

static int rluaDrawRectangleRoundedLines(lua_State *L)
{
    Rectangle rec = rluaRectangleCheck(L, 1);
    float roundness = (float)luaL_checknumber(L, 2);
    int segments = (int)luaL_checkinteger(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawRectangleRoundedLines(rec, roundness, segments, color);
    return 0;
}

static int rluaDrawRectangleRoundedLinesEx(lua_State *L)
{
    Rectangle rec = rluaRectangleCheck(L, 1);
    float roundness = (float)luaL_checknumber(L, 2);
    int segments = (int)luaL_checkinteger(L, 3);
    float thick = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawRectangleRoundedLinesEx(rec, roundness, segments, thick, color);
    return 0;
}

static int rluaDrawPoly(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    int sides = (int)luaL_checkinteger(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    float rotation = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawPoly(center, sides, radius, rotation, color);
    return 0;
}

static int rluaDrawPolyLines(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    int sides = (int)luaL_checkinteger(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    float rotation = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawPolyLines(center, sides, radius, rotation, color);
    return 0;
}

static int rluaDrawPolyLinesEx(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    int sides = (int)luaL_checkinteger(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    float rotation = (float)luaL_checknumber(L, 4);
    float thick = (float)luaL_checknumber(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawPolyLinesEx(center, sides, radius, rotation, thick, color);
    return 0;
}

static int rluaDrawCircle(lua_State *L)
{
    int centerX = (int)luaL_checkinteger(L, 1);
    int centerY = (int)luaL_checkinteger(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawCircle(centerX, centerY, radius, color);
    return 0;
}

static int rluaDrawCircleV(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawCircleV(center, radius, color);
    return 0;
}

static int rluaDrawCircleGradient(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    Color inner = rluaColorCheck(L, 3);
    Color outer = rluaColorCheck(L, 4);
    DrawCircleGradient(center, radius, inner, outer);
    return 0;
}

static int rluaDrawCircleSector(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    float startAngle = (float)luaL_checknumber(L, 3);
    float endAngle = (float)luaL_checknumber(L, 4);
    int segments = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawCircleSector(center, radius, startAngle, endAngle, segments, color);
    return 0;
}

static int rluaDrawCircleSectorLines(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    float startAngle = (float)luaL_checknumber(L, 3);
    float endAngle = (float)luaL_checknumber(L, 4);
    int segments = (int)luaL_checkinteger(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawCircleSectorLines(center, radius, startAngle, endAngle, segments, color);
    return 0;
}

static int rluaDrawCircleSectorLinesEx(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    float startAngle = (float)luaL_checknumber(L, 3);
    float endAngle = (float)luaL_checknumber(L, 4);
    int segments = (int)luaL_checkinteger(L, 5);
    float thick = (float)luaL_checknumber(L, 6);
    Color color = rluaColorCheck(L, 7);
    DrawCircleSectorLinesEx(center, radius, startAngle, endAngle, segments, thick, color);
    return 0;
}

static int rluaDrawCircleLines(lua_State *L)
{
    int centerX = (int)luaL_checkinteger(L, 1);
    int centerY = (int)luaL_checkinteger(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawCircleLines(centerX, centerY, radius, color);
    return 0;
}

static int rluaDrawCircleLinesV(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawCircleLinesV(center, radius, color);
    return 0;
}

static int rluaDrawCircleLinesEx(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    float thick = (float)luaL_checknumber(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawCircleLinesEx(center, radius, thick, color);
    return 0;
}

static int rluaDrawEllipse(lua_State *L)
{
    int centerX = (int)luaL_checkinteger(L, 1);
    int centerY = (int)luaL_checkinteger(L, 2);
    float radiusH = (float)luaL_checknumber(L, 3);
    float radiusV = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawEllipse(centerX, centerY, radiusH, radiusV, color);
    return 0;
}

static int rluaDrawEllipseV(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radiusH = (float)luaL_checknumber(L, 2);
    float radiusV = (float)luaL_checknumber(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawEllipseV(center, radiusH, radiusV, color);
    return 0;
}

static int rluaDrawEllipseLines(lua_State *L)
{
    int centerX = (int)luaL_checkinteger(L, 1);
    int centerY = (int)luaL_checkinteger(L, 2);
    float radiusH = (float)luaL_checknumber(L, 3);
    float radiusV = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawEllipseLines(centerX, centerY, radiusH, radiusV, color);
    return 0;
}

static int rluaDrawEllipseLinesV(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radiusH = (float)luaL_checknumber(L, 2);
    float radiusV = (float)luaL_checknumber(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawEllipseLinesV(center, radiusH, radiusV, color);
    return 0;
}

static int rluaDrawEllipseLinesEx(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radiusH = (float)luaL_checknumber(L, 2);
    float radiusV = (float)luaL_checknumber(L, 3);
    float thick = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawEllipseLinesEx(center, radiusH, radiusV, thick, color);
    return 0;
}

static int rluaDrawRing(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float innerRadius = (float)luaL_checknumber(L, 2);
    float outerRadius = (float)luaL_checknumber(L, 3);
    float startAngle = (float)luaL_checknumber(L, 4);
    float endAngle = (float)luaL_checknumber(L, 5);
    int segments = (int)luaL_checkinteger(L, 6);
    Color color = rluaColorCheck(L, 7);
    DrawRing(center, innerRadius, outerRadius, startAngle, endAngle, segments, color);
    return 0;
}

static int rluaDrawRingLines(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float innerRadius = (float)luaL_checknumber(L, 2);
    float outerRadius = (float)luaL_checknumber(L, 3);
    float startAngle = (float)luaL_checknumber(L, 4);
    float endAngle = (float)luaL_checknumber(L, 5);
    int segments = (int)luaL_checkinteger(L, 6);
    Color color = rluaColorCheck(L, 7);
    DrawRingLines(center, innerRadius, outerRadius, startAngle, endAngle, segments, color);
    return 0;
}

static int rluaDrawRingLinesEx(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float innerRadius = (float)luaL_checknumber(L, 2);
    float outerRadius = (float)luaL_checknumber(L, 3);
    float startAngle = (float)luaL_checknumber(L, 4);
    float endAngle = (float)luaL_checknumber(L, 5);
    int segments = (int)luaL_checkinteger(L, 6);
    float thick = (float)luaL_checknumber(L, 7);
    Color color = rluaColorCheck(L, 8);
    DrawRingLinesEx(center, innerRadius, outerRadius, startAngle, endAngle, segments, thick, color);
    return 0;
}

static int rluaDrawSplineLinear(lua_State *L)
{
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 1, &count);
    float thick = (float)luaL_checknumber(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawSplineLinear(points, count, thick, color);
    return 0;
}

static int rluaDrawSplineBasis(lua_State *L)
{
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 1, &count);
    float thick = (float)luaL_checknumber(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawSplineBasis(points, count, thick, color);
    return 0;
}

static int rluaDrawSplineCatmullRom(lua_State *L)
{
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 1, &count);
    float thick = (float)luaL_checknumber(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawSplineCatmullRom(points, count, thick, color);
    return 0;
}

static int rluaDrawSplineBezierQuadratic(lua_State *L)
{
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 1, &count);
    float thick = (float)luaL_checknumber(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawSplineBezierQuadratic(points, count, thick, color);
    return 0;
}

static int rluaDrawSplineBezierCubic(lua_State *L)
{
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 1, &count);
    float thick = (float)luaL_checknumber(L, 2);
    Color color = rluaColorCheck(L, 3);
    DrawSplineBezierCubic(points, count, thick, color);
    return 0;
}

static int rluaDrawSplineSegmentLinear(lua_State *L)
{
    Vector2 p1 = rluaVector2Check(L, 1);
    Vector2 p2 = rluaVector2Check(L, 2);
    float thick = (float)luaL_checknumber(L, 3);
    Color color = rluaColorCheck(L, 4);
    DrawSplineSegmentLinear(p1, p2, thick, color);
    return 0;
}

static int rluaDrawSplineSegmentBasis(lua_State *L)
{
    Vector2 p1 = rluaVector2Check(L, 1);
    Vector2 p2 = rluaVector2Check(L, 2);
    Vector2 p3 = rluaVector2Check(L, 3);
    Vector2 p4 = rluaVector2Check(L, 4);
    float thick = (float)luaL_checknumber(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawSplineSegmentBasis(p1, p2, p3, p4, thick, color);
    return 0;
}

static int rluaDrawSplineSegmentCatmullRom(lua_State *L)
{
    Vector2 p1 = rluaVector2Check(L, 1);
    Vector2 p2 = rluaVector2Check(L, 2);
    Vector2 p3 = rluaVector2Check(L, 3);
    Vector2 p4 = rluaVector2Check(L, 4);
    float thick = (float)luaL_checknumber(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawSplineSegmentCatmullRom(p1, p2, p3, p4, thick, color);
    return 0;
}

static int rluaDrawSplineSegmentBezierQuadratic(lua_State *L)
{
    Vector2 p1 = rluaVector2Check(L, 1);
    Vector2 c2 = rluaVector2Check(L, 2);
    Vector2 p3 = rluaVector2Check(L, 3);
    float thick = (float)luaL_checknumber(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawSplineSegmentBezierQuadratic(p1, c2, p3, thick, color);
    return 0;
}

static int rluaDrawSplineSegmentBezierCubic(lua_State *L)
{
    Vector2 p1 = rluaVector2Check(L, 1);
    Vector2 c2 = rluaVector2Check(L, 2);
    Vector2 c3 = rluaVector2Check(L, 3);
    Vector2 p4 = rluaVector2Check(L, 4);
    float thick = (float)luaL_checknumber(L, 5);
    Color color = rluaColorCheck(L, 6);
    DrawSplineSegmentBezierCubic(p1, c2, c3, p4, thick, color);
    return 0;
}

static int rluaGetSplinePointLinear(lua_State *L)
{
    Vector2 startPos = rluaVector2Check(L, 1);
    Vector2 endPos = rluaVector2Check(L, 2);
    float t = (float)luaL_checknumber(L, 3);
    rluaVector2Push(L, GetSplinePointLinear(startPos, endPos, t));
    return 1;
}

static int rluaGetSplinePointBasis(lua_State *L)
{
    Vector2 p1 = rluaVector2Check(L, 1);
    Vector2 p2 = rluaVector2Check(L, 2);
    Vector2 p3 = rluaVector2Check(L, 3);
    Vector2 p4 = rluaVector2Check(L, 4);
    float t = (float)luaL_checknumber(L, 5);
    rluaVector2Push(L, GetSplinePointBasis(p1, p2, p3, p4, t));
    return 1;
}

static int rluaGetSplinePointCatmullRom(lua_State *L)
{
    Vector2 p1 = rluaVector2Check(L, 1);
    Vector2 p2 = rluaVector2Check(L, 2);
    Vector2 p3 = rluaVector2Check(L, 3);
    Vector2 p4 = rluaVector2Check(L, 4);
    float t = (float)luaL_checknumber(L, 5);
    rluaVector2Push(L, GetSplinePointCatmullRom(p1, p2, p3, p4, t));
    return 1;
}

static int rluaGetSplinePointBezierQuadratic(lua_State *L)
{
    Vector2 p1 = rluaVector2Check(L, 1);
    Vector2 c2 = rluaVector2Check(L, 2);
    Vector2 p3 = rluaVector2Check(L, 3);
    float t = (float)luaL_checknumber(L, 4);
    rluaVector2Push(L, GetSplinePointBezierQuadratic(p1, c2, p3, t));
    return 1;
}

static int rluaGetSplinePointBezierCubic(lua_State *L)
{
    Vector2 p1 = rluaVector2Check(L, 1);
    Vector2 c2 = rluaVector2Check(L, 2);
    Vector2 c3 = rluaVector2Check(L, 3);
    Vector2 p4 = rluaVector2Check(L, 4);
    float t = (float)luaL_checknumber(L, 5);
    rluaVector2Push(L, GetSplinePointBezierCubic(p1, c2, c3, p4, t));
    return 1;
}

static int rluaCheckCollisionRecs(lua_State *L)
{
    Rectangle rec1 = rluaRectangleCheck(L, 1);
    Rectangle rec2 = rluaRectangleCheck(L, 2);
    lua_pushboolean(L, CheckCollisionRecs(rec1, rec2));
    return 1;
}

static int rluaCheckCollisionCircles(lua_State *L)
{
    Vector2 center1 = rluaVector2Check(L, 1);
    float radius1 = (float)luaL_checknumber(L, 2);
    Vector2 center2 = rluaVector2Check(L, 3);
    float radius2 = (float)luaL_checknumber(L, 4);
    lua_pushboolean(L, CheckCollisionCircles(center1, radius1, center2, radius2));
    return 1;
}

static int rluaCheckCollisionCircleRec(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    Rectangle rec = rluaRectangleCheck(L, 3);
    lua_pushboolean(L, CheckCollisionCircleRec(center, radius, rec));
    return 1;
}

static int rluaCheckCollisionCircleLine(lua_State *L)
{
    Vector2 center = rluaVector2Check(L, 1);
    float radius = (float)luaL_checknumber(L, 2);
    Vector2 p1 = rluaVector2Check(L, 3);
    Vector2 p2 = rluaVector2Check(L, 4);
    lua_pushboolean(L, CheckCollisionCircleLine(center, radius, p1, p2));
    return 1;
}

static int rluaCheckCollisionPointRec(lua_State *L)
{
    Vector2 point = rluaVector2Check(L, 1);
    Rectangle rec = rluaRectangleCheck(L, 2);
    lua_pushboolean(L, CheckCollisionPointRec(point, rec));
    return 1;
}

static int rluaCheckCollisionPointCircle(lua_State *L)
{
    Vector2 point = rluaVector2Check(L, 1);
    Vector2 center = rluaVector2Check(L, 2);
    float radius = (float)luaL_checknumber(L, 3);
    lua_pushboolean(L, CheckCollisionPointCircle(point, center, radius));
    return 1;
}

static int rluaCheckCollisionPointTriangle(lua_State *L)
{
    Vector2 point = rluaVector2Check(L, 1);
    Vector2 p1 = rluaVector2Check(L, 2);
    Vector2 p2 = rluaVector2Check(L, 3);
    Vector2 p3 = rluaVector2Check(L, 4);
    lua_pushboolean(L, CheckCollisionPointTriangle(point, p1, p2, p3));
    return 1;
}

static int rluaCheckCollisionPointLine(lua_State *L)
{
    Vector2 point = rluaVector2Check(L, 1);
    Vector2 p1 = rluaVector2Check(L, 2);
    Vector2 p2 = rluaVector2Check(L, 3);
    int threshold = (int)luaL_checkinteger(L, 4);
    lua_pushboolean(L, CheckCollisionPointLine(point, p1, p2, threshold));
    return 1;
}

static int rluaCheckCollisionPointPoly(lua_State *L)
{
    Vector2 point = rluaVector2Check(L, 1);
    int count = 0;
    Vector2 *points = rluaVector2ArrayCheck(L, 2, &count);
    lua_pushboolean(L, CheckCollisionPointPoly(point, points, count));
    return 1;
}

static int rluaCheckCollisionLines(lua_State *L)
{
    Vector2 startPos1 = rluaVector2Check(L, 1);
    Vector2 endPos1 = rluaVector2Check(L, 2);
    Vector2 startPos2 = rluaVector2Check(L, 3);
    Vector2 endPos2 = rluaVector2Check(L, 4);
    Vector2 collisionPoint = { 0 };
    bool collided = CheckCollisionLines(startPos1, endPos1, startPos2, endPos2, &collisionPoint);
    lua_pushboolean(L, collided);

    if (collided) rluaVector2Push(L, collisionPoint);
    else lua_pushnil(L);

    return 2;
}

static int rluaGetCollisionRec(lua_State *L)
{
    Rectangle rec1 = rluaRectangleCheck(L, 1);
    Rectangle rec2 = rluaRectangleCheck(L, 2);
    rluaRectanglePush(L, GetCollisionRec(rec1, rec2));
    return 1;
}

static void rluaShapesRegister(lua_State *L)
{
    REG(lua_pushcfunction(L, rluaSetShapesTexture), "SetShapesTexture");
    REG(lua_pushcfunction(L, rluaGetShapesTexture), "GetShapesTexture");
    REG(lua_pushcfunction(L, rluaGetShapesTextureRectangle), "GetShapesTextureRectangle");
    REG(lua_pushcfunction(L, rluaDrawPixel), "DrawPixel");
    REG(lua_pushcfunction(L, rluaDrawPixelV), "DrawPixelV");
    REG(lua_pushcfunction(L, rluaDrawLine), "DrawLine");
    REG(lua_pushcfunction(L, rluaDrawLineV), "DrawLineV");
    REG(lua_pushcfunction(L, rluaDrawLineEx), "DrawLineEx");
    REG(lua_pushcfunction(L, rluaDrawLineStrip), "DrawLineStrip");
    REG(lua_pushcfunction(L, rluaDrawLineBezier), "DrawLineBezier");
    REG(lua_pushcfunction(L, rluaDrawLineDashed), "DrawLineDashed");
    REG(lua_pushcfunction(L, rluaDrawTriangle), "DrawTriangle");
    REG(lua_pushcfunction(L, rluaDrawTriangleGradient), "DrawTriangleGradient");
    REG(lua_pushcfunction(L, rluaDrawTriangleLines), "DrawTriangleLines");
    REG(lua_pushcfunction(L, rluaDrawTriangleLinesEx), "DrawTriangleLinesEx");
    REG(lua_pushcfunction(L, rluaDrawTriangleFan), "DrawTriangleFan");
    REG(lua_pushcfunction(L, rluaDrawTriangleStrip), "DrawTriangleStrip");
    REG(lua_pushcfunction(L, rluaDrawRectangle), "DrawRectangle");
    REG(lua_pushcfunction(L, rluaDrawRectangleV), "DrawRectangleV");
    REG(lua_pushcfunction(L, rluaDrawRectangleRec), "DrawRectangleRec");
    REG(lua_pushcfunction(L, rluaDrawRectanglePro), "DrawRectanglePro");
    REG(lua_pushcfunction(L, rluaDrawRectangleGradientV), "DrawRectangleGradientV");
    REG(lua_pushcfunction(L, rluaDrawRectangleGradientH), "DrawRectangleGradientH");
    REG(lua_pushcfunction(L, rluaDrawRectangleGradientEx), "DrawRectangleGradientEx");
    REG(lua_pushcfunction(L, rluaDrawRectangleLines), "DrawRectangleLines");
    REG(lua_pushcfunction(L, rluaDrawRectangleLinesEx), "DrawRectangleLinesEx");
    REG(lua_pushcfunction(L, rluaDrawRectangleRounded), "DrawRectangleRounded");
    REG(lua_pushcfunction(L, rluaDrawRectangleRoundedLines), "DrawRectangleRoundedLines");
    REG(lua_pushcfunction(L, rluaDrawRectangleRoundedLinesEx), "DrawRectangleRoundedLinesEx");
    REG(lua_pushcfunction(L, rluaDrawPoly), "DrawPoly");
    REG(lua_pushcfunction(L, rluaDrawPolyLines), "DrawPolyLines");
    REG(lua_pushcfunction(L, rluaDrawPolyLinesEx), "DrawPolyLinesEx");
    REG(lua_pushcfunction(L, rluaDrawCircle), "DrawCircle");
    REG(lua_pushcfunction(L, rluaDrawCircleV), "DrawCircleV");
    REG(lua_pushcfunction(L, rluaDrawCircleGradient), "DrawCircleGradient");
    REG(lua_pushcfunction(L, rluaDrawCircleSector), "DrawCircleSector");
    REG(lua_pushcfunction(L, rluaDrawCircleSectorLines), "DrawCircleSectorLines");
    REG(lua_pushcfunction(L, rluaDrawCircleSectorLinesEx), "DrawCircleSectorLinesEx");
    REG(lua_pushcfunction(L, rluaDrawCircleLines), "DrawCircleLines");
    REG(lua_pushcfunction(L, rluaDrawCircleLinesV), "DrawCircleLinesV");
    REG(lua_pushcfunction(L, rluaDrawCircleLinesEx), "DrawCircleLinesEx");
    REG(lua_pushcfunction(L, rluaDrawEllipse), "DrawEllipse");
    REG(lua_pushcfunction(L, rluaDrawEllipseV), "DrawEllipseV");
    REG(lua_pushcfunction(L, rluaDrawEllipseLines), "DrawEllipseLines");
    REG(lua_pushcfunction(L, rluaDrawEllipseLinesV), "DrawEllipseLinesV");
    REG(lua_pushcfunction(L, rluaDrawEllipseLinesEx), "DrawEllipseLinesEx");
    REG(lua_pushcfunction(L, rluaDrawRing), "DrawRing");
    REG(lua_pushcfunction(L, rluaDrawRingLines), "DrawRingLines");
    REG(lua_pushcfunction(L, rluaDrawRingLinesEx), "DrawRingLinesEx");
    REG(lua_pushcfunction(L, rluaDrawSplineLinear), "DrawSplineLinear");
    REG(lua_pushcfunction(L, rluaDrawSplineBasis), "DrawSplineBasis");
    REG(lua_pushcfunction(L, rluaDrawSplineCatmullRom), "DrawSplineCatmullRom");
    REG(lua_pushcfunction(L, rluaDrawSplineBezierQuadratic), "DrawSplineBezierQuadratic");
    REG(lua_pushcfunction(L, rluaDrawSplineBezierCubic), "DrawSplineBezierCubic");
    REG(lua_pushcfunction(L, rluaDrawSplineSegmentLinear), "DrawSplineSegmentLinear");
    REG(lua_pushcfunction(L, rluaDrawSplineSegmentBasis), "DrawSplineSegmentBasis");
    REG(lua_pushcfunction(L, rluaDrawSplineSegmentCatmullRom), "DrawSplineSegmentCatmullRom");
    REG(lua_pushcfunction(L, rluaDrawSplineSegmentBezierQuadratic), "DrawSplineSegmentBezierQuadratic");
    REG(lua_pushcfunction(L, rluaDrawSplineSegmentBezierCubic), "DrawSplineSegmentBezierCubic");
    REG(lua_pushcfunction(L, rluaGetSplinePointLinear), "GetSplinePointLinear");
    REG(lua_pushcfunction(L, rluaGetSplinePointBasis), "GetSplinePointBasis");
    REG(lua_pushcfunction(L, rluaGetSplinePointCatmullRom), "GetSplinePointCatmullRom");
    REG(lua_pushcfunction(L, rluaGetSplinePointBezierQuadratic), "GetSplinePointBezierQuadratic");
    REG(lua_pushcfunction(L, rluaGetSplinePointBezierCubic), "GetSplinePointBezierCubic");
    REG(lua_pushcfunction(L, rluaCheckCollisionRecs), "CheckCollisionRecs");
    REG(lua_pushcfunction(L, rluaCheckCollisionCircles), "CheckCollisionCircles");
    REG(lua_pushcfunction(L, rluaCheckCollisionCircleRec), "CheckCollisionCircleRec");
    REG(lua_pushcfunction(L, rluaCheckCollisionCircleLine), "CheckCollisionCircleLine");
    REG(lua_pushcfunction(L, rluaCheckCollisionPointRec), "CheckCollisionPointRec");
    REG(lua_pushcfunction(L, rluaCheckCollisionPointCircle), "CheckCollisionPointCircle");
    REG(lua_pushcfunction(L, rluaCheckCollisionPointTriangle), "CheckCollisionPointTriangle");
    REG(lua_pushcfunction(L, rluaCheckCollisionPointLine), "CheckCollisionPointLine");
    REG(lua_pushcfunction(L, rluaCheckCollisionPointPoly), "CheckCollisionPointPoly");
    REG(lua_pushcfunction(L, rluaCheckCollisionLines), "CheckCollisionLines");
    REG(lua_pushcfunction(L, rluaGetCollisionRec), "GetCollisionRec");
}

// Text

static int rluaDrawFPS(lua_State *L)
{
    int posX = (int)luaL_checkinteger(L, 1);
    int posY = (int)luaL_checkinteger(L, 2);
    DrawFPS(posX, posY);
    return 0;
}

static int rluaDrawText(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int posX = (int)luaL_checkinteger(L, 2);
    int posY = (int)luaL_checkinteger(L, 3);
    int fontSize = (int)luaL_checkinteger(L, 4);
    Color color = rluaColorCheck(L, 5);
    DrawText(text, posX, posY, fontSize, color);
    return 0;
}

static int rluaDrawTextEx(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    const char *text = luaL_checkstring(L, 2);
    Vector2 position = rluaVector2Check(L, 3);
    float fontSize = (float)luaL_checknumber(L, 4);
    float spacing = (float)luaL_checknumber(L, 5);
    Color tint = rluaColorCheck(L, 6);
    DrawTextEx(*font, text, position, fontSize, spacing, tint);
    return 0;
}

static int rluaDrawTextPro(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    const char *text = luaL_checkstring(L, 2);
    Vector2 position = rluaVector2Check(L, 3);
    Vector2 origin = rluaVector2Check(L, 4);
    float rotation = (float)luaL_checknumber(L, 5);
    float fontSize = (float)luaL_checknumber(L, 6);
    float spacing = (float)luaL_checknumber(L, 7);
    Color tint = rluaColorCheck(L, 8);
    DrawTextPro(*font, text, position, origin, rotation, fontSize, spacing, tint);
    return 0;
}

static int rluaDrawTextCodepoint(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    int codepoint = (int)luaL_checkinteger(L, 2);
    Vector2 position = rluaVector2Check(L, 3);
    float fontSize = (float)luaL_checknumber(L, 4);
    Color tint = rluaColorCheck(L, 5);
    DrawTextCodepoint(*font, codepoint, position, fontSize, tint);
    return 0;
}

static int rluaDrawTextCodepoints(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    int codepointCount = 0;
    int *codepoints = rluaIntArrayCheck(L, 2, &codepointCount);
    Vector2 position = rluaVector2Check(L, 3);
    float fontSize = (float)luaL_checknumber(L, 4);
    float spacing = (float)luaL_checknumber(L, 5);
    Color tint = rluaColorCheck(L, 6);
    DrawTextCodepoints(*font, codepoints, codepointCount, position, fontSize, spacing, tint);
    return 0;
}

static int rluaSetTextLineSpacing(lua_State *L)
{
    SetTextLineSpacing((int)luaL_checkinteger(L, 1));
    return 0;
}

static int rluaMeasureText(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int fontSize = (int)luaL_checkinteger(L, 2);
    lua_pushinteger(L, MeasureText(text, fontSize));
    return 1;
}

static int rluaMeasureTextEx(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    const char *text = luaL_checkstring(L, 2);
    float fontSize = (float)luaL_checknumber(L, 3);
    float spacing = (float)luaL_checknumber(L, 4);
    rluaVector2Push(L, MeasureTextEx(*font, text, fontSize, spacing));
    return 1;
}

static int rluaMeasureTextCodepoints(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    int codepointCount = 0;
    int *codepoints = rluaIntArrayCheck(L, 2, &codepointCount);
    float fontSize = (float)luaL_checknumber(L, 3);
    float spacing = (float)luaL_checknumber(L, 4);
    rluaVector2Push(L, MeasureTextCodepoints(*font, codepoints, codepointCount, fontSize, spacing));
    return 1;
}

static int rluaGetGlyphIndex(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    int codepoint = (int)luaL_checkinteger(L, 2);
    lua_pushinteger(L, GetGlyphIndex(*font, codepoint));
    return 1;
}

static int rluaGetGlyphInfo(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    int codepoint = (int)luaL_checkinteger(L, 2);
    GlyphInfo info = GetGlyphInfo(*font, codepoint);

    lua_createtable(L, 0, 4);

    lua_pushinteger(L, info.value);
    lua_setfield(L, -2, "value");

    lua_pushinteger(L, info.offsetX);
    lua_setfield(L, -2, "offsetX");

    lua_pushinteger(L, info.offsetY);
    lua_setfield(L, -2, "offsetY");

    lua_pushinteger(L, info.advanceX);
    lua_setfield(L, -2, "advanceX");

    return 1;
}

static int rluaGetGlyphAtlasRec(lua_State *L)
{
    if (rlua_audio_in_callback) return luaL_error(L, "raylib functions cannot be called from an audio callback");

    Font *font = rluaFont_CheckValid(L, 1);
    int codepoint = (int)luaL_checkinteger(L, 2);
    rluaRectanglePush(L, GetGlyphAtlasRec(*font, codepoint));
    return 1;
}

static int rluaLoadUTF8(lua_State *L)
{
    int codepointCount = 0;
    int *codepoints = rluaIntArrayCheck(L, 1, &codepointCount);
    char *result = LoadUTF8(codepoints, codepointCount);
    lua_pushstring(L, result);
    MemFree(result);
    return 1;
}

static int rluaLoadCodepoints(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int count = 0;
    int *codepoints = LoadCodepoints(text, &count);

    if (codepoints == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    int result = rluaPushIntArray(L, codepoints, count);
    UnloadCodepoints(codepoints);
    return result;
}

static int rluaGetCodepointCount(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    lua_pushinteger(L, GetCodepointCount(text));
    return 1;
}

static int rluaGetCodepoint(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int codepointSize = 0;
    int codepoint = GetCodepoint(text, &codepointSize);
    lua_pushinteger(L, codepoint);
    lua_pushinteger(L, codepointSize);
    return 2;
}

static int rluaGetCodepointNext(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    lua_Integer position = luaL_optinteger(L, 2, 1);
    size_t length = strlen(text);

    if ((position < 1) || ((size_t)(position - 1) > length))
    {
        return luaL_error(L, "position out of range");
    }

    int codepointSize = 0;
    int codepoint = GetCodepointNext(text + position - 1, &codepointSize);
    lua_pushinteger(L, codepoint);
    lua_pushinteger(L, codepointSize);
    return 2;
}

static int rluaGetCodepointPrevious(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    size_t length = strlen(text);
    lua_Integer position = luaL_optinteger(L, 2, (lua_Integer)length + 1);

    if ((position < 1) || ((size_t)position > length + 1))
    {
        return luaL_error(L, "position out of range");
    }

    int codepointSize = 0;
    int codepoint = GetCodepointPrevious(text + position - 1, &codepointSize);
    lua_pushinteger(L, codepoint);
    lua_pushinteger(L, codepointSize);
    return 2;
}

static int rluaCodepointToUTF8(lua_State *L)
{
    int codepoint = (int)luaL_checkinteger(L, 1);
    int utf8Size = 0;
    const char *text = CodepointToUTF8(codepoint, &utf8Size);

    if (text == NULL)
    {
        lua_pushnil(L);
        lua_pushinteger(L, utf8Size);
        return 2;
    }

    lua_pushlstring(L, text, (size_t)utf8Size);
    lua_pushinteger(L, utf8Size);
    return 2;
}

static int rluaLoadTextLines(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    int count = 0;
    char **lines = LoadTextLines(text, &count);

    if (lines == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    int result = rluaPushStringArray(L, lines, count);
    UnloadTextLines(lines, count);
    return result;
}

static int rluaTextFormat(lua_State *L)
{
    int top = lua_gettop(L);

    lua_getglobal(L, "string");
    lua_getfield(L, -1, "format");
    lua_remove(L, -2);
    lua_insert(L, 1);
    lua_call(L, top, 1);

    return 1;
}

static int rluaGetTextBetween(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    const char *begin = luaL_checkstring(L, 2);
    const char *end = luaL_checkstring(L, 3);
    lua_pushstring(L, GetTextBetween(text, begin, end));
    return 1;
}

static int rluaTextReplaceBetweenAlloc(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    const char *begin = luaL_checkstring(L, 2);
    const char *end = luaL_checkstring(L, 3);
    const char *replacement = luaL_checkstring(L, 4);
    char *result = TextReplaceBetweenAlloc(text, begin, end, replacement);

    if (result == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, result);
    MemFree(result);
    return 1;
}

static int rluaTextInsertAlloc(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    const char *insert = luaL_checkstring(L, 2);
    int position = (int)luaL_checkinteger(L, 3);
    char *result = TextInsertAlloc(text, insert, position);

    if (result == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, result);
    MemFree(result);
    return 1;
}

static int rluaTextSplit(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    char delimiter = (char)luaL_checkinteger(L, 2);
    int count = 0;
    char **parts = TextSplit(text, delimiter, &count);

    if (parts == NULL)
    {
        lua_pushnil(L);
        return 1;
    }

    return rluaPushStringArray(L, parts, count);
}

static int rluaTextToPascal(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    lua_pushstring(L, TextToPascal(text));
    return 1;
}

static int rluaTextToSnake(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    lua_pushstring(L, TextToSnake(text));
    return 1;
}

static int rluaTextToCamel(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    lua_pushstring(L, TextToCamel(text));
    return 1;
}

static int rluaTextToInteger(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    lua_pushinteger(L, TextToInteger(text));
    return 1;
}

static int rluaTextToFloat(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    lua_pushnumber(L, TextToFloat(text));
    return 1;
}

static void rluaTextRegister(lua_State *L)
{
    REG(lua_pushcfunction(L, rluaDrawFPS), "DrawFPS");
    REG(lua_pushcfunction(L, rluaDrawText), "DrawText");
    REG(lua_pushcfunction(L, rluaSetTextLineSpacing), "SetTextLineSpacing");
    REG(lua_pushcfunction(L, rluaMeasureText), "MeasureText");
    REG(lua_pushcfunction(L, rluaLoadUTF8), "LoadUTF8");
    REG(lua_pushcfunction(L, rluaLoadCodepoints), "LoadCodepoints");
    REG(lua_pushcfunction(L, rluaGetCodepointCount), "GetCodepointCount");
    REG(lua_pushcfunction(L, rluaGetCodepoint), "GetCodepoint");
    REG(lua_pushcfunction(L, rluaGetCodepointNext), "GetCodepointNext");
    REG(lua_pushcfunction(L, rluaGetCodepointPrevious), "GetCodepointPrevious");
    REG(lua_pushcfunction(L, rluaCodepointToUTF8), "CodepointToUTF8");
    REG(lua_pushcfunction(L, rluaLoadTextLines), "LoadTextLines");
    REG(lua_pushcfunction(L, rluaTextFormat), "TextFormat");
    REG(lua_pushcfunction(L, rluaGetTextBetween), "GetTextBetween");
    REG(lua_pushcfunction(L, rluaTextReplaceBetweenAlloc), "TextReplaceBetweenAlloc");
    REG(lua_pushcfunction(L, rluaTextInsertAlloc), "TextInsertAlloc");
    REG(lua_pushcfunction(L, rluaTextSplit), "TextSplit");
    REG(lua_pushcfunction(L, rluaTextToPascal), "TextToPascal");
    REG(lua_pushcfunction(L, rluaTextToSnake), "TextToSnake");
    REG(lua_pushcfunction(L, rluaTextToCamel), "TextToCamel");
    REG(lua_pushcfunction(L, rluaTextToInteger), "TextToInteger");
    REG(lua_pushcfunction(L, rluaTextToFloat), "TextToFloat");
}

// }}}

// }}}

void rLuaRegister(lua_State *L)
{
    rlua_callback_L = L;
    rlua_audio_L = L;
    rlua_main_thread = pthread_self();
    lua_newtable(L);
    // raymath
    rluaRaymathRegister(L);
    rluaVector2Register(L);
    rluaVector3Register(L);
    rluaVector4Register(L);
    rluaQuaternionRegister(L);
    rluaMatrixRegister(L);
    // raylib
    rluaColorRegister(L);
    rluaImageRegister(L);
    rluaTextureRegister(L);
    rluaRenderTextureRegister(L);
    rluaFontRegister(L);
    rluaGlyphInfoArrayRegister(L);
    rluaShaderRegister(L);
    rluaVrStereoConfigRegister(L);
    rluaModelsRegister(L);
    rluaAudioRegister(L);
    rluaAutomationEventListRegister(L);
    rluaCoreRegister(L);
    rluaGesturesRegister(L);
    rluaCameraRegister(L);
    rluaShapesRegister(L);
    rluaTextRegister(L);

    rluaGuardFunctions(L);

    lua_setglobal(L, "rl");
}

// [HELPERS] {{{

static float *rluaFloatArrayCheck(lua_State *L, int index, int *count)
{
    luaL_checktype(L, index, LUA_TTABLE);
    int n = (int)luaL_len(L, index);
    float *values = (float *)lua_newuserdatauv(
        L,
        sizeof(float) * (n > 0 ? n : 1),
        0
    );

    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(L, index, i);
        values[i - 1] = (float)luaL_checknumber(L, -1);
        lua_pop(L, 1);
    }

    *count = n;
    return values;
}

static int *rluaIntArrayCheck(lua_State *L, int index, int *count)
{
    luaL_checktype(L, index, LUA_TTABLE);
    int n = (int)luaL_len(L, index);
    int *values = (int *)lua_newuserdatauv(
        L,
        sizeof(int) * (n > 0 ? n : 1),
        0
    );

    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(L, index, i);
        values[i - 1] = (int)luaL_checkinteger(L, -1);
        lua_pop(L, 1);
    }

    *count = n;
    return values;
}

static Vector2 *rluaVector2ArrayCheck(lua_State *L, int index, int *count)
{
    luaL_checktype(L, index, LUA_TTABLE);
    int n = (int)luaL_len(L, index);
    Vector2 *values = (Vector2 *)lua_newuserdatauv(
        L,
        sizeof(Vector2) * (n > 0 ? n : 1),
        0
    );

    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(L, index, i);
        values[i - 1] = rluaVector2Check(L, -1);
        lua_pop(L, 1);
    }

    *count = n;
    return values;
}

static Vector3 *rluaVector3ArrayCheck(lua_State *L, int index, int *count)
{
    luaL_checktype(L, index, LUA_TTABLE);
    int n = (int)luaL_len(L, index);
    Vector3 *values = (Vector3 *)lua_newuserdatauv(L, sizeof(Vector3)*(n > 0 ? n : 1), 0);

    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(L, index, i);
        values[i - 1] = rluaVector3Check(L, -1);
        lua_pop(L, 1);
    }

    *count = n;
    return values;
}

static Matrix *rluaMatrixArrayCheck(lua_State *L, int index, int *count)
{
    luaL_checktype(L, index, LUA_TTABLE);
    int n = (int)luaL_len(L, index);
    Matrix *values = (Matrix *)lua_newuserdatauv(L, sizeof(Matrix)*(n > 0 ? n : 1), 0);

    for (int i = 1; i <= n; i++)
    {
        lua_rawgeti(L, index, i);
        values[i - 1] = rluaMatrixCheck(L, -1);
        lua_pop(L, 1);
    }

    *count = n;
    return values;
}

static int rluaPushStringArray(lua_State *L, char **values, int count)
{
    lua_createtable(L, count, 0);

    for (int i = 0; i < count; i++)
    {
        lua_pushstring(L, values[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int rluaPushIntArray(lua_State *L, const int *values, int count)
{
    lua_createtable(L, count, 0);

    for (int i = 0; i < count; i++)
    {
        lua_pushinteger(L, values[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int rluaPushUIntArray(lua_State *L, const unsigned int *values, int count)
{
    lua_createtable(L, count, 0);

    for (int i = 0; i < count; i++)
    {
        lua_pushinteger(L, (lua_Integer)values[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static int rluaPushColorArray(lua_State *L, const Color *colors, int count)
{
    lua_createtable(L, count, 0);

    for (int i = 0; i < count; i++)
    {
        rluaColorPush(L, colors[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

static Ray rluaRayCheck(lua_State *L, int index)
{
    luaL_checktype(L, index, LUA_TTABLE);

    Ray ray = { 0 };
    lua_getfield(L, index, "position");
    ray.position = rluaVector3Check(L, -1);
    lua_pop(L, 1);
    lua_getfield(L, index, "direction");
    ray.direction = rluaVector3Check(L, -1);
    lua_pop(L, 1);
    return ray;
}

static void rluaRayPush(lua_State *L, Ray ray)
{
    lua_createtable(L, 0, 2);

    rluaVector3Push(L, ray.position);
    lua_setfield(L, -2, "position");

    rluaVector3Push(L, ray.direction);
    lua_setfield(L, -2, "direction");
}

// }}}

#endif // RLUA_IMPLEMENTATION

#endif // RAYLIB_LUA_H
