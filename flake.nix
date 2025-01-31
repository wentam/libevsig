{
  inputs = {
    nixpkgs.url = github:NixOS/nixpkgs/nixos-unstable;
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {self, nixpkgs, flake-utils, ...}:
  (flake-utils.lib.eachDefaultSystem (system: let pkgs = nixpkgs.legacyPackages.${system}; in rec {

    # Shell
    devShells.default = pkgs.clangStdenv.mkDerivation {
      name = "shell";
      buildInputs = with pkgs; [
        time
        libllvm
        include-what-you-use
      ];

      LD_LIBRARY_PATH = "build/lib/";
    };
  }));
}
