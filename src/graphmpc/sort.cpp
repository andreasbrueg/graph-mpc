#include "mp_protocol.h"

void MPProtocol::add_sort(std::vector<std::vector<Ring>> &bit_keys, std::vector<Ring> &output, size_t bits) {
    add_compaction(bit_keys[0], w.sort_perm);
    for (size_t bit = 1; bit < bits; ++bit) {
        add_sort_iteration(w.sort_perm, bit_keys[bit], w.sort_perm);
    }
    add_update(w.sort_perm, output);
}

void MPProtocol::add_sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys, std::vector<Ring> &output) {
    add_shuffle(perm, w.sort_perm);
    repeat_shuffle(keys, w.sort_bits);

    add_reveal(w.sort_perm, w.sort_perm);
    add_permute(w.sort_bits, w.sort_bits, w.sort_perm);

    add_compaction(w.sort_bits, w.sort_next_perm);

    add_permute(w.sort_next_perm, w.sort_next_perm, w.sort_perm, true);
    add_unshuffle(w.sort_next_perm, output);
}
