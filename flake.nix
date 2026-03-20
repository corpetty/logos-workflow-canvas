{
  description = "Logos Workflow Canvas - Visual workflow editor using QuickQanava";

  inputs = {
    logos-cpp-sdk.url = "github:logos-co/logos-cpp-sdk";
    nixpkgs.follows = "logos-cpp-sdk/nixpkgs";
    logos-liblogos.url = "github:logos-co/logos-liblogos";

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
          # ── QuickQanava as a shared Nix package ────────────────────────
          quickqanavaPackage = pkgs.stdenv.mkDerivation {
            pname = "quickqanava";
            version = "2.5.0";
            src = quickqanava;

            nativeBuildInputs = [
              pkgs.cmake pkgs.ninja pkgs.pkg-config
              pkgs.qt6.wrapQtAppsHook
              pkgs.patchelf
            ];
            buildInputs = [
              pkgs.qt6.qtbase pkgs.qt6.qtdeclarative
            ];

            # Patch: build as SHARED instead of STATIC, add stub FindOpenGL
            postPatch = ''
              # QuickQanava uses qt_add_qml_module with STATIC — change to SHARED
              # so the QML plugin self-registers at runtime via the shared library.
              sed -i '/^qt_add_qml_module(QuickQanava/,/^)/ s/^    STATIC$//' src/CMakeLists.txt

              # Insert qt_add_library(QuickQanava SHARED) before qt_add_qml_module
              sed -i 's/qt_add_qml_module(QuickQanava/qt_add_library(QuickQanava SHARED)\nqt_add_qml_module(QuickQanava/' src/CMakeLists.txt

              # Stub FindOpenGL for headless/Nix builds (QuickQanava uses OpenGL)
              mkdir -p cmake
              cat > cmake/FindOpenGL.cmake << 'FINDGL'
              if(NOT TARGET OpenGL::GL)
                add_library(OpenGL::GL INTERFACE IMPORTED)
              endif()
              if(NOT TARGET OpenGL::OpenGL)
                add_library(OpenGL::OpenGL INTERFACE IMPORTED)
                target_link_libraries(OpenGL::OpenGL INTERFACE OpenGL::GL)
              endif()
              set(OpenGL_FOUND TRUE)
              set(OPENGL_FOUND TRUE)
              set(OpenGL_GL_FOUND TRUE)
              FINDGL
            '';

            cmakeFlags = [
              "-GNinja"
              "-DCMAKE_BUILD_TYPE=Release"
              "-DQUICK_QANAVA_BUILD_SAMPLES=OFF"
              "-DCMAKE_MODULE_PATH=${placeholder "out"}/cmake"
            ];

            preConfigure = ''
              # Make our stub FindOpenGL available during configure
              export CMAKE_MODULE_PATH="$(pwd)/cmake:$CMAKE_MODULE_PATH"
              cmakeFlagsArray+=("-DCMAKE_MODULE_PATH=$(pwd)/cmake")
            '';

            installPhase = ''
              runHook preInstall

              mkdir -p $out/lib $out/include/QuickQanava

              # Shared library
              cp -P src/libQuickQanava.so* $out/lib/ 2>/dev/null || true
              cp -P src/libQuickQanava.dylib $out/lib/ 2>/dev/null || true

              # QML module (qmldir + plugin) — installed to standard Qt QML path
              mkdir -p $out/lib/qt-6/qml/QuickQanava
              if [ -d src/QuickQanava ]; then
                cp -r src/QuickQanava/* $out/lib/qt-6/qml/QuickQanava/
              fi

              # Fix RPATH on the QML plugin so it finds libQuickQanava.so
              if [ -f $out/lib/qt-6/qml/QuickQanava/libQuickQanavaplugin.so ]; then
                patchelf --set-rpath "$out/lib" $out/lib/qt-6/qml/QuickQanava/libQuickQanavaplugin.so
              fi

              # Headers (both .h and .hpp template implementation files)
              cp $src/src/*.h $src/src/*.hpp $out/include/QuickQanava/ 2>/dev/null || true
              cp -r $src/src/quickcontainers $out/include/QuickQanava/
              cp -r $src/src/gtpo $out/include/QuickQanava/

              runHook postInstall
            '';

            meta = {
              description = "QuickQanava - Qt6 C++/QML graph drawing library";
              platforms = pkgs.lib.platforms.unix;
            };
          };
        in
        {
          # Expose QuickQanava as a standalone package for inspection
          quickqanava = quickqanavaPackage;

          default = pkgs.stdenv.mkDerivation {
            pname = "logos-workflow-canvas";
            version = "1.0.0";
            src = ./.;

            nativeBuildInputs = [
              pkgs.cmake pkgs.ninja pkgs.pkg-config
              pkgs.qt6.wrapQtAppsHook
              pkgs.patchelf
            ];
            buildInputs = [
              pkgs.qt6.qtbase pkgs.qt6.qtdeclarative
              pkgs.zstd pkgs.krb5
              quickqanavaPackage
            ];
            cmakeFlags = [
              "-GNinja"
              "-DCMAKE_BUILD_TYPE=Release"
              "-DLOGOS_CPP_SDK_ROOT=${logosSdk}"
              "-DLOGOS_LIBLOGOS_ROOT=${logosLiblogos}"
              "-DQUICKQANAVA_ROOT=${quickqanavaPackage}"
            ];

            installPhase = ''
              runHook preInstall
              mkdir -p $out/lib

              # Canvas plugin
              cp workflow_canvas.so $out/lib/ 2>/dev/null || cp workflow_canvas.dylib $out/lib/ 2>/dev/null || true

              # Bundle the QuickQanava shared library alongside the plugin
              cp -P ${quickqanavaPackage}/lib/libQuickQanava.so* $out/lib/ 2>/dev/null || true
              cp -P ${quickqanavaPackage}/lib/libQuickQanava.dylib $out/lib/ 2>/dev/null || true

              # Bundle the QuickQanava QML module so the app can set QML2_IMPORT_PATH
              mkdir -p $out/lib/qt-6/qml
              cp -r ${quickqanavaPackage}/lib/qt-6/qml/QuickQanava $out/lib/qt-6/qml/
              chmod -R u+w $out/lib/qt-6/qml/QuickQanava

              # Fix RPATH: the QML plugin must load the SAME libQuickQanava.so as
              # workflow_canvas.so (both from $out/lib), not a separate nix store copy.
              if [ -f $out/lib/qt-6/qml/QuickQanava/libQuickQanavaplugin.so ]; then
                patchelf --set-rpath "$out/lib" $out/lib/qt-6/qml/QuickQanava/libQuickQanavaplugin.so
              fi

              runHook postInstall
            '';

            meta = {
              description = "Logos Workflow Canvas - Visual workflow editor";
              platforms = pkgs.lib.platforms.unix;
            };
          };
        }
      );
    };
}
