cmake_minimum_required(VERSION 3.20)
project(ScareCam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ScareCam WIN32
    main.cpp
    CameraCapture.h
    CameraCapture.cpp
    VirtualCamSender.h
    VirtualCamSender.cpp
    DiagnosticEffect.h
    DiagnosticEffect.cpp
    shared.inl
)

target_link_libraries(ScareCam PRIVATE
    user32
    gdi32
    ole32
    oleaut32
    mfplat
    mf
    mfreadwrite
    mfuuid
)

# Standard MSVC Release optimizations
if(MSVC)
    target_compile_options(ScareCam PRIVATE /O2 /Oi /GL /W3)
    target_link_options(ScareCam PRIVATE /LTCG /OPT:REF /OPT:ICF)
endif()
