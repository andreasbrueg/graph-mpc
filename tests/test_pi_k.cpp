#include <omp.h>

#include <algorithm>
#include <cassert>

#include "../setup/constants.h"
#include "../setup/setup.h"
#include "../src/protocol/deduplication.h"
#include "../src/protocol/message_passing.h"
#include "../src/utils/bits.h"
#include "../src/utils/perm.h"

std::vector<Ring> apply(std::vector<Ring> &old_payload, std::vector<Ring> &new_payload) { return new_payload; }

void pre_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits, MPPreprocessing &preproc) {
    preproc.deduplication_pre = deduplication_preprocess(id, rngs, network, n, n_bits);
}

void post_mp_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc) { return; }

void pre_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc, SecretSharedGraph &g) {
    deduplication_evaluate(id, rngs, network, n, preproc.deduplication_pre, g);
    preproc.dst_order = preproc.deduplication_pre.dst_sort;
}

void post_mp_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, SecretSharedGraph &g, MPPreprocessing &preproc,
                      std::vector<Ring> &payload) {
    g.payload = payload;
}

void test_pi_k(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_pi_k ------" << std::endl << std::endl;
    json output_data;
    network->init();

    /*
    Graph instance:
    v1 - v2
    || / ||
    v3   v4

    Which in list form is (kind of random order here for testing):
    (1,1,1) // 0
    (2,2,1) // 1
    (1,2,0) // 2
    (2,1,0) // 3
    (1,3,0) // 4
    (1,3,0) // 5
    (3,1,0) // 6
    (4,4,1) // 7
    (3,3,1) // 8
    (3,1,0) // 9
    (3,2,0) // 10
    (2,3,0) // 11
    (2,4,0) // 12
    (4,2,0) // 13
    (2,4,0) // 14
    (4,2,0) // 15
    */

    Graph g;

    g.add_list_entry(1, 1, 1);
    g.add_list_entry(2, 2, 1);
    g.add_list_entry(1, 2, 0);
    g.add_list_entry(2, 1, 0);
    g.add_list_entry(1, 3, 0);
    g.add_list_entry(1, 3, 0);
    g.add_list_entry(3, 1, 0);
    g.add_list_entry(4, 4, 1);
    g.add_list_entry(3, 3, 1);
    g.add_list_entry(3, 1, 0);
    g.add_list_entry(3, 2, 0);
    g.add_list_entry(2, 3, 0);
    g.add_list_entry(2, 4, 0);
    g.add_list_entry(4, 2, 0);
    g.add_list_entry(2, 4, 0);
    g.add_list_entry(4, 2, 0);

    n = g.size;
    std::vector<Ring> weights = {10000000, 100000, 1000, 1};
    const size_t n_vertices = 4;
    const size_t n_iterations = weights.size();
    size_t n_bits = std::ceil(std::log2(n_vertices + 2));

    if (id != D) g.print();

    SecretSharedGraph g_shared = share::random_share_graph(id, rngs, n_bits, g);

    /* Preprocessing */
    StatsPoint start_pre(*network);
    if (id != D) {
        size_t n_receive = pi_k_comm_pre(id, n, n_bits, n_iterations);
        network->add_recv(D, n_receive);
        network->recv_queue(D);
    }
    auto preproc = mp::preprocess(id, rngs, network, n, n_bits, n_iterations, pre_mp_preprocess, post_mp_preprocess);
    StatsPoint end_pre(*network);

    auto rbench_pre = end_pre - start_pre;
    output_data["benchmarks_pre"].push_back(rbench_pre);
    size_t bytes_sent_pre = 0;
    for (const auto &val : rbench_pre["communication"]) {
        bytes_sent_pre += val.get<int64_t>();
    }

    /* Preprocessing communication assertions */
    if (id == D) {
        /* n_elems * 4 Bytes per element */
        size_t total_comm = 4 * pi_k_comm_pre(id, n, n_bits, n_iterations);
        std::cout << "Expected comm: " << total_comm << " actual: " << bytes_sent_pre << std::endl;
        assert(bytes_sent_pre == total_comm);
    }

    StatsPoint start_online(*network);
    mp::evaluate(id, rngs, network, n, n_bits, n_iterations, n_vertices, g_shared, weights, apply, pre_mp_evaluate, post_mp_evaluate, preproc);
    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * pi_k_comm_online(n, n_bits, n_iterations);
        std::cout << "Expected comm: " << total_comm << " actual: " << bytes_sent << std::endl;
        assert(total_comm == bytes_sent);
    }

    auto res_g = share::reveal_graph(id, network, n_bits, g_shared);

    if (id != D) {
        res_g.print();

        assert(res_g.payload[0] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
        assert(res_g.payload[1] == 30513025);  // 3 of length 1, 5 of length 2, 13 of length 3, 25 of length 4
        assert(res_g.payload[2] == 20510023);  // 2 of length 1, 5 of length 2, 10 of length 3, 23 of length 4
        assert(res_g.payload[3] == 10305013);  // 1 of length 1, 3 of length 2,  5 of length 3, 13 of length 4
    }
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_pi_k);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}