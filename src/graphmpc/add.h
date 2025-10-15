#pragma once

#include "function.h"

class Add : public Function {
   public:
    Add(size_t f_id, ProtocolConfig *conf, std::vector<Ring> input1, std::vector<Ring> input2, std::vector<Ring> output)
        : Function(f_id, conf, {}, {}, input1, input2, output) {}

    void preprocess() override {}

    void evaluate_send() override {}

    void evaluate_recv() override {
#pragma omp parallel for if (size > 10000)
        for (size_t i = 0; i < size; ++i) {
            output[i] = input[i] + input2[i];
        }
    }
};