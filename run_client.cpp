#include <iostream>

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

#include "data_interface/input-sharing/input_client.h"
#include "setup/cmdline.h"
#include "utils/graph.h"
#include "utils/types.h"

int main(int argc, char **argv) {
    bpo::options_description desc("Arguments");
    desc.add_options()(
        "inputfile", bpo::value<std::string>()->default_value(""), "Path to graph slice input file to be used as the client's input. If not set, the client tries to read the file 'subgraph_[cid].data' for client ID 'cid'.")(
        "offset", bpo::value<size_t>()->default_value(0), "Graph slice offset. The client will provide the slice of the DAG list to the server that starts at the index specified by 'offset' and ends before 'offset' plus the client's input length. The clients need to coordinate that there are no overlaps between their slices, otherwise, the servers will abort the computation.")(
        "bits", bpo::value<size_t>()->default_value(32), "Bits used to represent vertices. This can be at most 32, but any lower sufficient value will greatly increase efficiency. Make sure this is set to the same value for all servers and clients!")(
        "cid", bpo::value<size_t>(), "Client ID. Clients need to be numbered 0, 1, 2, ..., clients-1")(
        "localhost", bpo::bool_switch(), "Set for local testing. This will cause all servers and clients to connect via localhost.")(
        "net-config", bpo::value<std::string>(), "Path to JSON file containing network details of all servers and clients.")(
        "clients", bpo::value<size_t>()->default_value(1), "Number of clients participating in the input-sharing.")(
        "port", bpo::value<int>()->default_value(10000), "Base port for networking.")(
        "input-port", bpo::value<int>()->default_value(4242), "Port used for receiving input shares from clients.")(
        "certificate_path", bpo::value<std::string>()->default_value("certs/cert1.pem"), "Path to full certificate chain file for TLS server connections")(
        "private_key_path", bpo::value<std::string>()->default_value("certs/key1.pem"), "Path to private key for TLS server connections")(
        "trusted_cert_path", bpo::value<std::string>()->default_value("certs/cert_ca.pem"), "Path with trusted certificate for TLS client connections")(
        "BLOCK_SIZE", bpo::value<size_t>()->default_value(100000), "BLOCK_SIZE for sending messages over the network.")(
        "help,h", "output the help message"
        );
    auto prog_opts(desc);

    bpo::options_description cmdline("Runs a PPGA client for providing parts of the input graph and receiving outputs from PPGA.");
    cmdline.add(prog_opts);

    bpo::variables_map opts;
    bpo::store(bpo::command_line_parser(argc, argv).options(cmdline).run(), opts);

    if (opts.count("help") != 0) {
        std::cout << cmdline << std::endl;
        return 0;
    }

    bpo::notify(opts);

    bool error = false;
    if (opts.count("cid") == 0) {
        std::cout << "ERROR: Expected client ID 'cid'." << std::endl;
        error = true;
    }
    if (!opts["localhost"].as<bool>() && (opts.count("net-config") == 0)) {
        std::cout << "ERROR: Expected one of 'localhost' or 'net-config'." << std::endl;
        error = true;
    }
    if (error) {
        std::cout << "Run './run_server --help' to display help page." << std::endl;
        return 1;
    }
    std::string input_file = opts["inputfile"].as<std::string>();
    size_t offset = opts["offset"].as<size_t>();
    size_t bits = opts["bits"].as<size_t>();
    size_t cid = opts["cid"].as<size_t>();
    size_t clients = opts["clients"].as<size_t>();
    if (input_file == "") {
        input_file = "subgraph_" + std::to_string(cid) + ".data";
    }
    if (bits == 0 || bits > 32) {
        std::cout << "ERROR: Invalid number of bits 'bits': " << bits << ". It needs to be at least 1 and at most 32." << std::endl;
        error = true;
    }
    if (cid >= clients) {
        std::cout << "ERROR: Provided client ID ('cid') " << cid << " which needs to be below the total number of 'clients' " << clients << ", but is not." << std::endl;
        error = true;
    }
    if (error) {
        std::cout << "Run './run_server --help' to display help page." << std::endl;
        return 1;
    }
    
    emp::PRG rng; // Constructor uses cryptographically secure random seed

    Graph g;
    try {
        g = Graph::parse_simplified_format(input_file);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }
    std::cout << "Providing the following graph slice as input:" << std::endl;
    g.print_pretty();

    std::cout << "----- Configuration -----" << std::endl;
    std::cout << "Client: " << cid << std::endl;
    std::cout << "Local graph size |V|+|E|: " << g.size << std::endl;
    std::cout << "Local nodes |V|: " << g.nodes << std::endl;
    std::cout << "Bits: " << bits << std::endl;
    std::cout << "#Clients: " << clients << std::endl << std::endl;

    auto network = setup::setupNetwork(P0, cid, clients, true, opts);

    /* Send shares*/
    InputClient client(network, bits);
    client.send_graph(rng, g, offset);
    network->sync();
    network->sync();
    network->sync();
    auto result = client.recv_result(g.size);
    for (size_t i = 0; i < g.size; ++i) {
        if (g.isV[i]) {
            std::cout << "node " << g.src[i] << " ";
            if (result[i] == 0) {
                std::cout << "not reached (state 0)" << std::endl;
            } else {
                assert(result[i] == 1);
                std::cout << "reached     (state 1)" << std::endl;
            }
        } else {
            assert(result[i] == 0);
        }
    }
    std::cout << std::endl;
    std::cout << "unformatted result vector: ";
    setup::print_vec(result);

    return 0;

    // TODO also output config data
}