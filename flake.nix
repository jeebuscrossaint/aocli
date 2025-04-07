{
  description = "Advent of Code CLI in C++";

  inputs = {
    nixpkgs.url = "github:nixpkgs/nixpkgs/nixos-unstable";
    utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, utils }:
    utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "aocli";
          version = "1.0.0";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            pkg-config
          ];

          buildInputs = with pkgs; [
            curl
            gumbo
          ];

          buildPhase = ''
            cd cli
            make
          '';

          installPhase = ''
            mkdir -p $out/bin
            install -m 755 build/bin/aocli $out/bin/
            mkdir -p $out/share/bash-completion/completions
            mkdir -p $out/share/zsh/site-functions
            mkdir -p $out/share/fish/vendor_completions.d
            install -m 644 completions/aocli_completions.bash $out/share/bash-completion/completions/aocli
            install -m 644 completions/aocli_completions.zsh $out/share/zsh/site-functions/_aocli
            install -m 644 completions/aocli_completions.fish $out/share/fish/vendor_completions.d/aocli.fish
          '';
        };

        devShell = pkgs.mkShell {
          buildInputs = with pkgs; [
            curl
            gumbo
            pkg-config
            clang
          ];
        };
      });
}
