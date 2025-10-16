#pragma once

#include "../src/graphmpc/mp_protocol.h"

class PiRProtocol : public MPProtocol {
   public:
    PiRProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network) : MPProtocol(conf, network) {
        w.mp_data_parallel.resize(nodes);
        for (size_t i = 0; i < nodes; ++i) {
            w.mp_data_parallel[i].resize(size);
            w.mp_data_parallel[i][i] = 1;  // Set index of node to 1
            w.mp_data_parallel[i] = share::random_share_secret_vec_2P(id, rngs, w.mp_data_parallel[i]);
        }
    }

    void pre_mp() override {}

    void apply() override {
        w.buf = add(w.data, w.buf);
        w.buf = w.data;
    }

    void post_mp() override {
        add_clip();
        w.data = construct_payload(w.mp_data_parallel);
    }
};