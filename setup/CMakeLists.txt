set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/setup)

add_library(GraphMPC_setup STATIC)
target_sources(GraphMPC_setup 
    PRIVATE 
    utils.cpp
    stats.cpp
    comm.h
    input-sharing/input_client.h
    input-sharing/input_server.h
    # Add further files if necessary
)
target_link_libraries(GraphMPC_setup PUBLIC OpenSSL::SSL EMPTool GMP nlohmann_json::nlohmann_json GraphMPC_lib)
target_include_directories(GraphMPC_setup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(launch_client input-sharing/launch_client.cpp)
target_link_libraries(launch_client GraphMPC_setup Boost::system Boost::program_options)