#include "../compaction.h"

std::vector<Ring> compaction::preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    std::vector<Ring> shares_P1;
    size_t idx = 0;

    auto triples = mul::preprocess(id, rngs, shares_P1, idx, n);
    return shares_P1;
}

std::vector<std::tuple<Ring, Ring, Ring>> compaction::preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &shares_P1,
                                                                         size_t &idx) {
    return mul::preprocess(id, rngs, shares_P1, idx, n);
}

std::vector<std::tuple<Ring, Ring, Ring>> compaction::preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n,
                                                                 size_t BLOCK_SIZE) {
    std::vector<Ring> vals_to_p1;
    size_t idx = 0;

    if (id == P1) recv_vec(D, network, n, vals_to_p1, BLOCK_SIZE);

    auto triples = mul::preprocess(id, rngs, vals_to_p1, idx, n);

    if (id == D) send_vec(P1, network, vals_to_p1.size(), vals_to_p1, BLOCK_SIZE);

    return triples;
}