#include <algorithm>
#include <cassert>

#include "../algorithms/bfs.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestBFS : public Test {
   public:
    TestBFS(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network) : Test(conf, network) {}

    Circuit *create_circuit() override {
        auto circ = new BFSCircuit(conf);
        circ->build();
        return circ;
    }

    Graph create_graph(RandomGenerators &rngs) override {
        /*
            Directed graph instance (-, |, etc. are both directions):
            v6
            |
            v
            v3 -> v2   v7
            || /  ||
            v1    v4 - v5

            Which in list form is (kind of random order here for testing):
            (2,2,1)
            (2,4,0)
            (4,2,0)
            (2,4,0)
            (1,2,0)
            (2,1,0)
            (3,3,1)
            (7,7,1)
            (4,5,0)
            (5,4,0)
            (4,4,1)
            (3,1,0)
            (1,3,0)
            (5,5,1)
            (6,6,1)
            (1,3,0)
            (3,1,0)
            (3,2,0)
            (1,1,1)
            (4,2,0)
            (6,3,0)
        */

        Graph g;
        g.add_list_entry(2, 2, 1, 1);// Starting point
        g.add_list_entry(2, 4, 0);
        g.add_list_entry(4, 2, 0);
        g.add_list_entry(2, 4, 0);
        g.add_list_entry(1, 2, 0);
        g.add_list_entry(2, 1, 0);
        g.add_list_entry(3, 3, 1);
        g.add_list_entry(7, 7, 1, 1);// Starting point
        g.add_list_entry(4, 5, 0);
        g.add_list_entry(5, 4, 0);
        g.add_list_entry(4, 4, 1, 1);// Starting point
        g.add_list_entry(3, 1, 0);
        g.add_list_entry(1, 3, 0);
        g.add_list_entry(5, 5, 1);
        g.add_list_entry(6, 6, 1);
        g.add_list_entry(1, 3, 0);
        g.add_list_entry(3, 1, 0);
        g.add_list_entry(3, 2, 0);
        g.add_list_entry(1, 1, 1);
        g.add_list_entry(4, 2, 0);
        g.add_list_entry(6, 3, 0);

        Graph g_shared = g.secret_share_parties(conf.id, rngs, network, conf.bits, P0);
        return g_shared;
    }

    void run_assertions(std::vector<Ring> &result, [[maybe_unused]] size_t &bytes_sent_pre, [[maybe_unused]] size_t &bytes_sent_eval) override {
        if (conf.id == D) {
            [[maybe_unused]] size_t expected_pre = 4 * (16 * conf.size * conf.bits + 27 * conf.size + 8 * conf.size * conf.depth + 6 * conf.nodes) +
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
            [[maybe_unused]] size_t expected_eval = 4 * (12 * conf.size * conf.bits + 17 * conf.size + 4 * conf.size * conf.depth + 12 * conf.nodes);
            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);

            result = share::reveal_vec(id, network, result);
            print_vec(result);

            // Assumes starting points v2, v4, v7 as specified in the input
            assert(result[0] == 1);
            assert(result[1] == 1);
            assert(result[2] == 0);
            assert(result[3] == 1);
            assert(result[4] == 1);
            assert(result[5] == 0);
            assert(result[6] == 1);

            std::cout << "test_bfs_multiple_sources passed." << std::endl;
        }
    }
};

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("Test for the secure computation of BFS where the inputs include multiple BFS sources.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        Party id = (Party)opts["pid"].as<int>();
        const size_t size = 21;
        const size_t nodes = 7;
        const size_t depth = 1;
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

        auto test = TestBFS(conf, network);
        test.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}