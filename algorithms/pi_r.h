#pragma once
#include "../src/core/circuit.h"

class PiRCircuit : public Circuit {
   public:
    PiRCircuit(ProtocolConfig &conf) : Circuit(conf, conf.nodes) {} // one column per node

   protected:
    std::optional<mp_val> init_nodes(size_t column) override {
        std::vector<Ring> ohv_column(nodes, 0);
        ohv_column[column] = 1;
        return set_const_vector(ohv_column);
    }

    mp_val apply(mp_val &state, mp_val &update, size_t /*i*/, size_t /*column*/) override {
        return add(state, update);
    }

    mp_val post_mp(mp_val &state, size_t /*column*/) override {
        return clip(state);
    }

    std::optional<mp_val> post_mp_aggregate(std::vector<mp_val> &states) override {
        return column_sums_to_nodes(states);
    }

    // others default to: setting message=state
};