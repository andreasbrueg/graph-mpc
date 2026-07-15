#pragma once

#include "../src/core/circuit.h"

class PiMCircuit : public Circuit {
   public:

    std::vector<Ring> weights;

    PiMCircuit(ProtocolConfig &conf, std::vector<Ring> &weights) : Circuit(conf), weights(weights) {
        if (weights.size() != conf.depth)
            throw std::runtime_error("Number of provided weights must match depth!");
        use_reverse_message_passing();
    }

    std::optional<SIMD_wire_id> init_node_data(size_t /*column*/) override {
        std::vector<Ring> all_zero(nodes, 0);
        return set_const_vec_SIMD(all_zero);
    }

    SIMD_wire_id pre_propagate(SIMD_wire_id &data, size_t i, size_t /*column*/) override {
        return add_const_SIMD(data, weights[weights.size() - 1 - i]);
    }

    SIMD_wire_id apply(SIMD_wire_id &/*data_old*/, SIMD_wire_id &data_new, size_t /*i*/, size_t /*column*/) override {
        return data_new;
    }
};