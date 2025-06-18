#include "../deduplication.h"

DeduplicationPreprocessing deduplication_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE) {
    DeduplicationPreprocessing preproc;

    size_t n_bits = 2 * sizeof(Ring) * 8;
    SortPreprocessing sort_preproc = sort::get_sort_preprocess(id, rngs, network, n, BLOCK_SIZE, n_bits);
    ShufflePre apply_perm_share = shuffle::get_shuffle(id, rngs, network, n, BLOCK_SIZE, true);
    std::vector<Ring> unshuffle_B = shuffle::get_unshuffle(id, rngs, network, n, BLOCK_SIZE, apply_perm_share);
    auto eqz_triples_1 = clip::equals_zero_preprocess(id, rngs, network, n, BLOCK_SIZE);
    auto eqz_triples_2 = clip::equals_zero_preprocess(id, rngs, network, n, BLOCK_SIZE);
    auto mul_triples = mul::preprocess(id, rngs, network, n, BLOCK_SIZE);
    auto B2A_triples = clip::B2A_preprocess(id, rngs, network, n, BLOCK_SIZE);

    preproc.sort_preproc = sort_preproc;
    preproc.apply_perm_share = apply_perm_share;
    preproc.unshuffle_B = unshuffle_B;
    preproc.eqz_triples_1 = eqz_triples_1;
    preproc.eqz_triples_2 = eqz_triples_2;
    preproc.mul_triples = mul_triples;
    preproc.B2A_triples = B2A_triples;

    return preproc;
}