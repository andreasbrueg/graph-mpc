#pragma once

#include "function.h"

class Flip : public Function {
   public:
    Flip(size_t f_id, ProtocolConfig *conf, std::vector<Ring> input, std::vector<Ring> output) : Function(f_id, conf, {}, {}, input, output, false) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
        assert(output.size() >= input.size());

#pragma omp parallel for if (input.size() > 10000)
        for (size_t i = 0; i < input.size(); ++i) {
            if (id == P0) {
                output[i] = 1 - input[i];
            } else if (id == P1) {
                output[i] = -input[i];
            }
        }
    }
};