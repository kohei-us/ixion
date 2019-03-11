
file(GLOB _HEADERS *.hpp)

install(FILES ${_HEADERS}
    DESTINATION include/ixion-${IXION_API_VERSION}/ixion/interface
)

