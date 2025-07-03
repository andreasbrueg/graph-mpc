#pragma once

#include "io/comm.h"
#include "shuffle.h"
#include "utils/sharing.h"
#include "utils/types.h"

struct SwitchPermPreprocessing {
    ShufflePre pi;         // size: n / 2n
    ShufflePre omega;      // size: n / 2n
    ShufflePre merged;     // size: n / 2n
    Permutation pi_p1;     // size: n
    Permutation omega_p2;  // size: n
};

namespace permute {

/**
 * ----- F_apply_perm -----
 */
std::vector<Ring> apply_perm(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                             std::vector<Ring> &input_share);

std::vector<Ring> apply_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &perm,
                                      ShufflePre &perm_share, std::vector<Ring> &input_share);

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

SwitchPermPreprocessing switch_perm_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n);

std::vector<Ring> switch_perm_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, Permutation &p1, Permutation &p2,
                                       SwitchPermPreprocessing &preproc, std::vector<Ring> &input_share);

};  // namespace permute