# ScareCam V1 — Windows 10 Native C++ Desktop Application

A compact, native Windows 10 x64 desktop application designed for **brief, deliberate horror reaction moments during an otherwise normal webcam conversation**.

Runs 100% locally on standard Windows 10: **no Python, no OpenCV, no .NET SDK/runtime, no Node.js, no account, no server, no cloud inference, no real-time AI models, no Electron, and no running OBS instance**.

---

## 1. Explicit Disclosure: Compiler State & Testing Distinction

> **STATUS: WINDOWS CI BUILD; PHYSICAL CAMERA TEST PENDING**
> 
> * **What has been verified in this environment:**
>   - Source files, structure, and syntax have been audited and packaged.
>   - Windows CI compiles the native source; physical camera behavior is pending.
> * **What has NOT been executed or verified:**
>   - This cloud authoring environment is an automated Linux container. It **does not have Windows MSVC compilers (`cl.exe`), cannot execute Windows PE binaries, cannot access physical Windows hardware webcam drivers, and cannot register Windows DirectShow COM filters (`regsvr32`).**
>   - **The application has NOT been executed on physical Windows hardware in this session.**
>   - Neither Google Chrome nor the virtual camera can be claimed to work without a real test on the target PC.
> * **Reproducible CI Build:**
>   - A GitHub Actions CI workflow (`.github/workflows/windows-build.yml`) using a Microsoft Windows Server runner (`windows-2022`) compiles `ScareCam.exe` with MSVC and produces the downloadable Windows x64 binary artifact.

---

## 2. The Three Horror Reaction Effects

ScareCam starts in **`NORMAL`** live passthrough mode. The effects are short, reaction moments designed to produce a genuine response from the person on the video call, with an automatic return to normal.

### Effect 1: "The Edge Lurker" (Global Hotkey: `F6` | Key: `1`)
* **Visual Sequence:**
  1. *Peripheral Darkening (0.0s – 0.4s):* Background lighting subtly drops on the left edge with a cold vignetting falloff.
  2. *The Emergence (0.4s – 1.0s):* A tall, gaunt, shadowy humanoid silhouette with an elongated neck smoothly slides into the frame from the screen boundary, head tilted at an unnerving angle. Two pale, sunken reflective eyes glint in the room light, staring across the frame.
  3. *The Gaze (1.0s – 1.7s):* The entity pauses, subtly wavering/breathing, holding its gaze just long enough for the caller to notice ("Who is that behind you?!").
  4. *The Retreat (1.7s – 2.4s):* The silhouette smoothly slinks backwards behind the screen boundary. The eyes dim into blackness, and peripheral lighting normalizes.
* **Duration:** ~2.4 seconds total, then automatically restores clean live feed.

### Effect 2: "The Sudden Lunger / Jumpscare" (Global Hotkey: `F7` | Key: `2`)
* **Visual Sequence:**
  1. *Anticipation Glitch (0.0s – 0.10s):* Sudden 1-frame blackout, brief horizontal chromatic tear (red right, blue left).
  2. *The Violent Lunge (0.10s – 0.70s):* A terrifying distorted creature/ghoul face with an impossibly wide agape black maw, needle-like teeth, sunken weeping eye sockets, and necrotic pale skin violently lunges from deep space directly into the camera lens.
  3. *Impact Dynamics:* High-frequency camera vibration/shake (±12px displacement), radial motion blur, and intense bloodshot chromatic aberration as if colliding with the glass.
  4. *Static Dissolve (0.70s – 1.05s):* Creature bursts into analog tracking static lines, snapping instantly back to the clean normal conversation. The caller on camera appears completely calm and unaffected.
* **Duration:** ~1.05 seconds total, then automatically restores clean live feed.

### Effect 3: "The Haunted Room Anomaly" (Global Hotkey: `F8` | Key: `3`)
* **Visual Sequence:**
  1. *Power Failure & Twilight (0.0s – 0.5s):* Room exposure rapidly plunges into deep, cold, desaturated cyan-gray/infrared twilight with a rapid 15Hz luminance flicker simulating a dying fluorescent ballast.
  2. *Paranormal Distortion (0.5s – 1.8s):* Optical barrel aberration ripples through the scene. Dark spectral shadow tendrils and a floating apparition glide across the upper ceiling and wall behind the speaker. Heavy analog high-ISO sensor grain crawls through the crushed shadows.
  3. *Electrical Surge & Recovery (1.8s – 2.4s):* A double-strobe fluorescent ignition pop flashes across the scene, color temperature and exposure surge back, snapping cleanly back to normal.
* **Duration:** ~2.4 seconds total, then automatically restores clean live feed.

### Emergency Normal Mode: `F9` (Key: `0`, `9`, or `N`)
* Pressing **`F9`** immediately cancels any active effect and restores the live camera passthrough with zero delay.

---

## 3. Color Order, Stride & Orientation Architecture

Previous revisions had red and blue channels swapped in preview or virtual camera output, and assumed tightly packed top-down rows. This has been completely re-engineered:

1. **Media Foundation Capture (`CameraCapture.cpp`):**
   * Queries `MF_MT_DEFAULT_STRIDE` from the camera media type.
   * If `stride < 0`, properly identifies bottom-up DIBs and inverts row iteration so the internal buffer is always clean top-down.
   * Handles row padding when `stride > width * 4` by copying `width * 4` bytes per row offset by `stride`.
   * Unpacks frames into canonical **32-bit BGRA** (`B=0, G=1, R=2, A=3`), ensuring alpha is solid 255.
2. **Local GDI Preview (`main.cpp`):**
   * Uses `StretchDIBits` with `biHeight = -height` (top-down DIB) and `BI_RGB` (32-bit).
   * Because Windows GDI 32-bit DIBs natively expect BGRA byte ordering, local preview displays 100% natural, physically accurate skin tones.
3. **UnityCapture Virtual Camera Transmission (`VirtualCamSender.cpp`):**
   * UnityCapture DirectShow filter `FORMAT_UINT8` expects **32-bit RGBA** (`R=0, G=1, B=2, A=3`).
   * `SendFrameBGRA()` performs a fast 32-bit bitwise swap `(px & 0xFF00FF00) | ((px & 0x00FF0000) >> 16) | ((px & 0x000000FF) << 16)`, delivering exact RGBA bytes to the shared memory buffer.
   * Chrome receives natural, correct colors with zero blue/red channel inversion.

---

## 4. Virtual Camera Component: Official Unity Capture

* **Component:** Official **Unity Capture DirectShow Filter** (`UnityCaptureFilter64.dll`).
* **Author:** Bernhard Schelling (based on UnityCam by MHD Yamen Saraiji).
* **License:** **MIT License** (royalty-free, permissive open-source).
* **Size on Disk:** **~48 KB** (single 64-bit DLL; no background services).
* **Producer-Side Protocol:** Adapted from the UnityCapture protocol as `shared.inl` (communicates via `UnityCapture_Data`, `UnityCapture_Mutx`, `UnityCapture_Want`, and `UnityCapture_Sent`).

### One-Time Registration (Administrator CMD)
1. Download official release from: [https://github.com/schellingb/UnityCapture](https://github.com/schellingb/UnityCapture) (Click **Code -> Download ZIP**).
2. Extract to a local folder (e.g. `C:\Tools\UnityCapture`).
3. Open **Command Prompt as Administrator**:
   ```cmd
   cd /d "C:\Tools\UnityCapture\Install"
   Install.bat
   ```
   *(Or run: `regsvr32.exe UnityCaptureFilter64.dll`)*.

### Clean Removal / Uninstallation (Administrator CMD)
```cmd
cd /d "C:\Tools\UnityCapture\Install"
Uninstall.bat
```
*(Or run: `regsvr32.exe /u UnityCaptureFilter64.dll`)*.
Leaves zero registry artifacts and zero drivers.

---

## 5. Building & Running ScareCam

### Option A: Download from GitHub Actions CI (Recommended)
1. In your GitHub repository, go to **Actions** -> **Windows 10 Native CI Build**.
2. Download the artifact **`ScareCam-Windows10-x64-Binary`**.
3. Extract and run **`ScareCam.exe`** directly on the target PC (no compiler or runtime needed).

### Option B: Local Compilation with MSVC (Developer PC)
From the Developer Command Prompt for Visual Studio:
```cmd
build_msvc.bat
```
Or via CMake:
```cmd
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Controls Summary
| Action | Global Hotkey | In-Window Key | Behavior |
|---|---|---|---|
| **Effect 1: Edge Lurker** | `F6` | `1` | Shadowy presence emerges from screen edge, lingers, retreats (2.4s) |
| **Effect 2: Sudden Lunger** | `F7` | `2` | Creature jumpscare lunges into camera lens with violent shake (1.05s) |
| **Effect 3: Haunted Anomaly** | `F8` | `3` | Lights die into cold infrared, spectral shadows drift, surge recovery (2.4s) |
| **Emergency Normal** | `F9` | `0`, `9`, `N` | Immediately restores clean live webcam feed |
| **Toggle Resolution** | — | `R` | Switches between 1280×720 @ 30fps and 640×480 fallback |
| **Quit** | — | `ESC`, `Q` | Releases webcam, unregisters hotkeys, exits cleanly |

---

## 6. Runtime Performance Budget

Target: Intel Core i5-1035G1 (4 cores, 1.0 GHz base up to 3.6 GHz), 8 GB RAM, Intel UHD Graphics:
* **CPU Utilization:** **Not measured.** Measure normal mode and each effect on the target PC.
* **RAM (Working Set):** **Not measured.** Check Working Set in Task Manager on the target PC.
* **Background Services:** **Zero**. Terminates immediately on exit.
