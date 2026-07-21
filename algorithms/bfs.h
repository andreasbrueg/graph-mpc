#pragma once
#include "../src/core/circuit.h"

class BFSCircuit : public Circuit {
   public:
    BFSCircuit(ProtocolConfig &conf) : Circuit(conf) {}

   protected:
    mp_val apply(mp_val &state, mp_val &update, size_t /*i*/, size_t /*column*/) override {
        return add(state, update);
    }

    mp_val post_mp(mp_val &state, size_t /*column*/) override {
        return clip(state);
    }

    // others default to: initializing states from inputs and setting message=state
};