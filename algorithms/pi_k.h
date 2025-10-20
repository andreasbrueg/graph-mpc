#pragma once

#include "../src/graphmpc/circuit.h"

class PiKCircuit : public Circuit {
   public:
    PiKCircuit(ProtocolConfig &conf) : Circuit(conf) {}

    void compute_sorts() override {
        // ctx.src_order = add_sort(g.src_order_bits, bits + 2);
        // ctx.dst_order = add_sort_iteration(ctx.dst_order, g.dst_order_bits[g.dst_order_bits.size() - 1]);
        // ctx.vtx_order = add_sort_iteration(ctx.src_order, g.isV_inv);
    }

    void pre_mp() override {
        // add_deduplication();
    }

    size_t apply(size_t &data_vtx) override { return data_vtx; }

    size_t post_mp(size_t &data) override { return data; }
};