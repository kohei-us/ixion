
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

target_link_libraries(ixion-parser libixion)
target_link_libraries(ixion-sorter libixion)
target_link_libraries(ixion-formula-tokenizer libixion)

# tests

# Copy the binaries necessary for running the parser tests below.
add_custom_command(TARGET ixion-parser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:libixion> $<TARGET_FILE:ixion-parser>
    ${CMAKE_CURRENT_BINARY_DIR}
)

file(GLOB PARSER_TEST_FILES ${PROJECT_SOURCE_DIR}/test/*.txt)

add_dependencies(check ixion-parser)

foreach(_I RANGE 8)
    add_test(parser-test-t${_I} ${CMAKE_CURRENT_BINARY_DIR}/ixion-parser -t ${_I} ${PARSER_TEST_FILES})
endforeach()

install(TARGETS ixion-parser ixion-sorter ixion-formula-tokenizer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
