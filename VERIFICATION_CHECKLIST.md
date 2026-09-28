# ScareCam — Windows 10 Prototype Verification Checklist

> **NOTICE:** This verification checklist must be conducted on the physical target hardware (Windows 10 build 19045, Intel Core i5-1035G1, 8 GB RAM, built-in webcam).
> The container authoring this repository is a Linux environment that cannot execute Windows binaries or test physical camera hardware.
> None of the checklist items below should be marked verified until confirmed on your PC.

---

## 1. Live Preview of Built-in Webcam
* **Target:** `ScareCam.exe` captures the built-in webcam and displays an upright, real-time preview window at 1280×720 @ 30 fps (or 640×480 fallback).
* **Test Procedure:**
  1. Close any application currently using the physical webcam (Teams, Zoom, Chrome).
  2. Double-click `ScareCam.exe`.
  3. Verify that the native Win32 window displays the live camera image.
  4. Verify that the HUD banner displays `MODE: NORMAL (LIVE PASSTHROUGH)`.
  5. Press `R` in the window and verify that resolution toggles cleanly between 720p and 480p fallback without crashing.
* **Pass Criteria:**
  - [ ] Preview window opens and updates at ~30 fps.
  - [ ] Image orientation and colors are correct (upright, standard RGB).
  - [ ] Resolution toggle (`R`) successfully switches camera capture dimensions.

---

## 2. Virtual Camera Visible in Chrome
* **Target:** Google Chrome discovers and lists the virtual camera device without requiring an OBS process.
* **Test Procedure:**
  1. Ensure official Unity Capture filter has been registered via `Install.bat` (Run as Administrator) from `https://github.com/schellingb/UnityCapture`.
  2. Launch `ScareCam.exe`.
  3. Open Google Chrome.
  4. Navigate to: `chrome://settings/content/camera`.
  5. Click the "Camera" device dropdown list.
* **Pass Criteria:**
  - [ ] Device named **"UnityCapture"** (or "Unity Video Capture") appears in Chrome's camera list.
  - [ ] ScareCam owns the physical webcam; Chrome opens "UnityCapture" without a device-in-use conflict.

---

## 3. Smooth Video on Camera Test Page
* **Target:** 1280×720 @ 30 fps smooth playback into Chrome without audio interference.
* **Test Procedure:**
  1. With `ScareCam.exe` running, open [https://webcamtests.com](https://webcamtests.com) or Google Meet in Chrome.
  2. Select "UnityCapture" as the camera.
  3. Start the test.
  4. Observe frame delivery on the test page.
  5. Speak into the microphone and confirm Chrome receives audio from the physical laptop microphone.
* **Pass Criteria:**
  - [ ] Video streams into Chrome smoothly at ~30 fps.
  - [ ] Latency is low (under 2 frames compared to physical camera).
  - [ ] Physical microphone functions directly without audio mixer interference.

---

## 4. Global Hotkeys While Chrome Has Focus
* **Target:** `F8` triggers diagnostic visual change; `F9` immediately restores NORMAL mode, even when Chrome is the active foreground window.
* **Test Procedure:**
  1. Click inside Google Chrome so Chrome has active keyboard focus (ScareCam window is in the background).
  2. Press **`F8`** on your keyboard.
  3. Observe the camera feed in Chrome:
     - The video stream immediately displays the 450 ms diagnostic pulse (chromatic red channel shift and inverted horizontal scanline bands).
     - It automatically reverts back to `NORMAL` mode after 450 ms.
  4. Press **`F8`** again and immediately press **`F9`**.
  5. Verify that `F9` forces an immediate return to `NORMAL` mode.
* **Pass Criteria:**
  - [ ] `F8` triggers the brief diagnostic pulse while Chrome has foreground focus.
  - [ ] `F9` immediately restores NORMAL mode.
  - [ ] Global hotkey hooks do not cause input lag or key repetition issues.

---

## 5. Approximate CPU and Memory Usage
* **Target:** Modest CPU and memory consumption on Intel Core i5-1035G1 (4 cores, 1.0 GHz base) and 8 GB RAM.
* **Test Procedure:**
  1. Open Windows Task Manager (`Ctrl + Shift + Esc`).
  2. Locate `ScareCam.exe` in the Details tab.
  3. Observe CPU utilization during active 720p streaming into Chrome.
  4. Observe Memory (Working Set) utilization.
  5. Close `ScareCam.exe` with `ESC` or the close box (`X`), and verify that all memory and camera devices are cleanly released.
* **Observations to Record on your PC:**
  - Observed CPU %: `____ %` (Target: ~1.5% to 4.0%)
  - Observed RAM: `____ MB` (Target: ~25 MB to 35 MB)
  - Clean exit with zero residual background processes: `[ YES / NO ]`
