{
  description = "Logos Workflow Canvas - Visual workflow editor using QuickQanava";

  inputs = {
    logos-cpp-sdk.url = "github:logos-co/logos-cpp-sdk";
    nixpkgs.follows = "logos-cpp-sdk/nixpkgs";
    logos-liblogos.url = "github:logos-co/logos-liblogos";

    # QuickQanava vendored as a flake input
    quickqanava = {
      url = "github:cneben/QuickQanava";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, logos-cpp-sdk, logos-liblogos, quickqanava }:
    let
      systems = [ "aarch64-darwin" "x86_64-darwin" "aarch64-linux" "x86_64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f {
        pkgs = import nixpkgs { inherit system; };
        logosSdk = logos-cpp-sdk.packages.${system}.default;
        logosLiblogos = logos-liblogos.packages.${system}.default;
      });
    in
    {
      packages = forAllSystems ({ pkgs, logosSdk, logosLiblogos }:
        let
          quickqanavaPkg = pkgs.stdenv.mkDerivation {
            pname = "quickqanava";
            version = "2.5.0";
            src = quickqanava;
            nativeBuildInputs = [ pkgs.cmake pkgs.ninja pkgs.pkg-config pkgs.qt6.wrapQtAppsHook ];
            buildInputs = [ pkgs.qt6.qtbase pkgs.qt6.qtdeclarative ];
            cmakeFlags = [ "-GNinja" "-DCMAKE_BUILD_TYPE=Release" ];
            meta.description = "QuickQanava graph visualization library";
          };
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "logos-workflow-canvas";
            version = "1.0.0";
            src = ./.;
            nativeBuildInputs = [
              pkgs.cmake pkgs.ninja pkgs.pkg-config
              pkgs.qt6.wrapQtAppsHook
            ];
            buildInputs = [
              pkgs.qt6.qtbase pkgs.qt6.qtdeclarative
              pkgs.qt6.qtquickcontrols2 or pkgs.qt6.qtdeclarative
              pkgs.zstd pkgs.krb5
              quickqanavaPkg
            ];
            cmakeFlags = [
              "-GNinja"
              "-DCMAKE_BUILD_TYPE=Release"
              "-DLOGOS_CPP_SDK_ROOT=${logosSdk}"
              "-DLOGOS_LIBLOGOS_ROOT=${logosLiblogos}"
              "-DQUICKQANAVA_ROOT=${quickqanavaPkg}"
            ];
            meta = {
              description = "Logos Workflow Canvas - Visual workflow editor";
              platforms = pkgs.lib.platforms.unix;
            };
          };
        }
      );
    };
}
