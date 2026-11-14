
add_library(ixion MODULE
    document.cpp
    global.cpp
    python.cpp
    sheet.cpp
)

target_link_libraries(ixion Python3::Module ixion-${IXION_API_VERSION})

install(TARGETS ixion
    DESTINATION ${PYTHON3_SITE_PACKAGES_DIR}
)
