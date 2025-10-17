{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs = {
    self,
    flake-parts,
    ...
  } @ inputs:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = ["x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin"];

      perSystem = {
        self',
        system,
        pkgs,
        ...
      }: {
        packages.default = self'.packages.boros;

        packages.boros = let
          boros_toml = builtins.fromTOML (builtins.readFile ./pyproject.toml);
        in pkgs.python3Packages.buildPythonPackage {
          pname = boros_toml.project.name;
          version = boros_toml.project.version;
          format = "pyproject";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            python3Packages.scikit-build-core
          ];

          dontUseCmakeConfigure = true;
          dontUseCmakeBuild = true;
          dontUseCmakeInstall = true;

          doCheck = false;
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            clang-tools
            cmake
            ninja
            python312
            uv
          ];
        };
      };
    };
}
