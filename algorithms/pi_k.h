#pragma once

#include "../src/graphmpc/mp_protocol.h"

class PiKProtocol : public MPProtocol {
   public:
    PiKProtocol(ProtocolConfig &conf, std::shared_ptr<io::NetIOMP> &network) : MPProtocol(conf, network) {}

    void pre_mp() override { add_deduplication(); }

    void apply() override {}

    void post_mp() override {}
};