#pragma once
#include "../src/core/circuit.h"

class CustomCircuit : public Circuit {
   public:

    CustomCircuit(ProtocolConfig &conf) : Circuit(conf) {}

   protected:

    mp_val apply(mp_val &state, mp_val &/*update*/, size_t /*i*/, size_t /*column*/) override {
        return state;
    }
};