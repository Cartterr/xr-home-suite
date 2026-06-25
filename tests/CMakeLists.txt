add_executable(xrh_core_tests
    core_tests.cpp
)

target_link_libraries(xrh_core_tests PRIVATE xrh_core)

if(MSVC)
    target_compile_options(xrh_core_tests PRIVATE /W4 /permissive-)
endif()

add_test(NAME xrh_core_tests COMMAND xrh_core_tests)
