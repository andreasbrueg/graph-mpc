#include "../message_passing.h"

MPPreprocessing_Dealer mp::preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n, size_t n_iterations) {
    MPPreprocessing_Dealer preproc;
    size_t n_bits = sizeof(Ring) * 8;

    preproc.src_order_pre = sort::get_sort_preprocess_Dealer(id, rngs, n, n_bits + 1);
    preproc.dst_order_pre = sort::get_sort_preprocess_Dealer(id, rngs, n, n_bits + 1);
    preproc.vtx_order_pre = sort::sort_iteration_preprocess_Dealer(id, rngs, n);

    auto [apply_perm_share, apply_B_0, apply_B_1] = shuffle::get_shuffle_1(id, rngs, n);
    preproc.apply_perm_pre = {apply_B_0, apply_B_1};

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Four sw_perm preprocessing */
        for (size_t j = 0; j < 4; ++j) preproc.sw_perm_pre.push_back(sort::switch_perm_preprocess_Dealer(id, rngs, n));
    }
    return preproc;
}

MPPreprocessing mp::preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, size_t n_iterations, std::vector<Ring> &vals, size_t &idx) {
    MPPreprocessing preproc;
    size_t n_bits = sizeof(Ring) * 8;

    preproc.src_order_pre = sort::get_sort_preprocess_Parties(id, rngs, n, n_bits + 1, vals, idx);
    preproc.dst_order_pre = sort::get_sort_preprocess_Parties(id, rngs, n, n_bits + 1, vals, idx);
    preproc.vtx_order_pre = sort::sort_iteration_preprocess_Parties(id, rngs, n, vals, idx);

    ShufflePre apply_perm_share = shuffle::get_shuffle_2(id, rngs, n, vals, idx, true);
    preproc.apply_perm_pre = apply_perm_share;

    for (size_t i = 0; i < n_iterations; ++i) {
        /* Four sw_perm preprocessing */
        for (size_t j = 0; j < 4; ++j) preproc.sw_perm_pre.push_back(sort::switch_perm_preprocess_Parties(id, rngs, n, vals, idx));
    }
    return preproc;
}

MPPreprocessing mp::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE, size_t n_iterations) {
    MPPreprocessing preproc;
    size_t n_bits = sizeof(Ring) * 8;

    preproc.src_order_pre = sort::get_sort_preprocess(id, rngs, network, n, BLOCK_SIZE, n_bits + 1);
    preproc.dst_order_pre = sort::get_sort_preprocess(id, rngs, network, n, BLOCK_SIZE, n_bits + 1);
    preproc.vtx_order_pre = sort::sort_iteration_preprocess(id, rngs, network, n, BLOCK_SIZE);

    ShufflePre apply_perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    preproc.apply_perm_pre = apply_perm_share;

    for (size_t i = 0; i < n_iterations; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            preproc.sw_perm_pre.push_back(sort::switch_perm_preprocess(id, rngs, network, n, BLOCK_SIZE));
        }
    }

    preproc.eqz_triples = clip::equals_zero_preprocess(id, rngs, network, n, BLOCK_SIZE);
    preproc.B2A_triples = clip::B2A_preprocess(id, rngs, network, n, BLOCK_SIZE);

    return preproc;
}
