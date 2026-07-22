#include <algorithm>
#include <cassert>

#include "../algorithms/bfs_3_parallel.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestBFS3Parallel : public Test {
   public:
    TestBFS3Parallel(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network) : Test(conf, network) {}

    Circuit *create_circuit() override {
        auto circ = new BFS3ParallelCircuit(conf);
        circ->build();
        return circ;
    }

    Graph create_graph(RandomGenerators &rngs) override {
        /*
            Directed graph instance:
            
            v1 -> v3 -> v5 -> v7
            ^     |     ^     |
            |     v     |     v
            v2 <- v4 <- v6 <- v8

            Which in list form is:
            (1,1,1)
            (2,2,1)
            (3,3,1)
            (4,4,1)
            (5,5,1)
            (6,6,1)
            (7,7,1)
            (8,8,1)
            (1,3,0)
            (3,5,0)
            (5,7,0)
            (8,6,0)
            (6,4,0)
            (4,2,0)
            (2,1,0)
            (3,4,0)
            (6,5,0)
            (7,8,0)
        */

        Graph g;
        g.add_list_entry(1, 1, 1);
        g.add_list_entry(2, 2, 1);
        g.add_list_entry(3, 3, 1);
        g.add_list_entry(4, 4, 1);
        g.add_list_entry(5, 5, 1, 5); // Input should be ignored as bfs_3_parallel overwrites it.
        g.add_list_entry(6, 6, 1);
        g.add_list_entry(7, 7, 1);
        g.add_list_entry(8, 8, 1);
        g.add_list_entry(1, 3, 0);
        g.add_list_entry(3, 5, 0);
        g.add_list_entry(5, 7, 0);
        g.add_list_entry(8, 6, 0);
        g.add_list_entry(6, 4, 0);
        g.add_list_entry(4, 2, 0);
        g.add_list_entry(2, 1, 0);
        g.add_list_entry(3, 4, 0);
        g.add_list_entry(6, 5, 0);
        g.add_list_entry(7, 8, 0);

        Graph g_shared = g.secret_share_parties(conf.id, rngs, network, conf.bits, P0);
        return g_shared;
    }

    void run_assertions(std::vector<Ring> &result, [[maybe_unused]] size_t &bytes_sent_pre, [[maybe_unused]] size_t &bytes_sent_eval) override {
        if (conf.id == D) {
            [[maybe_unused]] size_t expected_pre = 4 * (16 * conf.size * conf.bits + 6 * 3 * conf.nodes + 8 * conf.size * 3 * conf.depth +
                                       2 * conf.size * 3 + 25 * conf.size) +
                                  2 * sizeof(size_t);  // one element per party always sent to synchronize vector sizes
            expected_pre += 2 * (sizeof(size_t) + 3 * 2 * sizeof(uint64_t)); // initial PRF seed distribution (to 2 parties, #seeds and 3 seeds, each consisting of 2 uint64_t)
            [[maybe_unused]] size_t expected_eval = 0;

            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);
        } else {
            [[maybe_unused]] size_t expected_pre = 0;
            if (conf.id == P0) {
                expected_pre += 2 * sizeof(uint64_t); // initial PRF seed distribution, 1 seed to P1
            }
            [[maybe_unused]] size_t expected_eval = 4 * (12 * conf.size * conf.bits + 12 * 3 * conf.nodes + 4 * conf.size * 3 * conf.depth +
                                        conf.size * 3 + 16 * conf.size);
            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);

            result = share::reveal_vec(id, network, result);
            print_vec(result);

            // Starting points v1, v2, v3
            // Reaching {v1,v2,v3,v4,v5,v7}, {v1,v2,v3,v4,v5}, {v1,v2,v3,v4,v5,v7,v8}
            // v1:
            assert(result[0] == 1);
            assert(result[1] == 1);
            assert(result[2] == 1);
            assert(result[3] == 1);
            assert(result[4] == 1);
            assert(result[5] == 0);
            assert(result[6] == 1);
            assert(result[7] == 0);
            // v2:
            assert(result[1 * 18 + 0] == 1);
            assert(result[1 * 18 + 1] == 1);
            assert(result[1 * 18 + 2] == 1);
            assert(result[1 * 18 + 3] == 1);
            assert(result[1 * 18 + 4] == 1);
            assert(result[1 * 18 + 5] == 0);
            assert(result[1 * 18 + 6] == 0);
            assert(result[1 * 18 + 7] == 0);
            // v3:
            assert(result[2 * 18 + 0] == 1);
            assert(result[2 * 18 + 1] == 1);
            assert(result[2 * 18 + 2] == 1);
            assert(result[2 * 18 + 3] == 1);
            assert(result[2 * 18 + 4] == 1);
            assert(result[2 * 18 + 5] == 0);
            assert(result[2 * 18 + 6] == 1);
            assert(result[2 * 18 + 7] == 1);

            std::cout << "test_bfs_3_parallel passed." << std::endl;
        }
    }
};

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("Test for the secure computation of three parallel BFS instances.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        Party id = (Party)opts["pid"].as<int>();
        const size_t size = 18;
        const size_t nodes = 8;
        const size_t depth = 3;
        const size_t bits = std::ceil(std::log2(nodes + 2));
        bool ssd = false;

        ProtocolConfig conf = {id, size, nodes, depth, bits, ssd};
        auto network = setup::setupNetwork(opts);

        std::cout << "----- Test Configuration -----" << std::endl;
        std::cout << "Party: " << id << std::endl;
        std::cout << "Size: " << size << std::endl;
        std::cout << "Nodes: " << nodes << std::endl;
        std::cout << "Bits: " << bits << std::endl;
        std::cout << "SSD: " << ssd << std::endl;

        auto test = TestBFS3Parallel(conf, network);
        test.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}