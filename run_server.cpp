#include <iostream>

#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

#include "data_interface/input-sharing/input_server.h"
#include "setup/cmdline.h"
#include "src/core/circuit.h"
#include "src/core/evaluator.h"
#include "src/core/preprocessor.h"
#include "src/core/storage.h"
#include "src/utils/stats.h"
#include "utils/graph.h"
#include "utils/types.h"

#include "algorithms/bfs_3_parallel.h"
#include "algorithms/bfs.h"
#include "algorithms/custom.h"
#include "algorithms/pi_k.h"
#include "algorithms/pi_m.h"
#include "algorithms/pi_r.h"

enum Algorithm { BFS = 0, BFS3P = 1, PI_M = 2, PI_K = 3, PI_R = 4, CUSTOM = 5, NUMBER_OF_ALGOS = 6 };

std::string algo_description(Algorithm algo) {
    switch (algo) {
        case Algorithm::BFS: return std::string("BFS (user provided inputs, 1 on source nodes, 0 on others).");
        case Algorithm::BFS3P: return std::string("BFS (ignoring user input), running 3 parallel instances, each from one of the first three nodes.");
        case Algorithm::PI_M: return std::string("Multilayer truncated Katz centrality score. This is equivalent to the standard truncated Katz centrality score if no duplicate edges exist in the input.");
        case Algorithm::PI_K: return std::string("Truncated Katz score (supporting multilayer graphs). For graphs without duplicate edges, this is equivalent to the Multilayer score PI_M, but PI_M is more efficient, so that should be used.");
        case Algorithm::PI_R: return std::string("Reach centrality score.");
        case Algorithm::CUSTOM: return std::string("Custom algorithm that can be implemented in algorithms/custom.h.");
        default: throw std::runtime_error("Unknown algorithm!");
    }
}

int main(int argc, char **argv) {
    bpo::options_description desc("Arguments");
    std::string algos = "";
    for (size_t i = 0; i < Algorithm::NUMBER_OF_ALGOS; i++) {
        algos += std::to_string(i) + ") " + algo_description((Algorithm) i) + "      ";
    }
    desc.add_options()(
        "algorithm", bpo::value<size_t>(), algos.c_str())(
        "weights", bpo::value<vector<Ring>>()->multitoken(), "weights, only needed for (multilayer) Katz centrality, pass as, e.g., {1,2,3,4}. Needs to contain 'depth' many elements.")(
        "depth", bpo::value<size_t>(), "iteration depth parameter d")(
        "bits", bpo::value<size_t>()->default_value(32), "Bits used to represent vertices. This can be at most 32, but any lower sufficient value will greatly increase efficiency. Make sure this is set to the same value for all servers and clients!")(
        "pid", bpo::value<int>(), "Server ID. PPGA needs two main servers with IDs 0 and 1, and an additional dealer server with ID 2.")(
        "localhost", bpo::bool_switch(), "Set for local testing. This will cause all servers and clients to connect via localhost.")(
        "net-config", bpo::value<std::string>(), "Path to JSON file containing network details of all servers and clients.")(
        "clients", bpo::value<size_t>()->default_value(1), "Number of clients participating in the input-sharing.")(
        "port", bpo::value<int>()->default_value(10000), "Base port for networking.")(
        "input-port", bpo::value<int>()->default_value(4242), "Port used for receiving input shares from clients.")(
        "certificate_path", bpo::value<std::string>()->default_value("certs/cert1.pem"), "Path to full certificate chain file for TLS server connections")(
        "private_key_path", bpo::value<std::string>()->default_value("certs/key1.pem"), "Path to private key for TLS server connections")(
        "trusted_cert_path", bpo::value<std::string>()->default_value("certs/cert_ca.pem"), "Path with trusted certificate for TLS client connections")(
        "BLOCK_SIZE", bpo::value<size_t>()->default_value(100000), "BLOCK_SIZE for sending messages over the network.")(
        "ssd", bpo::bool_switch(), "Set to let preprocessing values be stored on disk. Caution: This can potentially write much data, degrading SSD durability.")(
        "help,h", "output the help message"
        );
    auto prog_opts(desc);

    bpo::options_description cmdline("Runs a PPGA server for computing privacy-preserving graph analysis.");
    cmdline.add(prog_opts);

    bpo::variables_map opts;
    bpo::store(bpo::command_line_parser(argc, argv).options(cmdline).run(), opts);

    if (opts.count("help") != 0) {
        std::cout << cmdline << std::endl;
        return 0;
    }

    bpo::notify(opts);

    bool error = false;
    if (opts.count("algorithm") == 0) {
        std::cout << "ERROR: Expected 'algorithm'." << std::endl;
        error = true;
    }
    if (opts.count("depth") == 0) {
        std::cout << "ERROR: Expected 'depth'." << std::endl;
        error = true;
    }
    if (opts.count("pid") == 0) {
        std::cout << "ERROR: Expected party ID 'pid'." << std::endl;
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
    size_t algorithm = opts["algorithm"].as<size_t>();
    size_t depth = opts["depth"].as<size_t>();
    size_t bits = opts["bits"].as<size_t>();
    Party pid = (Party)opts["pid"].as<int>();
    size_t clients = opts["clients"].as<size_t>();
    bool ssd = opts["ssd"].as<bool>();
    if (algorithm >= Algorithm::NUMBER_OF_ALGOS) {
        std::cout << "ERROR: Invalid algorithm ID 'algorithm': " << algorithm << ". It needs to be at below " << Algorithm::NUMBER_OF_ALGOS << "." << std::endl;
        error = true;
    }
    if (opts.count("weights") == 0 && (algorithm == Algorithm::PI_M || algorithm == Algorithm::PI_K)) {
        std::cout << "ERROR: Expected weights 'weights' for the specified algorithm." << std::endl;
        error = true;
    }
    std::vector<Ring> weights(0);
    if (algorithm == Algorithm::PI_M || algorithm == Algorithm::PI_K) {
        weights = opts["weights"].as<std::vector<Ring>>();
    }
    if ((algorithm == Algorithm::PI_M || algorithm == Algorithm::PI_K) && weights.size() != depth) {
        std::cout << "ERROR: Provided " << weights.size() << " weights for depth " << depth << ", but the number of weights needs to match the depth." << std::endl;
        error = true;
    }
    if (bits == 0 || bits > 32) {
        std::cout << "ERROR: Invalid number of bits 'bits': " << bits << ". It needs to be at least 1 and at most 32." << std::endl;
        error = true;
    }
    if (pid != 0 && pid != 1 && pid != 2) {
        std::cout << "ERROR: Invalid party ID 'pid': " << pid << std::endl;
        error = true;
    }
    if (clients == 0) {
        std::cout << "ERROR: Invalid number of clients 'clients': 0. This server is configured to run only on client-provided inputs, so at least one client is required." << std::endl;
        error = true;
    }
    if (error) {
        std::cout << "Run './run_server --help' to display help page." << std::endl;
        return 1;
    }

    Graph graph;
    auto network = setup::setupNetwork(pid, 0, clients, false, opts);
    if (pid != D) {
        InputServer server(network, clients);
        std::cout << "Awaiting graph slices from " << clients << " clients..." << std::endl << std::endl;
        graph = server.recv_graph(bits);
        std::cout << "Merged slices into complete input graph." << std::endl;
    } else {
        // For dealer, simply receive graph size as this determines the size of required correlated randomness.
        size_t nodes, size;
        InputServer server(network, clients);
        nodes = server.recv_nodes();
        server.recv_size(size);
        graph.nodes = nodes;
        graph.size = size;
    }

    ProtocolConfig conf = {pid, graph.size, graph.nodes, depth, bits, ssd};

    std::cout << "----- Configuration -----" << std::endl;
    std::cout << "Server: " << pid << std::endl;
    std::cout << "Graph size |V|+|E|: " << graph.size << std::endl;
    std::cout << "Nodes |V|: " << graph.nodes << std::endl;
    std::cout << "Bits: " << bits << std::endl;
    std::cout << "#Clients: " << clients << std::endl;
    std::cout << "Storing preprocessing values on disk: " << ssd << std::endl << std::endl;

    Circuit *circ;
    switch (algorithm) {
        case Algorithm::BFS: circ = new BFSCircuit(conf); break;
        case Algorithm::BFS3P: circ = new BFS3ParallelCircuit(conf); break;
        case Algorithm::PI_M: circ = new PiMCircuit(conf, weights); break;
        case Algorithm::PI_K: circ = new PiKCircuit(conf, weights); break;
        case Algorithm::PI_R: circ = new PiRCircuit(conf); break;
        case Algorithm::CUSTOM: circ = new CustomCircuit(conf); break;
    }
    std::cout << "Running algorithm " << algorithm << " " << algo_description((Algorithm) algorithm) << std::endl << std::endl;
    circ->provide_outputs_in_input_order();
    circ->build();

    auto io = Storage(conf, circ);
    Preprocessor *preproc = new Preprocessor(conf, &io, network);
    network->sync();
    size_t bytes_sent_pre = 0;
    size_t bytes_sent_eval = 0;

    /* Preprocessing */
    StatsPoint start_pre(*network);
    preproc->run(circ);
    StatsPoint end_pre(*network);
    auto rbench_pre = end_pre - start_pre;
    for (const auto &val : rbench_pre["communication"]) {
        bytes_sent_pre += val.get<int64_t>();
    }
    std::cout << "preprocessing time: " << rbench_pre["time"] << " ms" << std::endl;
    std::cout << "preprocessing sent: " << bytes_sent_pre << " bytes" << std::endl;

    Evaluator *eval = new Evaluator(circ, conf, &io, network, graph);
    network->sync();

    /* Evaluation */
    StatsPoint start_online(*network);
    eval->run();
    StatsPoint end_online(*network);
    auto rbench = end_online - start_online;
    for (const auto &val : rbench["communication"]) {
        bytes_sent_eval += val.get<int64_t>();
    }
    std::cout << "online time: " << rbench["time"] << " ms" << std::endl;
    std::cout << "online sent: " << bytes_sent_eval << " bytes" << std::endl;

    network->sync();
    std::cout << "Peak virtual memory: " << peakVirtualMemory() << std::endl;

    /* Sending the result to the clients */
    auto result = eval->result(); // These are just the shares held by this server
    for (size_t i = 0; i < clients; ++i) {
        network->send_result((int) i, result);
    }

    return 0;
}