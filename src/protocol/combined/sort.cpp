#include "../sort.h"

/**
 * ----- F_get_sort -----
 */
Permutation sort::get_sort(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                           std::vector<std::vector<Ring>> &bit_shares) {
    /* Compute compaction of x_0 */
    Permutation sigma = compaction::get_compaction(id, rngs, network, n, BLOCK_SIZE, bit_shares[0]);
    /* Proceed sorting with x_1, x_2, ... */
    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        sigma = sort_iteration(id, rngs, network, n, BLOCK_SIZE, sigma, bit_shares[i]);
    }
    return sigma;
}

/**
 * ----- F_sort_iteration -----
 */
Permutation sort::sort_iteration(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                 std::vector<Ring> &bit_vec_share) {
    /* Shuffle perm */
    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n, BLOCK_SIZE);

    /* Reveal shuffled perm */
    Permutation perm_open = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);

    /* Shuffle input using the previous order */
    std::vector<Ring> input_shuffled = shuffle::shuffle(id, rngs, network, bit_vec_share, perm_share, n, BLOCK_SIZE);

    /* Apply revealed permutation to shuffled input */
    std::vector<Ring> sorted_share = perm_open(input_shuffled);

    /* Compute compaction to stable sort bit_vec_share */
    Permutation sigma_p = compaction::get_compaction(id, rngs, network, n, BLOCK_SIZE, sorted_share);
    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    /* Unshuffle next */
    std::vector<Ring> B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, perm_share);
    return Permutation(shuffle::unshuffle(id, rngs, network, perm_share, B, next_perm, n, BLOCK_SIZE));
}

/**
 * ----- F_apply_perm -----
 */
std::vector<Ring> sort::apply_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                   std::vector<Ring> &input_share) {
    /* Shuffle permutation and open it */
    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    auto perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n, BLOCK_SIZE);
    auto perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);

    /* Shuffle input_share with same perm */
    auto shuffled_input_share = shuffle::shuffle(id, rngs, network, input_share, perm_share, n, BLOCK_SIZE);

    /* Apply opened perm to input_share */
    return perm_opened(shuffled_input_share);
}

/**
 * ----- F_reverse_perm -----
 */
std::vector<Ring> sort::reverse_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &perm,
                                     std::vector<Ring> &input_share) {
    ShufflePre pi = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    auto unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, pi);

    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, pi, n, BLOCK_SIZE);
    Permutation perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);
    auto permuted_share = perm_opened.inverse()(input_share);

    auto result = shuffle::unshuffle(id, rngs, network, pi, unshuffle_B, permuted_share, n, BLOCK_SIZE);
    return result;
}

/**
 * ----- F_sw_perm -----
 */
std::vector<Ring> sort::switch_perm(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, Permutation &p1,
                                    Permutation &p2, std::vector<Ring> &input_share) {
    ShufflePre pi = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    ShufflePre omega = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);

    auto shuffled_p1 = shuffle::shuffle(id, rngs, network, p1, pi, n, BLOCK_SIZE);
    auto shuffled_p2 = shuffle::shuffle(id, rngs, network, p2, omega, n, BLOCK_SIZE);

    auto revealed_p1 = Permutation(share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p1)); /* pi(p1.get_perm_vec()) == (p1 * pi.inv) */
    auto revealed_p2 = Permutation(share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p2)); /* omega(p2.get_perm_vec()) == p2 * omega.inv */

    auto merged = shuffle::get_merged_shuffle(id, rngs, network, n, BLOCK_SIZE, pi, omega); /* omega * pi.inv */

    auto permuted_input = revealed_p1.inverse()(input_share);
    auto permuted_input_shuffled =
        shuffle::shuffle(id, rngs, network, permuted_input, merged, n, BLOCK_SIZE); /* (omega * pi.inv * pi * p1.inv)(input) == (omega * p1.inv)(input) */
    auto switched = revealed_p2(permuted_input_shuffled);                           /* (p2 * omega.inv * omega * p1.inv)(input) == (p2 * p1.inv)(input) */

    return switched;
}
