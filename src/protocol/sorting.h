#pragma once

#include "../utils/perm.h"
#include "../utils/protocol_config.h"
#include "../utils/sharing.h"
#include "compaction.h"
#include "shuffle.h"

struct SortPreprocessing {
    std::vector<std::tuple<std::vector<Ring>, std::vector<Ring>, std::vector<Ring>>> comp_triples;
    std::vector<PermShare> perm_share_vec;
    std::vector<std::vector<Ring>> unshuffle_B_vec;
};

namespace sort {

/**
 * Runs the necessary preprocessing to compute a sort later on
 */
SortPreprocessing get_sort_preprocess(ProtocolConfig &c, size_t n_bits);

/**
 * Runs one sort_iteration using already computed preprocessing
 */
Permutation sort_iteration_evaluate(ProtocolConfig &c, Permutation &perm, std::vector<Ring> &bit_shares, SortPreprocessing &preproc);

/**
 * Runs the necessary preprocessing to compute a sort later on
 */
Permutation get_sort_evaluate(ProtocolConfig &c, std::vector<std::vector<Ring>> &bit_shares, SortPreprocessing &preproc);

/**
 * Runs one sort_iteration using ad-hoc preprocessing
 */
Permutation sort_iteration(ProtocolConfig &c, Permutation &perm, std::vector<Ring> &bit_vec_share);

/**
 * Interactively computes the sorting of bit-shares using ad-hoc preprocessing
 */
Permutation get_sort(ProtocolConfig &c, std::vector<std::vector<Ring>> &bit_shares);

/**
 * Evaluates one apply_perm using a preprocessed perm_share
 */
std::vector<Ring> apply_perm_evaluate(ProtocolConfig &c, Permutation &perm, PermShare &perm_share, std::vector<Ring> &input_share);

/**
 * Interactively applies a secret-shared permutation on a secret shared input using ad-hoc preprocessing
 */
std::vector<Ring> apply_perm(ProtocolConfig &c, Permutation &perm, std::vector<Ring> &input_share);

/**
 * Runs preprocessing for evaluating one switch_perm later on
 * Returns {pi, omega, merged}
 */
std::tuple<PermShare, PermShare, PermShare> switch_perm_preprocess(ProtocolConfig &c);

/**
 * Runs preprocessing for evaluating one switch_perm later on
 * Returns {pi, omega, merged}
 */
std::vector<Ring> switch_perm_evaluate(ProtocolConfig &c, Permutation &p1, Permutation &p2, PermShare &pi, PermShare &omega, PermShare &merged,
                                       std::vector<Ring> &input_share);

/**
 * Interactively computes p2(p1.inv) using ad-hoc preprocessing
 */
std::vector<Ring> switch_perm(ProtocolConfig &c, Permutation &p1, Permutation &p2, std::vector<Ring> &input_share);

};  // namespace sort