#include "../compaction.h"

Permutation compaction::get_compaction(Party id, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n, size_t BLOCK_SIZE,
                                       std::vector<Ring> &input_share) {
    auto triples = preprocess(id, rngs, network, n, BLOCK_SIZE);
    network->sync();
    return evaluate(id, rngs, network, n, BLOCK_SIZE, triples, input_share);
}
