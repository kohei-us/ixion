
add_library(libixion-${IXION_API_VERSION} SHARED
    address.cpp
    address_iterator.cpp
    calc_status.cpp
    cell.cpp
    cell_queue_manager.cpp
    compute_engine.cpp
    compute_engine_cuda.cpp
    concrete_formula_tokens.cpp
    config.cpp
    debug.cpp
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
    model_context_impl.cpp
    model_iterator.cpp
    module.cpp
    named_expressions_iterator.cpp
    queue_entry.cpp
    table.cpp
    types.cpp
    workbook.cpp
)

configure_file(constants.inl.in ${CMAKE_CURRENT_BINARY_DIR}/inl/constants.inl)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/inl)

target_compile_definitions(libixion-${IXION_API_VERSION} PRIVATE IXION_BUILD DLL_EXPORT)

install(TARGETS libixion-${IXION_API_VERSION}
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

target_link_libraries(ixion-test libixion-${IXION_API_VERSION})
target_link_libraries(ixion-test-track-deps libixion-${IXION_API_VERSION})
target_link_libraries(dirty-cell-tracker-test libixion-${IXION_API_VERSION})
target_link_libraries(compute-engine-test libixion-${IXION_API_VERSION})

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
