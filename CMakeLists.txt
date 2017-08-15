cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0048 NEW)

set(IXION_MAJOR_VERSION 0)
set(IXION_MINOR_VERSION 13)
set(IXION_MICRO_VERSION 0)
set(IXION_VERSION ${IXION_MAJOR_VERSION}.${IXION_MINOR_VERSION}.${IXION_MICRO_VERSION})

project(ixion VERSION ${IXION_VERSION} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

if(MSVC)
    set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} /wd4251 /wd4275 /MP")
endif(MSVC)

option(MDDS_INCLUDE_DIR "path to mdds header directory.")
option(BOOST_INCLUDE_DIR "path to boost header directory.")
option(BOOST_LIB_DIR "path to boost library directory.")

add_definitions(-DIXION_THREADS=1)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${BOOST_INCLUDE_DIR}
    ${MDDS_INCLUDE_DIR}
)

link_directories(${BOOST_LIB_DIR})

set(LIBIXION_SRC_DIR ${PROJECT_SOURCE_DIR}/src/libixion)

add_library(libixion SHARED
    ${LIBIXION_SRC_DIR}/address.cpp
    ${LIBIXION_SRC_DIR}/cell.cpp
    ${LIBIXION_SRC_DIR}/cell_listener_tracker.cpp
    ${LIBIXION_SRC_DIR}/cell_queue_manager.cpp
    ${LIBIXION_SRC_DIR}/concrete_formula_tokens.cpp
    ${LIBIXION_SRC_DIR}/config.cpp
    ${LIBIXION_SRC_DIR}/depends_tracker.cpp
    ${LIBIXION_SRC_DIR}/exceptions.cpp
    ${LIBIXION_SRC_DIR}/formula.cpp
    ${LIBIXION_SRC_DIR}/formula_function_opcode.cpp
    ${LIBIXION_SRC_DIR}/formula_functions.cpp
    ${LIBIXION_SRC_DIR}/formula_interpreter.cpp
    ${LIBIXION_SRC_DIR}/formula_lexer.cpp
    ${LIBIXION_SRC_DIR}/formula_name_resolver.cpp
    ${LIBIXION_SRC_DIR}/formula_parser.cpp
    ${LIBIXION_SRC_DIR}/formula_result.cpp
    ${LIBIXION_SRC_DIR}/formula_tokens.cpp
    ${LIBIXION_SRC_DIR}/formula_value_stack.cpp
    ${LIBIXION_SRC_DIR}/function_objects.cpp
    ${LIBIXION_SRC_DIR}/global.cpp
    ${LIBIXION_SRC_DIR}/info.cpp
    ${LIBIXION_SRC_DIR}/interface.cpp
    ${LIBIXION_SRC_DIR}/lexer_tokens.cpp
    ${LIBIXION_SRC_DIR}/matrix.cpp
    ${LIBIXION_SRC_DIR}/mem_str_buf.cpp
    ${LIBIXION_SRC_DIR}/model_context.cpp
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
    COMMAND python ${PROJECT_SOURCE_DIR}/bin/gen-files.py
        --properties
            IXION_MAJOR_VERSION=${IXION_MAJOR_VERSION}
            IXION_MINOR_VERSION=${IXION_MINOR_VERSION}
            IXION_MICRO_VERSION=${IXION_MICRO_VERSION}
        --files
            ${LIBIXION_SRC_DIR}/constants.inl
)

add_dependencies(libixion py_gen_files)

target_compile_definitions(libixion PRIVATE IXION_BUILD DLL_EXPORT)

target_link_libraries(ixion-parser libixion)
target_link_libraries(ixion-sorter libixion)
target_link_libraries(ixion-formula-tokenizer libixion)
