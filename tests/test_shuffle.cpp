#include <cassert>
#include <functional>
#include <random>

#include "../setup/utils.h"
#include "../src/graphmpc/shuffle.h"
#include "../src/utils/sharing.h"

void test_shuffle(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    std::cout << "------ test_shuffle ------" << std::endl << std::endl;
    bool save_to_disk = true;
    std::shared_ptr<io::NetIOMP> network = std::make_shared<io::NetIOMP>(net_conf, save_to_disk);

    std::vector<Ring> input_vector(n);
    for (size_t i = 0; i < n; i++) input_vector[i] = i;

    auto input_share = share::random_share_secret_vec_2P(id, rngs, input_vector);

    MPPreprocessing preproc;
    Party recv = P0;

    if (id != D) network->recv_buffered(D);
    shuffle::get_shuffle(id, rngs, network, n, preproc, recv, save_to_disk);
    shuffle::get_unshuffle(id, rngs, network, n, preproc, save_to_disk);
    shuffle::get_shuffle(id, rngs, network, n, preproc, recv, save_to_disk);
    shuffle::get_unshuffle(id, rngs, network, n, preproc, save_to_disk);
    if (id == D) network->send_all();

    auto shuffled_input = shuffle::shuffle(id, rngs, network, n, preproc, input_share, save_to_disk, true);
    auto repeated_input = shuffle::shuffle_repeat(id, rngs, network, n, preproc, input_share, save_to_disk);
    repeated_input = shuffle::shuffle_repeat(id, rngs, network, n, preproc, input_share, save_to_disk);
    auto unshuffled_input = shuffle::unshuffle(id, rngs, network, n, preproc, shuffled_input, save_to_disk);

    auto result_shuffle = share::reveal_vec(id, network, shuffled_input);
    auto result_repeat = share::reveal_vec(id, network, repeated_input);
    auto result_unshuffle = share::reveal_vec(id, network, unshuffled_input);

    setup::print_vec(result_shuffle);
    setup::print_vec(result_repeat);
    setup::print_vec(result_unshuffle);

    shuffled_input = shuffle::shuffle(id, rngs, network, n, preproc, input_share, save_to_disk, true);
    repeated_input = shuffle::shuffle_repeat(id, rngs, network, n, preproc, input_share, save_to_disk);
    unshuffled_input = shuffle::unshuffle(id, rngs, network, n, preproc, shuffled_input, save_to_disk);

    result_shuffle = share::reveal_vec(id, network, shuffled_input);
    result_repeat = share::reveal_vec(id, network, repeated_input);
    result_unshuffle = share::reveal_vec(id, network, unshuffled_input);

    setup::print_vec(result_shuffle);
    setup::print_vec(result_repeat);
    setup::print_vec(result_unshuffle);

    return;
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_shuffle);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}