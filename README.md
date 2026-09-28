# ScareCam — Windows 10 native prototype

Experimental C++ virtual-camera sender for Windows 10 x64. This repository contains source code, not a verified product. It starts with a normal webcam feed; F8 produces a short diagnostic pulse and F9 restores the normal feed. The three creative effects are not implemented yet.

## Status

The Windows build and real webcam behavior are unverified. See [STATUS.md](STATUS.md) and [VERIFICATION_CHECKLIST.md](VERIFICATION_CHECKLIST.md). A successful CI build proves compilation only; it does not prove Chrome compatibility, visual quality, latency, or CPU use. Do not install the virtual-camera filter until the binary and its source have been checked.

## Build

The GitHub Actions workflow builds an x64 executable on `windows-2022` and uploads an artifact named `ScareCam-Windows10-x64-Binary`. It can be run manually from the Actions tab. The initial `BUILD_NOW` marker requests one automatic build when this repository is populated; subsequent source pushes do not trigger builds.

On a Windows development machine with MSVC and CMake, run `build_msvc.bat` or:

```cmd
cmake -B build -S .
cmake --build build --config Release
```

The app itself uses Windows APIs and has no Python or .NET dependency. A separate Unity Capture DirectShow filter is required for Chrome to see the stream. Its installation and actual compatibility must be tested separately after the build is inspected. The physical microphone remains selected directly in the browser.

## Controls

- F8: short diagnostic pulse (not a finished horror effect).
- F9: normal video immediately.
- R while the preview is focused: try switching 720p/480p.
- Escape or Q: exit.

The original Unity Capture project is [schellingb/UnityCapture](https://github.com/schellingb/UnityCapture). The `shared.inl` in this prototype carries the upstream MIT notice and has one local mutex access change. Protocol interoperability has not been established by a live test.
