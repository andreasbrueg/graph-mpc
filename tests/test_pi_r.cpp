#include <algorithm>
#include <cassert>

#include "../algorithms/pi_r.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestPiR : public Test {
   public:
    TestPiR(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network) : Test(conf, network) {}

    Circuit *create_circuit() override {
        auto circ = new PiRCircuit(conf);
        circ->build();
        return circ;
    }

    Graph create_graph(RandomGenerators &rngs) override {
        /*
         Graph instance:
         v1 - v2
         ||   ||
         v3   v4 - v5

         Which in list form is (kind of random order here for testing):
         (1,1,1)
         (2,2,1)
         (1,2,0)
         (4,5,0)
         (2,1,0)
         (1,3,0)
         (1,3,0)
         (3,1,0)
         (5,5,1)
         (4,4,1)
         (3,3,1)
         (3,1,0)
         (2,4,0)
         (4,2,0)
         (2,4,0)
         (5,4,0)
         (4,2,0)
         */

        Graph g;

        if (id == P0) {
            g.add_list_entry(1, 1, 1);
            g.add_list_entry(2, 2, 1);
            g.add_list_entry(1, 2, 0);
            g.add_list_entry(4, 5, 0);
            g.add_list_entry(2, 1, 0);
            g.add_list_entry(1, 3, 0);
            g.add_list_entry(1, 3, 0);
            g.add_list_entry(3, 1, 0);
            g.add_list_entry(5, 5, 1);
            g.add_list_entry(4, 4, 1);
            g.add_list_entry(3, 3, 1);
            g.add_list_entry(3, 1, 0);
            g.add_list_entry(2, 4, 0);
            g.add_list_entry(4, 2, 0);
            g.add_list_entry(2, 4, 0);
            g.add_list_entry(5, 4, 0);
            g.add_list_entry(4, 2, 0);
        }

        Graph g_shared = g.secret_share_parties(conf.id, rngs, network, conf.bits, P0);

        return g_shared;
    }

    void run_assertions(std::vector<Ring> &result, size_t &bytes_sent_pre, size_t &bytes_sent_eval) override {
        if (conf.id == D) {
            size_t expected_pre = 4 * (16 * conf.size * conf.bits + 6 * conf.nodes * conf.nodes + 8 * conf.size * conf.nodes * conf.depth +
                                       2 * conf.size * conf.nodes + 25 * conf.size) +
                                  2 * sizeof(size_t);  // one element per party always sent to synchronize vector sizes
            expected_pre += 2 * (sizeof(size_t) + 3 * 2 * sizeof(uint64_t)); // initial PRF seed distribution (to 2 parties, #seeds and 3 seeds, each consisting of 2 uint64_t)
            size_t expected_eval = 0;
            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);
        } else {
            size_t expected_pre = 0;
            if (conf.id == P0) {
                expected_pre += 2 * sizeof(uint64_t); // initial PRF seed distribution, 1 seed to P1
            }
            size_t expected_eval = 4 * (12 * conf.size * conf.bits + 12 * conf.nodes * conf.nodes + 4 * conf.size * conf.nodes * conf.depth +
                                        conf.size * conf.nodes + 16 * conf.size);
            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);

            result = share::reveal_vec(id, network, result);
            print_vec(result);

            assert(result[0] == 4);
            assert(result[1] == 5);
            assert(result[2] == 3);
            assert(result[3] == 4);
            assert(result[4] == 3);

            std::cout << "test_pi_r passed." << std::endl;
        }
    }
};

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("Test for the secure computation of the Reach Score.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        Party id = (Party)opts["pid"].as<int>();
        const size_t size = 17;
        const size_t nodes = 5;
        const size_t depth = 2;
        const size_t bits = std::ceil(std::log2(nodes + 2));
        bool ssd = true;

        auto network = setup::setupNetwork(opts);
        ProtocolConfig conf = {id, size, nodes, depth, bits, ssd};

        auto test = TestPiR(conf, network);
        test.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}