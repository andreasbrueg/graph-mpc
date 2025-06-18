#include "../clip.h"

std::vector<std::tuple<Ring, Ring, Ring>> clip::equals_zero_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n,
                                                                       size_t BLOCK_SIZE) {
    const size_t n_layers = 5;
    const size_t n_triples = n_layers * n;

    return mul::preprocess_bin(id, rngs, network, n_triples, BLOCK_SIZE);
}

std::vector<Ring> clip::equals_zero_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    const size_t n_layers = 5;
    const size_t n_triples = n_layers * n;

    std::vector<Ring> vals_to_P1;
    size_t idx = 0;

    auto triples = mul::preprocess_bin(id, rngs, vals_to_P1, idx, n_triples);
    return vals_to_P1;
}

std::vector<std::tuple<Ring, Ring, Ring>> clip::equals_zero_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals,
                                                                               size_t &idx) {
    const size_t n_layers = 5;
    const size_t n_triples = n_layers * n;

    auto triples = mul::preprocess_bin(id, rngs, vals, idx, n_triples);
    return triples;
}

std::vector<std::tuple<Ring, Ring, Ring>> clip::B2A_preprocess(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n,
                                                               size_t BLOCK_SIZE) {
    return mul::preprocess(id, rngs, network, n, BLOCK_SIZE);
}

std::vector<Ring> clip::B2A_preprocess_Dealer(Party id, RandomGenerators &rngs, size_t n) {
    std::vector<Ring> vals_to_P1;
    size_t idx = 0;

    auto triples = mul::preprocess(id, rngs, vals_to_P1, idx, n);

    return vals_to_P1;
}

std::vector<std::tuple<Ring, Ring, Ring>> clip::B2A_preprocess_Parties(Party id, RandomGenerators &rngs, size_t n, std::vector<Ring> &vals, size_t &idx) {
    return mul::preprocess(id, rngs, vals, idx, n);
}
