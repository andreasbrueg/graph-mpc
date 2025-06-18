#include "../sort.h"

SortPreprocessing sort::get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                            size_t n_bits) {
    SortPreprocessing preproc;

    /* Get one compaction preprocessing */
    preproc.triples = compaction::preprocess(id, rngs, network, n, BLOCK_SIZE);

    /* Compute the preprocessing for n_bits-1 sort_iteratinos */
    for (size_t i = 0; i < n_bits - 1; ++i) {
        SortIterationPreprocessing sort_iteration_pre;

        ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
        std::vector<Ring> unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, perm_share);
        auto triples = compaction::preprocess(id, rngs, network, n, BLOCK_SIZE);

        sort_iteration_pre.perm_share = perm_share;
        sort_iteration_pre.unshuffle_B = unshuffle_B;
        sort_iteration_pre.triples = triples;

        preproc.sort_iteration_pre.push_back(sort_iteration_pre);
    }

    return preproc;
}

SortPreprocessing_Dealer sort::get_sort_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n, size_t n_bits) {
    SortPreprocessing_Dealer preproc;

    preproc.comp_shares_P1 = compaction::preprocess_Dealer(id, rngs, n);

    for (size_t i = 0; i < n_bits - 1; ++i) {
        SortIterationPreprocessing_Dealer sort_iteration_pre;
        auto [perm_share, B0, shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
        sort_iteration_pre.B0 = B0;
        sort_iteration_pre.shares_P1 = shares_P1;

        auto [unshuffle_B0, unshuffle_B1] = shuffle::get_unshuffle_1(id, rngs, n, perm_share);
        sort_iteration_pre.unshuffle_B0 = unshuffle_B0;
        sort_iteration_pre.unshuffle_B1 = unshuffle_B1;

        auto comp_shares_P1 = compaction::preprocess_Dealer(id, rngs, n);
        sort_iteration_pre.comp_vals_P1 = comp_shares_P1;

        preproc.sort_iteration_pre.push_back(sort_iteration_pre);
    }
    return preproc;
}

SortPreprocessing sort::get_sort_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, size_t n_bits, std::vector<Ring> &vals, size_t &idx) {
    SortPreprocessing preproc;

    auto triples = compaction::preprocess_Parties(id, rngs, n, vals, idx);
    preproc.triples = triples;

    for (size_t i = 0; i < n_bits - 1; ++i) {
        SortIterationPreprocessing sort_iteration_pre;

        ShufflePre shuffle_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
        auto triple = compaction::preprocess_Parties(id, rngs, n, vals, idx);
        std::vector<Ring> unshuffle_B = shuffle::get_unshuffle_2(id, n, vals, idx);

        sort_iteration_pre.perm_share = shuffle_share;
        sort_iteration_pre.triples = triple;
        sort_iteration_pre.unshuffle_B = unshuffle_B;
        preproc.sort_iteration_pre.push_back(sort_iteration_pre);
    }

    return preproc;
}

SortIterationPreprocessing sort::sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n,
                                                           size_t BLOCK_SIZE) {
    SortIterationPreprocessing preproc;

    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    auto triples = compaction::preprocess(id, rngs, network, n, BLOCK_SIZE);
    std::vector<Ring> unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, perm_share);

    preproc.perm_share = perm_share;
    preproc.triples = triples;
    preproc.unshuffle_B = unshuffle_B;

    return preproc;
}

SortIterationPreprocessing_Dealer sort::sort_iteration_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    SortIterationPreprocessing_Dealer preproc;
    auto [perm_share, B0, shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
    auto comp_vals_P1 = compaction::preprocess_Dealer(id, rngs, n);
    auto [unshuffle_B0, unshuffle_B1] = shuffle::get_unshuffle_1(id, rngs, n, perm_share);
    preproc.B0 = B0;
    preproc.shares_P1 = shares_P1;
    preproc.comp_vals_P1 = comp_vals_P1;
    preproc.unshuffle_B0 = unshuffle_B0;
    preproc.unshuffle_B1 = unshuffle_B1;
    return preproc;
}

SortIterationPreprocessing sort::sort_iteration_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx) {
    SortIterationPreprocessing preproc;
    preproc.perm_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    preproc.triples = compaction::preprocess_Parties(id, rngs, n, vals, idx);
    preproc.unshuffle_B = shuffle::get_unshuffle_2(id, n, vals, idx);
    return preproc;
}

SwitchPermPreprocessing sort::switch_perm_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    SwitchPermPreprocessing preproc;
    ShufflePre pi = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    ShufflePre omega = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    ShufflePre merged = shuffle::get_merged_shuffle(id, rngs, network, n, BLOCK_SIZE, pi, omega);

    preproc.pi_share = pi;
    preproc.omega_share = omega;
    preproc.merged_share = merged;

    return preproc;
}

SwitchPermPreprocessing_Dealer sort::switch_perm_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    SwitchPermPreprocessing_Dealer preproc;

    auto [pi_share, pi_B_0, pi_shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
    auto [omega_share, omega_B_0, omega_shares_P1] = shuffle::get_shuffle_1(id, rngs, n);
    auto [merged_share, merged_B_0, merged_B_1, sigma_0_p, sigma_1] = shuffle::get_merged_shuffle_1(id, rngs, n, pi_share, omega_share);

    preproc.pi_B_0 = pi_B_0;
    preproc.pi_shares_P1 = pi_shares_P1;
    preproc.omega_B_0 = omega_B_0;
    preproc.omega_shares_P1 = omega_shares_P1;
    preproc.merged_B_0 = merged_B_0;
    preproc.merged_B_1 = merged_B_1;
    preproc.sigma_0_p = sigma_0_p;
    preproc.sigma_1 = sigma_1;

    return preproc;
}

SwitchPermPreprocessing sort::switch_perm_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx) {
    SwitchPermPreprocessing preproc;

    ShufflePre pi_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    ShufflePre omega_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    ShufflePre merged_share = shuffle::get_merged_shuffle_2(id, n, vals, idx);

    preproc.pi_share = pi_share;
    preproc.omega_share = omega_share;
    preproc.merged_share = merged_share;
    return preproc;
}
