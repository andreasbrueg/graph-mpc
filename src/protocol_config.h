#pragma once

#include "io/netmp.h"
#include "random_generators.h"
#include "utils/types.h"

struct ProtocolConfig {
   public:
    Party pid;
    RandomGenerators rngs;
    std::shared_ptr<io::NetIOMP> network;
    size_t n_rows;
    const size_t BLOCK_SIZE;

    ProtocolConfig(Party pid, RandomGenerators &rngs, std::shared_ptr<io::NetIOMP> network, size_t n_rows, const size_t BLOCK_SIZE)
        : pid(pid), rngs(rngs), network(network), n_rows(n_rows), BLOCK_SIZE(BLOCK_SIZE) {};
};