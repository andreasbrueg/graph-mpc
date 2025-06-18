#include <cassert>

#include "../setup/setup.h"
#include "../src/protocol/clip.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"

void test_eqz(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    json output_data;

    std::vector<Ring> x_vec(n);
    std::vector<Ring> y_vec(n);

    std::vector<Ring> share_x_vec(n);
    std::vector<Ring> share_y_vec(n);

    for (size_t i = 0; i < n; ++i) {
        x_vec[i] = std::rand() % 2;
    }
    for (size_t i = 0; i < n / 2; ++i) {
        y_vec[i] = 1;
    }
    for (size_t i = n / 2; i < n; ++i) {
        y_vec[i] = 0;
    }

    share_x_vec = share::random_share_secret_vec_2P(id, rngs, x_vec);
    share_y_vec = share::random_share_secret_vec_2P(id, rngs, y_vec);

    auto triples = mul::preprocess(id, rngs, network, n, BLOCK_SIZE);
    auto x_y_share = mul::evaluate(id, network, n, BLOCK_SIZE, triples, share_x_vec, share_y_vec);
    auto x_y_revealed = share::reveal_vec(id, network, BLOCK_SIZE, x_y_share);

    if (id != D) {
        std::cout << "x: ";
        for (auto &elem : x_vec) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;

        std::cout << "y: ";
        for (auto &elem : y_vec) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;

        std::cout << "Result of multiplication: ";
        for (auto &elem : x_y_revealed) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl << "------ test_eqz ------" << std::endl;
    std::vector<Ring> test_vec(n);
    for (size_t i = 0; i < n; ++i) test_vec[i] = i;
    auto test_vec_share = share::random_share_secret_vec_2P(id, rngs, test_vec);

    auto eqz_vec = clip::equals_zero(id, rngs, network, BLOCK_SIZE, test_vec_share);
    std::vector<Ring> B2A_vec = clip::B2A(id, rngs, network, BLOCK_SIZE, eqz_vec);

    auto result = share::reveal_vec(id, network, BLOCK_SIZE, B2A_vec);

    if (id != D) {
        std::cout << "Result of eqz: ";
        for (auto &elem : result) {
            std::cout << elem << ", ";
        }
        std::cout << std::endl;

        for (size_t i = 0; i < n; ++i) {
            if (i == 0)
                assert(result[i] == 1);
            else
                assert(result[i] == 0);
        }
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
        setup::run_test(opts, test_eqz);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}