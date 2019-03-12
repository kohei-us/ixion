
add_subdirectory(libixion)

add_executable(ixion-parser
	ixion_parser.cpp
	model_parser.cpp
	session_handler.cpp
	table_handler.cpp
)

add_executable(ixion-sorter
	ixion_sorter.cpp
	sort_input_parser.cpp
)

add_executable(ixion-formula-tokenizer
	ixion_formula_tokenizer.cpp
)

target_link_libraries(ixion-parser libixion-${IXION_API_VERSION})
target_link_libraries(ixion-sorter libixion-${IXION_API_VERSION})
target_link_libraries(ixion-formula-tokenizer libixion-${IXION_API_VERSION})

# tests

# Copy the binaries necessary for running the parser tests below.
add_custom_command(TARGET ixion-parser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:libixion-${IXION_API_VERSION}> $<TARGET_FILE:ixion-parser>
    ${CMAKE_CURRENT_BINARY_DIR}
)

file(GLOB PARSER_TEST_FILES ${PROJECT_SOURCE_DIR}/test/*.txt)

add_dependencies(check ixion-parser)

# Register tests for all test files and all thread counts (0-8).
foreach(_I RANGE 8)
    foreach(_TESTFILE ${PARSER_TEST_FILES})
        get_filename_component(_TFNAME ${_TESTFILE} NAME_WE)
        add_test(parser-test-t${_I}-${_TFNAME} ${CMAKE_CURRENT_BINARY_DIR}/ixion-parser -t ${_I} ${_TESTFILE})
    endforeach()
endforeach()

install(TARGETS ixion-parser ixion-sorter ixion-formula-tokenizer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
