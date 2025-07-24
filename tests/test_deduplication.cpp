#include <cassert>

#include "../setup/constants.h"
#include "../setup/setup.h"
#include "../src/protocol/clip.h"
#include "../src/protocol/deduplication.h"
#include "../src/utils/bits.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"

void test_deduplication(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_deduplication ------" << std::endl << std::endl;
    json output_data;
    network->init();
    size_t n_bits = sizeof(Ring) * 8;

    Graph g;
    g.add_list_entry(1, 1, 1);
    g.add_list_entry(2, 2, 1);
    g.add_list_entry(1, 2, 0);
    g.add_list_entry(2, 1, 0);
    g.add_list_entry(1, 3, 0);
    g.add_list_entry(1, 3, 0);  // duplicate
    g.add_list_entry(3, 1, 0);
    g.add_list_entry(4, 4, 1);
    g.add_list_entry(3, 3, 1);
    g.add_list_entry(3, 1, 0);  // duplicate
    g.add_list_entry(3, 2, 0);
    g.add_list_entry(2, 3, 0);
    g.add_list_entry(2, 4, 0);
    g.add_list_entry(4, 2, 0);
    g.add_list_entry(2, 4, 0);  // duplicate
    g.add_list_entry(4, 2, 0);  // duplicate

    n = g.size;

    SecretSharedGraph g_shared = share::random_share_graph(id, rngs, n_bits, g);

    /* Preprocessing */
    StatsPoint start_pre(*network);
    if (id != D) {
        size_t n_receive = deduplication_comm_pre(id, n, n_bits);
        network->add_recv(D, n_receive);
        network->recv_queue(D);
    }

    auto preproc = deduplication_preprocess(id, rngs, network, g.size, n_bits);
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
        size_t total_comm = 4 * deduplication_comm_pre(id, n, n_bits);
        std::cout << "Expected comm: " << total_comm << " actual: " << bytes_sent_pre << std::endl;
        assert(bytes_sent_pre == total_comm);
    }

    StatsPoint start_online(*network);
    deduplication_evaluate(id, rngs, network, g.size, preproc, g_shared);
    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * deduplication_comm_online(n, n_bits);
        std::cout << "Expected comm: " << total_comm << " actual: " << bytes_sent << std::endl;
        assert(total_comm == bytes_sent);
    }

    /* Assertions for correctness */
    if (id != D) {
        auto MSBs = share::reveal_vec(id, network, g_shared.src_bits[n_bits]);
        assert(MSBs[0] == 0);
        assert(MSBs[1] == 0);
        assert(MSBs[2] == 0);
        assert(MSBs[3] == 0);
        assert(MSBs[4] == 0);
        assert(MSBs[5] == 1);
        assert(MSBs[6] == 0);
        assert(MSBs[7] == 0);
        assert(MSBs[8] == 0);
        assert(MSBs[9] == 1);
        assert(MSBs[10] == 0);
        assert(MSBs[11] == 0);
        assert(MSBs[12] == 0);
        assert(MSBs[13] == 0);
        assert(MSBs[14] == 1);
        assert(MSBs[15] == 1);
        std::cout << "Assertions passed." << std::endl;
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
        setup::run_test(opts, test_deduplication);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}