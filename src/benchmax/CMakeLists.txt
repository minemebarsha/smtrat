option( BENCHMAX_USE_SMTPARSER "Use parser for smtlib files." OFF )

configure_file( ${CMAKE_SOURCE_DIR}/src/benchmax/config.h.in ${CMAKE_SOURCE_DIR}/src/benchmax/config.h )

add_subdirectory(ssh)
add_subdirectory(tools)

add_executable( benchmax benchmax.cpp Benchmark.h Tool.h Benchmark.cpp Stats.cpp
			Settings.cpp Tool.cpp Smt2Input.cpp BenchmarkStatus.cpp ${ssh_sources} ${tool_sources})
include_directories(${carl_INCLUDE_DIR})
if(${STATICLIB_SWITCH} STREQUAL "ON")
  set_target_properties( benchmax PROPERTIES LINK_SEARCH_END_STATIC TRUE )
  set_target_properties( benchmax PROPERTIES LINK_SEARCH_START_STATIC TRUE )
endif()

if(${STATICLIB_SWITCH} STREQUAL "ON")
	target_link_libraries( benchmax ssh2 ssl crypto dl z pthread boost_program_options boost_filesystem boost_system boost_chrono rt smtlibParser ${GINAC_LIBRARY} ${CLN_LIBRARY} ${GMP_LIBRARY} )
else()
	target_link_libraries( benchmax ssh2 pthread boost_program_options boost_filesystem  boost_system boost_chrono rt smtlibParser ${GINAC_LIBRARY} ${CLN_LIBRARY} ${GMP_LIBRARY} )
endif()

# stripping
add_custom_target( strip-unneeded strip --strip-unneeded benchmax DEPENDS benchmax WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Stripping unneeded symbols" )
if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_custom_target( strip-debug ALL strip --strip-debug benchmax DEPENDS benchmax WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Stripping unneeded symbols" )
endif()

add_custom_target( benchmax_move mv -v ${CMAKE_CURRENT_BINARY_DIR}/benchmax ${CMAKE_BINARY_DIR}/
                   DEPENDS benchmax
                   COMMENT "Moving binary to ${CMAKE_BINARY_DIR}" )