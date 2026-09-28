@echo off
:: ============================================================================
:: ScareCam Native Windows 10 Build Script
:: Compiles ScareCam.exe with MSVC (Visual Studio 2019/2022 or Build Tools)
:: Target: Windows 10 x64, Intel Core i5
:: ============================================================================

setlocal
cd /d "%~dp0"

echo ============================================================================
echo   Building ScareCam (Native C++ Windows 10 x64)
echo ============================================================================
echo.

where cl >nul 2>&1
if %errorlevel% neq 0 (
    echo [INFO] Searching for Visual Studio vcvars64.bat environment...
    if exist "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "%ProgramFiles%\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    ) else if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" (
        call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    ) else (
        echo [ERROR] MSVC C++ compiler (cl.exe) not found.
        echo Please run this script from "Developer Command Prompt for VS" or "x64 Native Tools Command Prompt".
        pause
        exit /b 1
    )
)

if not exist build mkdir build
cd build

echo [*] Configuring and compiling ScareCam with CMake...
cmake -G "Visual Studio 17 2022" -A x64 ..
if %errorlevel% neq 0 (
    echo [*] Falling back to direct cl.exe compilation...
    cl /nologo /O2 /EHsc /std:c++17 ..\main.cpp ..\CameraCapture.cpp ..\VirtualCamSender.cpp ..\DiagnosticEffect.cpp /Fe:ScareCam.exe /link user32.lib gdi32.lib ole32.lib oleaut32.lib mfplat.lib mfreadwrite.lib mfuuid.lib /SUBSYSTEM:WINDOWS
) else (
    cmake --build . --config Release
    copy Release\ScareCam.exe . >nul 2>&1
)

if exist "ScareCam.exe" (
    echo.
    echo ============================================================================
    echo [SUCCESS] ScareCam.exe built successfully!
    echo Output binary: %CD%\ScareCam.exe
    echo ============================================================================
) else if exist "Release\ScareCam.exe" (
    echo.
    echo ============================================================================
    echo [SUCCESS] ScareCam.exe built successfully!
    echo Output binary: %CD%\Release\ScareCam.exe
    echo ============================================================================
) else (
    echo [ERROR] Build failed.
)

cd ..
pause
