add_library(xrh_d3d11
    src/d3d11_device.cpp
    src/d3d11_resources.cpp
    src/overlay_renderer.cpp
    src/screenshot_capture.cpp
)

target_include_directories(xrh_d3d11 PUBLIC include)
target_link_libraries(xrh_d3d11 PUBLIC xrh_core xrh_openxr d3d11 dxgi d3dcompiler)

if(MSVC)
    target_compile_options(xrh_d3d11 PRIVATE /W4 /permissive-)
else()
    target_compile_options(xrh_d3d11 PRIVATE -Wall -Wextra -Wpedantic)
endif()
