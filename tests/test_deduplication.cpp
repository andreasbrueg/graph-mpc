#include <cassert>

#include "../setup/setup.h"
#include "../src/protocol/clip.h"
#include "../src/protocol/deduplication.h"
#include "../src/utils/random_generators.h"
#include "../src/utils/sharing.h"
#include "constants.h"

void test_deduplication(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    std::cout << "------ test_deduplication ------" << std::endl << std::endl;
    json output_data;
    size_t n_bits = sizeof(Ring) * 8;

    std::vector<Ring> src(n);
    std::vector<Ring> dst(n);

    std::vector<std::vector<Ring>> src_bits(n_bits);
    std::vector<std::vector<Ring>> dst_bits(n_bits);
    std::vector<std::vector<Ring>> src_bits_share(n_bits);
    std::vector<std::vector<Ring>> dst_bits_share(n_bits);

    /* First half: Random values */
    for (size_t i = 0; i < n / 2; ++i) {
        src[i] = rand() % n;
        dst[i] = rand() % n;
    }

    /* Second half: duplicates*/
    for (size_t i = n / 2; i < n; ++i) {
        src[i] = 0;
        dst[i] = 1;
    }

    auto src_share = share::random_share_secret_vec_2P(id, rngs, src);
    auto dst_share = share::random_share_secret_vec_2P(id, rngs, dst);

    for (size_t i = 0; i < n_bits; ++i) {
        src_bits[i].resize(n);
        dst_bits[i].resize(n);
        for (size_t j = 0; j < n; ++j) {
            src_bits[i][j] = (src[j] & (1 << i)) >> i;
            dst_bits[i][j] = (dst[j] & (1 << i)) >> i;
        }
    }

    for (size_t i = 0; i < n_bits; ++i) {
        src_bits_share[i] = share::random_share_secret_vec_2P(id, rngs, src_bits[i]);
        dst_bits_share[i] = share::random_share_secret_vec_2P(id, rngs, dst_bits[i]);
    }

    /* Preprocessing */
    StatsPoint start_pre(*network);
    auto preproc = deduplication_preprocess(id, rngs, network, n, BLOCK_SIZE);
    StatsPoint end_pre(*network);

    auto rbench_pre = end_pre - start_pre;
    output_data["benchmarks_pre"].push_back(rbench_pre);
    size_t bytes_sent_pre = 0;
    for (const auto &val : rbench_pre["communication"]) {
        bytes_sent_pre += val.get<int64_t>();
    }

    /* Preprocessing communication assertions */
    if (id == D) {
        /* n_elems * 4 Bytes per element */
        size_t total_comm =
            4 * (sort_comm_pre(n, 2 * n_bits) + apply_perm_comm_pre(n) + 2 * eqz_comm_pre(n) + mul_comm_pre(n) + B2A_comm_pre(n) + unshuffle_comm_pre(n));
        assert(bytes_sent_pre == total_comm);
    }

    StatsPoint start_online(*network);
    deduplication_evaluate(id, rngs, network, n, BLOCK_SIZE, preproc, src_bits_share, dst_bits_share, src_share, dst_share);
    StatsPoint end_online(*network);

    auto rbench = end_online - start_online;
    output_data["benchmarks"].push_back(rbench);

    size_t bytes_sent = 0;
    for (const auto &val : rbench["communication"]) {
        bytes_sent += val.get<int64_t>();
    }

    /* Evaluation communication assertions */
    if (id != D) {
        size_t total_comm = 4 * (sort_comm_online(n, 2 * n_bits) + 2 * apply_perm_comm_online(n) + 2 * eqz_comm_online(n) + mul_comm_online(n) +
                                 B2A_comm_online(n) + reverse_perm_comm_online(n));
        assert(total_comm == bytes_sent);
    }

    auto MSB_src_rev = share::reveal_vec(id, network, BLOCK_SIZE, src_bits_share[n_bits]);
    auto MSB_dst_rev = share::reveal_vec(id, network, BLOCK_SIZE, dst_bits_share[n_bits]);

    /* Assertions for correctnes */
    if (id != D) {
        for (size_t i = n / 2 + 1; i < n; ++i) {
            assert(MSB_src_rev[i] == 1);
            assert(MSB_dst_rev[i] == 1);
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
        setup::run_test(opts, test_deduplication);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}