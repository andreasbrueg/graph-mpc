#pragma once

#include "../src/core/circuit.h"

class PiRCircuit : public Circuit {
   public:
    PiRCircuit(ProtocolConfig &conf) : Circuit(conf, conf.nodes) {}

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

    std::optional<SIMD_wire_id> post_mp_aggregate(std::vector<SIMD_wire_id> &data) override {
        return column_sums_to_nodes(data);
    }
};