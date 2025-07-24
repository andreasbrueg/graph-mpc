add_library(GraphMPC_setup STATIC)
target_sources(GraphMPC_setup 
    PRIVATE 
    utils.cpp
    stats.cpp
    comm.h
    # Add further files if necessary
)
target_link_libraries(GraphMPC_setup PUBLIC OpenSSL::SSL EMPTool GMP nlohmann_json::nlohmann_json)

target_include_directories(GraphMPC_setup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})