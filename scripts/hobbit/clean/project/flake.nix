{
  description = "Hobbit reconstructed source subset with the VC6 toolchain";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/64c08a7ca051951c8eae34e3e3cb1e202fe36786";
  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
      toolchain = import ./nix/toolchain.nix { inherit pkgs; };
    in {
      devShells.${system}.default = pkgs.mkShell {
        packages = [ pkgs.python3 pkgs.wineWow64Packages.staging ];
        MSVC_DIR = "${toolchain}/msvc";
        shellHook = ''
          export WINEPREFIX="$PWD/build/wineprefix"
          export WINEDEBUG="fixme-all,err-kerberos"
          export WINEDLLOVERRIDES="mscoree,mshtml="
        '';
      };
    };
}
