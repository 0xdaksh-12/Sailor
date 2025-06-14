{
  description = "Sailor file transfer project";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = {
    self,
    nixpkgs,
  }: let
    pkgs = import nixpkgs {
      system = "x86_64-linux";
    };
  in {
    devShells.x86_64-linux.default = pkgs.mkShell {
      packages = with pkgs; [
        # Compiler and Build System
        gcc
        cmake
        gdb
        valgrind
        pkg-config

        # TLS and Cryptography
        openssl

        # C/C++ Development & Formatting Tools
        clang-tools

        # Text Editor and Shell formatters
        neovim
        shfmt
        alejandra
      ];

      shellHook = ''
        echo "Welcome to Sailor"
      '';
    };
  };
}
