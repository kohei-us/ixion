cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0048 NEW)

set(IXION_MAJOR_VERSION 0)
set(IXION_MINOR_VERSION 15)
set(IXION_MICRO_VERSION 99)
set(IXION_MAJOR_API_VERSION 0)
set(IXION_MINOR_API_VERSION 15)
set(IXION_VERSION ${IXION_MAJOR_VERSION}.${IXION_MINOR_VERSION}.${IXION_MICRO_VERSION})
set(IXION_API_VERSION ${IXION_MAJOR_API_VERSION}.${IXION_MINOR_API_VERSION})

project(ixion VERSION ${IXION_VERSION} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(Boost COMPONENTS program_options filesystem)
find_package(Threads)
find_package(Python3)

include(GNUInstallDirs)

enable_testing()
set(CMAKE_CTEST_COMMAND ctest)

if(MSVC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /wd4251 /wd4275 /wd4267 /wd4244 /MP")
endif(MSVC)

set(MDDS_INCLUDEDIR "" CACHE STRING "path to mdds header directory.")
set(SPDLOG_INCLUDEDIR "" CACHE STRING "path to spdlog header directory.")

add_definitions(
    -DIXION_THREADS=1
    ${Boost_LIB_DIAGNOSTIC_DEFINITIONS}
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/include
    ${Boost_INCLUDE_DIR}
    ${MDDS_INCLUDEDIR}
    ${SPDLOG_INCLUDEDIR}
)

link_directories(${Boost_LIBRARY_DIRS})

add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

add_subdirectory(include)
add_subdirectory(src)
