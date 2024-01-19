
add_library(ixion MODULE
    document.cpp
    global.cpp
    python.cpp
    sheet.cpp
)

target_compile_definitions(ixion PRIVATE IXION_BUILD DLL_EXPORT)
target_link_libraries(ixion Python3::Module ixion-${IXION_API_VERSION})

#TODO : decide where to install the python module.
