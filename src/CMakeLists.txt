add_library(GraphMPC_lib STATIC)
target_sources(GraphMPC_lib 
    PRIVATE 
    processor.cpp
    utils/types.h
    utils/graph.h
    utils/random_generators.cpp 
    utils/sharing.cpp
    utils/mul.cpp
    utils/protocol_config.h
    io/tls_net_io_channel.h
    io/netmp.h
    io/comm.h
    protocol/sorting.cpp
    protocol/shuffle.cpp
    protocol/compaction.cpp
    protocol/message_passing.cpp
    # Add further files if necessary
)
target_link_libraries(GraphMPC_lib PUBLIC OpenSSL::SSL EMPTool NTL GMP OpenMP::OpenMP_CXX)
target_include_directories(GraphMPC_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include_directories("opt/homebrew/opt/libomp/include")
link_directories("/opt/homebrew/opt/libomp/lib/")


#add_executable(GraphMPC_src)
#target_sources(GraphMPC_src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)
#target_link_libraries(GraphMPC_src PUBLIC GraphMPC_lib)
