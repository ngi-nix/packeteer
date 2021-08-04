{
  description = "Packeteer provides a simplified, asynchronous, event-based socket API.";

  # Nixpkgs / NixOS version to use.
  inputs.nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-21.05";
  inputs.liberate.url = "github:ngi-nix/liberate";
  inputs.liberate.inputs.nixpkgs.follows = "nixpkgs";

  outputs = { self, nixpkgs, liberate }:
    let
      liberateflake = liberate;

      # Generate a user-friendly version numer.
      version = builtins.substring 0 8 self.lastModifiedDate;

      # System types to support.
      supportedSystems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" "aarch64-darwin" ];

      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = f: nixpkgs.lib.genAttrs supportedSystems (system: f system);

      # Nixpkgs instantiated for supported system types.
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; overlays = [ self.overlay liberateflake.overlay ]; config.allowUnfree = true; });

      removeMesonWrap = path: name: libname:
        let
          meson-build = builtins.toFile "meson.build" ''
            project('${name}')

            compiler = meson.get_compiler('cpp')
            ${name}_dep = dependency('${libname}', required: false, allow_fallback: false)
            if not ${name}_dep.found()
              ${name}_dep = compiler.find_library('${libname}', required: true)
            endif
          '';
        in ''
          rm ./subprojects/${path}.wrap
          mkdir ./subprojects/${path}
          ln -s ${meson-build} ./subprojects/${path}/meson.build
        '';

      packeteer =
        { stdenv, liberate, meson, ninja, gtest, lib }:
        stdenv.mkDerivation rec {
          name = "packeteer-${version}";

          src = ./.;

          postPatch = ''
            echo > ./benchmarks/meson.build
          ''
            + removeMesonWrap "liberate" "liberate" "erate"
            + removeMesonWrap "gtest" "gtest" "gtest";

          buildInputs = [ gtest liberate ];
          nativeBuildInputs = [ meson ninja ];

          doCheck = false; # Tests try to listen to TUN/TAP interfaces, needs privileges.

          meta = with lib; {
            description = "Packeteer provides a simplified, asynchronous, event-based socket API.";
            longDescription = ''
              Packeteer provides a cross-platform socket API in C++.
              The aim of the project is to get high performance, asynchronous inter-process
              I/O into any project without the need for a ton of external libraries, or for
              writing cross-platform boilerplate.
              Packeteer achieves this aim by:

              Picking the best available event notification for the target system.
              and providing a socket-like API for supported IPC.

              The project was started a long time ago, but was not public until recently.
              Some of the code has been extensively used in commercial products.
            '';
            homepage    = "https://interpeer.io/";
            license     = licenses.unfree;
            platforms   = platforms.all;
          };
        };

    in

    {

      overlay = final: prev: {
        packeteer = final.callPackage packeteer {};
      };

      packages = forAllSystems (system:
        {
          inherit (nixpkgsFor.${system}) packeteer;
        });

      defaultPackage = forAllSystems (system: self.packages.${system}.packeteer);

      hydraJobs.packeteer = self.defaultPackage;
    };
}
