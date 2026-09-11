cmake_minimum_required(VERSION 3.20)
project(vsbm-windows LANGUAGES CXX RC)

add_executable(vsbm-windows WIN32
    main.cpp
    ipc.cpp
    sandbox.cpp
    renderer_process.cpp
    gpu_process.cpp
    vsbm-windows.rc
    resource.h
    D3D11RaymarchApp.manifest
)

target_compile_features(vsbm-windows PRIVATE cxx_std_20)
target_compile_definitions(vsbm-windows PRIVATE UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)
target_link_libraries(vsbm-windows PRIVATE d3d11 dxgi d3dcompiler user32 gdi32 comctl32 advapi32)

add_custom_command(TARGET vsbm-windows POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/kernel.glsl
            $<TARGET_FILE_DIR:vsbm-windows>/kernel.glsl)
