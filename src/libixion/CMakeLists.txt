
add_library(ixion-${IXION_API_VERSION} SHARED
    address.cpp
    address_iterator.cpp
    calc_status.cpp
    cell.cpp
    cell_access.cpp
    cell_queue_manager.cpp
    compute_engine.cpp
    concrete_formula_tokens.cpp
    config.cpp
    debug.cpp
    dirty_cell_tracker.cpp
    document.cpp
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
    model_types.cpp
    module.cpp
    named_expressions_iterator.cpp
    queue_entry.cpp
    table.cpp
    types.cpp
    utf8.cpp
    utils.cpp
    workbook.cpp
)

if(BUILD_VULKAN)

    add_library(ixion-${IXION_API_VERSION}-vulkan SHARED
        compute_engine_vulkan.cpp
        vulkan_obj.cpp
    )

    target_link_libraries(ixion-${IXION_API_VERSION}-vulkan
        ixion-${IXION_API_VERSION}
        Vulkan::Vulkan
    )

    install(TARGETS ixion-${IXION_API_VERSION}-vulkan
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

endif()

configure_file(constants.inl.in ${CMAKE_CURRENT_BINARY_DIR}/inl/constants.inl)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/inl)

target_compile_definitions(ixion-${IXION_API_VERSION} PRIVATE IXION_BUILD DLL_EXPORT)

if(MSVC)
    target_compile_definitions(ixion-${IXION_API_VERSION} PRIVATE _USE_MATH_DEFINES)
endif()

install(TARGETS ixion-${IXION_API_VERSION}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# test programs

add_executable(document-test EXCLUDE_FROM_ALL
    document_test.cpp
)

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

target_include_directories(compute-engine-test PRIVATE ${PROJECT_SOURCE_DIR}/src/include)

target_link_libraries(document-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(ixion-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(ixion-test-track-deps ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(dirty-cell-tracker-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(compute-engine-test ixion-${IXION_API_VERSION} ixion-test)

add_dependencies(check
    document-test
    ixion-test
    ixion-test-track-deps
    dirty-cell-tracker-test
    compute-engine-test
)

add_test(document-test document-test)
add_test(ixion-test ixion-test)
add_test(ixion-test-track-deps ixion-test-track-deps)
add_test(dirty-cell-tracker-test dirty-cell-tracker-test)
add_test(compute-engine-test compute-engine-test)
