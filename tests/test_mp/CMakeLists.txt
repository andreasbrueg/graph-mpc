add_executable(test_mp
    test_mp.cpp
    ${TEST_SOURCES}
    ${TEST_HEADERS})
target_link_libraries(test_mp PUBLIC Boost::system Boost::program_options EMPTool GMP GraphMPC_setup GraphMPC_lib)
target_include_directories(test_mp PUBLIC ${TEST_INCLUDE_DIRS})
