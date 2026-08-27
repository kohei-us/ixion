
add_library(ixion-${IXION_API_VERSION} SHARED
    address.cpp
    address_range.cpp
    cell.cpp
    cell_access.cpp
    cell_queue_manager.cpp
    compute_engine.cpp
    config.cpp
    debug.cpp
    dirty_cell_tracker.cpp
    document.cpp
    exceptions.cpp
    formula.cpp
    formula_calc.cpp
    formula_functions.cpp
    formula_groups.cpp
    formula_function_opcode.cpp
    formula_interpreter.cpp
    formula_lexer.cpp
    formula_name_resolver.cpp
    formula_opcode.cpp
    formula_parser.cpp
    formula_result.cpp
    formula_tokens.cpp
    formula_value_stack.cpp
    global.cpp
    grid_dumper.cpp
    impl_types.cpp
    info.cpp
    interface.cpp
    lexer_tokens.cpp
    matrix.cpp
    model_context.cpp
    model_context_impl.cpp
    string_id_pool.cpp
    inline_string_pool.cpp
    model_cell_range.cpp
    model_iterator.cpp
    model_types.cpp
    module.cpp
    named_expressions_iterator.cpp
    queue_entry.cpp
    table.cpp
    table_store.cpp
    types.cpp
    utf8.cpp
    utils.cpp
    sheet_sort.cpp
    sheet_store.cpp
    sheet_view.cpp
)

if(BUILD_VULKAN)

    add_library(ixion-${IXION_API_VERSION}-vulkan MODULE
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

add_executable(column-store-test EXCLUDE_FROM_ALL
    column_store_test.cpp
)

add_executable(document-test EXCLUDE_FROM_ALL
    document_test.cpp
)

add_executable(general-test EXCLUDE_FROM_ALL
    general_test.cpp
)

add_executable(model-context-test EXCLUDE_FROM_ALL
    model_context_test.cpp
)

# The tested classes are internal to the library and their symbols stay
# hidden, so their sources get compiled into the test binary directly.
add_executable(sheet-sort-test EXCLUDE_FROM_ALL
    sheet_sort_test.cpp
    sheet_sort.cpp
    sheet_store.cpp
    formula_groups.cpp
)

add_executable(sheet-view-test EXCLUDE_FROM_ALL
    sheet_view_test.cpp
)

add_executable(name-resolver-test EXCLUDE_FROM_ALL
    name_resolver_test.cpp
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

add_executable(version-test EXCLUDE_FROM_ALL
    version_test.cpp
)

target_include_directories(compute-engine-test PRIVATE ${PROJECT_SOURCE_DIR}/src/include)

target_compile_definitions(model-context-test PRIVATE SRCDIR="${PROJECT_SOURCE_DIR}")

target_link_libraries(column-store-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(document-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(general-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(model-context-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(sheet-sort-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(sheet-view-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(ixion-test-track-deps ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(dirty-cell-tracker-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(compute-engine-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(name-resolver-test ixion-${IXION_API_VERSION} ixion-test)
target_link_libraries(version-test ixion-${IXION_API_VERSION} ixion-test)

add_dependencies(check
    column-store-test
    document-test
    general-test
    model-context-test
    sheet-sort-test
    sheet-view-test
    ixion-test-track-deps
    dirty-cell-tracker-test
    compute-engine-test
    name-resolver-test
    version-test
)

add_test(column-store-test column-store-test)
add_test(document-test document-test)
add_test(general-test general-test)
add_test(model-context-test model-context-test)
add_test(sheet-sort-test sheet-sort-test)
add_test(sheet-view-test sheet-view-test)
add_test(ixion-test-track-deps ixion-test-track-deps)
add_test(dirty-cell-tracker-test dirty-cell-tracker-test)
add_test(compute-engine-test compute-engine-test)
add_test(name-resolver-test name-resolver-test)
add_test(version-test version-test)
