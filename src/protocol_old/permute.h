#pragma once

#include "deduplication.h"
#include "io/comm.h"
#include "message_passing.h"
#include "shuffle.h"
#include "sort.h"
#include "utils/preprocessings.h"
#include "utils/sharing.h"
#include "utils/types.h"

namespace permute {

/**
 * ----- F_apply_perm -----
 */
std::vector<Ring> apply_perm(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                             std::vector<Ring> &input_share);

std::vector<Ring> apply_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                      ShufflePre &perm_share, std::vector<Ring> &input_share);

std::vector<Ring> apply_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                      MPPreprocessing &preproc, std::vector<Ring> &input_share);

/**
 * ----- F_reverse_perm -----
 */
std::vector<Ring> reverse_perm(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                               std::vector<Ring> &input_share);

std::vector<Ring> reverse_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                        ShufflePre &pi, std::vector<Ring> &unshuffle_B, std::vector<Ring> &input_share);

/**
 * ----- F_sw_perm -----
 */
std::vector<Ring> switch_perm(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &p1, Permutation &p2,
                              std::vector<Ring> &input_share);

void switch_perm_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, MPPreprocessing &preproc);

std::vector<Ring> switch_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &p1, Permutation &p2,
                                       MPPreprocessing &preproc, std::vector<Ring> &input_share);

};  // namespace permute