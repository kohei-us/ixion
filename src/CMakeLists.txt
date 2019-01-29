
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

add_custom_command(TARGET ixion-parser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:libixion>
    $<TARGET_FILE_DIR:ixion-parser>
)

# test programs

file(GLOB PARSER_TEST_FILES ${PROJECT_SOURCE_DIR}/test/*.txt)

foreach(_I RANGE 8)
    set(_IS "${_I}")
    add_custom_target(parser-test-t${_IS} COMMAND $<TARGET_FILE:ixion-parser> -t ${_I} ${PARSER_TEST_FILES} DEPENDS ixion-parser)
endforeach()

foreach(_I RANGE 7)
    set(_IS "${_I}")
    math(EXPR _INEXT "${_I}+1")
    add_dependencies(parser-test-t${_INEXT} parser-test-t${_IS})
endforeach()

add_dependencies(check
    parser-test-t0
    parser-test-t1
    parser-test-t2
    parser-test-t3
    parser-test-t4
    parser-test-t5
    parser-test-t6
    parser-test-t7
    parser-test-t8
)

install(TARGETS ixion-parser ixion-sorter ixion-formula-tokenizer
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
