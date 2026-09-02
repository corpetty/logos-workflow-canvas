{
  description = "Logos Workflow Canvas - Visual workflow editor using QuickQanava";

  inputs = {
    logos-module-builder.url = "github:logos-co/logos-module-builder";

    # The three workflow modules this view drives. All on the modernize
    # branch until it merges: master still carries the pre-universal
    # versions, which publish no .lidl contract for the typed modules()
    # accessors to be generated from.
    logos-workflow-registry = {
      url = "github:corpetty/logos-workflow-registry/modernize-phase0";
      inputs.logos-module-builder.follows = "logos-module-builder";
    };
    logos-workflow-engine = {
      url = "github:corpetty/logos-workflow-engine/modernize-phase0";
      inputs.logos-module-builder.follows = "logos-module-builder";
      inputs.logos-workflow-registry.follows = "logos-workflow-registry";
    };
    logos-workflow-scheduler = {
      url = "github:corpetty/logos-workflow-scheduler/modernize-phase0";
      inputs.logos-module-builder.follows = "logos-module-builder";
      inputs.logos-workflow-engine.follows = "logos-workflow-engine";
    };

    quickqanava = {
      url = "github:cneben/QuickQanava";
      flake = false;
    };
  };

  outputs = inputs@{ self, logos-module-builder, quickqanava, ... }:
    let
      nixpkgs = logos-module-builder.inputs.nixpkgs;
      systems = [ "aarch64-darwin" "x86_64-darwin" "aarch64-linux" "x86_64-linux" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system);

      # ── QuickQanava as a shared Nix package ──────────────────────────
      #
      # Built SHARED, not static: the view engine loads QuickQanava's own QML
      # plugin, and this module's QML plugin links the same library. Two copies
      # would give the QML types two distinct C++ registrations and the graph
      # would not accept our node classes.
      quickqanavaFor = system:
        let pkgs = import nixpkgs { inherit system; };
        in pkgs.stdenv.mkDerivation {
          pname = "quickqanava";
          version = "2.5.0";
          src = quickqanava;

          nativeBuildInputs = [
            pkgs.cmake pkgs.ninja pkgs.pkg-config
            pkgs.qt6.wrapQtAppsHook pkgs.patchelf
          ];
          buildInputs = [ pkgs.qt6.qtbase pkgs.qt6.qtdeclarative ];

          # QuickQanava's qt_add_qml_module is STATIC; make it SHARED so the
          # QML plugin self-registers at runtime through the shared library.
          # The FindOpenGL stub is for headless Nix builds.
          postPatch = ''
            sed -i '/^qt_add_qml_module(QuickQanava/,/^)/ s/^    STATIC$//' src/CMakeLists.txt
            sed -i 's/qt_add_qml_module(QuickQanava/qt_add_library(QuickQanava SHARED)\nqt_add_qml_module(QuickQanava/' src/CMakeLists.txt

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
          ];

          preConfigure = ''
            export CMAKE_MODULE_PATH="$(pwd)/cmake:$CMAKE_MODULE_PATH"
            cmakeFlagsArray+=("-DCMAKE_MODULE_PATH=$(pwd)/cmake")
          '';

          installPhase = ''
            runHook preInstall
            mkdir -p $out/lib $out/include/QuickQanava $out/qml/QuickQanava

            cp -P src/libQuickQanava.so* $out/lib/ 2>/dev/null || true
            cp -P src/libQuickQanava.dylib $out/lib/ 2>/dev/null || true

            # The QML module (qmldir + plugin), kept in a plain qml/ tree: the
            # module install stages it next to the view, not into a Qt prefix.
            if [ -d src/QuickQanava ]; then
              cp -r src/QuickQanava/* $out/qml/QuickQanava/
            fi

            # CMake leaves the BUILD directory on these RPATHs, which the
            # tmpdir audit rejects (and which would be a dangling path at
            # runtime anyway). Point them at the installed library instead.
            # The module install rewrites them once more when it stages both
            # halves side by side.
            for so in $out/lib/libQuickQanava.so* $out/qml/QuickQanava/*.so; do
              [ -f "$so" ] || continue
              patchelf --set-rpath "$out/lib" "$so" || true
            done

            cp $src/src/*.h $src/src/*.hpp $out/include/QuickQanava/ 2>/dev/null || true
            cp -r $src/src/quickcontainers $out/include/QuickQanava/
            cp -r $src/src/gtpo $out/include/QuickQanava/

            runHook postInstall
          '';

          meta = {
            description = "QuickQanava - Qt6 C++/QML graph drawing library";
            platforms = nixpkgs.lib.platforms.unix;
          };
        };

      # Called INSIDE forAllSystems, not once: QuickQanava is a per-system
      # package, and mkLogosQmlModule takes plain values (no cmakeFlags hook),
      # so the only way to hand it a system-specific path is to build one
      # module per system and index it. Same shape as muster/module.
      moduleFor = system:
        let qq = quickqanavaFor system;
        in logos-module-builder.lib.mkLogosQmlModule {
          src = ./.;
          configFile = ./metadata.json;
          flakeInputs = {
            workflow_registry  = inputs.logos-workflow-registry;
            workflow_engine    = inputs.logos-workflow-engine;
            workflow_scheduler = inputs.logos-workflow-scheduler;
          } // inputs;

          extraBuildInputs = [ qq ];

          # CMakeLists.txt reads QUICKQANAVA_ROOT for the view-side QML plugin.
          preConfigure = ''
            export QUICKQANAVA_ROOT=${qq}
          '';

          # Stage QuickQanava beside the plugin. The view engine resolves QML
          # imports from the module's install directory (basecamp's QmlSandbox
          # prepends it), so QuickQanava's QML module has to travel INSIDE the
          # module rather than sit in a Qt prefix the host knows nothing about.
          #
          # Both QML plugins — QuickQanava's and this module's — must resolve
          # the SAME libQuickQanava, which is why the RPATHs are rewritten to
          # the staged copy rather than left pointing at the nix store.
          # The plugin build has its own installPhase and never runs `ninja
          # install`, so CMake install() rules are ignored — the view-side QML
          # module has to be staged from the build tree by hand.
          postInstall = ''
            # Our QML module: `import WorkflowCanvas 1.0` in the view.
            mkdir -p $out/lib/WorkflowCanvas
            cp -r qml-modules/WorkflowCanvas/. $out/lib/WorkflowCanvas/
            cp -P libworkflowcanvasqml.so* $out/lib/ 2>/dev/null || true
            cp -P libworkflowcanvasqml.dylib $out/lib/ 2>/dev/null || true
            chmod -R u+w $out/lib/WorkflowCanvas

            # QuickQanava's QML module: `import QuickQanava as Qan`. It has to
            # travel inside the module because the view engine resolves imports
            # from the module's install directory (basecamp's QmlSandbox
            # prepends it) and knows nothing about a Qt prefix.
            mkdir -p $out/lib/QuickQanava
            cp -r ${qq}/qml/QuickQanava/. $out/lib/QuickQanava/
            chmod -R u+w $out/lib/QuickQanava

            # Both QML plugins sit one level below the libraries they need.
            for so in $out/lib/QuickQanava/*.so $out/lib/WorkflowCanvas/*.so; do
              [ -f "$so" ] || continue
              patchelf --set-rpath "\$ORIGIN/..:${qq}/lib" "$so" || true
            done
          '';
        };
    in {
      packages = forAllSystems (system: (moduleFor system).packages.${system});
      apps     = forAllSystems (system: (moduleFor system).apps.${system} or {});
      devShells = forAllSystems (system: (moduleFor system).devShells.${system} or {});
    };
}
