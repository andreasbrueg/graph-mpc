#pragma once

#include <tuple>
#include <vector>

#include "perm.h"
#include "types.h"

struct ShufflePre {
    Permutation pi_0;
    Permutation pi_1;
    Permutation pi_0_p;
    Permutation pi_1_p;
    std::vector<Ring> B;
    std::vector<Ring> R;
    bool save;
};

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
    std::vector<std::tuple<Ring, Ring, Ring>> triples;           // size: n_bits * 3n
    std::vector<SortIterationPreprocessing> sort_iteration_pre;  // size: (n_bits-1) * (5n resp. 6n)
};

struct DeduplicationPreprocessing {
    SortPreprocessing dst_sort_pre;
    std::vector<SortIterationPreprocessing> src_sort_pre;
    ShufflePre apply_perm_share;
    std::vector<Ring> unshuffle_B;
    std::vector<std::vector<std::tuple<Ring, Ring, Ring>>> eqz_triples_1;
    std::vector<std::vector<std::tuple<Ring, Ring, Ring>>> eqz_triples_2;
    std::vector<std::tuple<Ring, Ring, Ring>> mul_triples;
    std::vector<std::tuple<Ring, Ring, Ring>> B2A_triples;
    Permutation dst_sort;
    bool is_null() { return unshuffle_B.size() == 0; }
};

enum Switch { vtx_src_1, vtx_src_2, src_dst, dst_vtx };

struct MPPreprocessing {
    SortPreprocessing src_order_pre;  // size: (3 + (5 resp. 6) * (n_bits-1)) * n
    SortPreprocessing dst_order_pre;  // size: (3 + (5 resp. 6) * (n_bits-1)) * n
    SortIterationPreprocessing dst_order_sort_iteration_pre;
    SortIterationPreprocessing vtx_order_pre;  // size: 5n resp. 6n
    Permutation src_order;
    Permutation dst_order;
    Permutation vtx_order;
    ShufflePre vtx_order_shuffle;  // size: n resp. 2n
    ShufflePre src_order_shuffle;
    ShufflePre dst_order_shuffle;
    ShufflePre vtx_src_merge;
    ShufflePre src_dst_merge;
    ShufflePre dst_vtx_merge;
    Permutation clear_shuffled_vtx_order;
    Permutation clear_shuffled_src_order;
    Permutation clear_shuffled_dst_order;
    std::vector<std::vector<std::tuple<Ring, Ring, Ring>>> eqz_triples;  // size: 3n per eqz
    std::vector<std::tuple<Ring, Ring, Ring>> B2A_triples;
    DeduplicationPreprocessing deduplication_pre;
    Switch next_switch = vtx_src_1;  // First switch perm is from vtx to src order
};
