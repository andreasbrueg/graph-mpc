#include "mp_protocol.h"

std::vector<Ring> MPProtocol::add_sort(std::vector<std::vector<Ring>> &bit_keys, size_t bits) {
    w.sort_perm = add_compaction(bit_keys[0]);
    for (size_t bit = 1; bit < bits; ++bit) {
        w.sort_perm = add_sort_iteration(w.sort_perm, bit_keys[bit]);
    }
    return w.sort_perm;
}

std::vector<Ring> MPProtocol::add_sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys) {
    w.sort_perm = add_shuffle(perm);
    w.sort_bits = repeat_shuffle(keys);

    w.sort_perm = add_reveal(w.sort_perm);
    w.sort_perm = add_permute(w.sort_bits, w.sort_bits);

    w.sort_next_perm = add_compaction(w.sort_bits);

    w.sort_next_perm = add_permute(w.sort_next_perm, w.sort_perm, true);
    return add_unshuffle(w.sort_next_perm);
}
