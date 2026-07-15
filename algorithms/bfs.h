#pragma once

#include "../src/core/circuit.h"

class BFSCircuit : public Circuit {
   public:

    BFSCircuit(ProtocolConfig &conf) : Circuit(conf) {}

    SIMD_wire_id apply(SIMD_wire_id &data_old, SIMD_wire_id &data_new, size_t /*i*/, size_t /*column*/) override {
        return add_SIMD(data_old, data_new);
    }

    SIMD_wire_id post_mp(SIMD_wire_id &data, size_t /*column*/) override {
        return clip(data);
    }
};