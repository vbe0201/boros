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
      systems = ["x86_64-linux" "aarch64-linux"];

      perSystem = {
        self',
        system,
        pkgs,
        ...
      }: {
        packages.default = self'.packages.boros;

        packages.boros = let
          boros_toml = builtins.fromTOML (builtins.readFile ./pyproject.toml);
        in
          pkgs.python312Packages.buildPythonPackage {
            pname = boros_toml.project.name;
            version = boros_toml.project.version;
            pyproject = true;
            src = ./.;

            build-system = with pkgs; [
              python312Packages.meson-python
            ];

            nativeBuildInputs = with pkgs; [
              pkg-config
            ];

            pythonImportsCheck = [boros_toml.project.name];
          };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            clang-tools
            cmake
            just
            liburing
            meson
            ninja
            pkg-config
            python312
            uv
          ];
        };
      };
    };
}
