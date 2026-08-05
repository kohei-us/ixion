cmake_minimum_required(VERSION 3.18)
cmake_policy(SET CMP0048 NEW)

set(IXION_API_VERSION_MAJOR 0)
set(IXION_API_VERSION_MINOR 21)
set(IXION_API_VERSION ${IXION_API_VERSION_MAJOR}.${IXION_API_VERSION_MINOR})

option(BUILD_VULKAN "Build Vulkan compute engine.")
option(IXION_COW "Enable copy-on-write on the cell storage." ON)

project(ixion VERSION 0.21.0 LANGUAGES CXX)
set(IXION_VERSION "${ixion_VERSION}")
set(IXION_VERSION_MAJOR ${ixion_VERSION_MAJOR})
set(IXION_VERSION_MINOR ${ixion_VERSION_MINOR})
set(IXION_VERSION_MICRO ${ixion_VERSION_PATCH})  # NB: PATCH -> MICRO
message(STATUS "ixion version: ${IXION_VERSION}")
set(CMAKE_CXX_STANDARD 20)

find_package(Boost COMPONENTS program_options)
find_package(Threads)
find_package(Python3 COMPONENTS Interpreter Development.Module)

if(Python3_FOUND)
    message(STATUS "Python3 executable: ${Python3_EXECUTABLE}")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import sysconfig as sc; print(sc.get_paths()['purelib'])"
        OUTPUT_VARIABLE PYTHON3_SITE_PACKAGES_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "Python3 site-packages directory: ${PYTHON3_SITE_PACKAGES_DIR}")
endif()

include(GNUInstallDirs)

enable_testing()
set(CMAKE_CTEST_COMMAND ctest)

if(MSVC)
    add_compile_options(/wd4251 /wd4275 /wd4267 /wd4244 /MP)
endif(MSVC)

set(MDDS_INCLUDEDIR "" CACHE STRING "path to mdds header directory.")

add_compile_definitions(
    IXION_THREADS=1
    ${Boost_LIB_DIAGNOSTIC_DEFINITIONS}
)

if(IXION_COW)
    add_compile_definitions(IXION_COW=1)
else()
    add_compile_definitions(IXION_COW=0)
endif()

if(BUILD_VULKAN)
    find_package(Vulkan)
    add_compile_definitions(BUILD_VULKAN)
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/include
    ${Boost_INCLUDE_DIR}
    ${MDDS_INCLUDEDIR}
)

link_directories(${Boost_LIBRARY_DIRS})

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

add_subdirectory(include)
add_subdirectory(src)
