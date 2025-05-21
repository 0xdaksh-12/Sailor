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

        # C/C++ Development & Formatting Tools
        clang-tools
        openssl

        # Text Editor and Shell formatters
        neovim
        shfmt # Added for shell scripts
        alejandra # Added for Nix formatting
      ];

      # Optional: Ensure Neovim knows exactly where to find environment tools
      shellHook = ''
        echo "Welcome to Sailor"
      '';
    };
  };
}
