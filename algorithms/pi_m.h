#pragma once

#include "../src/core/circuit.h"

class PiMCircuit : public Circuit {
   public:

    std::vector<Ring> weights;

    PiMCircuit(ProtocolConfig &conf, std::vector<Ring> &weights) : Circuit(conf), weights(weights) {
        if (weights.size() != conf.depth)
            throw std::runtime_error("Number of provided weights must match depth!");
        build();
    }

    void pre_mp() override {}

    SIMD_wire_id pre_propagate(SIMD_wire_id &data, size_t i) override {
        return add_const_SIMD(data, weights[weights.size() - 1 - i]);
    }

    SIMD_wire_id apply(SIMD_wire_id &data_old, SIMD_wire_id &data_new, size_t i) override {
        return data_new;
    }

    SIMD_wire_id post_mp(SIMD_wire_id &data) override { return data; }
};