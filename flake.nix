{
  description = "CS142B Homework 2 - Parser and Tokenizer.";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, ... }:
  let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in
  {
    devShells.x86_64-linux.default = pkgs.mkShell {
      nativeBuildInputs = with pkgs; [
        bear
        clang-tools
        # Nix shells actually start with "stdenv" which
        # includes tools such as gcc, make, etc.
        # gnumake
        gdb
      ];
    shellHook = ''
      export HELIX_RUNTIME="$PWD/runtime"
    '';
    };
  };
}
