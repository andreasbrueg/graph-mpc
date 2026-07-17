#include "../algorithms/pi_k.h"
#include "benchmark.h"

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsBenchmark());

    bpo::options_description cmdline("Benchmark the secure computation of the Truncated Katz Score.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        auto network = setup::setupNetwork(opts);
        auto conf = setup::setupProtocol(opts, network);
        auto b_conf = setup::setupBenchmark(opts);

        std::vector<Ring> weights = {10000000, 100000, 1000, 1};
        auto circuit = PiKCircuit(conf, weights);
        circuit.build();

        auto graph_generator = [](ProtocolConfig &conf, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network) {
            return Graph::benchmark_graph(conf, rngs, network);
        };

        auto benchmark = Benchmark(conf, b_conf, &circuit, network, graph_generator);
        size_t assert_comm_pre, assert_comm_eval;
        if (conf.id == D) {
            assert_comm_pre = 4 * (24 * conf.size * conf.bits + 64 * conf.size - 12 + 8 * conf.size * conf.depth) +
                                  2 * sizeof(size_t);  // one element per party always sent to synchronize vector sizes
            assert_comm_pre += 2 * (sizeof(size_t) + 3 * 2 * sizeof(uint64_t)); // initial PRF seed distribution (to 2 parties, #seeds and 3 seeds, each consisting of 2 uint64_t)
            if (conf.depth == 0) {
                // Lazy evaluation: double-shuffles are not set up if we use no iteration, each is 8t setup, so we expect 24t setup less
                assert_comm_pre -= 24 * conf.size;
            }
            assert_comm_eval = 0;
        } else {
            assert_comm_pre = 0;
            if (conf.id == P0) {
                assert_comm_pre += 2 * sizeof(uint64_t); // initial PRF seed distribution, 1 seed to P1
            }
            assert_comm_eval = 4 * (18 * conf.size * conf.bits + 58 * conf.size - 24 + 4 * conf.size * conf.depth);
        }
        benchmark.run(assert_comm_pre, assert_comm_eval);

    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}
