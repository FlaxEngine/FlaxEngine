# Flax Mobile

Flax Engine demo game for Android phones. Built with GitHub Actions CI, shipped as a signed APK.

This project is part of the "Flax Mobile" fork: the engine repository contains an Android CI pipeline that:
1. Builds Flax Engine for Android (ARM64, Release)
2. Builds the Linux editor
3. Cooks this demo game headlessly (`FlaxEditor -std -headless -build "Android.Android"`)
4. Packages a debug-signed APK via Gradle
5. Publishes it to a GitHub Release named **Flax Mobile**

## The demo

- Touch: single-finger drag orbits the camera around a spinning wireframe cube, second finger slides to zoom
- Desktop parity: mouse drag orbits, wheel zooms, WASD flies
- HUD: title, FPS + resolution stats, exit button
- No binary assets — the scene is built entirely from C# scripts (`Source/Game.cs`), so the repository stays lightweight

## Layout

```
FlaxMobile/
  FlaxMobile.flaxproj     # project descriptor (references engine via $(EnginePath))
  Content/Main.scene      # minimal scene, bootstraps Game.cs
  Content/GameSettings.json
  Content/Settings/*.json # engine settings incl. Android + Build presets
  Source/Game.cs          # scene bootstrap
  Source/TouchCameraRig.cs
  Source/DemoCube.cs      # DebugDraw wireframe cube + ground grid
  Source/DemoPanel.cs     # HUD
```

## Build

Push to `master` or run the **Flax Mobile CI** workflow manually (workflow_dispatch). The APK lands in the
`Flax Mobile` GitHub Release.

## License

The demo code in this folder is released under the same license as the engine (Apache 2.0).
Flax Engine is Copyright (c) Wojciech Figat — https://flaxengine.com
