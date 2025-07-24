#include "deduplication.h"

/* ----- Preprocessing ----- */
DeduplicationPreprocessing deduplication_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, size_t n_bits) {
    DeduplicationPreprocessing preproc;

    SortPreprocessing dst_sort_pre = sort::get_sort_preprocess(id, rngs, network, n, n_bits + 1);

    auto src_sort_pre = std::vector<SortIterationPreprocessing>();

    for (size_t i = 0; i < n_bits; ++i) {
        src_sort_pre.push_back(sort::sort_iteration_preprocess(id, rngs, network, n));
    }

    ShufflePre apply_perm_share = shuffle::get_shuffle(id, rngs, network, n, true);
    std::vector<Ring> unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, apply_perm_share);
    auto eqz_triples_1 = clip::equals_zero_preprocess(id, rngs, network, n - 1);
    auto eqz_triples_2 = clip::equals_zero_preprocess(id, rngs, network, n - 1);
    auto mul_triples = mul::preprocess(id, rngs, network, n - 1);
    auto B2A_triples = clip::B2A_preprocess(id, rngs, network, n - 1);

    preproc.dst_sort_pre = dst_sort_pre;
    preproc.src_sort_pre = src_sort_pre;
    preproc.apply_perm_share = apply_perm_share;
    preproc.unshuffle_B = unshuffle_B;
    preproc.eqz_triples_1 = eqz_triples_1;
    preproc.eqz_triples_2 = eqz_triples_2;
    preproc.mul_triples = mul_triples;
    preproc.B2A_triples = B2A_triples;

    return preproc;
}

/* ----- Evaluation ----- */
void deduplication_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, DeduplicationPreprocessing &preproc,
                            SecretSharedGraph &g) {
    Permutation perm = sort::get_sort_evaluate(id, rngs, network, n, g.dst_order_bits, preproc.dst_sort_pre);
    preproc.dst_sort = perm;

    for (size_t i = 0; i < g.src_bits.size(); ++i) {
        perm = sort::sort_iteration_evaluate(id, rngs, network, n, perm, g.src_bits[i], preproc.src_sort_pre[i]);
    }

    Permutation src_dst_sort = perm;

    auto perm_shuffled = shuffle::shuffle(id, rngs, network, src_dst_sort, preproc.apply_perm_share, n);
    auto perm_opened = share::reveal_perm(id, network, perm_shuffled);
    auto shuffled_src_share = shuffle::shuffle(id, rngs, network, g.src, preproc.apply_perm_share, n);
    auto shuffled_dst_share = shuffle::shuffle(id, rngs, network, g.dst, preproc.apply_perm_share, n);

    auto src_p = perm_opened(shuffled_src_share);
    auto dst_p = perm_opened(shuffled_dst_share);

    std::vector<Ring> src_dupl(n - 1);
    std::vector<Ring> dst_dupl(n - 1);

    for (size_t i = 1; i < n; ++i) {
        src_dupl[i - 1] = src_p[i] - src_p[i - 1];
        dst_dupl[i - 1] = dst_p[i] - dst_p[i - 1];
    }

    auto src_eqz = clip::equals_zero_evaluate(id, rngs, network, preproc.eqz_triples_1, src_dupl);
    auto dst_eqz = clip::equals_zero_evaluate(id, rngs, network, preproc.eqz_triples_2, dst_dupl);

    auto src_and_dst = mul::evaluate_bin(id, network, n - 1, preproc.mul_triples, src_eqz, dst_eqz);
    auto duplicates = clip::B2A_evaluate(id, rngs, network, preproc.B2A_triples, src_and_dst);

    /* Reverse perm */
    duplicates.insert(duplicates.begin(), 0);  // first element is never a duplicate
    duplicates = perm_opened.inverse()(duplicates);
    auto duplicates_reversed = shuffle::unshuffle(id, rngs, network, preproc.apply_perm_share, preproc.unshuffle_B, duplicates, n);

    /* Append MSB */
    g.src_bits.push_back(duplicates_reversed);
    g.dst_bits.push_back(duplicates_reversed);
    g.src_order_bits.push_back(duplicates_reversed);
    g.dst_order_bits.push_back(duplicates_reversed);
}

/* ----- Ad-Hoc Preprocessing ----- */
void deduplication(Party id, RandomGenerators &rngs, std::shared_ptr<NetworkInterface> network, size_t n, std::vector<std::vector<Ring>> &src_bits,
                   std::vector<std::vector<Ring>> &dst_bits, std::vector<Ring> &src, std::vector<Ring> &dst) {
    std::vector<std::vector<Ring>> src_dst_bits;
    src_dst_bits.insert(src_dst_bits.end(), src_bits.begin(), src_bits.end());
    src_dst_bits.insert(src_dst_bits.end(), dst_bits.begin(), dst_bits.end());

    Permutation rho = sort::get_sort(id, rngs, network, n, src_dst_bits);
    auto src_p = permute::apply_perm(id, rngs, network, n, rho, src);
    auto dst_p = permute::apply_perm(id, rngs, network, n, rho, dst);

    std::vector<Ring> src_dupl(n);
    std::vector<Ring> dst_dupl(n);

    src_dupl[0] = 1;
    dst_dupl[0] = 1;

    for (size_t i = 1; i < n; ++i) {
        src_dupl[i] = src_p[i] - src_p[i - 1];
        dst_dupl[i] = dst_p[i] - dst_p[i - 1];
    }

    auto src_eqz = clip::equals_zero(id, rngs, network, src_dupl);
    auto dst_eqz = clip::equals_zero(id, rngs, network, dst_dupl);

    auto triples = mul::preprocess_bin(id, rngs, network, n);

    auto src_and_dst = mul::evaluate_bin(id, network, n, triples, src_eqz, dst_eqz);
    auto duplicates = clip::B2A(id, rngs, network, src_and_dst);

    /* Reverse perm */
    auto duplicates_reversed = permute::reverse_perm(id, rngs, network, n, rho, duplicates);

    /* Append MSB */
    src_bits.push_back(duplicates_reversed);
    dst_bits.push_back(duplicates_reversed);
}
