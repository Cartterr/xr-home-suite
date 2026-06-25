add_executable(openxr_probe
    main.cpp
)

target_link_libraries(openxr_probe PRIVATE
    xrh_core
    xrh_openxr
    xrh_d3d11
    xrh_meta
)

if(MSVC)
    target_compile_options(openxr_probe PRIVATE /W4 /permissive-)
endif()

add_custom_command(TARGET openxr_probe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENXR_LOADER_DLL}"
        "$<TARGET_FILE_DIR:openxr_probe>/openxr_loader.dll"
)
