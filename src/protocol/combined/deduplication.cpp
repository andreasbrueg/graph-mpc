#include "../deduplication.h"

void deduplication(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                   std::vector<std::vector<Ring>> &src_bits, std::vector<std::vector<Ring>> &dst_bits, std::vector<Ring> &src, std::vector<Ring> &dst) {
    std::vector<std::vector<Ring>> src_dst_bits;
    src_dst_bits.insert(src_dst_bits.end(), src_bits.begin(), src_bits.end());
    src_dst_bits.insert(src_dst_bits.end(), dst_bits.begin(), dst_bits.end());

    Permutation rho = sort::get_sort(id, rngs, network, n, BLOCK_SIZE, src_dst_bits);
    auto src_p = sort::apply_perm(id, rngs, network, n, BLOCK_SIZE, rho, src);
    auto dst_p = sort::apply_perm(id, rngs, network, n, BLOCK_SIZE, rho, dst);

    std::vector<Ring> src_dupl(n);
    std::vector<Ring> dst_dupl(n);

    src_dupl[0] = 1;
    dst_dupl[0] = 1;

    for (size_t i = 1; i < n; ++i) {
        src_dupl[i] = src_p[i] - src_p[i - 1];
        dst_dupl[i] = dst_p[i] - dst_p[i - 1];
    }

    auto src_eqz = clip::equals_zero(id, rngs, network, BLOCK_SIZE, src_dupl);
    auto dst_eqz = clip::equals_zero(id, rngs, network, BLOCK_SIZE, dst_dupl);

    auto triples = mul::preprocess_bin(id, rngs, network, n, BLOCK_SIZE);

    auto src_and_dst = mul::evaluate_bin(id, network, n, BLOCK_SIZE, triples, src_eqz, dst_eqz);
    auto duplicates = clip::B2A(id, rngs, network, BLOCK_SIZE, src_and_dst);

    /* Reverse perm */
    auto duplicates_reversed = sort::reverse_perm(id, rngs, network, n, BLOCK_SIZE, rho, duplicates);

    /* Reveal for debugging */
    auto src_dupl_rev = share::reveal_vec(id, network, BLOCK_SIZE, src_dupl);
    auto dst_dupl_rev = share::reveal_vec(id, network, BLOCK_SIZE, dst_dupl);
    auto src_eqz_rev = share::reveal_vec_bin(id, network, BLOCK_SIZE, src_eqz);
    auto dst_eqz_rev = share::reveal_vec_bin(id, network, BLOCK_SIZE, dst_eqz);
    auto src_and_dst_rev = share::reveal_vec_bin(id, network, BLOCK_SIZE, src_and_dst);
    auto duplicates_rev = share::reveal_vec(id, network, BLOCK_SIZE, duplicates);
    auto duplicates_reversed_rev = share::reveal_vec(id, network, BLOCK_SIZE, duplicates_reversed);

    /* Append MSB */
    src_bits.push_back(duplicates_reversed);
    dst_bits.push_back(duplicates_reversed);
}
