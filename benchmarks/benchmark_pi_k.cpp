#include <algorithm>

#include "../setup/constants.h"
#include "../setup/setup.h"
#include "../src/protocol/message_passing.h"
#include "../src/utils/graph.h"

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

void benchmark(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t BLOCK_SIZE, size_t repeat, size_t n_vertices,
               bool save_output, std::string save_file) {
    json output_data;
    network->init();

    Graph g;

    for (size_t i = 0; i < n_vertices; i++) g.add_list_entry(i + 1, i + 1, 1);
    for (size_t i = 0; i < n - n_vertices; i++) g.add_list_entry(1, 2, 0);

    size_t n_bits = std::ceil(std::log2(n_vertices + 2));
    std::cout << "n_bits: " << n_bits << std::endl;
    size_t n_iterations = 1;
    std::vector<Ring> weights(n_iterations);

    SecretSharedGraph g_shared = share::random_share_graph(id, rngs, n_bits, g);

    for (size_t r = 0; r < repeat; ++r) {
        std::cout << "--- Repetition " << r + 1 << " ---" << std::endl;

        StatsPoint start_pre(*network);
        if (id != D) {
            size_t n_receive = pi_k_comm_pre(id, n, n_bits, n_iterations);
            network->add_recv(D, n_receive);
            network->recv_queue(D);
        }
        auto preproc = mp::preprocess(id, rngs, network, g.size, n_bits, n_iterations, pre_mp_preprocess, post_mp_preprocess);
        StatsPoint end_pre(*network);
        network->sync();

        auto rbench = end_pre - start_pre;
        output_data["benchmarks"].push_back(rbench);

        size_t bytes_sent = 0;
        for (const auto &val : rbench["communication"]) {
            bytes_sent += val.get<int64_t>();
        }

        std::cout << "preprocessing time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "preprocessing sent: " << bytes_sent << " bytes" << std::endl;

        StatsPoint start(*network);
        mp::evaluate(id, rngs, network, g.size, n_bits, n_iterations, n_vertices, g_shared, weights, apply, pre_mp_evaluate, post_mp_evaluate, preproc);
        StatsPoint end(*network);

        rbench = end - start;
        output_data["benchmarks"].push_back(rbench);

        bytes_sent = 0;
        for (const auto &val : rbench["communication"]) {
            bytes_sent += val.get<int64_t>();
        }

        std::cout << "Communication (elements): " << bytes_sent / 4 / n << "n" << std::endl;

        std::cout << "online time: " << rbench["time"] << " ms" << std::endl;
        std::cout << "online sent: " << bytes_sent << " bytes" << std::endl;

        std::cout << std::endl;
    }

    output_data["stats"] = {{"peak_virtual_memory", peakVirtualMemory()}, {"peak_resident_set_size", peakResidentSetSize()}};

    std::cout << "--- Overall Statistics ---\n";
    for (const auto &[key, value] : output_data["stats"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    if (save_output) {
        saveJson(output_data, save_file);
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
        setup::run_benchmark(opts, benchmark);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}