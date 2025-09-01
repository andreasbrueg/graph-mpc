#include "../setup/input-sharing/input_client.h"
#include "../setup/input-sharing/input_server.h"
#include "../setup/utils.h"
#include "../src/utils/graph.h"

void test_socket(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file, Graph &g) {
    auto network = std::make_shared<io::NetIOMP>(net_conf);

    if (id == P0) {
        InputServer server(id, std::to_string(4242), 2);  // Server expecting two clients
        server.connect_clients();
        g = server.recv_graph();
        g.print();
    }

    if (id == P1) {
        InputServer server(id, std::to_string(4243), 2);  // Server expecting two clients
        server.connect_clients();
        g = server.recv_graph();
        g.print();
    }

    network->sync();
    std::cout << "Terminating..." << std::endl;

    return;
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for sorting.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_socket);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}