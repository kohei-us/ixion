cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0048 NEW)

set(IXION_MAJOR_VERSION 0)
set(IXION_MINOR_VERSION 14)
set(IXION_MICRO_VERSION 99)
set(IXION_MAJOR_API_VERSION 0)
set(IXION_MINOR_API_VERSION 15)
set(IXION_VERSION ${IXION_MAJOR_VERSION}.${IXION_MINOR_VERSION}.${IXION_MICRO_VERSION})
set(IXION_API_VERSION ${IXION_MAJOR_API_VERSION}.${IXION_MINOR_API_VERSION})

project(ixion VERSION ${IXION_VERSION} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

set(Boost_USE_STATIC_LIBS 1)
find_package(Boost COMPONENTS program_options filesystem)
find_package(Threads)
find_package(Python3)

if(MSVC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /wd4251 /wd4275 /MP")
endif(MSVC)

option(MDDS_INCLUDEDIR "path to mdds header directory.")
option(SPDLOG_INCLUDEDIR "path to spdlog header directory.")

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

set(LIBIXION_SRC_DIR ${PROJECT_SOURCE_DIR}/src/libixion)

add_library(libixion SHARED
    ${LIBIXION_SRC_DIR}/address.cpp
    ${LIBIXION_SRC_DIR}/address_iterator.cpp
    ${LIBIXION_SRC_DIR}/calc_status.cpp
    ${LIBIXION_SRC_DIR}/cell.cpp
    ${LIBIXION_SRC_DIR}/cell_queue_manager.cpp
    ${LIBIXION_SRC_DIR}/compute_engine.cpp
    ${LIBIXION_SRC_DIR}/compute_engine_cuda.cpp
    ${LIBIXION_SRC_DIR}/concrete_formula_tokens.cpp
    ${LIBIXION_SRC_DIR}/config.cpp
    ${LIBIXION_SRC_DIR}/dirty_cell_tracker.cpp
    ${LIBIXION_SRC_DIR}/exceptions.cpp
    ${LIBIXION_SRC_DIR}/formula.cpp
    ${LIBIXION_SRC_DIR}/formula_calc.cpp
    ${LIBIXION_SRC_DIR}/formula_functions.cpp
    ${LIBIXION_SRC_DIR}/formula_function_opcode.cpp
    ${LIBIXION_SRC_DIR}/formula_interpreter.cpp
    ${LIBIXION_SRC_DIR}/formula_lexer.cpp
    ${LIBIXION_SRC_DIR}/formula_name_resolver.cpp
    ${LIBIXION_SRC_DIR}/formula_parser.cpp
    ${LIBIXION_SRC_DIR}/formula_result.cpp
    ${LIBIXION_SRC_DIR}/formula_tokens.cpp
    ${LIBIXION_SRC_DIR}/formula_value_stack.cpp
    ${LIBIXION_SRC_DIR}/global.cpp
    ${LIBIXION_SRC_DIR}/grouped_ranges.cpp
    ${LIBIXION_SRC_DIR}/info.cpp
    ${LIBIXION_SRC_DIR}/interface.cpp
    ${LIBIXION_SRC_DIR}/lexer_tokens.cpp
    ${LIBIXION_SRC_DIR}/matrix.cpp
    ${LIBIXION_SRC_DIR}/mem_str_buf.cpp
    ${LIBIXION_SRC_DIR}/model_context.cpp
    ${LIBIXION_SRC_DIR}/model_iterator.cpp
    ${LIBIXION_SRC_DIR}/module.cpp
    ${LIBIXION_SRC_DIR}/queue_entry.cpp
    ${LIBIXION_SRC_DIR}/table.cpp
    ${LIBIXION_SRC_DIR}/types.cpp
    ${LIBIXION_SRC_DIR}/workbook.cpp
)

add_executable(ixion-parser
	${PROJECT_SOURCE_DIR}/src/ixion_parser.cpp
	${PROJECT_SOURCE_DIR}/src/model_parser.cpp
	${PROJECT_SOURCE_DIR}/src/session_handler.cpp
	${PROJECT_SOURCE_DIR}/src/table_handler.cpp
)

add_executable(ixion-sorter
	${PROJECT_SOURCE_DIR}/src/ixion_sorter.cpp
	${PROJECT_SOURCE_DIR}/src/sort_input_parser.cpp
)

add_executable(ixion-formula-tokenizer
	${PROJECT_SOURCE_DIR}/src/ixion_formula_tokenizer.cpp
)

add_custom_target(
    py_gen_files
    COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/bin/gen-files.py
        --properties
            IXION_MAJOR_VERSION=${IXION_MAJOR_VERSION}
            IXION_MINOR_VERSION=${IXION_MINOR_VERSION}
            IXION_MICRO_VERSION=${IXION_MICRO_VERSION}
            IXION_MAJOR_API_VERSION=${IXION_MAJOR_API_VERSION}
            IXION_MINOR_API_VERSION=${IXION_MINOR_API_VERSION}
        --files
            ${LIBIXION_SRC_DIR}/constants.inl
)

add_dependencies(libixion py_gen_files)

target_compile_definitions(libixion PRIVATE IXION_BUILD DLL_EXPORT)

target_link_libraries(ixion-parser libixion)
target_link_libraries(ixion-sorter libixion)
target_link_libraries(ixion-formula-tokenizer libixion)

install(DIRECTORY
    ${PROJECT_SOURCE_DIR}/include/ixion
    DESTINATION include/ixion-${IXION_API_VERSION}
    FILES_MATCHING PATTERN "*.hpp"
)

install(TARGETS libixion ixion-parser ixion-sorter ixion-formula-tokenizer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
