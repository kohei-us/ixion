
add_subdirectory(interface)

configure_file(
    ${PROJECT_SOURCE_DIR}/include/ixion/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
)

file(GLOB _HEADERS *.hpp)

install(FILES ${_HEADERS}
    ${CMAKE_CURRENT_BINARY_DIR}/version.hpp
    DESTINATION include/ixion-${IXION_API_VERSION}/ixion
)
