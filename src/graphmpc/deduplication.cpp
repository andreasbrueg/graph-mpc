#include "deduplication.h"

/* ----- Preprocessing ----- */
void deduplication_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                              Party &recv_shuffle, Party &recv_mul, bool save_to_disk) {
    sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1, preproc, recv_shuffle, recv_mul, save_to_disk);

    for (size_t i = 0; i < n_bits; ++i) {
        sort::sort_iteration_preprocess(id, rngs, network, n, preproc, recv_shuffle, recv_mul, save_to_disk);
    }

    shuffle::get_shuffle(id, rngs, network, n, preproc, recv_shuffle, save_to_disk);
    shuffle::get_unshuffle(id, rngs, network, n, preproc, save_to_disk);

    clip::equals_zero_preprocess(id, rngs, network, n - 1, preproc, recv_mul, save_to_disk);  // src eqz
    clip::equals_zero_preprocess(id, rngs, network, n - 1, preproc, recv_mul, save_to_disk);  // dst eqz
    mul::preprocess(id, rngs, network, n - 1, preproc, recv_mul, false, save_to_disk);        // Multiplication
    mul::preprocess(id, rngs, network, n - 1, preproc, recv_mul, false, save_to_disk);        // B2A

    preproc.deduplication = true;
}

/* ----- Evaluation ----- */
void deduplication_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc, Graph &g,
                            bool save_to_disk) {
    Permutation perm = sort::get_sort_evaluate(id, rngs, network, n, preproc, g.dst_order_bits, save_to_disk);
    preproc.dst_order = perm;

    for (size_t i = 0; i < g.src_bits().size(); ++i) {
        perm = sort::sort_iteration_evaluate(id, rngs, network, n, perm, preproc, g.src_bits()[i], save_to_disk);
    }

    Permutation src_dst_sort = perm;

    auto src = g.src();
    auto dst = g.dst();
    auto perm_shuffled = shuffle::shuffle(id, rngs, network, n, preproc, src_dst_sort, save_to_disk, true);
    auto perm_opened = share::reveal_perm(id, network, perm_shuffled);
    auto shuffled_src_share = shuffle::shuffle_repeat(id, rngs, network, n, preproc, src, save_to_disk);
    auto shuffled_dst_share = shuffle::shuffle_repeat(id, rngs, network, n, preproc, dst, save_to_disk);

    auto src_p = perm_opened(shuffled_src_share);
    auto dst_p = perm_opened(shuffled_dst_share);

    std::vector<Ring> src_dupl(n - 1);
    std::vector<Ring> dst_dupl(n - 1);

    for (size_t i = 1; i < n; ++i) {
        src_dupl[i - 1] = src_p[i] - src_p[i - 1];
        dst_dupl[i - 1] = dst_p[i] - dst_p[i - 1];
    }

    auto src_eqz = clip::equals_zero_evaluate(id, rngs, network, preproc, src_dupl, save_to_disk);
    auto dst_eqz = clip::equals_zero_evaluate(id, rngs, network, preproc, dst_dupl, save_to_disk);
    auto src_and_dst = mul::evaluate(id, network, n - 1, preproc, src_eqz, dst_eqz, true, save_to_disk);
    auto duplicates = clip::B2A_evaluate(id, rngs, network, n - 1, preproc, src_and_dst, save_to_disk);

    /* Reverse perm */
    duplicates.insert(duplicates.begin(), 0);  // first element is never a duplicate
    duplicates = perm_opened.inverse()(duplicates);
    auto duplicates_reversed = shuffle::unshuffle(id, rngs, network, n, preproc, duplicates, save_to_disk);

    /* Append MSB */
    g.src_order_bits.push_back(duplicates_reversed);
    g.dst_order_bits.push_back(duplicates_reversed);
}
