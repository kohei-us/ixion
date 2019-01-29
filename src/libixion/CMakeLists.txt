
add_library(libixion SHARED
    address.cpp
    address_iterator.cpp
    calc_status.cpp
    cell.cpp
    cell_queue_manager.cpp
    compute_engine.cpp
    compute_engine_cuda.cpp
    concrete_formula_tokens.cpp
    config.cpp
    dirty_cell_tracker.cpp
    exceptions.cpp
    formula.cpp
    formula_calc.cpp
    formula_functions.cpp
    formula_function_opcode.cpp
    formula_interpreter.cpp
    formula_lexer.cpp
    formula_name_resolver.cpp
    formula_parser.cpp
    formula_result.cpp
    formula_tokens.cpp
    formula_value_stack.cpp
    global.cpp
    info.cpp
    interface.cpp
    lexer_tokens.cpp
    matrix.cpp
    mem_str_buf.cpp
    model_context.cpp
    model_iterator.cpp
    module.cpp
    queue_entry.cpp
    table.cpp
    types.cpp
    workbook.cpp
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
            ${CMAKE_CURRENT_SOURCE_DIR}/constants.inl
)

add_dependencies(libixion py_gen_files)

target_compile_definitions(libixion PRIVATE IXION_BUILD DLL_EXPORT)

install(TARGETS libixion
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# test programs

add_executable(ixion-test EXCLUDE_FROM_ALL
    ixion_test.cpp
)

add_executable(ixion-test-track-deps EXCLUDE_FROM_ALL
    ixion_test_track_deps.cpp
)

add_executable(dirty-cell-tracker-test EXCLUDE_FROM_ALL
    dirty_cell_tracker_test.cpp
)

add_executable(compute-engine-test EXCLUDE_FROM_ALL
    compute_engine_test.cpp
)

target_link_libraries(ixion-test libixion)
target_link_libraries(ixion-test-track-deps libixion)
target_link_libraries(dirty-cell-tracker-test libixion)
target_link_libraries(compute-engine-test libixion)

add_dependencies(check
    ixion-test
    ixion-test-track-deps
    dirty-cell-tracker-test
    compute-engine-test
)

add_test(ixion-test ixion-test)
add_test(ixion-test-track-deps ixion-test-track-deps)
add_test(dirty-cell-tracker-test dirty-cell-tracker-test)
add_test(compute-engine-test compute-engine-test)
