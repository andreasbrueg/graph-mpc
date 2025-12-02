#pragma once

#include "../src/graphmpc/circuit.h"

class CustomCircuit : public Circuit {
    void pre_mp() override {}

    SIMD_wire_id apply(SIMD_wire_id &data, SIMD_wire_id &update) override {}

    SIMD_wire_id post_mp(SIMD_wire_id &data) override { return data; }
};