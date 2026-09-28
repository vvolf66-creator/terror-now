# ScareCam Windows 10 technical build candidate

This package contains **source code only**. There is no executable in this ZIP, and no successful Windows CI run has been observed. Do not install the virtual camera filter or run the app until a Windows build succeeds and the binary is reviewed.

The included GitHub Actions workflow runs once when the initial `BUILD_NOW` marker is added and supports manual runs (`workflow_dispatch`) later. A standard runner in a public GitHub repository is free under GitHub's current published billing rules. Publishing the source makes it publicly accessible. A private repository may consume the account's remaining Actions quota.

Review changes from the AI Studio ZIP:
- Removed automatic CI triggers.
- Fixed a mutex access right in the shared-memory sender.
- Rejected unnegotiated webcam formats instead of treating them as RGB32.
- Fixed 640x480 fallback failure handling.
- Converted Media Foundation BGRX pixels to RGBA for Unity Capture.

Still unverified:
- Windows compilation and Unity Capture protocol interoperability.
- The webcam's native media types, stride and row orientation on the target PC.
- Chrome enumeration, frame rate, latency, CPU and RAM usage.

The actual pass criterion is an observed preview and live virtual camera feed on the target PC.
