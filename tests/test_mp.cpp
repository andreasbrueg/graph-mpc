#include <cassert>
#include <random>

#include "../setup/setup.h"
#include "../src/protocol/message_passing.h"
#include "../src/utils/perm.h"

std::vector<Ring> apply(std::vector<Ring> &old_payload, std::vector<Ring> &new_payload) {
    std::vector<Ring> result(old_payload.size());

    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = old_payload[i] + new_payload[i];
    }
    return result;
}

void test_mp(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_mp ------" << std::endl << std::endl;

    json output_data;
    /**
     *      0 == 1
     *       \  //
     *         2
     */

    Graph g;
    g.size = 8;
    g.src = std::vector<Ring>({0, 1, 2, 0, 1, 2, 2, 2});
    g.dst = std::vector<Ring>({0, 1, 2, 1, 0, 1, 0, 1});
    g.isV = std::vector<Ring>({1, 1, 1, 0, 0, 0, 0, 0});
    g.payload = std::vector<Ring>({1, 2, 3, 0, 0, 0, 0, 0});

    SecretSharedGraph g_shared = share::random_share_graph(id, rngs, g);

    auto preproc = mp::preprocess(id, rngs, network, g.size, BLOCK_SIZE, 2);
    mp::evaluate(id, rngs, network, g.size, BLOCK_SIZE, g_shared, 2, 3, preproc, apply);
    auto res_g = share::reveal_graph(id, network, BLOCK_SIZE, g_shared);

    if (id != D) {
        res_g.print();
        assert(res_g.payload[0] == 1);
        assert(res_g.payload[1] == 1);
        assert(res_g.payload[2] == 1);
        assert(res_g.payload[3] == 0);
        assert(res_g.payload[4] == 0);
        assert(res_g.payload[5] == 0);
        assert(res_g.payload[6] == 0);
        assert(res_g.payload[7] == 0);
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
        setup::run_test(opts, test_mp);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}