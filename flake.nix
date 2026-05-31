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
        pkgs.zig_0_17

        # raylib deps
        pkgs.libGL
        #
        pkgs.libX11
        pkgs.libXcursor
        pkgs.libXi
        pkgs.libXinerama
        pkgs.libXrandr
        #
        pkgs.wayland
        pkgs.wayland-scanner
        pkgs.libxkbcommon
      ];
    };
  };
}
