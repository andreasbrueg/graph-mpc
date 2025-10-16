#pragma once

#include "function.h"

class Reveal : public Function {
   public:
    Reveal(size_t f_id, ProtocolConfig *conf, std::vector<Ring> *online_vals, std::vector<Ring> input, std::vector<Ring> output)
        : Function(f_id, conf, {}, online_vals, input, output, true) {}

    void preprocess() override {}

    void evaluate_send() override { online_vals->insert(online_vals->end(), input.begin(), input.end()); }

    void evaluate_recv() override { output = read_online(size); }
};
