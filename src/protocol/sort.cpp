#include "sort.h"

/* ----- Preprocessing ----- */
SortIterationPreprocessing sort::sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n) {
    SortIterationPreprocessing preproc;

    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, true);
    auto triples = compaction::preprocess(id, rngs, network, n);
    std::vector<Ring> unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, perm_share);

    preproc.perm_share = perm_share;
    preproc.triples = triples;
    preproc.unshuffle_B = unshuffle_B;

    return preproc;
}

SortPreprocessing sort::get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits) {
    SortPreprocessing preproc;

    /* Get one compaction preprocessing */
    preproc.triples = compaction::preprocess(id, rngs, network, n);

    /* Compute the preprocessing for n_bits-1 sort_iteratinos */
    for (size_t i = 0; i < n_bits - 1; ++i) {
        SortIterationPreprocessing sort_iteration_pre = sort_iteration_preprocess(id, rngs, network, n);
        preproc.sort_iteration_pre.push_back(sort_iteration_pre);
    }

    return preproc;
}

/* ----- Evaluation ----- */
Permutation sort::sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                          std::vector<Ring> &bit_shares, SortIterationPreprocessing &preproc) {
    if (id == D) return Permutation(n);
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, preproc.perm_share, n);

    Permutation perm_open = share::reveal_perm(id, network, perm_shuffled);

    std::vector<Ring> input_shuffled = shuffle::shuffle(id, rngs, network, bit_shares, preproc.perm_share, n);
    std::vector<Ring> input_sorted = perm_open(input_shuffled);

    auto triples = preproc.triples;
    Permutation sigma_p = compaction::evaluate(id, rngs, network, n, triples, input_sorted);

    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    auto unshuffle_B = preproc.unshuffle_B;
    Permutation next_perm_shuffled = Permutation(shuffle::unshuffle(id, rngs, network, preproc.perm_share, unshuffle_B, next_perm, n));

    return next_perm_shuffled;
}

Permutation sort::get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n,
                                    std::vector<std::vector<Ring>> &bit_shares, SortPreprocessing &preproc) {
    if (id == D) return Permutation(n);
    Permutation sigma = compaction::evaluate(id, rngs, network, n, preproc.triples, bit_shares[0]);

    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        auto sort_iteration_pre = preproc.sort_iteration_pre[i - 1];
        sigma = sort_iteration_evaluate(id, rngs, network, n, sigma, bit_shares[i], sort_iteration_pre);
    }
    return sigma;
}

/* ----- Ad-Hoc Preprocessing ----- */
/**
 * ----- F_get_sort -----
 */
Permutation sort::get_sort(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::vector<Ring>> &bit_shares) {
    /* Compute compaction of x_0 */
    Permutation sigma = compaction::get_compaction(id, rngs, network, n, bit_shares[0]);
    /* Proceed sorting with x_1, x_2, ... */
    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        sigma = sort_iteration(id, rngs, network, n, sigma, bit_shares[i]);
    }
    return sigma;
}

/**
 * ----- F_sort_iteration -----
 */
Permutation sort::sort_iteration(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                 std::vector<Ring> &bit_vec_share) {
    /* Shuffle perm */
    ShufflePre perm_share = shuffle::get_shuffle(id, rngs, network, n, true);
    network->send_all();
    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, perm, perm_share, n);

    /* Reveal shuffled perm */
    Permutation perm_open = share::reveal_perm(id, network, perm_shuffled);

    /* Shuffle input using the previous order */
    std::vector<Ring> input_shuffled = shuffle::shuffle(id, rngs, network, bit_vec_share, perm_share, n);

    /* Apply revealed permutation to shuffled input */
    std::vector<Ring> sorted_share = perm_open(input_shuffled);

    /* Compute compaction to stable sort bit_vec_share */
    Permutation sigma_p = compaction::get_compaction(id, rngs, network, n, sorted_share);
    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    /* Unshuffle next */
    std::vector<Ring> B = shuffle::get_unshuffle(id, rngs, network, n, perm_share);
    network->send_all();
    return Permutation(shuffle::unshuffle(id, rngs, network, perm_share, B, next_perm, n));
}
