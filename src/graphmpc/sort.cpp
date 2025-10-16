#include "mp_protocol.h"

std::vector<Ring> MPProtocol::add_sort(std::vector<std::vector<Ring>> &bit_keys, size_t bits) {
    auto perm = compaction(bit_keys[0]);
    for (size_t bit = 1; bit < bits; ++bit) {
        perm = add_sort_iteration(perm, bit_keys[bit]);
    }
    return perm;
}

std::vector<Ring> MPProtocol::add_sort_iteration(std::vector<Ring> &perm, std::vector<Ring> &keys) {
    auto perm_shuffled = shuffle(perm, shuffle_idx);
    auto keys_shuffled = shuffle(keys, shuffle_idx);

    auto clear_perm_shuffled = reveal(perm_shuffled);
    auto keys_sorted = permute(keys_shuffled, clear_perm_shuffled);

    auto perm_next = compaction(keys_sorted);

    perm_next = permute(perm_next, clear_perm_shuffled, true);
    perm_next = unshuffle(perm_next, shuffle_idx);
    shuffle_idx++;
    return perm_next;
}
