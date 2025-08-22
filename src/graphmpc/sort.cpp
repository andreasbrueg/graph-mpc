#include "sort.h"

/* ----- Preprocessing ----- */
void sort::get_sort_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t n_bits, MPPreprocessing &preproc,
                               Party &recv_shuffle, Party &recv_mul, bool save_to_disk) {
    mul::preprocess(id, rngs, network, n, preproc, recv_mul, false, save_to_disk);

    for (size_t i = 0; i < n_bits - 1; ++i) {
        sort_iteration_preprocess(id, rngs, network, n, preproc, recv_shuffle, recv_mul, save_to_disk);
    }
}

void sort::sort_iteration_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                                     Party &recv_shuffle, Party &recv_mul, bool save_to_disk) {
    shuffle::get_shuffle(id, rngs, network, n, preproc, recv_shuffle, save_to_disk);
    mul::preprocess(id, rngs, network, n, preproc, recv_mul, false, save_to_disk);
    shuffle::get_unshuffle(id, rngs, network, n, preproc, save_to_disk);
}

/* ----- Evaluation ----- */
Permutation sort::get_sort_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, MPPreprocessing &preproc,
                                    std::vector<std::vector<Ring>> &bit_shares, bool save_to_disk) {
    if (id == D) return Permutation(n);

    Permutation sigma = compaction::evaluate(id, rngs, network, n, preproc, bit_shares[0], save_to_disk);

    size_t n_bits = bit_shares.size();
    for (size_t i = 1; i < n_bits; ++i) {
        sigma = sort_iteration_evaluate(id, rngs, network, n, sigma, preproc, bit_shares[i], save_to_disk);
    }
    return sigma;
}

Permutation sort::sort_iteration_evaluate(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, Permutation &perm,
                                          MPPreprocessing &preproc, std::vector<Ring> &bit_shares, bool save_to_disk) {
    if (id == D) return Permutation(n);

    Permutation perm_shuffled = shuffle::shuffle(id, rngs, network, n, preproc, perm, save_to_disk, true);

    Permutation perm_open = share::reveal_perm(id, network, perm_shuffled);

    std::vector<Ring> input_shuffled = shuffle::shuffle_repeat(id, rngs, network, n, preproc, bit_shares, save_to_disk);
    std::vector<Ring> input_sorted = perm_open(input_shuffled);

    Permutation sigma_p = compaction::evaluate(id, rngs, network, n, preproc, input_sorted, save_to_disk);

    auto sigma_p_vec = sigma_p.get_perm_vec();
    std::vector<Ring> next_perm = perm_open.inverse()(sigma_p_vec);

    Permutation next_perm_shuffled = Permutation(shuffle::unshuffle(id, rngs, network, n, preproc, next_perm, save_to_disk));

    return next_perm_shuffled;
}
