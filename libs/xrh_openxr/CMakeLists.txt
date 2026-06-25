add_library(xrh_openxr
    src/openxr_context.cpp
)

target_include_directories(xrh_openxr PUBLIC include)
target_link_libraries(xrh_openxr PUBLIC xrh_core openxr_loader)

if(MSVC)
    target_compile_options(xrh_openxr PRIVATE /W4 /permissive-)
else()
    target_compile_options(xrh_openxr PRIVATE -Wall -Wextra -Wpedantic)
endif()
