#include <algorithm>
#include <cassert>

#include "../algorithms/pi_m.h"
#include "../src/utils/comm.h"
#include "test.h"

class TestPiMClients : public Test {
   public:
    TestPiMClients(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> network, Graph &graph) : Test(conf, network) { g = graph; }

    Circuit *create_circuit() override {
        std::vector<Ring> weights = {10000000, 100000, 1000, 1};
        auto circ = new PiMCircuit(conf, weights);
        circ->provide_outputs_in_input_order();
        circ->build();
        return circ;
    }

    Graph create_graph(RandomGenerators &) override { return g; }

    void run_assertions(std::vector<Ring> &result, size_t &bytes_sent_pre, size_t &bytes_sent_eval) override {
        if (conf.id == D) {
            size_t expected_pre = 4 * (16 * conf.size * conf.bits + 27 * conf.size + 8 * conf.size * conf.depth) +
                                  2 * sizeof(size_t);  // one element per party always sent to synchronize vector sizes
            expected_pre += 4 * 2 * conf.size; // final shuffle to switch back to input order
            expected_pre += 2 * (sizeof(size_t) + 3 * 2 * sizeof(uint64_t)); // initial PRF seed distribution (to 2 parties, #seeds and 3 seeds, each consisting of 2 uint64_t)
            size_t expected_eval = 0;

            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);
        } else {
            size_t expected_pre = 0;
            if (conf.id == P0) {
                expected_pre += 2 * sizeof(uint64_t); // initial PRF seed distribution, 1 seed to P1
            }
            size_t expected_eval = 4 * (12 * conf.size * conf.bits + 17 * conf.size + 4 * conf.size * conf.depth);
            expected_eval += 4 * 1 * conf.size; // final shuffle to switch back to input order
            assert(bytes_sent_pre == expected_pre);
            assert(bytes_sent_eval == expected_eval);

            /* Sending the result to the clients */
            for (int i = 0; i < (network->n_total - network->nP); ++i) {
                network->send_result(i, result);
            }

            result = share::reveal_vec(id, network, result);
            print_vec(result);

            assert(result[0] == 31030096);  // 3 of length 1, 10 of length 2, 30 of length 3,  96 of length 4
            assert(result[1] == 41036100);  // 4 of length 1, 10 of length 2, 36 of length 3, 100 of length 4
            assert(result[8] == 31030096);  // 3 of length 1, 10 of length 2, 30 of length 3,  96 of length 4
            assert(result[7] == 20820072);  // 2 of length 1,  8 of length 2, 20 of length 3,  72 of length 4

            std::cout << "test_pi_m_clients passed." << std::endl;
        }
    }
};

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("(Server-side) test for the secure computation of the Multilayer Truncated Katz Score where the input graph is provided by clients.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);
    size_t clients = 2;

    try {
        Party id = (Party)opts["pid"].as<int>();

        Graph graph;
        auto network = setup::setupNetwork(opts, clients);
        if (id != D) {
            InputServer server(network, clients);
            std::cout << "Awaiting " << clients << " packets" << std::endl << std::endl;
            graph = server.recv_graph();
            std::cout << "Finished graph construction." << std::endl;
        } else {
            size_t nodes, size;
            InputServer server(network, clients);
            nodes = server.recv_nodes();
            server.recv_size(size);
            graph.nodes = nodes;
            graph.size = size;
        }
        const size_t depth = 4;
        const size_t bits = 3;
        bool ssd = true;

        ProtocolConfig conf = {id, graph.size, graph.nodes, depth, bits, ssd};

        // auto g_rev = graph.reveal(id, network);
        // g_rev.print();

        std::cout << "----- Test Configuration -----" << std::endl;
        std::cout << "Party: " << id << std::endl;
        std::cout << "Size: " << graph.size << std::endl;
        std::cout << "Nodes: " << graph.nodes << std::endl;
        std::cout << "Bits: " << bits << std::endl;
        std::cout << "SSD: " << ssd << std::endl;

        auto test = TestPiMClients(conf, network, graph);
        test.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}