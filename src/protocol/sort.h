#pragma once

#include "../utils/perm.h"
#include "../utils/random_generators.h"
#include "../utils/sharing.h"
#include "compaction.h"
#include "shuffle.h"

/**
 * Preprocessing for performing one sort iteration in the online phase
 * Total size after preprocessing: 5n / 6n for P0 / P1
 */
struct SortIterationPreprocessing {
    ShufflePre perm_share;                              // size: n/2n for P0/P1
    std::vector<std::tuple<Ring, Ring, Ring>> triples;  // size: 3n
    std::vector<Ring> unshuffle_B;                      // size: n
};

/**
 * Preprocessing for performing one sort in the online phase
 * Total size: 3n + (n_bits-1) * (5n resp. 6n)
 */
struct SortPreprocessing {
    std::vector<std::tuple<Ring, Ring, Ring>> triples;           // size: 3n
    std::vector<SortIterationPreprocessing> sort_iteration_pre;  // size: (n_bits-1) * (5n resp. 6n)
};

namespace sort {

/**
 * ----- F_get_sort -----
 */

Permutation get_sort(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::vector<Ring>> &bit_shares);

SortPreprocessing get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits);

Permutation get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::vector<Ring>> &bit_shares,
                              SortPreprocessing &preproc);

/**
 * ----- F_sort_iteration -----
 */
Permutation sort_iteration(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                           std::vector<Ring> &bit_vec_share);

SortIterationPreprocessing sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n);

Permutation sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                    std::vector<Ring> &bit_shares, SortIterationPreprocessing &preproc);

};  // namespace sort