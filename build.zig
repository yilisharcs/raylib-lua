const std = @import("std");

pub fn build(b: *std.Build) void {
    const raylib = b.dependency("raylib", .{
        .target = b.standardTargetOptions(.{}),
        .optimize = b.standardOptimizeOption(.{}),
        // platform glue
        .linkage = .static,
        .linux_display_backend = .Both, // X11, Wayland
        //.android_ndk = "",
        //.android_api_version = "35",
        .platform = .glfw,
        // features
        .raudio = true,
        .rmodels = true,
        .rshapes = true,
        .rtext = true,
        .rtextures = true,
        // capabilities
        .raygui = false,
        .opengl_version = .auto,
        .config = "",
    });
    b.installArtifact(raylib.artifact("raylib"));
}
