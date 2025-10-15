#pragma once

#include "function.h"

class Update : public Function {
   public:
    Update(size_t f_id, ProtocolConfig *conf, std::vector<Ring> input, std::vector<Ring> output) : Function(f_id, conf, {}, {}, input, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override { output = input; }
};