#pragma once

#include <omp.h>

#include "../utils/perm.h"
#include "../utils/random_generators.h"
#include "../utils/types.h"
#include "io/comm.h"
#include "io/network_interface.h"

/**
 * Contains the preprocessing of a secure shuffle
 * After preprocessing, B is set and pi_1_p for P1
 * The size after preprocessing is therefore: n / 2n for P0 / P1
 * If save is set to true, then pi_0, pi_0_p resp. pi_1 are set for P0 resp. P1 and also R for both parties (for repeating the same shuffle later)
 */
struct ShufflePre {
    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Ring> B;
    std::vector<Ring> R;
    bool save;
};

namespace shuffle {

ShufflePre get_shuffle_compute(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &shared_secret_D0, std::vector<Ring> &shared_secret_D1, bool save);

ShufflePre get_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, bool save);

std::vector<Ring> get_unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, ShufflePre &perm_share);

ShufflePre get_merged_shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, ShufflePre &pi_share,
                              ShufflePre &omega_share);

std::vector<Ring> shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, std::vector<Ring> &input_share, ShufflePre &perm_share,
                          size_t n);

Permutation shuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, Permutation &input_share, ShufflePre &perm_share, size_t n);

std::vector<Ring> unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, ShufflePre &shuffle_share, std::vector<Ring> &B,
                            std::vector<Ring> &input_share, size_t n);

Permutation unshuffle(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, ShufflePre &shuffle_share, std::vector<Ring> &B,
                      Permutation &input_share);

};  // namespace shuffle