{
  description = "raylib-lua";

  inputs.nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";

  outputs = {
    self,
    nixpkgs,
  }: let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in {
    devShells.${system}.default = pkgs.mkShell {
      buildInputs = [
        pkgs.lua5_5
        # opengl
        pkgs.libGL
        # x11
        pkgs.libX11
        pkgs.libXcursor
        pkgs.libXi
        pkgs.libXinerama
        pkgs.libXrandr
        # wayland
        pkgs.wayland
        pkgs.wayland-scanner
        pkgs.libxkbcommon
        ## web support
        # pkgs.emscripten
      ];

      ## audio
      # LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [pkgs.alsa-lib];
    };
  };
}
