#pragma once

#include "../src/graphmpc/circuit.h"

class PiMCircuit : public Circuit {
   public:
    PiMCircuit(ProtocolConfig &conf) : Circuit(conf) {}

    void pre_mp() override {}

    size_t apply(size_t &data_vtx) override { return data_vtx; }

    size_t post_mp(size_t &data) override { return data; }
};