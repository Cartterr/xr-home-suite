add_library(xrh_meta
    src/environment_depth.cpp
    src/hand_tracking.cpp
    src/meta_features.cpp
    src/passthrough.cpp
    src/private_camera.cpp
)

target_include_directories(xrh_meta PUBLIC include)
target_link_libraries(xrh_meta PUBLIC xrh_core xrh_openxr)

if(MSVC)
    target_compile_options(xrh_meta PRIVATE /W4 /permissive-)
else()
    target_compile_options(xrh_meta PRIVATE -Wall -Wextra -Wpedantic)
endif()
