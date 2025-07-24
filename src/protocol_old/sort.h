#pragma once

#include "../utils/perm.h"
#include "../utils/random_generators.h"
#include "../utils/sharing.h"
#include "compaction.h"
#include "shuffle.h"
#include "utils/preprocessings.h"

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