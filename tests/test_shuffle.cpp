#include <cassert>

#include "../setup/setup.h"
#include "../src/protocol/shuffle.h"
#include "../src/utils/sharing.h"

void test_shuffle(const bpo::variables_map &opts) {
    std::cout << "------ test_shuffle ------" << std::endl << std::endl;
    auto vec_size = opts["vec-size"].as<size_t>();

    size_t pid, nP, repeat, threads, shuffle_num, nodes;
    std::shared_ptr<io::NetIOMP> network = nullptr;
    uint64_t seeds_h[9];
    uint64_t seeds_l[9];
    json output_data;
    bool save_output;
    std::string save_file;

    setup::setupExecution(opts, pid, nP, repeat, threads, shuffle_num, nodes, network, seeds_h, seeds_l, save_output, save_file);
    output_data["details"] = {{"pid", pid},         {"num_parties", nP}, {"threads", threads},  {"seeds_h", seeds_h},
                              {"seeds_l", seeds_l}, {"repeat", repeat},  {"vec-size", vec_size}};

    std::cout << "--- Details ---\n";
    for (const auto &[key, value] : output_data["details"].items()) {
        std::cout << key << ": " << value << "\n";
    }
    std::cout << std::endl;

    /* Setting up the input vector */
    std::vector<Ring> input_vector(vec_size);
    for (size_t i = 0; i < vec_size; i++) input_vector[i] = i;

    Party party = (pid == 0) ? P0 : ((pid == 1) ? P1 : D);
    RandomGenerators rngs(seeds_h, seeds_l);
    const size_t BLOCK_SIZE = 100000;

    const size_t n_shuffles = 3;
    const size_t n_repeats = 1;
    const size_t n_unshuffles = 2;
    const size_t n_merged_shuffles = 2;

    /* Communication (per party) */
    const size_t shuffle_comm_pre = 3 * vec_size;         // 3n: B_0, B_1, pi_1_p
    const size_t unshuffle_comm_pre = 2 * vec_size;       // 2n: B_0, B_1
    const size_t merged_shuffle_comm_pre = 4 * vec_size;  // 4n: sigma_0_p, sigma_1, B_0, B_1
    const size_t shuffle_comm_online = vec_size;          // n: t_0/1
    const size_t unshuffle_comm_online = vec_size;        // n: t_0/1
    const size_t merged_shuffle_comm_online = vec_size;   // n: t_0/1

    std::vector<Ring> share = share::random_share_secret_vec_2P(party, rngs, input_vector);

    /* Preprocessing */
    StatsPoint start_pre(*network);

    ShufflePre perm_share_one = shuffle::get_shuffle(party, rngs, network, vec_size, BLOCK_SIZE, true);
    ShufflePre perm_share_two = shuffle::get_shuffle(party, rngs, network, vec_size, BLOCK_SIZE, true);
    std::vector<Ring> unshuffle_B_one = shuffle::get_unshuffle(party, rngs, network, vec_size, BLOCK_SIZE, perm_share_one);
    std::vector<Ring> unshuffle_B_two = shuffle::get_unshuffle(party, rngs, network, vec_size, BLOCK_SIZE, perm_share_two);
    ShufflePre perm_share_merged_one = shuffle::get_merged_shuffle(party, rngs, network, vec_size, BLOCK_SIZE, perm_share_one, perm_share_two);
    ShufflePre perm_share_three = shuffle::get_shuffle(party, rngs, network, vec_size, BLOCK_SIZE, false);
    ShufflePre perm_share_merged_two = shuffle::get_merged_shuffle(party, rngs, network, vec_size, BLOCK_SIZE, perm_share_two, perm_share_three);

    StatsPoint end_pre(*network);

    auto rbench_pre = end_pre - start_pre;
    output_data["benchmarks_pre"].push_back(rbench_pre);
    size_t bytes_sent_pre = 0;
    for (const auto &val : rbench_pre["communication"]) {
        bytes_sent_pre += val.get<int64_t>();
    }

    /* Preprocessing communication assertions */
    if (party == D) {
        /* n_elems * 4 Bytes per element */
        size_t total_comm = 4 * (n_shuffles * shuffle_comm_pre + n_unshuffles * unshuffle_comm_pre + n_merged_shuffles * merged_shuffle_comm_pre);
        assert(bytes_sent_pre == total_comm);
    }

    StatsPoint start_online(*network);

    std::vector<Ring> shuffle_share_one = shuffle::shuffle(party, rngs, network, share, perm_share_one, vec_size, BLOCK_SIZE);
    std::vector<Ring> repeat_share = shuffle::shuffle(party, rngs, network, share, perm_share_one, vec_size, BLOCK_SIZE);
    std::vector<Ring> unshuffle_share_one = shuffle::unshuffle(party, rngs, network, perm_share_one, unshuffle_B_one, repeat_share, vec_size, BLOCK_SIZE);
    std::vector<Ring> shuffle_share_two = shuffle::shuffle(party, rngs, network, share, perm_share_two, vec_size, BLOCK_SIZE);
    std::vector<Ring> unshuffle_share_two = shuffle::unshuffle(party, rngs, network, perm_share_two, unshuffle_B_two, shuffle_share_two, vec_size, BLOCK_SIZE);
    std::vector<Ring> merged_share_one = shuffle::shuffle(party, rngs, network, share, perm_share_merged_one, vec_size, BLOCK_SIZE);
    std::vector<Ring> shuffle_share_three = shuffle::shuffle(party, rngs, network, share, perm_share_three, vec_size, BLOCK_SIZE);
    std::vector<Ring> merged_share_two = shuffle::shuffle(party, rngs, network, share, perm_share_merged_two, vec_size, BLOCK_SIZE);

    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (party != D) {
        size_t total_comm = 4 * (n_shuffles * shuffle_comm_online + n_repeats * shuffle_comm_online + n_unshuffles * unshuffle_comm_online +
                                 n_merged_shuffles * merged_shuffle_comm_online);
        assert(total_comm == bytes_sent);
    }

    std::vector<Ring> res = share::reveal_vec(party, network, BLOCK_SIZE, shuffle_share_one);

    if (pid != D) {
        std::cout << std::endl << "Result of shuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < res.size(); ++i) {
            if (res[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
    }

    auto repeat_res = share::reveal_vec(party, network, BLOCK_SIZE, repeat_share);

    if (pid != D) {
        std::cout << std::endl << "Result of repeat: ";
        for (int i = 0; i < repeat_res.size() - 1; ++i) {
            std::cout << repeat_res[i] << ", ";
        }
        std::cout << repeat_res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        assert(res == repeat_res);
    }

    res = share::reveal_vec(party, network, BLOCK_SIZE, unshuffle_share_one);

    if (pid != D) {
        std::cout << std::endl << "Result of unshuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        assert(res == input_vector);
    }

    auto new_shuffle = share::reveal_vec(party, network, BLOCK_SIZE, shuffle_share_two);

    if (pid != D) {
        std::cout << std::endl << "Result of second shuffle: ";
        for (int i = 0; i < new_shuffle.size() - 1; ++i) {
            std::cout << new_shuffle[i] << ", ";
        }
        std::cout << new_shuffle[new_shuffle.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < new_shuffle.size(); ++i) {
            if (new_shuffle[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
        assert(res != new_shuffle);
    }

    res = share::reveal_vec(party, network, BLOCK_SIZE, unshuffle_share_two);

    if (pid != D) {
        std::cout << std::endl << "Result of second unshuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        assert(res == input_vector);
    }

    res = share::reveal_vec(party, network, BLOCK_SIZE, merged_share_one);

    if (pid != D) {
        std::cout << std::endl << "Result of merged shuffle: ";
        for (int i = 0; i < res.size() - 1; ++i) {
            std::cout << res[i] << ", ";
        }
        std::cout << res[res.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < res.size(); ++i) {
            if (res[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
    }

    auto third_shuffle = share::reveal_vec(party, network, BLOCK_SIZE, shuffle_share_three);

    if (pid != D) {
        std::cout << std::endl << "Result of third shuffle: ";
        for (int i = 0; i < third_shuffle.size() - 1; ++i) {
            std::cout << third_shuffle[i] << ", ";
        }
        std::cout << third_shuffle[third_shuffle.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < third_shuffle.size(); ++i) {
            if (third_shuffle[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
    }

    auto merged_shuffle_two = share::reveal_vec(party, network, BLOCK_SIZE, merged_share_two);

    if (pid != D) {
        std::cout << std::endl << "Result of second merged shuffle: ";
        for (int i = 0; i < merged_shuffle_two.size() - 1; ++i) {
            std::cout << merged_shuffle_two[i] << ", ";
        }
        std::cout << merged_shuffle_two[merged_shuffle_two.size() - 1] << std::endl;
        std::cout << std::endl << std::endl;

        bool shuffled = false;
        for (size_t i = 0; i < merged_shuffle_two.size(); ++i) {
            if (merged_shuffle_two[i] != i) shuffled = shuffled || true;
        }
        assert(shuffled);
    }
    exit(0);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Benchmark a simple test for shuffling and unshuffling");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        test_shuffle(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}