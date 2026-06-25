add_library(xrh_core
    src/app_options.cpp
    src/checks.cpp
    src/probe_report.cpp
    src/shutdown_signal_win32.cpp
)

target_include_directories(xrh_core PUBLIC include)
target_link_libraries(xrh_core PUBLIC openxr_headers)

if(MSVC)
    target_compile_options(xrh_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(xrh_core PRIVATE -Wall -Wextra -Wpedantic)
endif()
