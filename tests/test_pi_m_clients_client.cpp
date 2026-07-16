#include <chrono>
#include <iostream>
#include <thread>

#include "setup/cmdline.h"
#include "setup/input-sharing/input_client.h"

void launch_client(std::shared_ptr<io::NetIOMP> network, emp::PRG rng, size_t client_id, size_t start_idx, size_t bits, std::string input_file = "") {
    Graph g;
    if (input_file != "") {
        g = Graph::parse(input_file);
    } else {
        /* Optionally define graph here */
    }
    g.print();

    /* Send shares*/
    InputClient client(network, bits);
    client.send_graph(rng, g, start_idx);
    network->sync();
    network->sync();
    network->sync();
    auto result = client.recv_result(g.size);
    setup::print_vec(result);
    if (client_id == 0) {
        assert(result[0] == 31030096);
        assert(result[1] == 41036100);
    } else {
        assert(result[0] == 20820072);
        assert(result[1] == 31030096);
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::clientProgramOptions());

    bpo::options_description cmdline("Launching a client that sends part of the full graph.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    size_t client_id = opts["cid"].as<int>();

    size_t start_idx, bits;
    std::string input_file;

    if (client_id == 0) {
        start_idx = 0;
        bits = 3;
        input_file = "subgraph_0.data";
    } else if (client_id == 1) {
        start_idx = 7;
        bits = 3;
        input_file = "subgraph_1.data";
    } else {
        throw std::runtime_error("Invalid client id: this test is designed for 2 clients only, 0 and 1");
    }

    auto network = setup::setupNetwork(opts, 2);

    /* Dummy PRG */
    emp::PRG rng; // TODO ??? security?
    auto seed_block = emp::makeBlock(14132, 68436);
    rng.reseed(&seed_block, 0);

    launch_client(network, rng, client_id, start_idx, bits, input_file);
}