cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0048 NEW)

project(ixion VERSION 0.12.99 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)

option(MDDS_INCLUDE_DIR "path to mdds header directory.")
option(BOOST_INCLUDE_DIR "path to boost header directory.")
option(BOOST_LIB_DIR "path to boost library directory.")

add_definitions(-DIXION_THREADS=1)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${BOOST_INCLUDE_DIR}
    ${MDDS_INCLUDE_DIR}
)

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
