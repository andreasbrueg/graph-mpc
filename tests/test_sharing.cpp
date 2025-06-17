#include <cassert>

#include "../setup/setup.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"

void test_sharing(const bpo::variables_map &opts) {
    std::cout << "------ test_sharing ------" << std::endl << std::endl;
    auto vec_size = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num, nodes;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[9];
    uint64_t seeds_l[9];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, shuffle_num, nodes, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},         {"num_parties", nP}, {"threads", threads},  {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l}, {"repeat", repeat},  {"vec-size", vec_size}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    const size_t BLOCK_SIZE = 100000;

    std::vector<Ring> share(vec_size);
    std::vector<Ring> input_table(vec_size);
    std::vector<Ring> reconstructed;

    for (size_t i = 0; i < vec_size; ++i) {
        input_table[i] = i;
    }

    StatsPoint start_sharing(*network);
    share = share::random_share_secret_vec_2P(party, rngs, input_table);
    StatsPoint end_sharing(*network);

    auto rbench_sharing = end_sharing - start_sharing;
    output_data["benchmarks_pre"].push_back(rbench_sharing);
    size_t bytes_sent_sharing = 0;
    for (const auto &val : rbench_sharing["communication"]) {
        bytes_sent_sharing += val.get<int64_t>();
    }

    /* No communication when secret sharing a vector */
    assert(bytes_sent_sharing == 0);

    StatsPoint start_reveal(*network);
    reconstructed = share::reveal_vec(party, network, BLOCK_SIZE, share);
    StatsPoint end_reveal(*network);

    auto rbench_reveal = end_reveal - start_reveal;
    output_data["benchmarks_pre"].push_back(rbench_reveal);
    size_t bytes_sent_reveal = 0;
    for (const auto &val : rbench_reveal["communication"]) {
        bytes_sent_reveal += val.get<int64_t>();
    }

    /* Each party sends its share during reveal */
    assert(bytes_sent_reveal == 4 * vec_size);

    std::cout << "Final share: ";
    for (const auto &elem : share) {
        std::cout << elem << ", ";
    }
    std::cout << std::endl << std::endl;

    assert(input_table == reconstructed);

    std::cout << "Reconstructed: ";
    for (const auto &elem : reconstructed) {
        std::cout << elem << ", ";
    }
    std::cout << std::endl << std::endl;

    /* Test Graph Sharing */
    Graph g;
    g.size = 8;
    std::vector<Ring> src({0, 1, 2, 3, 0, 1, 2, 3});
    std::vector<Ring> dst({0, 1, 2, 3, 1, 2, 0, 2});
    std::vector<Ring> isV({1, 1, 1, 1, 0, 0, 0, 0});
    std::vector<Ring> payload({1, 2, 3, 4, 0, 0, 0, 0});
    g.src = src;
    g.dst = dst;
    g.isV = isV;
    g.payload = payload;

    SecretSharedGraph shared_graph = share::random_share_graph(party, rngs, g);
    Graph reconstructed_graph = share::reveal_graph(party, network, BLOCK_SIZE, shared_graph);

    std::cout << "Initial graph: " << std::endl;
    g.print();
    std::cout << "Reconstructed graph: " << std::endl;
    reconstructed_graph.print();

    assert(g.src == reconstructed_graph.src);
    assert(g.dst == reconstructed_graph.dst);
    assert(g.isV == reconstructed_graph.isV);
    assert(g.payload == reconstructed_graph.payload);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        test_sharing(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}