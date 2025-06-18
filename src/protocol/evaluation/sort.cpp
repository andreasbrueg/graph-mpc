#include "../sort.h"

Permutation sort::get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                    std::vector<std::vector<Ring>> &bit_shares, SortPreprocessing &preproc) {
    auto triples = preproc.triples;
    Permutation sigma = compaction::evaluate(id, rngs, network, n, BLOCK_SIZE, triples, bit_shares[0]);

    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        auto sort_iteration_pre = preproc.sort_iteration_pre[i - 1];
        sigma = sort_iteration_evaluate(id, rngs, network, n, BLOCK_SIZE, sigma, bit_shares[i], sort_iteration_pre);
    }
    return sigma;
}

Permutation sort::sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                          Permutation &perm, std::vector<Ring> &bit_shares, SortIterationPreprocessing &preproc) {
    ShufflePre perm_share = preproc.perm_share;
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n, BLOCK_SIZE);

    Permutation perm_open = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);

    std::vector<Ring> input_shuffled = shuffle::shuffle(id, rngs, network, bit_shares, perm_share, n, BLOCK_SIZE);

    std::vector<Ring> input_sorted = perm_open(input_shuffled);

    auto triples = preproc.triples;
    Permutation sigma_p = compaction::evaluate(id, rngs, network, n, BLOCK_SIZE, triples, input_sorted);

    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    auto unshuffle_B = preproc.unshuffle_B;
    Permutation next_perm_shuffled = Permutation(shuffle::unshuffle(id, rngs, network, perm_share, unshuffle_B, next_perm, n, BLOCK_SIZE));

    return next_perm_shuffled;
}

std::tuple<std::vector<Ring>, std::vector<Ring>> sort_iteration_evaluate_1(Party id, RandomGenerators &rngs, Permutation &perm,
                                                                           std::vector<Ring> &bit_vec_share, SortIterationPreprocessing &preproc, size_t n) {
    ShufflePre perm_share = preproc.perm_share;
    std::vector<Ring> vec_A_1 = shuffle::shuffle_1(id, rngs, n, perm, perm_share);
    std::vector<Ring> vec_A_2 = shuffle::shuffle_1(id, rngs, n, bit_vec_share, perm_share);
    return {vec_A_1, vec_A_2};
}

std::vector<Ring> sort::apply_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                            Permutation &perm, ShufflePre &perm_share, std::vector<Ring> &input_share) {
    auto perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n, BLOCK_SIZE);
    auto perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);
    auto shuffled_input_share = shuffle::shuffle(id, rngs, network, input_share, perm_share, n, BLOCK_SIZE);
    return perm_opened(shuffled_input_share);
}

std::vector<Ring> sort::reverse_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                              Permutation &perm, ShufflePre &pi, std::vector<Ring> &unshuffle_B, std::vector<Ring> &input_share) {
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, pi, n, BLOCK_SIZE);
    Permutation perm_opened = share::reveal_perm(id, network, BLOCK_SIZE, perm_shuffled);
    auto permuted_share = perm_opened.inverse()(input_share);

    auto result = shuffle::unshuffle(id, rngs, network, pi, unshuffle_B, permuted_share, n, BLOCK_SIZE);
    return result;
}

std::vector<Ring> sort::switch_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                             Permutation &p1, Permutation &p2, ShufflePre &pi, ShufflePre &omega, ShufflePre &merged,
                                             std::vector<Ring> &input_share) {
    auto shuffled_p1 = shuffle::shuffle(id, rngs, network, p1, pi, n, BLOCK_SIZE);
    auto shuffled_p2 = shuffle::shuffle(id, rngs, network, p2, omega, n, BLOCK_SIZE);

    auto revealed_p1 = share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p1);
    auto revealed_p2 = share::reveal_perm(id, network, BLOCK_SIZE, shuffled_p2);

    auto permuted_input = revealed_p1.inverse()(input_share);
    auto permuted_input_shuffled = shuffle::shuffle(id, rngs, network, permuted_input, merged, n, BLOCK_SIZE);
    auto switched = revealed_p2(permuted_input_shuffled);

    return switched;
}
