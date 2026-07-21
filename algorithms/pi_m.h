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

   protected:
    std::optional<mp_val> init_nodes(size_t /*column*/) override {
        std::vector<Ring> all_zero(nodes, 0);
        return set_const_vector(all_zero);
    }

    mp_val prepare(mp_val &state, size_t i, size_t /*column*/) override {
        return add_constant(state, weights[weights.size() - 1 - i]);
    }

    mp_val apply(mp_val &/*state*/, mp_val &update, size_t /*i*/, size_t /*column*/) override {
        return update;
    }

    // others default to: no postprocessing of states
};