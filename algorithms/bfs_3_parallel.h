#pragma once

#include "../src/core/circuit.h"

class BFS3ParallelCircuit : public Circuit {
   public:

    BFS3ParallelCircuit(ProtocolConfig &conf) : Circuit(conf, 3) {
        if (conf.nodes < 3)
            throw std::runtime_error("Not enough nodes (expects at least 3)!");
        build();
    }

    std::optional<SIMD_wire_id> init_node_data(size_t column) override {
        std::vector<Ring> ohv_column(nodes, 0);
        ohv_column[column] = 1;
        return set_const_vec_SIMD(ohv_column);
    }

    SIMD_wire_id apply(SIMD_wire_id &data_old, SIMD_wire_id &data_new, size_t /*i*/, size_t /*column*/) override {
        return add_SIMD(data_old, data_new);
    }

    SIMD_wire_id post_mp(SIMD_wire_id &data, size_t /*column*/) override {
        return clip(data);
    }
};