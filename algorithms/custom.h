#pragma once
#include "../src/graphmpc/circuit.h"

class CustomCircuit : public Circuit {
   public:
    // Space for any member variables if needed

    CustomCircuit(ProtocolConfig &conf) : Circuit(conf/*, number of columns if >1 needed*/) {
        /* ... */
    }

   protected:
    // Remove to use inputs as initial states instead:
    std::optional<mp_val> init_nodes(size_t column) override {
        return /* ... */;
    }

    // Can be removed if current state shall be used as message:
    mp_val prepare(mp_val &state, size_t i, size_t column) override {
        return /* ... */;;
    }

    mp_val apply(mp_val &state, mp_val &update, size_t i, size_t column) override {
        return /* ... */;
    }

    // Can be removed to not run additional computation on states after message passing:
    mp_val post_mp(mp_val &state, size_t column) override {
        return /* ... */;
    }

    // Must be removed if no columns>1 were specified, and needs to be removed otherwise if states
    // for all columns shall be given to output without aggregating to a single column before:
    std::optional<mp_val> post_mp_aggregate(std::vector<mp_val> &states) override {
        return /* ... */;
    }
};